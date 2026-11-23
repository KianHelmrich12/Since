# since – BLE-Protokoll (Version 1)

Gemeinsamer Vertrag zwischen Firmware (`firmware/src/ble.cpp`, NimBLE-Arduino) und App
(`app/`, `@capacitor-community/bluetooth-le`).

## Wann ist BLE aktiv?
BLE läuft nur im **Setup-Modus**. Der Setup-Modus startet mit einem kurzen Druck auf KEY0 und
läuft parallel zum WLAN-Hotspot. Er endet nach 10 min ohne Aktivität, mit `exitSetup` oder im
Portal mit „Fertig“. Außerhalb des Setup-Modus sind Funk und BLE aus, damit der Strom spart.

## Advertising
- Gerätename: `since-XXXX`. XXXX sind die letzten 2 Bytes der WLAN-MAC in Hex, Großbuchstaben, also derselbe Name wie die SSID.
- Advertised Service-UUID: `acdc0440-c5bd-4d15-815e-bced996cd9f9`

## GATT
| Characteristic | UUID | Eigenschaften |
|---|---|---|
| `rx` | `c45a9546-5845-44f9-899a-844c4eaba12d` | Write (mit Response), verschlüsselt + MITM |
| `tx` | `b4bc784e-edea-420a-9637-108af3fab72a` | Notify, verschlüsselt + MITM |

## Sicherheit
- **Kopplung:** LE Secure Connections mit Passkey Entry. Das Gerät zeigt einen zufälligen sechsstelligen Passkey auf dem E-Paper (Setup-Screen), das Handy fragt ihn im System-Dialog ab.
- **Zugriff:** Auf `rx` und `tx` geht nur eine verschlüsselte, authentifizierte Verbindung. Der erste Zugriff löst die Kopplung aus.
- **Nicht über BLE:** Werksreset und Firmware-Update. Beides geht nur am Gerät bzw. im Portal.

## Framing
Nachrichten sind UTF-8-JSON und werden in Frames gestückelt.
```
Byte 0   Flags: 0x01 = FIRST, 0x02 = LAST (eine Nachricht in einem Frame: 0x03)
Byte 1…  Nutzdaten (max. MTU − 3 − 1 Bytes)
```
- **Größe:** Eine Nachricht ist höchstens 4096 Bytes groß. Ein FIRST-Frame verwirft eine halb empfangene Nachricht.
- **MTU:** Die App fordert MTU 247 an. Ist die ausgehandelte MTU kleiner, stückelt sie entsprechend.
- **Richtung:** App → Gerät schreibt Frames auf `rx`, Gerät → App sendet Frames als Notify auf `tx`.

## Anfragen und Antworten
Anfrage:
```json
{ "id": 7, "op": "getConfig" }
```
Antwort, gleiche `id`:
```json
{ "id": 7, "ok": true, "data": { … } }
{ "id": 7, "ok": false, "error": "invalid", "issues": [ { "field": "wifi.password", "code": "length" } ] }
```

| `op` | Parameter | `data` der Antwort |
|---|---|---|
| `getInfo` | – | `{ "proto": 1, "fw": "1.0.0", "name": "since-AB12" }` |
| `getConfig` | – | Config-JSON wie `GET /api/config`, also ohne Passwort, mit `wifi.passwordSet` |
| `setConfig` | `"data": { … }` | gespeicherte Config, wie `getConfig`. `data` wird gemergt, wie bei `POST /api/config` |
| `setTime` | `"epoch": 1795000000` | `{ "epoch": … }`, Unix-Sekunden UTC |
| `getStatus` | – | wie `GET /api/status` |
| `redraw` | – | `{}`. Das Display wird nach Verlassen des Setups neu gezeichnet |
| `exitSetup` | – | `{}`. Danach beendet das Gerät BLE und Hotspot und zeichnet neu |

Fehlercodes in `error`:

| Code | Bedeutung |
|---|---|
| `bad_request` | kein JSON, `id` oder `op` fehlt |
| `unknown_op` | unbekannte `op` |
| `invalid_json` | Config ist kein gültiges JSON |
| `wrong_type` | Feld mit falschem Typ |
| `unsupported_version` | Config-Schema-Version wird nicht unterstützt |
| `invalid` | Validierung fehlgeschlagen, Details in `issues` |
| `too_large` | Nachricht größer als 4096 Bytes |
| `rtc_error` | RTC nicht erreichbar |

## Status-Objekt
Gilt für `GET /api/status` und `getStatus`.
```json
{
  "fw": "1.0.0",
  "name": "since-AB12",
  "batteryMv": 3912,
  "batteryPercent": 78,
  "charging": false,
  "rtcOk": true,
  "rtcLostPower": false,
  "nowUtc": 1795000000,
  "lastSync": 1794400000,
  "setupRemainingS": 512
}
```
`lastSync` ist `0`, wenn noch nie synchronisiert wurde. `charging` ist `null`, wenn das Gerät den Ladezustand nicht ermitteln kann.

## Config-Schema
Maßgeblich sind `firmware/lib/since_core/src/config.h` und die Fixtures in `shared/fixtures/config/`.

| Feld | Typ | Default | Regeln |
|---|---|---|---|
| `schemaVersion` | int | 1 | eine neuere Version wird abgelehnt |
| `name` | string | `""` | ≤ 32 Zeichen |
| `occasion` | string | `""` | ≤ 64 Zeichen |
| `event` | string | `""` | `YYYY-MM-DDTHH:MM[:SS]` in Lokalzeit; `""` = kein Ereignis |
| `tz` | string | `CET-1CEST,M3.5.0,M10.5.0/3` | POSIX-TZ, ≤ 64 Bytes |
| `layout` | `"standard"` \| `"minimal"` | `standard` | |
| `showTime` | bool | false | zeigt zusätzlich die Stunden |
| `update.mode` | `"auto"` \| `"interval"` | `auto` | |
| `update.intervalMinutes` | int | 60 | 10–1440, wird nur bei `interval` geprüft |
| `wifi.ssid` | string | `""` | ≤ 32 Bytes |
| `wifi.password` | string | – | `""` oder 8–63 Bytes; wird nur geschrieben, nie gelesen |
| `wifi.passwordSet` | bool | – | nur in Antworten |
| `ntp` | bool | false | erfordert `wifi.ssid` |
| `portalSecure` | bool | true | Hotspot mit zufälligem WPA2-Passwort, das das Display anzeigt |
