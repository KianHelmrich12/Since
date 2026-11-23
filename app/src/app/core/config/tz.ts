// SPDX-License-Identifier: GPL-3.0-or-later
// Minimaler, deterministischer POSIX-TZ-Parser, portiert aus
// firmware/lib/since_core/src/tz.cpp (gleiche Regeln, gleiche Grenzen).
// Unterstützt: Namen (alphabetisch oder <...>), Offsets [+-]hh[:mm[:ss]],
// Regeln Mm.w.d, Jn und n, jeweils mit optionaler /Zeit (auch negativ / > 24 h).

import {
  DateTime,
  Epoch,
  daysFromCivil,
  daysInMonth,
  fromSeconds,
  isLeapYear,
  toSeconds,
  weekday,
} from './civil';

export const DEFAULT_TZ = 'CET-1CEST,M3.5.0,M10.5.0/3';

type RuleKind = 'mwd' | 'julian1' | 'julian0';

interface Rule {
  kind: RuleKind;
  month: number;
  week: number;
  wday: number;
  yday: number;
  time: number; // Sekunden nach lokaler Mitternacht
}

function newRule(): Rule {
  return { kind: 'mwd', month: 0, week: 0, wday: 0, yday: 0, time: 7200 };
}

// Wie std::isalpha/isalnum/isdigit im "C"-Locale: nur ASCII.
const isAlpha = (ch: string) => /^[A-Za-z]$/.test(ch);
const isDigit = (ch: string) => /^[0-9]$/.test(ch);
const isAlnum = (ch: string) => isAlpha(ch) || isDigit(ch);

class Cursor {
  i = 0;
  constructor(readonly s: string) {}
  done(): boolean {
    return this.i >= this.s.length;
  }
  peek(): string {
    return this.done() ? '\0' : this.s[this.i];
  }
}

function parseName(c: Cursor): boolean {
  if (c.peek() === '<') {
    ++c.i;
    let n = 0;
    while (!c.done() && c.peek() !== '>') {
      const ch = c.peek();
      if (!(isAlnum(ch) || ch === '+' || ch === '-')) return false;
      ++c.i;
      ++n;
    }
    if (c.peek() !== '>' || n < 3) return false;
    ++c.i;
    return true;
  }
  let n = 0;
  while (isAlpha(c.peek())) {
    ++c.i;
    ++n;
  }
  return n >= 3;
}

function parseNumber(c: Cursor, maxDigits: number): number | null {
  let v = 0;
  let n = 0;
  while (n < maxDigits && isDigit(c.peek())) {
    v = v * 10 + Number(c.peek());
    ++c.i;
    ++n;
  }
  return n === 0 ? null : v;
}

/** [+-]hh[:mm[:ss]] → Sekunden. maxHours begrenzt die Stunden. */
function parseHms(c: Cursor, maxHours: number): number | null {
  let sign = 1;
  if (c.peek() === '+' || c.peek() === '-') {
    if (c.peek() === '-') sign = -1;
    ++c.i;
  }
  const h = parseNumber(c, 3);
  if (h === null || h > maxHours) return null;
  let m = 0;
  let s = 0;
  if (c.peek() === ':') {
    ++c.i;
    const mm = parseNumber(c, 2);
    if (mm === null || mm > 59) return null;
    m = mm;
    if (c.peek() === ':') {
      ++c.i;
      const ss = parseNumber(c, 2);
      if (ss === null || ss > 59) return null;
      s = ss;
    }
  }
  return sign * (h * 3600 + m * 60 + s);
}

function parseRule(c: Cursor, r: Rule): boolean {
  if (c.peek() !== ',') return false;
  ++c.i;
  if (c.peek() === 'M') {
    ++c.i;
    r.kind = 'mwd';
    const month = parseNumber(c, 2);
    if (month === null || month < 1 || month > 12) return false;
    r.month = month;
    if (c.peek() !== '.') return false;
    ++c.i;
    const week = parseNumber(c, 1);
    if (week === null || week < 1 || week > 5) return false;
    r.week = week;
    if (c.peek() !== '.') return false;
    ++c.i;
    const wday = parseNumber(c, 1);
    if (wday === null || wday > 6) return false;
    r.wday = wday;
  } else if (c.peek() === 'J') {
    ++c.i;
    r.kind = 'julian1';
    const yday = parseNumber(c, 3);
    if (yday === null || yday < 1 || yday > 365) return false;
    r.yday = yday;
  } else {
    r.kind = 'julian0';
    const yday = parseNumber(c, 3);
    if (yday === null || yday > 365) return false;
    r.yday = yday;
  }
  r.time = 7200;
  if (c.peek() === '/') {
    ++c.i;
    const t = parseHms(c, 167);
    if (t === null) return false;
    r.time = t;
  }
  return true;
}

