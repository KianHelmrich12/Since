// SPDX-License-Identifier: GPL-3.0-or-later
// Hardware-Zugriffe: Panel-Versorgung, RTC (DS3231), Akku, Schlaf.
#pragma once

#include <cstdint>

#include "civil.h"

namespace hw {

// --- Versorgung VCC_3V3 (Panel, Onboard-Flash, DS3231-VCC)
void railOn();
void railOff();

// --- RTC
bool rtcBegin();            // I²C starten, DS3231 konfigurieren (railOn() vorher!)
bool rtcOk();
since::Epoch rtcNow();      // UTC, 0 bei Fehler
bool rtcSet(since::Epoch utc);
bool rtcLostPower();        // OSF-Flag
bool rtcSetAlarm(since::Epoch utc);  // Alarm 1 auf Datum/Uhrzeit (UTC)
void rtcClearAlarm();

// --- Akku
int batteryMillivolts();    // Mittelwert, ADC-Teiler kurz eingeschaltet

// --- Schlaf
enum class WakeReason { kPowerOn, kTimer, kRtcAlarm, kKey0, kKey1, kKey2, kBrownout, kOther };
WakeReason wakeReason();
// Tief schlafen bis wakeUtc (RTC-Alarm) bzw. Fallback-Timer, Taster wecken immer.
[[noreturn]] void deepSleepUntil(since::Epoch nowUtc, since::Epoch wakeUtc);

}  // namespace hw
