// SPDX-License-Identifier: GPL-3.0-or-later
// Minimaler, deterministischer POSIX-TZ-Parser (z. B. "CET-1CEST,M3.5.0,M10.5.0/3").
// Unterstützt: Namen (alphabetisch oder <...>), Offsets [+-]hh[:mm[:ss]],
// Regeln Mm.w.d, Jn und n, jeweils mit optionaler /Zeit (auch negativ / > 24 h).
#pragma once

#include <string>

#include "civil.h"

namespace since {

constexpr const char* kDefaultTz = "CET-1CEST,M3.5.0,M10.5.0/3";

class TimeZone {
 public:
  // Parst eine POSIX-TZ-Zeichenkette. Gibt false zurück, wenn sie ungültig ist;
  // dann bleibt *out unverändert.
  static bool parse(const std::string& spec, TimeZone* out);
  static TimeZone utc();

  // Offset Lokalzeit − UTC in Sekunden für einen UTC-Zeitpunkt.
  int offsetAt(Epoch utc) const;
  bool isDstAt(Epoch utc) const;

  DateTime toLocal(Epoch utc) const;
  // Lokale Wanduhrzeit → UTC. Nicht existierende Zeit (Sprung vorwärts): wird mit dem
  // Normalzeit-Offset interpretiert, landet also eine Stunde später (02:30 → 03:30).
  // Doppelte Zeit (Sprung rückwärts): die erste (Sommerzeit-)Variante gewinnt.
  Epoch toUtc(const DateTime& local) const;

  bool hasDst() const { return hasDst_; }

 private:
  struct Rule {
    enum Kind { kMonthWeekDay, kJulian1, kJulian0 } kind = kMonthWeekDay;
    int month = 0, week = 0, wday = 0;  // Mm.w.d
    int yday = 0;                       // Jn (1..365) bzw. n (0..365)
    int time = 7200;                    // Sekunden nach lokaler Mitternacht
  };
  int64_t ruleDay(const Rule& r, int year) const;  // Tage seit Epoch
  // UTC-Zeitpunkte von DST-Beginn und -Ende im angegebenen Jahr
  void transitions(int year, Epoch* start, Epoch* end) const;

  int stdOffset_ = 0;  // Sekunden östlich von UTC
  int dstOffset_ = 0;
  bool hasDst_ = false;
  Rule start_, end_;
};

}  // namespace since
