// SPDX-License-Identifier: GPL-3.0-or-later
#include "schedule.h"

#include "tz.h"

namespace since {
namespace {

// Nächster Zeitpunkt > nowUtc, an dem die Lokalzeit hh:mm:ss erreicht (heute oder morgen,
// DST-Lücken werden von toUtc nach vorn verschoben).
Epoch nextLocalTimeOfDay(const TimeZone& tz, Epoch nowUtc, int h, int m, int s) {
  const DateTime local = tz.toLocal(nowUtc);
  const int64_t today = daysFromCivil(local.date());
  for (int64_t d = today; d <= today + 2; ++d) {
    const Date date = civilFromDays(d);
    const DateTime want{date.year, date.month, date.day, h, m, s};
    Epoch t = tz.toUtc(want);
    // Liegt die Wanduhrzeit in einer DST-Lücke, ändert sich der Text schon beim Sprung:
    // frühesten Zeitpunkt suchen, dessen Lokalzeit die gewünschte erreicht.
    const Epoch wantWall = toSeconds(want);
    if (toSeconds(tz.toLocal(t)) != wantWall) {
      Epoch lo = t - 3 * 3600, hi = t;
      while (hi - lo > 1) {
        const Epoch mid = lo + (hi - lo) / 2;
        if (toSeconds(tz.toLocal(mid)) >= wantWall) hi = mid;
        else lo = mid;
      }
      t = hi;
    }
    if (t > nowUtc) return t;
  }
  return nowUtc + kMaxSleepS;
}

// Nächster Zeitpunkt > nowUtc mit Lokal-Minute:Sekunde = m:s
Epoch nextLocalMinuteMark(const TimeZone& tz, Epoch nowUtc, int m, int s) {
  const DateTime local = tz.toLocal(nowUtc);
  const Epoch base = tz.toUtc(DateTime{local.year, local.month, local.day, local.hour, m, s});
  for (int k = -1; k <= 3; ++k) {
    const Epoch t = base + k * 3600;
    if (t > nowUtc) return t;
  }
  return nowUtc + 3600;
}

}  // namespace

Epoch nextWakeUtc(const Config& cfg, Epoch nowUtc, bool batteryCritical) {
  const Epoch cap = nowUtc + kMaxSleepS;
  if (batteryCritical || !cfg.hasEvent) return cap;
  if (cfg.updateMode == UpdateMode::kInterval) {
    const Epoch t = nowUtc + static_cast<Epoch>(cfg.intervalMinutes) * 60;
    return t < cap ? t : cap;
  }
  TimeZone tz;
  if (!TimeZone::parse(cfg.tz, &tz)) tz = TimeZone::utc();

  const DateTime& ev = cfg.event;
  Epoch next = cap;
  auto consider = [&](Epoch t) {
    if (t > nowUtc && t < next) next = t;
  };
  consider(nextLocalTimeOfDay(tz, nowUtc, ev.hour, ev.minute, ev.second));
  consider(nextLocalTimeOfDay(tz, nowUtc, 0, 0, 0));
  if (cfg.showTime) consider(nextLocalMinuteMark(tz, nowUtc, ev.minute, ev.second));
  consider(tz.toUtc(ev));
  return next + kWakeMarginS;
}

bool isSyncDue(const Config& cfg, Epoch nowUtc, Epoch lastSyncUtc, Epoch lastAttemptUtc,
               bool rtcLostPower) {
  if (!cfg.ntp || cfg.wifiSsid.empty()) return false;
  if (lastAttemptUtc > lastSyncUtc && nowUtc - lastAttemptUtc < kSyncRetryS && !rtcLostPower)
    return false;
  if (rtcLostPower || lastSyncUtc == 0) return true;
  return nowUtc - lastSyncUtc >= kSyncIntervalS;
}

}  // namespace since
