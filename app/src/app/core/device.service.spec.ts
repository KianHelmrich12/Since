import { TestBed } from '@angular/core/testing';

import { DeviceService } from './device.service';
import { DeviceError } from './protocol/protocol';
import { DEVICE_TRANSPORT } from './transport/device-transport';
import { MockDevice } from './transport/mock-device';
import { MockTransport } from './transport/mock-transport';

describe('DeviceService with MockTransport', () => {
  let transport: MockTransport;
  let service: DeviceService;

  beforeEach(() => {
    transport = new MockTransport({ latencyMs: 1, mtu: 40, devices: [new MockDevice()] });
    TestBed.configureTestingModule({
      providers: [{ provide: DEVICE_TRANSPORT, useValue: transport }],
    });
    service = TestBed.inject(DeviceService);
  });

  async function connect() {
    await service.connect({ id: 'mock:since-AB12', name: 'since-AB12' });
  }

  it('connects and reads info and config', async () => {
    await connect();
    expect(service.connected()).toBe(true);
    expect(service.info()?.name).toBe('since-AB12');
    expect(service.config()?.tz).toBe('CET-1CEST,M3.5.0,M10.5.0/3');
  });

  it('saves a config patch and keeps the returned config', async () => {
    await connect();
    await service.saveConfig({ name: 'Mila', wifi: { ssid: 'Heimnetz', password: 'geheim123' } });
    expect(service.config()?.name).toBe('Mila');
    expect(service.config()?.wifi).toEqual({ ssid: 'Heimnetz', passwordSet: true });
  });

  it('sends the phone time as whole Unix seconds', async () => {
    await connect();
    const before = Math.floor(Date.now() / 1000);
    const epoch = await service.setTimeFromPhone();
    expect(Number.isInteger(epoch)).toBe(true);
    expect(epoch).toBeGreaterThanOrEqual(before);
    const status = await service.loadStatus();
    expect(Math.abs(status.nowUtc - epoch)).toBeLessThanOrEqual(1);
  });

  it('exitSetup disconnects', async () => {
    await connect();
    await service.exitSetup();
    expect(service.connected()).toBe(false);
    expect(transport.devices[0].setupEnded).toBe(true);
  });

  it('flags a dropped connection', async () => {
    await connect();
    transport.simulateDrop();
    expect(service.connected()).toBe(false);
    expect(service.lostConnection()).toBe(true);
    await expect(service.loadStatus()).rejects.toBeInstanceOf(DeviceError);
  });
});
