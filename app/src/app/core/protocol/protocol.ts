// SPDX-License-Identifier: GPL-3.0-or-later
// BLE-Protokoll Version 1. Verbindlich ist docs/ble-protocol.md.

import { ApiConfig, ConfigPatch, ValidationIssue } from '../config/config';

export const PROTOCOL_VERSION = 1;

export const SERVICE_UUID = 'acdc0440-c5bd-4d15-815e-bced996cd9f9';
/** App → Gerät, Write mit Response */
export const RX_UUID = 'c45a9546-5845-44f9-899a-844c4eaba12d';
/** Gerät → App, Notify */
export const TX_UUID = 'b4bc784e-edea-420a-9637-108af3fab72a';

/** Die App fordert diese MTU an; ist die ausgehandelte kleiner, wird entsprechend gestückelt. */
export const REQUESTED_MTU = 247;
/** Höchstgröße einer Nachricht in Bytes (UTF-8-JSON). */
export const MAX_MESSAGE_BYTES = 4096;

export const FLAG_FIRST = 0x01;
export const FLAG_LAST = 0x02;

export interface DeviceInfo {
  proto: number;
  fw: string;
  name: string;
}

export interface DeviceStatus {
  fw: string;
  name: string;
  batteryMv: number;
  batteryPercent: number;
  /** null, wenn das Gerät den Ladezustand nicht ermitteln kann */
  charging: boolean | null;
  rtcOk: boolean;
  rtcLostPower: boolean;
  /** Unix-Sekunden UTC */
  nowUtc: number;
  /** 0 = noch nie synchronisiert */
  lastSync: number;
  setupRemainingS: number;
}

/** Parameter und Antwortdaten je `op`. */
export interface OpMap {
  getInfo: { params: undefined; result: DeviceInfo };
  getConfig: { params: undefined; result: ApiConfig };
  setConfig: { params: { data: ConfigPatch }; result: ApiConfig };
  setTime: { params: { epoch: number }; result: { epoch: number } };
  getStatus: { params: undefined; result: DeviceStatus };
  redraw: { params: undefined; result: Record<string, never> };
  exitSetup: { params: undefined; result: Record<string, never> };
}

export type Op = keyof OpMap;
export type OpParams<K extends Op> = OpMap[K]['params'];
export type OpResult<K extends Op> = OpMap[K]['result'];

export interface Request {
  id: number;
  op: string;
  [param: string]: unknown;
}

export interface Response {
  id: number;
  ok: boolean;
  data?: unknown;
  error?: string;
  issues?: ValidationIssue[];
}

/** Fehlercodes des Geräts plus die lokalen Fehler der App (timeout, disconnected, …). */
export type DeviceErrorCode =
  | 'bad_request'
  | 'unknown_op'
  | 'invalid_json'
  | 'wrong_type'
  | 'unsupported_version'
  | 'invalid'
  | 'too_large'
  | 'rtc_error'
  | 'timeout'
  | 'disconnected'
  | 'not_connected'
  | 'bad_response'
  | 'ble_unavailable';

export class DeviceError extends Error {
  constructor(
    readonly code: DeviceErrorCode | string,
    readonly issues: ValidationIssue[] = [],
    message?: string,
  ) {
    super(message ?? code);
    this.name = 'DeviceError';
  }
}
