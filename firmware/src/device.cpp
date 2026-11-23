// SPDX-License-Identifier: GPL-3.0-or-later
#include "device.h"

#include <Arduino.h>
#include <Preferences.h>
#include <esp_mac.h>

#include "battery.h"
#include "config_store.h"
#include "hw.h"
#include "setup.h"
#include "version.h"

namespace device {
namespace {
SetupState g_setup;
Hooks g_hooks;
constexpr const char* kNs = "since";
}  // namespace

SetupState& setup() { return g_setup; }
Hooks& hooks() { return g_hooks; }

std::string deviceName() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  return since::deviceName(mac);
}

since::Config Hooks::loadConfig() { return since::loadConfig(); }

bool Hooks::saveConfig(const since::Config& c) { return since::saveConfig(c); }

bool Hooks::setTime(since::Epoch utc) {
  if (!hw::rtcSet(utc)) return false;
  g_setup.timeChanged = true;
  return true;
}

since::DeviceStatus Hooks::status() {
  since::DeviceStatus s;
  s.fw = SINCE_FW_VERSION;
  s.name = g_setup.name.empty() ? deviceName() : g_setup.name;
  s.batteryMv = hw::batteryMillivolts();
  s.batteryPercent = s.batteryMv < since::kBatteryAbsentMv ? 0 : since::batteryPercent(s.batteryMv);
  s.charging = -1;  // PMIC-Status ist auf dem EE04 nicht mit einem GPIO verbunden
  s.rtcOk = hw::rtcOk();
  s.rtcLostPower = hw::rtcLostPower();
  s.nowUtc = hw::rtcNow();
  s.lastSync = lastSync();
  const unsigned long idle = millis() - g_setup.lastActivityMs;
  s.setupRemainingS = idle >= kSetupTimeoutMs ? 0 : static_cast<int>((kSetupTimeoutMs - idle) / 1000);
  return s;
}

void Hooks::requestRedraw() { g_setup.redrawRequested = true; }
void Hooks::requestExitSetup() { g_setup.exitRequested = true; }
void Hooks::touch() { g_setup.lastActivityMs = millis(); }

since::Epoch lastSync() {
  Preferences p;
  if (!p.begin(kNs, true)) return 0;
  const since::Epoch v = p.getLong64("lastSync", 0);
  p.end();
  return v;
}

since::Epoch lastSyncAttempt() {
  Preferences p;
  if (!p.begin(kNs, true)) return 0;
  const since::Epoch v = p.getLong64("lastTry", 0);
  p.end();
  return v;
}

void storeSync(since::Epoch attempt, bool success) {
  Preferences p;
  if (!p.begin(kNs, false)) return;
  p.putLong64("lastTry", attempt);
  if (success) p.putLong64("lastSync", attempt);
  p.end();
}

void clearSyncState() {
  Preferences p;
  if (!p.begin(kNs, false)) return;
  p.remove("lastTry");
  p.remove("lastSync");
  p.end();
}

}  // namespace device
