// SPDX-License-Identifier: GPL-3.0-or-later
#include "portal.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <Update.h>
#include <WebServer.h>
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <esp_task_wdt.h>

#include "api.h"
#include "config_store.h"
#include "device.h"

namespace portal {
namespace {

WebServer* g_server = nullptr;
DNSServer* g_dns = nullptr;
bool g_reboot = false;
bool g_fsOk = false;
String g_otaError;

const IPAddress kIp(192, 168, 4, 1);
const IPAddress kMask(255, 255, 255, 0);
constexpr size_t kMaxBody = 4096;

void sendApi(const since::ApiResponse& r) {
  g_server->sendHeader("Cache-Control", "no-store");
  g_server->send(r.httpStatus(), "application/json", r.body.c_str());
}

std::string body() {
  if (!g_server->hasArg("plain")) return "";
  const String& b = g_server->arg("plain");
  if (b.length() > kMaxBody) return "";
  return b.c_str();
}

void redirectToPortal() {
  g_server->sendHeader("Location", "http://192.168.4.1/", true);
  g_server->send(302, "text/plain", "");
}

// Liefert eine Datei aus LittleFS, bevorzugt die .gz-Variante.
bool serveStatic(String path) {
  if (!g_fsOk) return false;
  if (path.endsWith("/")) path += "index.html";
  String type = "text/plain";
  if (path.endsWith(".html")) type = "text/html; charset=utf-8";
  else if (path.endsWith(".css")) type = "text/css";
  else if (path.endsWith(".js")) type = "application/javascript";
  else if (path.endsWith(".svg")) type = "image/svg+xml";
  else if (path.endsWith(".ico")) type = "image/x-icon";
  const String gz = path + ".gz";
  const char* candidate = LittleFS.exists(gz) ? gz.c_str() : (LittleFS.exists(path) ? path.c_str() : nullptr);
  if (candidate == nullptr) return false;
  File f = LittleFS.open(candidate, "r");
  if (!f) return false;
  g_server->sendHeader("Cache-Control", "max-age=300");
  g_server->streamFile(f, type);  // setzt Content-Encoding bei .gz automatisch
  f.close();
  return true;
}

// OTA und Werksreset verlangen den Code vom Display (Header X-Setup-Code). So reicht es
// nicht, im (ggf. offenen) Setup-WLAN zu sein oder eine fremde Webseite offen zu haben.
bool authorized() {
  const String code = g_server->header("X-Setup-Code");
  return !device::setup().passkey.empty() && code == device::setup().passkey.c_str();
}

void handleOtaUpload() {
  HTTPUpload& up = g_server->upload();
  device::hooks().touch();
  esp_task_wdt_reset();  // ein Upload läuft komplett in einem handleClient()-Aufruf
  if (up.status == UPLOAD_FILE_START) {
    g_otaError = "";
    if (!authorized()) {
      g_otaError = "unauthorized";
      return;
    }
    const int cmd = g_server->arg("type") == "fs" ? U_SPIFFS : U_FLASH;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) g_otaError = Update.errorString();
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (g_otaError.isEmpty() && Update.isRunning() && Update.write(up.buf, up.currentSize) != up.currentSize)
      g_otaError = Update.errorString();
  } else if (up.status == UPLOAD_FILE_END) {
    if (g_otaError.isEmpty() && Update.isRunning() && !Update.end(true)) g_otaError = Update.errorString();
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    g_otaError = "aborted";
  }
}

void routes() {
  auto& hooks = device::hooks();
  g_server->on("/api/config", HTTP_GET, [&hooks]() { sendApi(since::callApi(hooks, "getConfig", "{}")); });
  g_server->on("/api/config", HTTP_POST, [&hooks]() {
    sendApi(since::callApi(hooks, "setConfig", body()));
  });
  g_server->on("/api/time", HTTP_POST, [&hooks]() { sendApi(since::callApi(hooks, "setTime", body())); });
  g_server->on("/api/status", HTTP_GET, [&hooks]() { sendApi(since::callApi(hooks, "getStatus", "{}")); });
  g_server->on("/api/redraw", HTTP_POST, [&hooks]() { sendApi(since::callApi(hooks, "redraw", "{}")); });
  g_server->on("/api/exit", HTTP_POST, [&hooks]() { sendApi(since::callApi(hooks, "exitSetup", "{}")); });
  g_server->on("/api/factory-reset", HTTP_POST, []() {
    if (!authorized()) {
      g_server->send(403, "application/json", "{\"error\":\"unauthorized\"}");
      return;
    }
    if (body().find("\"confirm\":\"RESET\"") == std::string::npos) {
      g_server->send(400, "application/json", "{\"error\":\"bad_request\"}");
      return;
    }
    since::eraseConfig();
    device::clearSyncState();
    NimBLEDevice::deleteAllBonds();  // BLE läuft im Setup-Modus parallel
    g_server->send(200, "application/json", "{}");
    g_reboot = true;
  });
  g_server->on(
      "/api/ota", HTTP_POST,
      []() {
        if (g_otaError == "unauthorized") {
          g_server->send(403, "application/json", "{\"error\":\"unauthorized\"}");
        } else if (g_otaError.isEmpty()) {
          g_server->send(200, "application/json", "{}");
          g_reboot = true;
        } else {
          String msg = "{\"error\":\"ota_failed\",\"detail\":\"" + g_otaError + "\"}";
          g_server->send(500, "application/json", msg);
        }
      },
      handleOtaUpload);
  // Captive-Portal-Erkennung von Android, iOS, Windows
  for (const char* p : {"/generate_204", "/gen_204", "/hotspot-detect.html", "/connecttest.txt",
                        "/ncsi.txt", "/fwlink"}) {
    g_server->on(p, HTTP_ANY, redirectToPortal);
  }
  g_server->onNotFound([]() {
    // Nur echte Portal-Aufrufe zählen als Aktivität; Hintergrund-Traffic des Handys
    // (per Wildcard-DNS hierher umgeleitet) darf den Setup-Modus nicht offen halten.
    const bool own = g_server->hostHeader() == "192.168.4.1" || g_server->hostHeader().isEmpty();
    if (own) device::hooks().touch();
    const String uri = g_server->uri();
    if (uri.startsWith("/api/")) {
      g_server->send(404, "application/json", "{\"error\":\"unknown_op\"}");
      return;
    }
    if (g_server->hostHeader() != "192.168.4.1" && !g_server->hostHeader().isEmpty()) {
      redirectToPortal();
      return;
    }
    if (!serveStatic(uri)) {
      if (!serveStatic("/index.html")) {
        g_server->send(503, "text/plain", "Web-UI fehlt (pio run -t uploadfs)");
      }
    }
  });
}

}  // namespace

bool start() {
  auto& st = device::setup();
  WiFi.persistent(false);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(kIp, kIp, kMask);
  const char* pw = st.password.empty() ? nullptr : st.password.c_str();
  if (!WiFi.softAP(st.name.c_str(), pw, 6, 0, 2)) return false;

  g_fsOk = LittleFS.begin(false);
  g_dns = new DNSServer();
  g_dns->setErrorReplyCode(DNSReplyCode::NoError);
  g_dns->start(53, "*", kIp);
  g_server = new WebServer(80);
  const char* headers[] = {"X-Setup-Code"};
  g_server->collectHeaders(headers, 1);
  routes();
  g_server->begin();
  return true;
}

void loop() {
  if (g_dns != nullptr) g_dns->processNextRequest();
  if (g_server != nullptr) g_server->handleClient();
}

void stop() {
  if (g_server != nullptr) {
    g_server->stop();
    delete g_server;
    g_server = nullptr;
  }
  if (g_dns != nullptr) {
    g_dns->stop();
    delete g_dns;
    g_dns = nullptr;
  }
  if (g_fsOk) LittleFS.end();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
}

bool rebootRequested() { return g_reboot; }

}  // namespace portal
