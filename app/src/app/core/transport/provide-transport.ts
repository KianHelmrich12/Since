// SPDX-License-Identifier: GPL-3.0-or-later
// Wählt die Transportschicht: auf dem Handy (Capacitor nativ) Bluetooth LE, im Browser das
// simulierte Gerät. Überschreiben mit ?transport=mock|ble|emu oder localStorage "since.transport".
// Für emu: ?emu=http://localhost:8080 (Adresse von firmware/tools/emulator.py).

import { Provider } from '@angular/core';
import { Capacitor } from '@capacitor/core';

import { applyJson, defaultConfig } from '../config/config';
import { BleTransport } from './ble-transport';
import { DEVICE_TRANSPORT, DeviceTransport } from './device-transport';
import { EmuTransport } from './emu-transport';
import { MockDevice } from './mock-device';
import { MockTransport } from './mock-transport';

export const TRANSPORT_STORAGE_KEY = 'since.transport';
export const EMU_URL_STORAGE_KEY = 'since.emu';
const DEFAULT_EMU_URL = 'http://localhost:8080';

function readOverride(): string | null {
  try {
    const fromUrl = new URLSearchParams(globalThis.location?.search ?? '').get('transport');
    const emu = new URLSearchParams(globalThis.location?.search ?? '').get('emu');
    if (emu) globalThis.localStorage?.setItem(EMU_URL_STORAGE_KEY, emu);
    if (fromUrl === 'mock' || fromUrl === 'ble' || fromUrl === 'emu') {
      globalThis.localStorage?.setItem(TRANSPORT_STORAGE_KEY, fromUrl);
      return fromUrl;
    }
    return globalThis.localStorage?.getItem(TRANSPORT_STORAGE_KEY) ?? null;
  } catch {
    return null;
  }
}

/** Beispielgerät für den Browser: schon eingerichtet, damit alle Seiten etwas zeigen. */
export function demoMockTransport(): MockTransport {
  const config =
    applyJson(
      {
        name: 'Mila',
        occasion: 'seit deiner Geburt',
        event: '2023-03-12T08:15',
        wifi: { ssid: 'Heimnetz', password: 'beispiel123' },
      },
      defaultConfig(),
    ).config ?? defaultConfig();
  return new MockTransport({
    devices: [
      new MockDevice({ name: 'since-AB12', config }),
      new MockDevice({ name: 'since-7F03' }),
    ],
  });
}

function emuUrl(): string {
  try {
    return (globalThis.localStorage?.getItem(EMU_URL_STORAGE_KEY) ?? DEFAULT_EMU_URL).replace(
      /\/$/,
      '',
    );
  } catch {
    return DEFAULT_EMU_URL;
  }
}

export function createTransport(): DeviceTransport {
  const override = readOverride();
  if (override === 'mock') return demoMockTransport();
  if (override === 'ble') return new BleTransport();
  if (override === 'emu') return new EmuTransport(emuUrl());
  return Capacitor.isNativePlatform() ? new BleTransport() : demoMockTransport();
}

export function provideDeviceTransport(): Provider {
  return { provide: DEVICE_TRANSPORT, useFactory: createTransport };
}
