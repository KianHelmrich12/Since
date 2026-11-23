// SPDX-License-Identifier: GPL-3.0-or-later
// Virtuelles since-Gerät für den Host. Bildet den Ablauf von src/main.cpp nach und nutzt
// dieselbe Logik (since::runCycle, since::callApi, since::handleRpc) und dasselbe Rendering
// (src/render.cpp) wie die Firmware. Simuliert werden RTC (DS3231), Akku, NVS, Tasten,
// Setup-Modus und optional ein erfolgreicher NTP-Sync.
//
// C-Schnittstelle für tools/emulator.py (ctypes). Rückgaben sind JSON-Strings, gültig bis
// zum nächsten Aufruf. Nicht threadsicher: der Aufrufer serialisiert.
#include <Adafruit_GFX.h>
#include <ArduinoJson.h>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <random>
#include <sstream>
#include <string>

#include "render.h"
#include "since_core.h"

using namespace since;

namespace {

constexpr Epoch kRtcResetEpoch = 946684800;  // DS3231 nach Stromverlust: 2000-01-01
constexpr Epoch kSetupTimeoutS = 10 * 60;
constexpr Epoch kSetupMaxS = 30 * 60;

struct World {
  Epoch realUtc = 0;   // wahre Zeit der simulierten Welt
  int batteryMv = 4000;
  bool internet = false;  // WLAN mit Internet erreichbar (NTP)
};

struct Rtc {
  Epoch offset = 0;  // RTC = real + offset
  bool lostPower = false;
};

struct Nvs {
  std::string configJson;  // mit Secrets, wie auf dem Gerät
  Epoch lastSync = 0;
  Epoch lastTry = 0;
};

struct SetupSession {
  bool active = false;
  std::string ssid, password, passkey;
  Epoch start = 0, lastActivity = 0;
  bool exitRequested = false;
};

World g_world;
Rtc g_rtc;
Nvs g_nvs;
SetupSession g_setup;
std::string g_statePath;
// RTC-Slow-Memory (überlebt Deep Sleep, nicht Stromverlust)
uint32_t g_lastFingerprint = 0;
BatteryLevel g_level = BatteryLevel::kOk;
Epoch g_nextWake = 0;
bool g_drawnOnce = false;

GFXcanvas1 g_canvas(render::kWidth, render::kHeight);
std::mt19937 g_rng(0x51CE);
std::string g_out;
std::string g_log;

Epoch rtcNow() { return g_world.realUtc + g_rtc.offset; }

void log(const std::string& s) {
  if (!g_log.empty()) g_log += "\n";
  g_log += s;
}

void persist() {
  if (g_statePath.empty()) return;
  JsonDocument d;
  d["config"] = g_nvs.configJson;
  d["lastSync"] = g_nvs.lastSync;
  d["lastTry"] = g_nvs.lastTry;
  std::ofstream f(g_statePath);
  serializeJsonPretty(d, f);
}

Config loadConfig() {
  Config c = defaultConfig();
  if (!g_nvs.configJson.empty()) {
    Config cand = defaultConfig();
    if (applyJson(g_nvs.configJson, &cand).ok()) c = cand;
  }
  return c;
}

bool saveConfig(const Config& c) {
  if (!validate(c).empty()) return false;
  g_nvs.configJson = toJson(c, true);
  persist();
  return true;
}

class EmuHooks : public DeviceHooks {
 public:
  Config loadConfig() override { return ::loadConfig(); }
  bool saveConfig(const Config& c) override { return ::saveConfig(c); }
  bool setTime(Epoch utc) override {
    g_rtc.offset = utc - g_world.realUtc;
    g_rtc.lostPower = false;
    log("RTC gestellt");
    return true;
  }
  DeviceStatus status() override {
    DeviceStatus s;
    s.fw = "emulator";
    s.name = g_setup.ssid.empty() ? "since-EMU0" : g_setup.ssid;
    s.batteryMv = g_world.batteryMv;
    s.batteryPercent = batteryPercent(g_world.batteryMv);
    s.charging = -1;
    s.rtcOk = true;
    s.rtcLostPower = g_rtc.lostPower;
    s.nowUtc = rtcNow();
    s.lastSync = g_nvs.lastSync;
    const Epoch idle = g_world.realUtc - g_setup.lastActivity;
    s.setupRemainingS = g_setup.active && idle < kSetupTimeoutS ? static_cast<int>(kSetupTimeoutS - idle) : 0;
    return s;
  }
  void requestRedraw() override {}
  void requestExitSetup() override { g_setup.exitRequested = true; }
  void touch() override { g_setup.lastActivity = g_world.realUtc; }
};
EmuHooks g_hooks;

void modelJson(JsonObject o, const ScreenModel& m) {
  static const char* kKinds[] = {"standard", "minimal", "milestone", "battery", "setup", "settime", "welcome"};
  o["kind"] = kKinds[static_cast<int>(m.kind)];
  o["title"] = m.title;
  o["subtitle"] = m.subtitle;
  o["headline"] = m.headline;
  o["line1"] = m.line1;
  o["line2"] = m.line2;
  o["banner"] = m.banner;
  o["batteryPercent"] = m.batteryPercent;
}

const char* result(bool drawn, const ScreenModel* m) {
  JsonDocument d;
  d["drawn"] = drawn;
  d["setup"] = g_setup.active;
  d["nextWakeUtc"] = g_setup.active ? 0 : g_nextWake;
  d["rtcUtc"] = rtcNow();
  d["rtcLostPower"] = g_rtc.lostPower;
  d["level"] = static_cast<int>(g_level);
  if (m != nullptr) modelJson(d["screen"].to<JsonObject>(), *m);
  d["log"] = g_log;
  g_log.clear();
  g_out.clear();
  serializeJson(d, g_out);
  return g_out.c_str();
}

void draw(const ScreenModel& m) {
  render::draw(g_canvas, m);
  g_drawnOnce = true;
  log("Display neu gezeichnet (" + m.title + ")");
}

std::string randomDigits() {
  return randomPasskey([]() { return static_cast<uint32_t>(g_rng()); });
}

const char* enterSetup() {
  const Config cfg = loadConfig();
  g_setup = SetupSession();
  g_setup.active = true;
  g_setup.ssid = "since-EMU0";
  g_setup.password = cfg.portalSecure ? randomWifiPassword([]() { return static_cast<uint32_t>(g_rng()); }) : "";
  g_setup.passkey = randomDigits();
  g_setup.start = g_setup.lastActivity = g_world.realUtc;

  ScreenInputs in;
  in.config = cfg;
  in.nowUtc = rtcNow();
  in.batteryMv = g_world.batteryMv;
  in.battery = g_level;
  in.setupMode = true;
  in.setup.ssid = g_setup.ssid;
  in.setup.password = g_setup.password;
  in.setup.passkey = g_setup.passkey;
  const ScreenModel m = buildScreen(in);
  draw(m);
  g_lastFingerprint = 0;
  log("Setup-Modus: Hotspot " + g_setup.ssid + " + BLE aktiv");
  return result(true, &m);
}

// Normaler Zyklus wie in main.cpp nach der Tastenauswertung
const char* cycle(bool force, bool brownout) {
  Config cfg = loadConfig();
  bool lost = g_rtc.lostPower;
  Epoch now = rtcNow();
  const bool critical = batteryLevel(g_world.batteryMv, g_level) == BatteryLevel::kCritical || brownout;
  if (!critical && isSyncDue(cfg, now, g_nvs.lastSync, g_nvs.lastTry, lost)) {
    if (g_world.internet) {
      g_rtc.offset = 0;
      g_rtc.lostPower = false;
      lost = false;
      now = rtcNow();
      g_nvs.lastSync = g_nvs.lastTry = now;
      log("NTP-Sync erfolgreich");
    } else {
      g_nvs.lastTry = now;
      log("NTP-Sync fehlgeschlagen (kein Internet)");
    }
    persist();
  }
  CycleInputs ci;
  ci.config = cfg;
  ci.nowUtc = now;
  ci.rtcLostPower = lost;
  ci.batteryMv = g_world.batteryMv;
  ci.previousLevel = g_level;
  ci.brownout = brownout;
  ci.forceRedraw = force || !g_drawnOnce;
  ci.lastFingerprint = g_lastFingerprint;
  const CycleResult r = runCycle(ci);
  g_level = r.level;
  g_lastFingerprint = r.fingerprint;
  if (r.draw) draw(r.model);
  else log("Text unverändert, kein Neuzeichnen");
  // nextWake ist RTC-Zeit; in Weltzeit umrechnen
  g_nextWake = r.nextWakeUtc - g_rtc.offset;
  return result(r.draw, &r.model);
}

}  // namespace

