// SPDX-License-Identifier: GPL-3.0-or-later
// Ablauf pro Aufwachen:
//   Grund prüfen → RTC lesen → (Taster: Setup / Neu zeichnen / Layout / Werksreset)
//   → ggf. NTP-Sync → Bildschirm-Modell bauen → nur bei Änderung zeichnen
//   → Panel schlafen legen → nächsten Alarm setzen → Deep Sleep.
#include <Arduino.h>
#include <driver/rtc_io.h>
#include <esp_random.h>
#include <esp_task_wdt.h>

#include "ble.h"
#include "board_ee04.h"
#include "config_store.h"
#include "device.h"
#include "display.h"
#include "hw.h"
#include "ntp_sync.h"
#include "portal.h"
#include "since_core.h"
#include "version.h"

namespace {

// Überlebt den Deep Sleep (RTC-Slow-Memory), nicht aber Stromverlust.
RTC_DATA_ATTR uint32_t g_lastFingerprint = 0;
RTC_DATA_ATTR int g_batteryLevel = static_cast<int>(since::BatteryLevel::kOk);

constexpr uint32_t kWatchdogMs = 60000;
constexpr unsigned long kFactoryResetHoldMs = 10000;

void setupWatchdog() {
  esp_task_wdt_config_t cfg = {};
  cfg.timeout_ms = kWatchdogMs;
  cfg.idle_core_mask = 0;
  cfg.trigger_panic = true;
  if (esp_task_wdt_reconfigure(&cfg) != ESP_OK) esp_task_wdt_init(&cfg);
  esp_task_wdt_add(nullptr);
}

void releaseWakePins() {
  for (int pin : {board::kKey0, board::kKey1, board::kKey2, board::kRtcInt}) {
    rtc_gpio_deinit(static_cast<gpio_num_t>(pin));
  }
  pinMode(board::kKey0, INPUT);  // externe Pull-ups
  pinMode(board::kKey1, INPUT);
  pinMode(board::kKey2, INPUT);
  pinMode(board::kRtcInt, INPUT_PULLUP);
}

// true, wenn KEY0 kFactoryResetHoldMs lang gedrückt bleibt
bool heldForFactoryReset() {
  const unsigned long start = millis();
  while (digitalRead(board::kKey0) == LOW) {
    esp_task_wdt_reset();
    if (millis() - start >= kFactoryResetHoldMs) return true;
    delay(20);
  }
  return false;
}

since::ScreenInputs baseInputs(const since::Config& cfg, since::Epoch now, int mv,
                               since::BatteryLevel level, bool lostPower) {
  since::ScreenInputs in;
  in.config = cfg;
  in.nowUtc = now;
  in.batteryMv = mv;
  in.battery = level;
  in.rtcLostPower = lostPower;
  return in;
}

void runSetup(since::Config& cfg, int mv, since::BatteryLevel level) {
  auto& st = device::setup();
  const since::RandomFn rnd = []() { return esp_random(); };
  st.name = device::deviceName();
  st.password = cfg.portalSecure ? since::randomWifiPassword(rnd) : "";
  st.passkey = since::randomPasskey(rnd, &st.passkeyNumeric);
  st.lastActivityMs = millis();

  since::ScreenInputs in = baseInputs(cfg, hw::rtcNow(), mv, level, false);
  in.setupMode = true;
  in.setup.ssid = st.name;
  in.setup.password = st.password;
  in.setup.passkey = st.passkey;
  display::show(since::buildScreen(in));
  g_lastFingerprint = 0;

  const bool portalOk = portal::start();
  const bool bleOk = ble::start();
  Serial.printf("setup: %s portal=%d ble=%d\n", st.name.c_str(), portalOk, bleOk);

  while (!st.exitRequested && millis() - st.lastActivityMs < device::kSetupTimeoutMs) {
    esp_task_wdt_reset();
    portal::loop();
    ble::loop();
    if (portal::rebootRequested()) {
      delay(500);  // Antwort ausliefern
      ble::stop();
      portal::stop();
      ESP.restart();
    }
    delay(2);
  }
  ble::stop();
  portal::stop();
  cfg = since::loadConfig();
}

}  // namespace

void setup() {
  const hw::WakeReason reason = hw::wakeReason();
  Serial.begin(115200);
  if (reason == hw::WakeReason::kPowerOn) {
    const unsigned long start = millis();
    while (!Serial && millis() - start < 1500) delay(10);
  }
  Serial.printf("since %s, wake=%d\n", SINCE_FW_VERSION, static_cast<int>(reason));
  setupWatchdog();
  releaseWakePins();

  hw::railOn();
  hw::rtcBegin();
  hw::rtcClearAlarm();

  since::Config cfg = since::loadConfig();
  const int mv = hw::batteryMillivolts();
  auto level = since::batteryLevel(mv, static_cast<since::BatteryLevel>(g_batteryLevel));
  if (reason == hw::WakeReason::kBrownout) level = since::BatteryLevel::kCritical;
  g_batteryLevel = static_cast<int>(level);
  const bool critical = level == since::BatteryLevel::kCritical;

  bool forceRedraw = reason == hw::WakeReason::kPowerOn;
  bool setupMode = false;
  switch (reason) {
    case hw::WakeReason::kKey0:
      if (heldForFactoryReset()) {
        Serial.println("factory reset");
        since::eraseConfig();
        device::clearSyncState();
        ble::clearBonds();
        g_lastFingerprint = 0;
        ESP.restart();
      }
      setupMode = true;
      break;
    case hw::WakeReason::kKey1:
      forceRedraw = true;
      break;
    case hw::WakeReason::kKey2:
      cfg.layout = cfg.layout == since::Layout::kStandard ? since::Layout::kMinimal
                                                          : since::Layout::kStandard;
      since::saveConfig(cfg);
      break;
    default:
      break;
  }

  if (setupMode && reason != hw::WakeReason::kBrownout) {
    runSetup(cfg, mv, level);
    forceRedraw = true;
  }

  bool lostPower = !hw::rtcOk() || hw::rtcLostPower();
  since::Epoch now = hw::rtcNow();

  if (!critical && !setupMode &&
      since::isSyncDue(cfg, now, device::lastSync(), device::lastSyncAttempt(), lostPower)) {
    const since::Epoch synced = ntp::sync(cfg);
    device::storeSync(synced != 0 ? synced : now, synced != 0);
    if (synced != 0) {
      now = synced;
      lostPower = false;
    }
  }

  since::CycleInputs ci;
  ci.config = cfg;
  ci.nowUtc = now;
  ci.rtcLostPower = lostPower;
  ci.batteryMv = mv;
  ci.previousLevel = level;
  ci.brownout = reason == hw::WakeReason::kBrownout;
  ci.forceRedraw = forceRedraw;
  ci.lastFingerprint = g_lastFingerprint;
  const since::CycleResult cr = since::runCycle(ci);  // gleiche Logik wie im Emulator
  if (cr.draw) display::show(cr.model);
  g_lastFingerprint = cr.fingerprint;
  const since::Epoch wake = cr.nextWakeUtc;
  Serial.printf("sleep until %lld (in %lld s)\n", static_cast<long long>(wake),
                static_cast<long long>(wake - now));
  hw::deepSleepUntil(now, wake);
}

void loop() {}  // nie erreicht
