// SPDX-License-Identifier: GPL-3.0-or-later
// LiPo-1S-Ladezustand aus der Ruhespannung.
#pragma once

namespace since {

enum class BatteryLevel { kOk, kLow, kCritical };

constexpr int kBatteryLowMv = 3710;        // ≈ 15 % → Warnsymbol
constexpr int kBatteryCriticalMv = 3500;   // ≈ 3 % → „Bitte laden“, lange schlafen
constexpr int kBatteryRecoverMv = 3650;    // Hysterese: erst darüber wieder normal
constexpr int kBatteryAbsentMv = 2500;     // darunter: kein Akku (USB-Betrieb)

// 0..100 nach typischer LiPo-Entladekurve (lineare Interpolation)
int batteryPercent(int millivolts);

// Hysterese: aus kCritical kommt man erst über kBatteryRecoverMv heraus.
BatteryLevel batteryLevel(int millivolts, BatteryLevel previous);

}  // namespace since
