// SPDX-License-Identifier: GPL-3.0-or-later
#include "hw.h"

#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#include "board_ee04.h"

namespace hw {
namespace {

RTC_DS3231 g_rtc;
bool g_rtcOk = false;

constexpr uint8_t kDs3231Addr = 0x68;
constexpr uint8_t kRegControl = 0x0E;
constexpr uint8_t kBitBbsqw = 0x40;   // INT auch im Batteriebetrieb
constexpr uint8_t kBitIntcn = 0x04;   // INT statt Rechteck
constexpr uint8_t kBitA1ie = 0x01;

bool readReg(uint8_t reg, uint8_t* value) {
  Wire.beginTransmission(kDs3231Addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(kDs3231Addr, static_cast<uint8_t>(1)) != 1) return false;
  *value = Wire.read();
  return true;
}

bool writeReg(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(kDs3231Addr);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

// Fallback-Timer: wacht etwas nach dem RTC-Alarm auf, falls INT nicht verdrahtet ist.
constexpr since::Epoch kTimerFallbackS = 60;

}  // namespace

void railOn() {
  pinMode(board::kEpdPowerEnable, OUTPUT);
  digitalWrite(board::kEpdPowerEnable, HIGH);
  delay(5);
}

void railOff() { digitalWrite(board::kEpdPowerEnable, LOW); }

bool rtcBegin() {
  Wire.begin(board::kI2cSda, board::kI2cScl, board::kI2cHz);
  g_rtcOk = g_rtc.begin(&Wire);
  if (!g_rtcOk) return false;
  g_rtc.disable32K();
  uint8_t ctrl = 0;
  if (readReg(kRegControl, &ctrl)) {
    ctrl |= kBitBbsqw | kBitIntcn | kBitA1ie;
    writeReg(kRegControl, ctrl);
  }
  g_rtc.disableAlarm(2);
  return true;
}

bool rtcOk() { return g_rtcOk; }

since::Epoch rtcNow() {
  if (!g_rtcOk) return 0;
  const DateTime t = g_rtc.now();
  return since::toSeconds(since::DateTime{t.year(), t.month(), t.day(), t.hour(), t.minute(),
                                          t.second()});
}

bool rtcSet(since::Epoch utc) {
  if (!g_rtcOk) return false;
  const since::DateTime d = since::fromSeconds(utc);
  g_rtc.adjust(DateTime(d.year, d.month, d.day, d.hour, d.minute, d.second));  // löscht OSF
  return true;
}

bool rtcLostPower() { return g_rtcOk && g_rtc.lostPower(); }

bool rtcSetAlarm(since::Epoch utc) {
  if (!g_rtcOk) return false;
  g_rtc.clearAlarm(1);
  const since::DateTime d = since::fromSeconds(utc);
  return g_rtc.setAlarm1(DateTime(d.year, d.month, d.day, d.hour, d.minute, d.second),
                         DS3231_A1_Date);
}

void rtcClearAlarm() {
  if (g_rtcOk) g_rtc.clearAlarm(1);
}

int batteryMillivolts() {
  pinMode(board::kBatteryAdcEnable, OUTPUT);
  digitalWrite(board::kBatteryAdcEnable, HIGH);
  delay(10);
  uint32_t sum = 0;
  constexpr int kSamples = 16;
  for (int i = 0; i < kSamples; ++i) sum += analogReadMilliVolts(board::kBatteryAdc);
  digitalWrite(board::kBatteryAdcEnable, LOW);
  return static_cast<int>(sum / kSamples) * board::kBatteryDividerNum / board::kBatteryDividerDen;
}

WakeReason wakeReason() {
  if (esp_reset_reason() == ESP_RST_BROWNOUT) return WakeReason::kBrownout;
  switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_TIMER:
      return WakeReason::kTimer;
    case ESP_SLEEP_WAKEUP_EXT1: {
      const uint64_t st = esp_sleep_get_ext1_wakeup_status();
      if (st & (1ULL << board::kKey0)) return WakeReason::kKey0;
      if (st & (1ULL << board::kKey1)) return WakeReason::kKey1;
      if (st & (1ULL << board::kKey2)) return WakeReason::kKey2;
      if (st & (1ULL << board::kRtcInt)) return WakeReason::kRtcAlarm;
      return WakeReason::kOther;
    }
    case ESP_SLEEP_WAKEUP_UNDEFINED:
      return WakeReason::kPowerOn;
    default:
      return WakeReason::kOther;
  }
}

void deepSleepUntil(since::Epoch nowUtc, since::Epoch wakeUtc) {
  if (wakeUtc <= nowUtc) wakeUtc = nowUtc + 60;
  const bool alarmSet = rtcSetAlarm(wakeUtc);
  railOff();  // DS3231 läuft weiter auf der Knopfzelle (BBSQW gesetzt)

  // INT der DS3231 ist Open-Drain: interner Pull-up auf dem RTC-GPIO im Schlaf
  rtc_gpio_init(static_cast<gpio_num_t>(board::kRtcInt));
  rtc_gpio_set_direction(static_cast<gpio_num_t>(board::kRtcInt), RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pullup_en(static_cast<gpio_num_t>(board::kRtcInt));
  rtc_gpio_pulldown_dis(static_cast<gpio_num_t>(board::kRtcInt));
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);  // für den Pull-up

  esp_sleep_enable_ext1_wakeup(board::kWakeMask, ESP_EXT1_WAKEUP_ANY_LOW);
  const since::Epoch delta = wakeUtc - nowUtc + (alarmSet ? kTimerFallbackS : 0);
  esp_sleep_enable_timer_wakeup(static_cast<uint64_t>(delta) * 1000000ULL);
  Serial.flush();
  esp_deep_sleep_start();
}

}  // namespace hw
