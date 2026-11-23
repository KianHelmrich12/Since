// SPDX-License-Identifier: GPL-3.0-or-later
#include "elapsed.h"

namespace since {
namespace {

DateTime withTime(const Date& d, const DateTime& t) {
  return DateTime{d.year, d.month, d.day, t.hour, t.minute, t.second};
}

// Differenz from → to (from ≤ to), beide als lokale Wanduhrzeit.
// toUtcSec ist der echte Zeitpunkt von „to“ (für den Stunden-/Minutenrest).
Elapsed diff(const DateTime& from, const DateTime& to, Epoch toUtcSec, const TimeZone& tz) {
  Elapsed e;
  int64_t months = (to.year - from.year) * 12 + (to.month - from.month);
  // Monatsanker: Datum + n Monate (begrenzt), Uhrzeit des Startpunkts
  auto anchorAt = [&](int64_t n) {
    return withTime(addMonthsClamped(from.date(), static_cast<int>(n)), from);
  };
  while (months > 0 && compare(anchorAt(months), to) > 0) --months;
  const DateTime anchor = anchorAt(months);

  int64_t days = daysFromCivil(to.date()) - daysFromCivil(anchor.date());
  auto dayAt = [&](int64_t n) {
    return withTime(civilFromDays(daysFromCivil(anchor.date()) + n), from);
  };
  while (days > 0 && compare(dayAt(days), to) > 0) --days;
  const DateTime dayAnchor = dayAt(days);

  // Rest in echten Sekunden (berücksichtigt Sommer-/Winterzeit-Wechsel).
  int64_t rest = toUtcSec - tz.toUtc(dayAnchor);
  if (rest < 0) rest = 0;

  int64_t totalDays = daysFromCivil(to.date()) - daysFromCivil(from.date());
  if (totalDays > 0 && compare(withTime(to.date(), from), to) > 0) --totalDays;

  e.totalMonths = months;
  e.years = static_cast<int>(months / 12);
  e.months = static_cast<int>(months % 12);
  e.days = static_cast<int>(days);
  e.hours = static_cast<int>(rest / 3600);
  e.minutes = static_cast<int>((rest % 3600) / 60);
  e.totalDays = totalDays;
  e.totalWeeks = totalDays / 7;
  return e;
}

}  // namespace

Elapsed elapsedSince(const DateTime& eventLocal, Epoch nowUtc, const TimeZone& tz) {
  const Epoch eventUtc = tz.toUtc(eventLocal);
  const DateTime nowLocal = tz.toLocal(nowUtc);
  Elapsed e;
  if (nowUtc >= eventUtc) {
    // Ereignislokalzeit kann in einer DST-Lücke liegen → normalisieren
    e = diff(tz.toLocal(eventUtc), nowLocal, nowUtc, tz);
    e.totalSeconds = nowUtc - eventUtc;
  } else {
    e = diff(nowLocal, tz.toLocal(eventUtc), eventUtc, tz);
    e.future = true;
    e.totalSeconds = eventUtc - nowUtc;
  }
  return e;
}

}  // namespace since
