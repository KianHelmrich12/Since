// SPDX-License-Identifier: GPL-3.0-or-later
// Entscheidungslogik eines Weck-Zyklus (ohne Hardware). Wird von der Firmware (src/main.cpp)
// und vom Geräte-Emulator (tools/sim) identisch verwendet.
#pragma once

#include <cstdint>

#include "battery.h"
#include "civil.h"
#include "config.h"
#include "screen.h"

namespace since {

struct CycleInputs {
  Config config;
  Epoch nowUtc = 0;             // 0 = RTC nicht lesbar
  bool rtcLostPower = false;
  int batteryMv = 0;
  BatteryLevel previousLevel = BatteryLevel::kOk;
  bool brownout = false;        // Reset-Grund Unterspannung
  bool forceRedraw = false;     // Einschalten, Taste 2, nach Setup
  uint32_t lastFingerprint = 0;
};

struct CycleResult {
  BatteryLevel level = BatteryLevel::kOk;
  bool draw = false;
  ScreenModel model;
  uint32_t fingerprint = 0;  // nach dem Zyklus zu merken (unverändert, wenn nicht gezeichnet)
  Epoch nextWakeUtc = 0;
};

CycleResult runCycle(const CycleInputs& in);

}  // namespace since
