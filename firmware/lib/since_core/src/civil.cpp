// SPDX-License-Identifier: GPL-3.0-or-later
#include "civil.h"

namespace since {

bool isLeapYear(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }

int daysInMonth(int y, int m) {
  static const int kDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  if (m < 1 || m > 12) return 0;
  return (m == 2 && isLeapYear(y)) ? 29 : kDays[m - 1];
}

int64_t daysFromCivil(int y, int m, int d) {
  y -= m <= 2;
  const int64_t era = (y >= 0 ? y : y - 399) / 400;
  const int64_t yoe = y - era * 400;
  const int64_t doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  const int64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097 + doe - 719468;
}

Date civilFromDays(int64_t z) {
  z += 719468;
  const int64_t era = (z >= 0 ? z : z - 146096) / 146097;
  const int64_t doe = z - era * 146097;
  const int64_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const int64_t doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const int64_t mp = (5 * doy + 2) / 153;
  const int d = static_cast<int>(doy - (153 * mp + 2) / 5 + 1);
  const int m = static_cast<int>(mp < 10 ? mp + 3 : mp - 9);
  const int y = static_cast<int>(yoe + era * 400 + (m <= 2));
  return Date{y, m, d};
}

int weekday(int64_t days) {
  const int64_t w = (days + 4) % 7;  // 1970-01-01 war ein Donnerstag
  return static_cast<int>(w < 0 ? w + 7 : w);
}

bool isValidDate(const Date& d) {
  return d.year >= 1900 && d.year <= 2999 && d.month >= 1 && d.month <= 12 && d.day >= 1 &&
         d.day <= daysInMonth(d.year, d.month);
}

bool isValidDateTime(const DateTime& dt) {
  return isValidDate(dt.date()) && dt.hour >= 0 && dt.hour <= 23 && dt.minute >= 0 &&
         dt.minute <= 59 && dt.second >= 0 && dt.second <= 59;
}

Epoch toSeconds(const DateTime& dt) {
  return daysFromCivil(dt.year, dt.month, dt.day) * 86400 + dt.hour * 3600 + dt.minute * 60 +
         dt.second;
}

DateTime fromSeconds(Epoch s) {
  int64_t days = s / 86400;
  int64_t rem = s % 86400;
  if (rem < 0) {
    rem += 86400;
    --days;
  }
  const Date d = civilFromDays(days);
  return DateTime{d.year,
                  d.month,
                  d.day,
                  static_cast<int>(rem / 3600),
                  static_cast<int>((rem % 3600) / 60),
                  static_cast<int>(rem % 60)};
}

Date addMonthsClamped(const Date& d, int months) {
  int total = d.year * 12 + (d.month - 1) + months;
  int y = total / 12;
  int m = total % 12;
  if (m < 0) {
    m += 12;
    --y;
  }
  ++m;
  const int dim = daysInMonth(y, m);
  return Date{y, m, d.day > dim ? dim : d.day};
}

int compare(const DateTime& a, const DateTime& b) {
  const Epoch x = toSeconds(a);
  const Epoch y = toSeconds(b);
  return x < y ? -1 : (x > y ? 1 : 0);
}

}  // namespace since
