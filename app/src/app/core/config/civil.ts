// SPDX-License-Identifier: GPL-3.0-or-later
// Kalenderrechnung, portiert aus firmware/lib/since_core/src/civil.cpp.

/** Sekunden seit 1970-01-01T00:00:00 (UTC oder lokale Wanduhr, je nach Kontext). */
export type Epoch = number;

export interface DateTime {
  year: number;
  month: number;
  day: number;
  hour: number;
  minute: number;
  second: number;
}

export function isLeapYear(y: number): boolean {
  return (y % 4 === 0 && y % 100 !== 0) || y % 400 === 0;
}

const DAYS = [31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31];

export function daysInMonth(y: number, m: number): number {
  if (m < 1 || m > 12) return 0;
  return m === 2 && isLeapYear(y) ? 29 : DAYS[m - 1];
}

/** Ganzzahlige Division mit Abrunden gegen 0, wie in C++. */
function idiv(a: number, b: number): number {
  return Math.trunc(a / b);
}

export function daysFromCivil(year: number, m: number, d: number): number {
  const y = year - (m <= 2 ? 1 : 0);
  const era = idiv(y >= 0 ? y : y - 399, 400);
  const yoe = y - era * 400;
  const doy = idiv(153 * (m + (m > 2 ? -3 : 9)) + 2, 5) + d - 1;
  const doe = yoe * 365 + idiv(yoe, 4) - idiv(yoe, 100) + doy;
  return era * 146097 + doe - 719468;
}

export function civilFromDays(days: number): { year: number; month: number; day: number } {
  const z = days + 719468;
  const era = idiv(z >= 0 ? z : z - 146096, 146097);
  const doe = z - era * 146097;
  const yoe = idiv(doe - idiv(doe, 1460) + idiv(doe, 36524) - idiv(doe, 146096), 365);
  const doy = doe - (365 * yoe + idiv(yoe, 4) - idiv(yoe, 100));
  const mp = idiv(5 * doy + 2, 153);
  const day = doy - idiv(153 * mp + 2, 5) + 1;
  const month = mp < 10 ? mp + 3 : mp - 9;
  const year = yoe + era * 400 + (month <= 2 ? 1 : 0);
  return { year, month, day };
}

/** Wochentag 0 = Sonntag … 6 = Samstag. */
export function weekday(days: number): number {
  const w = (days + 4) % 7; // 1970-01-01 war ein Donnerstag
  return w < 0 ? w + 7 : w;
}

export function isValidDateTime(dt: DateTime): boolean {
  return (
    dt.year >= 1900 &&
    dt.year <= 2999 &&
    dt.month >= 1 &&
    dt.month <= 12 &&
    dt.day >= 1 &&
    dt.day <= daysInMonth(dt.year, dt.month) &&
    dt.hour >= 0 &&
    dt.hour <= 23 &&
    dt.minute >= 0 &&
    dt.minute <= 59 &&
    dt.second >= 0 &&
    dt.second <= 59
  );
}

export function toSeconds(dt: DateTime): Epoch {
  return (
    daysFromCivil(dt.year, dt.month, dt.day) * 86400 + dt.hour * 3600 + dt.minute * 60 + dt.second
  );
}

export function fromSeconds(s: Epoch): DateTime {
  let days = idiv(s, 86400);
  let rem = s % 86400;
  if (rem < 0) {
    rem += 86400;
    --days;
  }
  const d = civilFromDays(days);
  return {
    ...d,
    hour: idiv(rem, 3600),
    minute: idiv(rem % 3600, 60),
    second: rem % 60,
  };
}
