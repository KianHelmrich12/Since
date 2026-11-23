// SPDX-License-Identifier: GPL-3.0-or-later
#include "cycle.h"

#include "schedule.h"

namespace since {

CycleResult runCycle(const CycleInputs& in) {
  CycleResult r;
  r.level = in.brownout ? BatteryLevel::kCritical : batteryLevel(in.batteryMv, in.previousLevel);
  const bool critical = r.level == BatteryLevel::kCritical;
  const bool lostPower = in.rtcLostPower || in.nowUtc == 0;

  r.fingerprint = in.lastFingerprint;
  // Bei Brownout nicht zeichnen: das Panel würde den Akku weiter belasten.
  if (!in.brownout) {
    ScreenInputs si;
    si.config = in.config;
    si.nowUtc = in.nowUtc;
    si.batteryMv = in.batteryMv;
    si.battery = r.level;
    si.rtcLostPower = lostPower;
    r.model = buildScreen(si);
    const uint32_t fp = screenFingerprint(r.model);
    r.draw = in.forceRedraw || fp != in.lastFingerprint;
    if (r.draw) r.fingerprint = fp;
  }

  // Ohne gültige Uhrzeit kein sinnvoller Alarm: nur Tasten bzw. täglicher Timer.
  r.nextWakeUtc = lostPower ? in.nowUtc + kMaxSleepS : nextWakeUtc(in.config, in.nowUtc, critical);
  return r;
}

}  // namespace since