export class TimeZone {
  private stdOffset = 0; // Sekunden östlich von UTC
  private dstOffset = 0;
  private dst = false;
  private start: Rule = newRule();
  private end: Rule = newRule();

  /** Parst eine POSIX-TZ-Zeichenkette. Gibt null zurück, wenn sie ungültig ist. */
  static parse(spec: string): TimeZone | null {
    const c = new Cursor(spec);
    const tz = new TimeZone();
    if (!parseName(c)) return null;
    let off = parseHms(c, 24);
    if (off === null) return null;
    tz.stdOffset = 0 - off; // POSIX: positiv = westlich von UTC
    if (c.done()) return tz;
    if (!parseName(c)) return null;
    tz.dst = true;
    tz.dstOffset = tz.stdOffset + 3600;
    if (!c.done() && c.peek() !== ',') {
      off = parseHms(c, 24);
      if (off === null) return null;
      tz.dstOffset = 0 - off;
    }
    if (c.done()) {
      // Keine Regeln angegeben: Default wie glibc (US-Regeln)
      return TimeZone.parse(spec + ',M3.2.0,M11.1.0');
    }
    if (!parseRule(c, tz.start)) return null;
    if (!parseRule(c, tz.end)) return null;
    if (!c.done()) return null;
    return tz;
  }

  static utc(): TimeZone {
    return new TimeZone();
  }

  hasDst(): boolean {
    return this.dst;
  }

  /** Offset Lokalzeit − UTC in Sekunden für einen UTC-Zeitpunkt. */
  offsetAt(utc: Epoch): number {
    return this.isDstAt(utc) ? this.dstOffset : this.stdOffset;
  }

  isDstAt(utc: Epoch): boolean {
    if (!this.dst) return false;
    const year = fromSeconds(utc + this.stdOffset).year;
    const [start, end] = this.transitions(year);
    if (start < end) return utc >= start && utc < end;
    return utc >= start || utc < end; // Südhalbkugel
  }

  toLocal(utc: Epoch): DateTime {
    return fromSeconds(utc + this.offsetAt(utc));
  }

  /**
   * Lokale Wanduhrzeit → UTC. Nicht existierende Zeit (Sprung vorwärts) wird mit dem
   * Normalzeit-Offset interpretiert; doppelte Zeit (Sprung rückwärts): Sommerzeit gewinnt.
   */
  toUtc(local: DateTime): Epoch {
    const wall = toSeconds(local);
    if (!this.dst) return wall - this.stdOffset;
    const asDst = wall - this.dstOffset;
    const asStd = wall - this.stdOffset;
    return this.isDstAt(asDst) ? asDst : asStd;
  }

  private ruleDay(r: Rule, year: number): number {
    const jan1 = daysFromCivil(year, 1, 1);
    switch (r.kind) {
      case 'julian1': {
        // 1..365, der 29.02. wird nie gezählt
        let d = jan1 + r.yday - 1;
        if (isLeapYear(year) && r.yday >= 60) ++d;
        return d;
      }
      case 'julian0':
        return jan1 + r.yday;
      case 'mwd':
      default: {
        const first = daysFromCivil(year, r.month, 1);
        let day = 1 + ((r.wday - weekday(first) + 7) % 7) + (r.week - 1) * 7;
        while (day > daysInMonth(year, r.month)) day -= 7;
        return first + day - 1;
      }
    }
  }

  /** UTC-Zeitpunkte von DST-Beginn und -Ende. Beginn in Normalzeit, Ende in Sommerzeit. */
  private transitions(year: number): [Epoch, Epoch] {
    return [
      this.ruleDay(this.start, year) * 86400 + this.start.time - this.stdOffset,
      this.ruleDay(this.end, year) * 86400 + this.end.time - this.dstOffset,
    ];
  }
}
