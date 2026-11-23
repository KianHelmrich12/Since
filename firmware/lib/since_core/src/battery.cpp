// SPDX-License-Identifier: GPL-3.0-or-later
#include "battery.h"

namespace since {
namespace {
struct Point {
  int mv;
  int pct;
};
// Typische Ruhespannung einer LiPo-Zelle (absteigend)
const Point kCurve[] = {{4200, 100}, {4150, 95}, {4110, 90}, {4080, 85}, {4020, 80}, {3980, 75},
                        {3950, 70},  {3910, 65}, {3870, 60}, {3850, 55}, {3840, 50}, {3820, 45},
                        {3800, 40},  {3790, 35}, {3770, 30}, {3750, 25}, {3730, 20}, {3710, 15},
                        {3690, 10},  {3610, 5},  {3500, 3},  {3300, 0}};
constexpr int kCount = sizeof(kCurve) / sizeof(kCurve[0]);
}  // namespace

int batteryPercent(int mv) {
  if (mv >= kCurve[0].mv) return 100;
  if (mv <= kCurve[kCount - 1].mv) return 0;
  for (int i = 1; i < kCount; ++i) {
    if (mv >= kCurve[i].mv) {
      const Point hi = kCurve[i - 1];
      const Point lo = kCurve[i];
      return lo.pct + (mv - lo.mv) * (hi.pct - lo.pct) / (hi.mv - lo.mv);
    }
  }
  return 0;
}

BatteryLevel batteryLevel(int mv, BatteryLevel previous) {
  if (mv < kBatteryAbsentMv) return BatteryLevel::kOk;  // USB ohne Akku
  if (mv < kBatteryCriticalMv) return BatteryLevel::kCritical;
  if (previous == BatteryLevel::kCritical && mv < kBatteryRecoverMv) return BatteryLevel::kCritical;
  if (mv < kBatteryLowMv) return BatteryLevel::kLow;
  return BatteryLevel::kOk;
}

}  // namespace since
