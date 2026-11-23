import { readFixture } from '../config/fixtures.testing';
import { DeviceError } from '../protocol/protocol';
import { MockDevice } from './mock-device';
import { MockTransport } from './mock-transport';

describe('MockDevice', () => {
  let nowMs = 1_795_000_000_000;
  const device = () => new MockDevice({ now: () => nowMs, rtcDriftS: -60 });

  it('answers getInfo', () => {
    expect(device().handle('{"id":1,"op":"getInfo"}')).toEqual({
      id: 1,
      ok: true,
      data: { proto: 1, fw: '1.0.0', name: 'since-AB12' },
    });
  });

  it('protocol errors', () => {
    const d = device();
    expect(d.handle('kein json')).toMatchObject({ ok: false, error: 'bad_request' });
    expect(d.handle('{"op":"getInfo"}')).toMatchObject({ ok: false, error: 'bad_request' });
    expect(d.handle('{"id":4}')).toMatchObject({ id: 4, ok: false, error: 'bad_request' });
    expect(d.handle('{"id":5,"op":"factoryReset"}')).toEqual({
      id: 5,
      ok: false,
      error: 'unknown_op',
    });
    expect(d.handle(`{"id":6,"op":"getInfo","x":"${'x'.repeat(4096)}"}`)).toMatchObject({
      ok: false,
      error: 'too_large',
    });
  });

  it('setConfig merges, hides the password and reports passwordSet', () => {
    const d = device();
    const data = JSON.parse(readFixture('valid', 'full.json'));
    const r = d.handle(JSON.stringify({ id: 2, op: 'setConfig', data }));
    expect(r['ok']).toBe(true);
    const cfg = r['data'] as Record<string, Record<string, unknown>>;
    expect(cfg['wifi']).toEqual({ ssid: 'Heimnetz', passwordSet: true });
    expect(JSON.stringify(r)).not.toContain('geheim123');
    expect(d.config.wifiPassword).toBe('geheim123');

    const r2 = d.handle('{"id":3,"op":"setConfig","data":{"name":"Ben"}}');
    expect((r2['data'] as Record<string, unknown>)['name']).toBe('Ben');
    expect((r2['data'] as Record<string, unknown>)['occasion']).toBe('seit deiner Geburt');
    expect(d.config.wifiPassword).toBe('geheim123');
  });

  it('setConfig maps config errors to protocol error codes', () => {
    const d = device();
    const send = (data: unknown) => d.handle(JSON.stringify({ id: 9, op: 'setConfig', data }));
    expect(send([1])).toEqual({ id: 9, ok: false, error: 'invalid_json' });
    expect(send({ name: 42 })).toEqual({ id: 9, ok: false, error: 'wrong_type' });
    expect(send({ schemaVersion: 99 })).toEqual({ id: 9, ok: false, error: 'unsupported_version' });
    expect(send({ wifi: { ssid: 'Netz', password: 'kurz' } })).toEqual({
      id: 9,
      ok: false,
      error: 'invalid',
      issues: [{ field: 'wifi.password', code: 'length' }],
    });
    expect(d.config.wifiSsid).toBe('');
  });

  it('setTime sets the clock that getStatus reports', () => {
    const d = device();
    const before = d.handle('{"id":1,"op":"getStatus"}')['data'] as { nowUtc: number };
    expect(before.nowUtc).toBe(1_795_000_000 - 60);
    expect(d.handle('{"id":2,"op":"setTime","epoch":1795000000}')).toEqual({
      id: 2,
      ok: true,
      data: { epoch: 1795000000 },
    });
    nowMs += 5000;
    const after = d.handle('{"id":3,"op":"getStatus"}')['data'] as { nowUtc: number };
    expect(after.nowUtc).toBe(1_795_000_005);
    expect(d.handle('{"id":4,"op":"setTime","epoch":"jetzt"}')).toMatchObject({
      ok: false,
      error: 'bad_request',
    });
    // außerhalb 2024–2100, wie die Firmware
    expect(d.handle('{"id":5,"op":"setTime","epoch":5}')).toMatchObject({
      ok: false,
      error: 'bad_request',
    });
  });

  it('status has every field of the protocol', () => {
    const s = device().handle('{"id":1,"op":"getStatus"}')['data'] as Record<string, unknown>;
    expect(Object.keys(s).sort()).toEqual(
      [
        'batteryMv',
        'batteryPercent',
        'charging',
        'fw',
        'lastSync',
        'name',
        'nowUtc',
        'rtcLostPower',
        'rtcOk',
        'setupRemainingS',
      ].sort(),
    );
  });

  it('redraw and exitSetup', () => {
    const d = device();
    expect(d.handle('{"id":1,"op":"redraw"}')).toEqual({ id: 1, ok: true, data: {} });
    expect(d.redrawRequested).toBe(true);
    expect(d.handle('{"id":2,"op":"exitSetup"}')).toEqual({ id: 2, ok: true, data: {} });
    expect(d.setupEnded).toBe(true);
  });
});

describe('MockTransport', () => {
  const transport = () => new MockTransport({ latencyMs: 1, mtu: 23 });

  it('scans and finds the simulated device', async () => {
    const t = transport();
    const found: string[] = [];
    await t.scan((d) => found.push(d.name), 600);
    expect(found).toEqual(['since-AB12']);
  });

  it('runs requests through framing and the simulated device', async () => {
    const t = transport();
    await t.connect('mock:since-AB12', () => undefined);
    await expect(t.request('getInfo')).resolves.toMatchObject({ name: 'since-AB12', proto: 1 });
    const cfg = await t.request('setConfig', {
      data: {
        name: 'Jörg & Änne',
        occasion: 'seit eurer Hochzeit »Ja«',
        event: '2020-02-29T00:00',
      },
    });
    expect(cfg.name).toBe('Jörg & Änne');
    expect(cfg.event).toBe('2020-02-29T00:00');
    await expect(t.request('getConfig')).resolves.toEqual(cfg);
  });

  it('surfaces validation issues as DeviceError', async () => {
    const t = transport();
    await t.connect('mock:since-AB12', () => undefined);
    const err = await t.request('setConfig', { data: { ntp: true } }).catch((e: unknown) => e);
    expect(err).toBeInstanceOf(DeviceError);
    expect((err as DeviceError).issues).toEqual([{ field: 'ntp', code: 'requires_wifi' }]);
  });

  it('exitSetup ends the connection', async () => {
    const t = transport();
    const dropped = new Promise<void>((resolve) => void t.connect('mock:since-AB12', resolve));
    await new Promise((r) => setTimeout(r, 10));
    await expect(t.request('exitSetup')).resolves.toEqual({});
    await dropped;
    const err = await t.request('getInfo').catch((e: unknown) => e);
    expect((err as DeviceError).code).toBe('not_connected');
  });

  it('rejects requests when not connected and unknown devices', async () => {
    const t = transport();
    expect(((await t.request('getInfo').catch((e: unknown) => e)) as DeviceError).code).toBe(
      'not_connected',
    );
    await expect(t.connect('mock:nope', () => undefined)).rejects.toBeInstanceOf(DeviceError);
  });
});
