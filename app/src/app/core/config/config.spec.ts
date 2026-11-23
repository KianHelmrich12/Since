// Kontrakttests gegen shared/fixtures/config und Port von firmware/test/test_config.
import {
  CONFIG_SCHEMA_VERSION,
  Config,
  ConfigResult,
  applyJson,
  defaultConfig,
  formatIsoLocal,
  parseIsoLocal,
  toJson,
  utf8Length,
  validate,
} from './config';
import { loadFixtures, readFixture } from './fixtures.testing';
import { DEFAULT_TZ } from './tz';

const hasIssue = (r: ConfigResult, field: string, code: string) =>
  r.issues.some((i) => i.field === field && i.code === code);

function applyOk(json: string | unknown, cfg: Config = defaultConfig()): Config {
  const r = applyJson(json, cfg);
  expect(r.status, JSON.stringify(r.issues)).toBe('ok');
  return r.config!;
}

describe('shared fixtures (contract)', () => {
  const valid = loadFixtures('valid');
  const invalid = loadFixtures('invalid');

  it('finds the fixture files', () => {
    expect(valid.length).toBeGreaterThanOrEqual(3);
    expect(invalid.length).toBeGreaterThanOrEqual(8);
  });

  it.each(valid.map((f) => [f.name, f.text]))('accepts valid/%s', (_name, text) => {
    const r = applyJson(text, defaultConfig());
    expect(r.status, JSON.stringify(r.issues)).toBe('ok');
  });

  it.each(invalid.map((f) => [f.name, f.text]))(
    'rejects invalid/%s and leaves the config unchanged',
    (_name, text) => {
      const c = defaultConfig();
      c.name = 'Unverändert';
      const before = JSON.stringify(c);
      const r = applyJson(text, c);
      expect(r.status).not.toBe('ok');
      expect(r.config).toBeUndefined();
      expect(JSON.stringify(c)).toBe(before);
    },
  );

  it.each(valid.map((f) => [f.name, f.text]))('round-trips valid/%s', (_name, text) => {
    const a = applyOk(text);
    const b = applyOk(toJson(a, true));
    expect(toJson(b, true)).toEqual(toJson(a, true));
  });

  it('expects the specific error status per invalid fixture', () => {
    const status = (name: string) =>
      applyJson(readFixture('invalid', name), defaultConfig()).status;
    expect(status('not_json.json')).toBe('invalid_json');
    expect(status('wrong_types.json')).toBe('wrong_type');
    expect(status('future_schema.json')).toBe('unsupported_version');
    expect(status('bad_event.json')).toBe('invalid');
    expect(status('bad_interval.json')).toBe('invalid');
    expect(status('bad_tz.json')).toBe('invalid');
    expect(status('ntp_without_wifi.json')).toBe('invalid');
    expect(status('short_password.json')).toBe('invalid');
  });
});

