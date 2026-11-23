// SPDX-License-Identifier: GPL-3.0-or-later
#include "tz.h"

#include <cctype>

namespace since {
namespace {

struct Cursor {
  const std::string& s;
  size_t i = 0;
  bool done() const { return i >= s.size(); }
  char peek() const { return done() ? '\0' : s[i]; }
};

bool parseName(Cursor& c) {
  if (c.peek() == '<') {
    ++c.i;
    size_t n = 0;
    while (!c.done() && c.peek() != '>') {
      const char ch = c.peek();
      if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '+' || ch == '-')) return false;
      ++c.i;
      ++n;
    }
    if (c.peek() != '>' || n < 3) return false;
    ++c.i;
    return true;
  }
  size_t n = 0;
  while (std::isalpha(static_cast<unsigned char>(c.peek()))) {
    ++c.i;
    ++n;
  }
  return n >= 3;
}

bool parseNumber(Cursor& c, int maxDigits, int* out) {
  int v = 0, n = 0;
  while (n < maxDigits && std::isdigit(static_cast<unsigned char>(c.peek()))) {
    v = v * 10 + (c.peek() - '0');
    ++c.i;
    ++n;
  }
  if (n == 0) return false;
  *out = v;
  return true;
}

// [+-]hh[:mm[:ss]] → Sekunden. maxHours begrenzt die Stunden.
bool parseHms(Cursor& c, int maxHours, int* out) {
  int sign = 1;
  if (c.peek() == '+' || c.peek() == '-') {
    if (c.peek() == '-') sign = -1;
    ++c.i;
  }
  int h = 0, m = 0, s = 0;
  if (!parseNumber(c, 3, &h) || h > maxHours) return false;
  if (c.peek() == ':') {
    ++c.i;
    if (!parseNumber(c, 2, &m) || m > 59) return false;
    if (c.peek() == ':') {
      ++c.i;
      if (!parseNumber(c, 2, &s) || s > 59) return false;
    }
  }
  *out = sign * (h * 3600 + m * 60 + s);
  return true;
}

}  // namespace

TimeZone TimeZone::utc() { return TimeZone(); }

bool TimeZone::parse(const std::string& spec, TimeZone* out) {
  Cursor c{spec};
  TimeZone tz;
  if (!parseName(c)) return false;
  int off = 0;
  if (!parseHms(c, 24, &off)) return false;
  tz.stdOffset_ = -off;  // POSIX: positiv = westlich von UTC
  if (c.done()) {
    *out = tz;
    return true;
  }
  if (!parseName(c)) return false;
  tz.hasDst_ = true;
  tz.dstOffset_ = tz.stdOffset_ + 3600;
  if (!c.done() && c.peek() != ',') {
    if (!parseHms(c, 24, &off)) return false;
    tz.dstOffset_ = -off;
  }
  if (c.done()) {
    // Keine Regeln angegeben: Default wie glibc (US-Regeln)
    return parse(spec + ",M3.2.0,M11.1.0", out);
  }
  Rule* rules[2] = {&tz.start_, &tz.end_};
  for (Rule* r : rules) {
    if (c.peek() != ',') return false;
    ++c.i;
    if (c.peek() == 'M') {
      ++c.i;
      r->kind = Rule::kMonthWeekDay;
      if (!parseNumber(c, 2, &r->month) || r->month < 1 || r->month > 12) return false;
      if (c.peek() != '.') return false;
      ++c.i;
      if (!parseNumber(c, 1, &r->week) || r->week < 1 || r->week > 5) return false;
      if (c.peek() != '.') return false;
      ++c.i;
      if (!parseNumber(c, 1, &r->wday) || r->wday > 6) return false;
    } else if (c.peek() == 'J') {
      ++c.i;
      r->kind = Rule::kJulian1;
      if (!parseNumber(c, 3, &r->yday) || r->yday < 1 || r->yday > 365) return false;
    } else {
      r->kind = Rule::kJulian0;
      if (!parseNumber(c, 3, &r->yday) || r->yday > 365) return false;
    }
    r->time = 7200;
    if (c.peek() == '/') {
      ++c.i;
      if (!parseHms(c, 167, &r->time)) return false;
    }
  }
  if (!c.done()) return false;
  *out = tz;
  return true;
}

int64_t TimeZone::ruleDay(const Rule& r, int year) const {
  const int64_t jan1 = daysFromCivil(year, 1, 1);
  switch (r.kind) {
    case Rule::kJulian1: {
      // 1..365, der 29.02. wird nie gezählt
      int64_t d = jan1 + r.yday - 1;
      if (isLeapYear(year) && r.yday >= 60) ++d;
      return d;
    }
    case Rule::kJulian0:
      return jan1 + r.yday;
    case Rule::kMonthWeekDay:
    default: {
      const int64_t first = daysFromCivil(year, r.month, 1);
      int day = 1 + (r.wday - weekday(first) + 7) % 7 + (r.week - 1) * 7;
      while (day > daysInMonth(year, r.month)) day -= 7;
      return first + day - 1;
    }
  }
}

void TimeZone::transitions(int year, Epoch* start, Epoch* end) const {
  // Beginn wird in Normalzeit angegeben, Ende in Sommerzeit.
  *start = ruleDay(start_, year) * 86400 + start_.time - stdOffset_;
  *end = ruleDay(end_, year) * 86400 + end_.time - dstOffset_;
}

bool TimeZone::isDstAt(Epoch utc) const {
  if (!hasDst_) return false;
  const int year = fromSeconds(utc + stdOffset_).year;
  Epoch start, end;
  transitions(year, &start, &end);
  if (start < end) return utc >= start && utc < end;
  return utc >= start || utc < end;  // Südhalbkugel
}

int TimeZone::offsetAt(Epoch utc) const { return isDstAt(utc) ? dstOffset_ : stdOffset_; }

DateTime TimeZone::toLocal(Epoch utc) const { return fromSeconds(utc + offsetAt(utc)); }

Epoch TimeZone::toUtc(const DateTime& local) const {
  const Epoch wall = toSeconds(local);
  if (!hasDst_) return wall - stdOffset_;
  const Epoch asDst = wall - dstOffset_;
  const Epoch asStd = wall - stdOffset_;
  if (isDstAt(asDst)) return asDst;  // gültige (oder erste doppelte) Sommerzeit
  return asStd;                      // Normalzeit oder Lücke
}

}  // namespace since
