import { FrameAssembler, decodeJson, encodeJson, payloadSize, splitFrames } from './framing';
import { FLAG_FIRST, FLAG_LAST, MAX_MESSAGE_BYTES } from './protocol';

const bytes = (n: number) => Uint8Array.from({ length: n }, (_, i) => i % 251);

function reassemble(frames: Uint8Array[]): Uint8Array {
  const a = new FrameAssembler();
  let out: Uint8Array | undefined;
  frames.forEach((f, i) => {
    const r = a.push(f);
    if (i < frames.length - 1) expect(r.kind).toBe('partial');
    else if (r.kind === 'message') out = r.bytes;
  });
  expect(out).toBeDefined();
  return out!;
}

describe('payloadSize', () => {
  it('is MTU - 3 - 1', () => {
    expect(payloadSize(247)).toBe(243);
    expect(payloadSize(23)).toBe(19);
    expect(payloadSize(5)).toBe(1);
  });
  it('rejects MTUs without room for payload', () => {
    expect(() => payloadSize(4)).toThrow(RangeError);
  });
});

describe('splitFrames', () => {
  it('one frame with 0x03 for a short message', () => {
    const frames = splitFrames(bytes(10), 247);
    expect(frames.length).toBe(1);
    expect(frames[0][0]).toBe(FLAG_FIRST | FLAG_LAST);
    expect(frames[0].length).toBe(11);
  });

  it('empty message is a single 0x03 frame', () => {
    const frames = splitFrames(new Uint8Array(0), 247);
    expect(frames).toEqual([Uint8Array.of(0x03)]);
  });

  it.each([
    [243, 1],
    [244, 2],
    [486, 2],
    [487, 3],
  ])('%i bytes at MTU 247 give %i frames', (n, count) => {
    const frames = splitFrames(bytes(n), 247);
    expect(frames.length).toBe(count);
    frames.forEach((f) => expect(f.length).toBeLessThanOrEqual(244));
    expect(frames[0][0] & FLAG_FIRST).toBe(FLAG_FIRST);
    expect(frames.at(-1)![0] & FLAG_LAST).toBe(FLAG_LAST);
    frames.slice(1, -1).forEach((f) => expect(f[0]).toBe(0));
    if (count > 1) {
      expect(frames[0][0]).toBe(FLAG_FIRST);
      expect(frames.at(-1)![0]).toBe(FLAG_LAST);
    }
  });

  it.each([23, 24, 100, 185, 247])('round-trips 4096 bytes at MTU %i', (mtu) => {
    const msg = bytes(MAX_MESSAGE_BYTES);
    const frames = splitFrames(msg, mtu);
    expect(frames.length).toBe(Math.ceil(MAX_MESSAGE_BYTES / (mtu - 4)));
    expect(reassemble(frames)).toEqual(msg);
  });
});

describe('FrameAssembler', () => {
  it('FIRST resets a half received message', () => {
    const a = new FrameAssembler();
    const stale = splitFrames(bytes(300), 100);
    expect(a.push(stale[0]).kind).toBe('partial');
    expect(a.push(stale[1]).kind).toBe('partial');
    expect(a.inProgress).toBe(true);
    const fresh = encodeJson({ id: 1, ok: true });
    const r = a.push(splitFrames(fresh, 247)[0]);
    expect(r.kind).toBe('message');
    expect(r.kind === 'message' && decodeJson(r.bytes)).toEqual({ id: 1, ok: true });
    expect(a.inProgress).toBe(false);
  });

  it('drops continuation frames without FIRST', () => {
    const a = new FrameAssembler();
    expect(a.push(Uint8Array.of(0x00, 1, 2))).toEqual({ kind: 'dropped', reason: 'no_first' });
    expect(a.push(Uint8Array.of(FLAG_LAST, 1, 2))).toEqual({ kind: 'dropped', reason: 'no_first' });
  });

  it('drops empty frames', () => {
    expect(new FrameAssembler().push(new Uint8Array(0))).toEqual({
      kind: 'dropped',
      reason: 'empty',
    });
  });

  it('accepts exactly the maximum and drops anything larger', () => {
    expect(reassemble(splitFrames(bytes(MAX_MESSAGE_BYTES), 247)).length).toBe(MAX_MESSAGE_BYTES);
    const a = new FrameAssembler();
    const frames = splitFrames(bytes(MAX_MESSAGE_BYTES + 1), 247);
    const results = frames.map((f) => a.push(f));
    expect(results.at(-1)).toEqual({ kind: 'dropped', reason: 'too_large' });
    expect(results.some((r) => r.kind === 'message')).toBe(false);
    // danach geht es mit dem nächsten FIRST normal weiter
    expect(a.push(Uint8Array.of(0x03, 0x7b, 0x7d)).kind).toBe('message');
  });

  it('a frame with FIRST|LAST and no payload is an empty message', () => {
    const r = new FrameAssembler().push(Uint8Array.of(0x03));
    expect(r).toEqual({ kind: 'message', bytes: new Uint8Array(0) });
  });

  it('does not keep references to the caller buffer', () => {
    const a = new FrameAssembler();
    const f1 = Uint8Array.of(FLAG_FIRST, 1, 2);
    a.push(f1);
    f1[1] = 99;
    const r = a.push(Uint8Array.of(FLAG_LAST, 3));
    expect(r).toEqual({ kind: 'message', bytes: Uint8Array.of(1, 2, 3) });
  });
});

describe('JSON coding', () => {
  it('keeps UTF-8 intact across frame borders', () => {
    const value = { id: 3, name: 'Jörg & Änne 🎂'.repeat(20) };
    const frames = splitFrames(encodeJson(value), 23);
    expect(decodeJson(reassemble(frames))).toEqual(value);
  });

  it('rejects invalid UTF-8', () => {
    expect(() => decodeJson(Uint8Array.of(0x22, 0xff, 0x22))).toThrow();
  });
});
