import { FrameAssembler, decodeJson, encodeJson, splitFrames } from './framing';
import { DeviceError, Request } from './protocol';
import { RpcChannel } from './rpc-channel';

/** Fängt geschriebene Frames ab und setzt sie zu Anfragen zusammen. */
class FakeLink {
  readonly frames: Uint8Array[] = [];
  readonly requests: Request[] = [];
  mtuValue = 247;
  failWrites = false;
  private readonly assembler = new FrameAssembler();

  mtu = () => this.mtuValue;
  write = async (frame: Uint8Array) => {
    if (this.failWrites) throw new Error('GATT write failed');
    this.frames.push(frame);
    const r = this.assembler.push(frame);
    if (r.kind === 'message') this.requests.push(decodeJson(r.bytes) as Request);
  };
}

function reply(channel: RpcChannel, value: unknown, mtu = 247) {
  for (const f of splitFrames(encodeJson(value), mtu)) channel.receive(f);
}

const quiet = <T>(p: Promise<T>) => p.catch(() => undefined);
const flush = () => new Promise((r) => setTimeout(r, 0));

describe('RpcChannel', () => {
  it('sends a request with id and op and resolves with data', async () => {
    const link = new FakeLink();
    const ch = new RpcChannel(link);
    const p = ch.request('getInfo');
    await flush();
    expect(link.requests).toEqual([{ id: 1, op: 'getInfo' }]);
    reply(ch, { id: 1, ok: true, data: { proto: 1, fw: '1.0.0', name: 'since-AB12' } });
    await expect(p).resolves.toEqual({ proto: 1, fw: '1.0.0', name: 'since-AB12' });
    expect(ch.pendingCount).toBe(0);
  });

  it('puts parameters next to id and op', async () => {
    const link = new FakeLink();
    const ch = new RpcChannel(link);
    void quiet(ch.request('setTime', { epoch: 1795000000 }));
    void quiet(ch.request('setConfig', { data: { name: 'Mila' } }));
    await flush();
    expect(link.requests).toEqual([
      { id: 1, op: 'setTime', epoch: 1795000000 },
      { id: 2, op: 'setConfig', data: { name: 'Mila' } },
    ]);
    ch.close();
  });

  it('matches responses by id, also out of order', async () => {
    const ch = new RpcChannel(new FakeLink());
    const a = ch.request('getInfo');
    const b = ch.request('getStatus');
    reply(ch, { id: 2, ok: true, data: { b: true } });
    reply(ch, { id: 1, ok: true, data: { a: true } });
    await expect(a).resolves.toEqual({ a: true });
    await expect(b).resolves.toEqual({ b: true });
  });

  it('ignores responses with unknown ids', async () => {
    const ch = new RpcChannel(new FakeLink());
    const a = ch.request('getInfo');
    reply(ch, { id: 99, ok: true, data: {} });
    expect(ch.pendingCount).toBe(1);
    reply(ch, { id: 1, ok: true });
    await expect(a).resolves.toEqual({});
  });

  it('rejects with the device error code and issues', async () => {
    const ch = new RpcChannel(new FakeLink());
    const p = ch.request('setConfig', { data: { wifi: { password: 'kurz' } } });
    reply(ch, {
      id: 1,
      ok: false,
      error: 'invalid',
      issues: [{ field: 'wifi.password', code: 'length' }],
    });
    const err = await p.catch((e: unknown) => e);
    expect(err).toBeInstanceOf(DeviceError);
    expect((err as DeviceError).code).toBe('invalid');
    expect((err as DeviceError).issues).toEqual([{ field: 'wifi.password', code: 'length' }]);
  });

  it('chunks requests by the negotiated MTU, capped at 247', async () => {
    const link = new FakeLink();
    link.mtuValue = 23;
    const ch = new RpcChannel(link);
    void quiet(ch.request('setConfig', { data: { occasion: 'x'.repeat(60) } }));
    await flush();
    expect(link.frames.length).toBeGreaterThan(1);
    link.frames.forEach((f) => expect(f.length).toBeLessThanOrEqual(20));
    expect(link.requests.length).toBe(1);

    const big = new FakeLink();
    big.mtuValue = 517;
    const ch2 = new RpcChannel(big);
    void quiet(ch2.request('setConfig', { data: { occasion: 'x'.repeat(600) } }));
    await flush();
    big.frames.forEach((f) => expect(f.length).toBeLessThanOrEqual(244));
    ch.close();
    ch2.close();
  });

  it('does not interleave the frames of two requests', async () => {
    const link = new FakeLink();
    link.mtuValue = 30;
    const ch = new RpcChannel(link);
    void quiet(ch.request('setConfig', { data: { name: 'a'.repeat(30) } }));
    void quiet(ch.request('setConfig', { data: { name: 'b'.repeat(30) } }));
    await flush();
    await flush();
    expect(link.requests.map((r) => r.id)).toEqual([1, 2]);
    ch.close();
  });

  it('reassembles multi-frame notifications', async () => {
    const ch = new RpcChannel(new FakeLink());
    const p = ch.request('getConfig');
    reply(ch, { id: 1, ok: true, data: { occasion: 'ö'.repeat(500) } }, 23);
    await expect(p).resolves.toEqual({ occasion: 'ö'.repeat(500) });
  });

  it('times out and ignores the late answer', async () => {
    vi.useFakeTimers();
    try {
      const ch = new RpcChannel(new FakeLink(), { timeoutMs: 1000 });
      const p = ch.request('getStatus');
      const caught = p.catch((e: unknown) => e);
      await vi.advanceTimersByTimeAsync(999);
      expect(ch.pendingCount).toBe(1);
      await vi.advanceTimersByTimeAsync(1);
      expect(((await caught) as DeviceError).code).toBe('timeout');
      expect(ch.pendingCount).toBe(0);
      expect(() => reply(ch, { id: 1, ok: true })).not.toThrow();
    } finally {
      vi.useRealTimers();
    }
  });

  it('rejects requests larger than 4096 bytes without sending', async () => {
    const link = new FakeLink();
    const ch = new RpcChannel(link);
    const err = await ch
      .request('setConfig', { data: { name: 'x'.repeat(5000) } })
      .catch((e: unknown) => e);
    expect((err as DeviceError).code).toBe('too_large');
    expect(link.frames.length).toBe(0);
  });

  it('close() rejects pending requests and refuses new ones', async () => {
    const ch = new RpcChannel(new FakeLink());
    const p = ch.request('getInfo').catch((e: unknown) => e);
    ch.close();
    expect(((await p) as DeviceError).code).toBe('disconnected');
    expect(((await ch.request('getInfo').catch((e: unknown) => e)) as DeviceError).code).toBe(
      'not_connected',
    );
  });

  it('a failing write rejects the request', async () => {
    const link = new FakeLink();
    link.failWrites = true;
    const ch = new RpcChannel(link);
    const err = await ch.request('getInfo').catch((e: unknown) => e);
    expect((err as DeviceError).code).toBe('disconnected');
  });

  it('ignores garbage notifications', async () => {
    const ch = new RpcChannel(new FakeLink());
    const p = ch.request('getInfo');
    ch.receive(Uint8Array.of(0x03, 0xff));
    ch.receive(Uint8Array.of(0x03, ...new TextEncoder().encode('[1]')));
    expect(ch.pendingCount).toBe(1);
    reply(ch, { id: 1, ok: true, data: 1 });
    await expect(p).resolves.toBe(1);
  });
});
