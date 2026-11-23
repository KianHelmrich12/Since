// SPDX-License-Identifier: GPL-3.0-or-later
// Kalenderdifferenz zwischen einem Ereignis (lokale Zeit) und „jetzt“ (UTC).
#pragma once

#include <cstdint>

#include "civil.h"
#include "tz.h"

namespace since {

struct Elapsed {
  bool future = false;  // Ereignis liegt in der Zukunft → Countdown-Werte
  int years = 0;
  int months = 0;  // 0..11
  int days = 0;    // 0..30
  int hours = 0;   // 0..23 (echte vergangene Stunden, DST-korrekt)
  int minutes = 0;
  int64_t totalMonths = 0;
  int64_t totalDays = 0;   // volle Tage (lokaler Kalender)
  int64_t totalWeeks = 0;  // totalDays / 7
  int64_t totalSeconds = 0;  // echte Sekunden (UTC-Differenz, immer ≥ 0)
};

// Jahre/Monate/Tage werden im lokalen Kalender gezählt (Geburtstag um die lokale
// Uhrzeit des Ereignisses), Stunden/Minuten als echte Zeit seit dem letzten
// „Tagestag“. Monatsenden werden begrenzt (siehe addMonthsClamped).
Elapsed elapsedSince(const DateTime& eventLocal, Epoch nowUtc, const TimeZone& tz);

}  // namespace since
