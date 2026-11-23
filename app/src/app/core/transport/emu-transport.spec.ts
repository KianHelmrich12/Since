// SPDX-License-Identifier: GPL-3.0-or-later
import { describe, expect, it } from 'vitest';

import { MockDevice } from './mock-device';
import { EmuTransport } from './emu-transport';

/** Fake-Emulator: /emu/state + /api/rpc, beantwortet RPC mit dem MockDevice. */
function fakeEmulator(active = true) {
  const device = new MockDevice({ name: 'since-EMU0' });
  const state = { setup: { active, ssid: 'since-EMU0' } };
  const fn = (async (url: string, init?: RequestInit) => {
    if (url.endsWith('/emu/state')) return new Response(JSON.stringify(state));
    if (url.endsWith('/api/rpc')) {
      const reply = device.handle(new TextEncoder().encode(String(init?.body)));
      return new Response(JSON.stringify(reply));
    }
    return new Response('{}', { status: 404 });
  }) as typeof fetch;
  return { fn, state, device };
}

describe('EmuTransport', () => {
  it('finds the device only in setup mode', async () => {
    const emu = fakeEmulator(false);
    const t = new EmuTransport('http://emu', emu.fn);
    const found: string[] = [];
    await t.scan((d) => found.push(d.name), 300);
    expect(found).toEqual([]);
    emu.state.setup.active = true;
    await t.scan((d) => found.push(d.name), 2000);
    expect(found).toEqual(['since-EMU0']);
  });

  it('runs requests through framing and RPC', async () => {
    const emu = fakeEmulator();
    const t = new EmuTransport('http://emu', emu.fn);
    await t.connect('emu:since-EMU0', () => undefined);
    const info = await t.request('getInfo');
    expect(info.name).toBe('since-EMU0');
    const cfg = await t.request('setConfig', { data: { name: 'Ida' } });
    expect(cfg.name).toBe('Ida');
    await t.disconnect();
  });

  it('refuses to connect outside setup mode', async () => {
    const t = new EmuTransport('http://emu', fakeEmulator(false).fn);
    await expect(t.connect('emu:x', () => undefined)).rejects.toThrow();
  });
});