extern "C" {

void emu_init(const char* statePath) {
  g_canvas.fillScreen(0xFFFF);  // neues Panel: weiß
  g_statePath = statePath ? statePath : "";
  g_nvs = Nvs();
  std::ifstream f(g_statePath);
  if (f) {
    std::stringstream ss;
    ss << f.rdbuf();
    JsonDocument d;
    if (deserializeJson(d, ss.str()) == DeserializationError::Ok) {
      g_nvs.configJson = d["config"] | "";
      g_nvs.lastSync = d["lastSync"] | 0;
      g_nvs.lastTry = d["lastTry"] | 0;
    }
  }
}

void emu_set_world(long long realUtc, int batteryMv, int internet) {
  g_world.realUtc = realUtc;
  g_world.batteryMv = batteryMv;
  g_world.internet = internet != 0;
}

// Knopfzelle raus: DS3231 verliert die Zeit und setzt OSF
void emu_rtc_lose_power() {
  g_rtc.offset = kRtcResetEpoch - g_world.realUtc;
  g_rtc.lostPower = true;
}

// Akku/USB komplett getrennt und wieder verbunden (RTC läuft auf der Knopfzelle weiter)
void emu_power_cycle() {
  g_lastFingerprint = 0;
  g_level = BatteryLevel::kOk;
  g_setup = SetupSession();
}

// reason: power_on, rtc_alarm, timer, key0, key0_hold, key1, key2, brownout
const char* emu_wake(const char* reasonC) {
  const std::string reason = reasonC ? reasonC : "";
  log("Aufwachen: " + reason);
  if (g_setup.active) return result(false, nullptr);  // im Setup reagiert die Firmware nicht
  if (reason == "power_on") emu_power_cycle();
  if (reason == "key0_hold") {
    g_nvs = Nvs();
    persist();
    emu_power_cycle();
    log("Werksreset (Taste 1, 10 s)");
    return cycle(true, false);
  }
  if (reason == "key0") return enterSetup();
  if (reason == "key2") {
    Config c = loadConfig();
    c.layout = c.layout == Layout::kStandard ? Layout::kMinimal : Layout::kStandard;
    saveConfig(c);
    log(std::string("Layout: ") + layoutName(c.layout));
  }
  return cycle(reason == "power_on" || reason == "key1", reason == "brownout");
}

// Regelmäßig im Setup-Modus aufrufen: beendet ihn bei „Fertig“, Timeout oder Obergrenze.
const char* emu_setup_tick() {
  if (!g_setup.active) return result(false, nullptr);
  const Epoch idle = g_world.realUtc - g_setup.lastActivity;
  const Epoch total = g_world.realUtc - g_setup.start;
  if (!g_setup.exitRequested && idle < kSetupTimeoutS && total < kSetupMaxS) {
    return result(false, nullptr);
  }
  log(g_setup.exitRequested ? "Setup beendet (Fertig)" : "Setup beendet (Timeout)");
  g_setup = SetupSession();
  return cycle(true, false);
}

const char* emu_setup_info() {
  JsonDocument d;
  d["active"] = g_setup.active;
  d["ssid"] = g_setup.ssid;
  d["password"] = g_setup.password;
  d["passkey"] = g_setup.passkey;
  g_out.clear();
  serializeJson(d, g_out);
  return g_out.c_str();
}

// Portal-API: op wie in api.h, zusätzlich factoryReset und ota. Antwort {"http":…,"body":…}
const char* emu_api(const char* opC, const char* params, const char* code) {
  const std::string op = opC ? opC : "";
  JsonDocument d;
  if (!g_setup.active) {
    d["http"] = 503;
    d["body"] = "{\"error\":\"hotspot_off\"}";
  } else if (op == "factoryReset" || op == "ota") {
    g_hooks.touch();
    if (code == nullptr || g_setup.passkey != code) {
      d["http"] = 403;
      d["body"] = "{\"error\":\"unauthorized\"}";
    } else if (op == "ota") {
      d["http"] = 501;
      d["body"] = "{\"error\":\"ota_not_emulated\"}";
    } else {
      g_nvs = Nvs();
      persist();
      g_setup = SetupSession();
      emu_power_cycle();
      log("Werksreset über das Portal");
      cycle(true, false);
      d["http"] = 200;
      d["body"] = "{}";
    }
  } else {
    const ApiResponse r = callApi(g_hooks, op, params ? params : "{}");
    d["http"] = r.httpStatus();
    d["body"] = r.body;
  }
  g_out.clear();
  serializeJson(d, g_out);
  return g_out.c_str();
}

// BLE-RPC (wie handleRpc auf dem Gerät), nur im Setup-Modus
const char* emu_rpc(const char* request) {
  if (!g_setup.active) {
    g_out = "{\"id\":null,\"ok\":false,\"error\":\"ble_off\"}";
    return g_out.c_str();
  }
  g_out = handleRpc(g_hooks, request ? request : "");
  return g_out.c_str();
}

// 1-bit-Bildspeicher 800×480, zeilenweise, MSB zuerst, Bit 1 = weiß
const uint8_t* emu_framebuffer() { return g_canvas.getBuffer(); }
int emu_framebuffer_size() { return render::kWidth * render::kHeight / 8; }

// Zeichenabdeckung aller Gerätefonts prüfen (siehe render::missingGlyphs)
const char* emu_missing_glyphs(const char* utf8) {
  std::string report;
  render::missingGlyphs(g_canvas, utf8 ? utf8 : "", &report);
  g_out = report;
  return g_out.c_str();
}

}  // extern "C"
