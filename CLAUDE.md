# CLAUDE.md – since

## Überblick
**since** zeigt auf einem E-Paper-Display, wie viel Zeit seit einem Ereignis vergangen ist,
z. B. „3 Jahre · 2 Monate · 14 Tage“. Gedacht zuerst als Geschenk zur Geburt eines Kindes,
aber universell (Hochzeit, Jahrestag, Firmengründung). Akkubetrieben, Konfiguration über ein
Webportal, das per Taster startet. Physisches Produkt mit 3D-gedrucktem Gehäuse.

Ziele v1.0:
- Läuft monatelang mit einer Akkuladung (Deep Sleep, E-Paper hält das Bild ohne Strom).
- Funktioniert komplett ohne WLAN beim Empfänger. Die Uhrzeit hält eine DS3231-RTC.
- Einrichtung per Handy: Taster drücken → eigener Hotspot → Webportal.
- Parametrisches 3D-Gehäuse für einen Bambu-Drucker.
- Open Source: Firmware, Hardware-Doku und Gehäuse.

Der Phasenplan steht in [PLAN.md](PLAN.md). Phasen der Reihe nach abarbeiten; die nächste
beginnt erst, wenn die Akzeptanzkriterien der aktuellen erfüllt sind.

## Hardware
| Komponente | Teil | Hinweis |
|---|---|---|
| MCU-Board | Seeed XIAO ePaper Display Board EE04 (XIAO ESP32-S3 Plus) | Laden, JST-Akku mit Schalter, 1× Reset + 3× User-Taster, 24/50-Pin-FPC |
| Display | 7,5″ E-Paper mono 800×480, 24-Pin | Jumper auf EE04 auf 24 Pin |
| RTC | DS3231 (I²C) mit LIR2032 | Power-LED am ZS042-Modul entfernen; INT/SQW als Weckquelle |
| Akku | LiPo 1S 3,7 V ~2000 mAh, JST-PH 2.0, mit Schutzschaltung | Polarität vor dem Einstecken prüfen |

Pinbelegungen des EE04 werden **nie geraten**: nur aus Seeed-Wiki/Schaltplan, mit Quellenangabe
in `firmware/include/board_ee04.h`.

## Tech-Stack
- Firmware: PlatformIO, Arduino-Framework (ESP32 Core 3.x), C++17
- Display: Seeed_GFX (offizielle EE04-Beispiele), Alternative GxEPD2
- RTC: RTClib · Config: Preferences (NVS) · JSON: ArduinoJson 7
- Portal: WebServer + DNSServer (Captive Portal), Assets gzip in LittleFS,
  Vanilla HTML/CSS/JS ohne CDN, Ziel < 30 KB
- Kernlogik (Zeitrechnung, Formatierung, Config-Validierung) als hardwarefreie Bibliothek
  `since_core`, getestet mit `pio test -e native`
- Gehäuse: OpenSCAD (parametrisch), Export STL/3MF für Bambu Studio
- App (Phase 6b): Angular + zenit-ui (`@zenit-hosting/zenit-ui`, Alias `zenit-ui`) + Capacitor
  (Android), BLE via `@capacitor-community/bluetooth-le`; Firmware-Seite NimBLE-Arduino.
  BLE ist zusätzlich zum Portal und nur im Setup-Modus aktiv. Config-Schema wird von since_core
  und App geteilt (`shared/fixtures/`).

## Repo-Struktur
```
since/
├── CLAUDE.md · PLAN.md · README.md · CHANGELOG.md
├── LICENSE                # Firmware: GPL-3.0-or-later
├── firmware/
│   ├── platformio.ini     # envs: ee04, native
│   ├── include/           # board_ee04.h, version.h
│   ├── lib/since_core/    # reine Logik, auf dem Host testbar
│   ├── src/               # main, power, display, portal, rtc, battery
│   ├── data/              # Web-UI → LittleFS
│   └── test/              # Unity-Tests (native)
├── hardware/              # Verdrahtung, BOM · LICENSE: CERN-OHL-S-2.0
├── enclosure/             # OpenSCAD, STL/3MF · LICENSE: CC BY-SA 4.0
├── app/                   # Android-App: Angular + zenit-ui + Capacitor (Phase 6b)
├── shared/fixtures/       # Config-JSON-Fixtures für Firmware- und App-Tests
└── docs/                  # Bauanleitung, Fotos, Strommessungen, ble-protocol.md
```

## Befehle
```sh
cd firmware
pio run -e ee04                # bauen
pio run -e ee04 -t upload      # flashen (USB-C)
pio run -e ee04 -t uploadfs    # Web-UI nach LittleFS
pio device monitor -b 115200   # serielle Ausgabe
pio test -e native             # Host-Tests ohne Hardware
pio run -e ee04 -t buildfs     # LittleFS-Image (Web-UI wird vorher per tools/build_web.py gzip't)
python3 tools/portal_mock.py   # Portal-UI ohne Hardware auf http://localhost:8080/
python3 tools/preview.py       # Display-Vorschauen nach docs/previews/ (nach pio test -e native)
```
Der native Test-Env braucht einen Host-C++-Compiler (gcc oder clang) im PATH.

App (ab Phase 6b, braucht Node LTS, JDK 21, Android SDK):
```sh
cd app
npm ci
npx ng test --watch=false      # Unit-Tests
npx ng serve                   # Browser mit Mock-Gerät
npx cap sync android && (cd android && ./gradlew assembleDebug)
```

## Regeln
- Commit-Nachrichten und Patches enthalten keinerlei Claude-Verweise
  (kein Co-Authored-By, keine Session-Links, kein „Generated with“).
- Conventional Commits, kleine thematische Commits. Nach jeder Phase Häkchen in PLAN.md setzen
  und committen.
- OS-neutral: keine Arch-spezifischen Pfade oder Tools voraussetzen.
- Strom sparen ist ein Feature: WLAN nur im Portal und beim NTP-Sync. Keine Änderung erhöht
  den Sleep-Strom, ohne dass sie dokumentiert ist.
- Zeit intern immer UTC. Lokalzeit nur für Anzeige/Eingabe über POSIX-TZ
  (Default `CET-1CEST,M3.5.0,M10.5.0/3`).
- Alles Testbare gehört in `since_core` und bekommt Tests. `since_core` darf weder Arduino.h
  noch ESP-IDF-Header einbinden.
- Hardware-Schritte als **MANUELL PRÜFEN** kennzeichnen, nie Erfolg annehmen.
- Keine Secrets im Repo (WLAN-Daten nur im NVS des Geräts).
- Bei Designentscheidungen mit mehreren sinnvollen Optionen nachfragen statt raten.
