// Portiert aus firmware/test/test_tz/test_main.cpp.
import { DateTime, toSeconds } from './civil';
import { DEFAULT_TZ, TimeZone } from './tz';

const utc = (y: number, mo: number, d: number, h: number, mi: number, s = 0) =>
  toSeconds({ year: y, month: mo, day: d, hour: h, minute: mi, second: s });

function berlin(): TimeZone {
  const tz = TimeZone.parse(DEFAULT_TZ);
  expect(tz).not.toBeNull();
  return tz!;
}

describe('TimeZone.parse', () => {
  it.each([
    'UTC0',
    'CET-1CEST,M3.5.0,M10.5.0/3',
    'EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00',
    '<+0530>-5:30',
    'EST5EDT',
    'XXX3YYY,J60/0,300',
    'GMT0BST,M3.5.0/1,M10.5.0',
  ])('accepts %s', (spec) => {
    expect(TimeZone.parse(spec)).not.toBeNull();
  });

  it.each([
    '',
    'CET',
    'C-1',
    'CET-1CEST,M13.5.0,M10.5.0',
    'CET-1CEST,M3.5.0',
    'CET-1CEST,M3.5.0,M10.5.0/3x',
    'Europe/Berlin',
    'Mars',
  ])('rejects %j', (spec) => {
    expect(TimeZone.parse(spec)).toBeNull();
  });

  it('knows whether a zone has DST', () => {
    expect(TimeZone.parse('UTC0')!.hasDst()).toBe(false);
    expect(berlin().hasDst()).toBe(true);
  });
});

describe('TimeZone offsets', () => {
  it('Berlin winter and summer', () => {
    const tz = berlin();
    expect(tz.offsetAt(utc(2026, 1, 15, 12, 0))).toBe(3600);
    expect(tz.offsetAt(utc(2026, 7, 15, 12, 0))).toBe(7200);
  });

  it('Berlin transition instants', () => {
    const tz = berlin();
    expect(tz.isDstAt(utc(2026, 3, 29, 0, 59, 59))).toBe(false);
    expect(tz.isDstAt(utc(2026, 3, 29, 1, 0, 0))).toBe(true);
    expect(tz.isDstAt(utc(2026, 10, 25, 0, 59, 59))).toBe(true);
    expect(tz.isDstAt(utc(2026, 10, 25, 1, 0, 0))).toBe(false);
  });

  it('local round trip', () => {
    const tz = berlin();
    const local: DateTime = { year: 2026, month: 3, day: 12, hour: 8, minute: 15, second: 0 };
    const u = tz.toUtc(local);
    expect(u).toBe(utc(2026, 3, 12, 7, 15));
    expect(tz.toLocal(u)).toEqual(local);
  });

  it('DST gap moves forward, overlap takes first', () => {
    const tz = berlin();
    expect(tz.toUtc({ year: 2026, month: 3, day: 29, hour: 2, minute: 30, second: 0 })).toBe(
      utc(2026, 3, 29, 1, 30),
    );
    expect(tz.toUtc({ year: 2026, month: 10, day: 25, hour: 2, minute: 30, second: 0 })).toBe(
      utc(2026, 10, 25, 0, 30),
    );
  });

  it('US and southern hemisphere', () => {
    const ny = TimeZone.parse('EST5EDT,M3.2.0,M11.1.0')!;
    expect(ny.offsetAt(utc(2026, 1, 10, 12, 0))).toBe(-5 * 3600);
    expect(ny.offsetAt(utc(2026, 3, 8, 7, 0))).toBe(-4 * 3600);
    const syd = TimeZone.parse('AEST-10AEDT,M10.1.0,M4.1.0/3')!;
    expect(syd.offsetAt(utc(2026, 1, 10, 0, 0))).toBe(11 * 3600);
    expect(syd.offsetAt(utc(2026, 7, 10, 0, 0))).toBe(10 * 3600);
  });

  it('fixed offset with minutes', () => {
    expect(TimeZone.parse('<+0530>-5:30')!.offsetAt(0)).toBe(5 * 3600 + 1800);
  });

  it('UK rules', () => {
    const uk = TimeZone.parse('GMT0BST,M3.5.0/1,M10.5.0')!;
    expect(uk.offsetAt(utc(2026, 1, 10, 12, 0))).toBe(0);
    expect(uk.offsetAt(utc(2026, 7, 10, 12, 0))).toBe(3600);
  });
});