describe('config', () => {
  it('defaults are valid', () => {
    const c = defaultConfig();
    expect(validate(c)).toEqual([]);
    expect(c.schemaVersion).toBe(CONFIG_SCHEMA_VERSION);
    expect(c.event).toBeNull();
    expect(c.tz).toBe(DEFAULT_TZ);
    expect(c.updateMode).toBe('auto');
    expect(c.ntp).toBe(false);
    expect(c.portalSecure).toBe(true);
  });

  it('reads the values of full.json', () => {
    const c = applyOk(readFixture('valid', 'full.json'));
    expect(c.name).toBe('Mila');
    expect(c.event?.year).toBe(2023);
    expect(c.event?.minute).toBe(15);
    expect(c.wifiSsid).toBe('Heimnetz');
    expect(c.wifiPassword).toBe('geheim123');
    expect(c.ntp).toBe(true);
    expect(c.portalSecure).toBe(false);
  });

  it('API JSON hides the password and writing it back keeps it', () => {
    const c = applyOk(readFixture('valid', 'full.json'));
    const api = toJson(c, false);
    expect(JSON.stringify(api)).not.toContain('geheim123');
    expect(api.wifi.passwordSet).toBe(true);
    expect(applyOk(api, c).wifiPassword).toBe('geheim123');
  });

  it('merges partial updates; empty event clears it', () => {
    let c = applyOk(readFixture('valid', 'full.json'));
    c = applyOk('{"name":"Ben"}', c);
    expect(c.name).toBe('Ben');
    expect(c.occasion).toBe('seit deiner Geburt');
    expect(c.event).not.toBeNull();
    c = applyOk('{"event":""}', c);
    expect(c.event).toBeNull();
  });

  it('status codes', () => {
    const c = defaultConfig();
    expect(applyJson('[1,2]', c).status).toBe('invalid_json');
    expect(applyJson('{', c).status).toBe('invalid_json');
    expect(applyJson('{"name":1}', c).status).toBe('wrong_type');
    expect(applyJson('{"schemaVersion":"1"}', c).status).toBe('wrong_type');
    expect(applyJson('{"schemaVersion":2}', c).status).toBe('unsupported_version');
    expect(applyJson('{"schemaVersion":0}', c).status).toBe('unsupported_version');
    expect(applyJson('{"layout":"riesig"}', c).status).toBe('invalid');
    expect(applyJson('{"update":{"mode":"oft"}}', c).status).toBe('invalid');
    expect(applyJson('{"update":5}', c).status).toBe('wrong_type');
    expect(applyJson('{"wifi":"x"}', c).status).toBe('wrong_type');
    expect(applyJson('{"update":{"intervalMinutes":10.5}}', c).status).toBe('wrong_type');
  });

  it('validation issue fields', () => {
    const c = defaultConfig();
    expect(
      hasIssue(applyJson('{"wifi":{"ssid":"N","password":"kurz"}}', c), 'wifi.password', 'length'),
    ).toBe(true);
    expect(hasIssue(applyJson('{"ntp":true}', c), 'ntp', 'requires_wifi')).toBe(true);
    expect(
      hasIssue(
        applyJson('{"update":{"mode":"interval","intervalMinutes":2000}}', c),
        'update.intervalMinutes',
        'out_of_range',
      ),
    ).toBe(true);
    expect(hasIssue(applyJson('{"tz":"Mars"}', c), 'tz', 'invalid')).toBe(true);
    expect(hasIssue(applyJson(`{"tz":"${'A'.repeat(65)}"}`, c), 'tz', 'too_long')).toBe(true);
    expect(
      hasIssue(applyJson(`{"wifi":{"ssid":"${'s'.repeat(33)}"}}`, c), 'wifi.ssid', 'too_long'),
    ).toBe(true);
    expect(
      hasIssue(
        applyJson(`{"wifi":{"ssid":"N","password":"${'p'.repeat(64)}"}}`, c),
        'wifi.password',
        'length',
      ),
    ).toBe(true);
    expect(applyJson(`{"wifi":{"ssid":"N","password":"${'p'.repeat(63)}"}}`, c).status).toBe('ok');
    expect(applyJson('{"wifi":{"ssid":"N","password":""}}', c).status).toBe('ok');
  });

  it('interval is only checked in interval mode', () => {
    expect(
      applyJson('{"update":{"mode":"auto","intervalMinutes":1}}', defaultConfig()).status,
    ).toBe('ok');
    expect(
      applyJson('{"update":{"mode":"interval","intervalMinutes":10}}', defaultConfig()).status,
    ).toBe('ok');
    expect(
      applyJson('{"update":{"mode":"interval","intervalMinutes":1440}}', defaultConfig()).status,
    ).toBe('ok');
    expect(
      applyJson('{"update":{"mode":"interval","intervalMinutes":9}}', defaultConfig()).status,
    ).toBe('invalid');
  });

  it('name length counts code points', () => {
    const c = defaultConfig();
    c.name = 'Ä'.repeat(32);
    expect(validate(c)).toEqual([]);
    c.name += 'x';
    expect(validate(c)).toEqual([{ field: 'name', code: 'too_long' }]);
    c.name = '\uD800';
    expect(validate(c)[0].code).toBe('invalid_utf8');
    c.name = '';
    c.occasion = '🎂'.repeat(64);
    expect(validate(c)).toEqual([]);
  });

  it('utf8Length', () => {
    expect(utf8Length('')).toBe(0);
    expect(utf8Length('Jörg')).toBe(4);
    expect(utf8Length('🎂')).toBe(1);
    expect(utf8Length('\uDC00')).toBe(-1);
  });

  it('ISO parse and format', () => {
    const dt = parseIsoLocal('2023-03-12T08:15');
    expect(dt).not.toBeNull();
    expect(formatIsoLocal(dt!)).toBe('2023-03-12T08:15');
    expect(parseIsoLocal('2023-03-12T08:15:30')?.second).toBe(30);
    expect(parseIsoLocal('2023-3-12T08:15')).toBeNull();
    expect(parseIsoLocal('2023-03-12 08:15')).toBeNull();
    expect(parseIsoLocal('2023-03-12T25:00')).toBeNull();
    expect(parseIsoLocal('2023-03-12T08:15Z')).toBeNull();
    expect(parseIsoLocal('2023-02-29T00:00')).toBeNull();
    expect(parseIsoLocal('2024-02-29T00:00')).not.toBeNull();
  });

  it('rejects signs and spaces in ISO dates like the firmware', () => {
    expect(parseIsoLocal('2026-03-12T08:15:+3')).toBeNull();
    expect(parseIsoLocal('2026-03- 2T08:15')).toBeNull();
    expect(parseIsoLocal('2026-+3-12T08:15')).toBeNull();
  });

  it('keeps non-zero seconds when formatting', () => {
    const dt = parseIsoLocal('2023-03-12T08:15:30')!;
    expect(formatIsoLocal(dt)).toBe('2023-03-12T08:15:30');
    expect(formatIsoLocal(parseIsoLocal('2023-03-12T08:15')!)).toBe('2023-03-12T08:15');
  });

  it('drops the stored password when the SSID changes without a new one', () => {
    let c = applyJson('{"wifi":{"ssid":"A","password":"geheim123"}}', defaultConfig()).config!;
    c = applyJson('{"wifi":{"ssid":"A"}}', c).config!;
    expect(c.wifiPassword).toBe('geheim123');
    c = applyJson('{"wifi":{"ssid":"Offen"}}', c).config!;
    expect(c.wifiPassword).toBe('');
    c = applyJson('{"wifi":{"password":"geheim123"}}', c).config!;
    c = applyJson('{"wifi":{"password":""}}', c).config!;
    expect(c.wifiPassword).toBe('');
  });
});
