// SPDX-License-Identifier: GPL-3.0-or-later
#include "format_de.h"

#include <cstdio>

namespace since {

const char* unitName(Unit u, int64_t value, GrammarCase gc) {
  const bool one = value == 1 || value == -1;
  const bool dat = gc == GrammarCase::kDative;
  switch (u) {
    case Unit::kYear:
      return one ? "Jahr" : (dat ? "Jahren" : "Jahre");
    case Unit::kMonth:
      return one ? "Monat" : (dat ? "Monaten" : "Monate");
    case Unit::kWeek:
      return one ? "Woche" : "Wochen";
    case Unit::kDay:
      return one ? "Tag" : (dat ? "Tagen" : "Tage");
    case Unit::kHour:
      return one ? "Stunde" : "Stunden";
    case Unit::kMinute:
    default:
      return one ? "Minute" : "Minuten";
  }
}

std::string formatThousands(int64_t value) {
  const bool neg = value < 0;
  uint64_t v = neg ? static_cast<uint64_t>(-(value + 1)) + 1 : static_cast<uint64_t>(value);
  std::string digits = std::to_string(v);
  std::string out;
  const size_t n = digits.size();
  for (size_t i = 0; i < n; ++i) {
    out += digits[i];
    const size_t left = n - i - 1;
    if (left > 0 && left % 3 == 0) out += '.';
  }
  return neg ? "-" + out : out;
}

std::string formatQuantity(int64_t value, Unit u, GrammarCase gc) {
  return formatThousands(value) + " " + unitName(u, value, gc);
}

std::string formatElapsed(const Elapsed& e, const FormatOptions& opt) {
  struct Part {
    bool enabled;
    int64_t value;
    Unit unit;
  };
  const Part parts[] = {
      {opt.years, e.years, Unit::kYear},       {opt.months, e.months, Unit::kMonth},
      {opt.days, e.days, Unit::kDay},          {opt.hours, e.hours, Unit::kHour},
      {opt.minutes, e.minutes, Unit::kMinute},
  };
  // Deaktivierte größere Einheiten werden in die nächstkleinere aktive umgerechnet,
  // damit z. B. „nur Monate + Tage“ keine Jahre verliert.
  int64_t carryMonths = 0;
  int64_t values[5] = {e.years, e.months, e.days, e.hours, e.minutes};
  if (!opt.years) {
    carryMonths = e.years * 12;
    values[0] = 0;
  }
  values[1] += carryMonths;
  if (!opt.years && !opt.months) {
    values[1] = 0;
    values[2] = e.totalDays;
  }

  std::string out;
  int count = 0;
  int lastEnabled = -1;
  for (int i = 0; i < 5; ++i) {
    if (!parts[i].enabled) continue;
    lastEnabled = i;
    if (values[i] == 0 || count >= opt.maxParts) continue;
    if (count > 0) out += opt.separator;
    out += formatQuantity(values[i], parts[i].unit, opt.grammarCase);
    ++count;
  }
  if (count == 0 && lastEnabled >= 0) {
    out = formatQuantity(0, parts[lastEnabled].unit, opt.grammarCase);
  }
  return out;
}

std::string formatDate(const Date& d) {
  char buf[16];
  std::snprintf(buf, sizeof buf, "%02d.%02d.%04d", d.day, d.month, d.year);
  return buf;
}

std::string formatDateTime(const DateTime& dt) {
  char buf[8];
  std::snprintf(buf, sizeof buf, "%02d:%02d", dt.hour, dt.minute);
  return formatDate(dt.date()) + ", " + buf;
}

}  // namespace since
