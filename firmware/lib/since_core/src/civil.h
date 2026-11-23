// SPDX-License-Identifier: GPL-3.0-or-later
// Kalender-Grundlagen (proleptischer Gregorianischer Kalender), ohne Zeitzonen.
#pragma once

#include <cstdint>

namespace since {

// Sekunden seit 1970-01-01T00:00:00Z (Unix-Zeit, 64 bit).
using Epoch = int64_t;

struct Date {
  int year;
  int month;  // 1..12
  int day;    // 1..31
};

struct DateTime {
  int year;
  int month;   // 1..12
  int day;     // 1..31
  int hour;    // 0..23
  int minute;  // 0..59
  int second;  // 0..59

  Date date() const { return Date{year, month, day}; }
};

bool isLeapYear(int year);
int daysInMonth(int year, int month);

// Tage seit 1970-01-01 (negativ davor). Algorithmus nach H. Hinnant.
int64_t daysFromCivil(int year, int month, int day);
inline int64_t daysFromCivil(const Date& d) { return daysFromCivil(d.year, d.month, d.day); }
Date civilFromDays(int64_t days);

// 0 = Sonntag … 6 = Samstag
int weekday(int64_t days);

bool isValidDate(const Date& d);
bool isValidDateTime(const DateTime& dt);

// DateTime als „Wanduhrzeit“ in Sekunden (ohne Zeitzone, d. h. so als wäre es UTC).
Epoch toSeconds(const DateTime& dt);
DateTime fromSeconds(Epoch s);

// Addiert Monate; ist der Zieltag im Monat nicht vorhanden, wird auf das Monatsende
// begrenzt (31.01. + 1 Monat = 28./29.02.; 29.02. + 12 Monate = 28.02.).
Date addMonthsClamped(const Date& d, int months);

int compare(const DateTime& a, const DateTime& b);

}  // namespace since
