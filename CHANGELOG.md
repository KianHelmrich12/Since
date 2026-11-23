# Changelog

Format nach [Keep a Changelog](https://keepachangelog.com/de/1.1.0/),
Versionierung nach [Semantic Versioning](https://semver.org/lang/de/).

## [Unreleased]
### Added
- Geräte-Emulator im Browser (`tools/emulator.py`):
  - echtes Firmware-Rendering mit den echten Fonts, Tasten, Zeitraffer, Akku, RTC-Verlust
  - Portal und BLE-RPC für die App (`?transport=emu`)
- Automatischer Emulator-Test mit Prüfung der Zeichenabdeckung der Fonts (CI).
- Validierung: Name und Anlass nur mit Zeichen, die das Display darstellen kann (`unsupported_char`).

### Changed
- Rendering nach `src/render.cpp` ausgelagert (gemeinsam für Gerät und Emulator).
- Display-Fonts FreeUniversal statt logisoso: logisoso enthält kein „·“ (mit dem Emulator gefunden).

### Fixed
- NTP-Sync: Der Sync-Status wurde doppelt gelesen, deshalb wurde die RTC nie gestellt.
- Setup-Ende: Antworten werden noch ausgeliefert, BLE-Verbindungen sauber getrennt.
- BLE: MTU pro Verbindung, `notify()` mit Wiederholung.
- OTA: Watchdog wird während des Uploads gefüttert.
- Setup-Timeout zählt nur echte Portal-Aufrufe; harte Obergrenze 30 min.
- ISO-Datum strikt wie in der App; Sekunden bleiben erhalten.
- Altes WLAN-Passwort wird beim Netzwechsel verworfen; „Passwort entfernen“ im Portal.
- Weckzeit bei Ereignis-Uhrzeit in einer DST-Lücke.

### Security
- OTA und Werksreset im Portal verlangen den Code vom Display (`X-Setup-Code`).

## [0.9.0] – 2026-11-23
Release-Kandidat: Software vollständig, Hardware-Abnahme (docs/abnahme.md) steht aus.
Nach bestandener Abnahme folgt 1.0.0.

### Added
- Repo-Grundgerüst, PlatformIO-Projekt mit `ee04`- und `native`-Env, Lizenzdateien.
- since_core: Kalenderdifferenz mit Countdown, POSIX-TZ, deutsche Formatierung, Meilensteine.
- Config-Schema v1 mit Validierung, JSON-Merge, NVS-Speicher und gemeinsamen Fixtures.
- Firmware für das EE04:
  - GxEPD2-Rendering mit UTF-8-Fonts
  - DS3231-Alarm als Weckquelle, Deep Sleep, nur Neuzeichnen bei Änderung
  - Akkuschutz mit Hysterese
- Setup-Modus: Hotspot mit QR-Code und Captive Portal (Einstellungen, Uhrzeit vom Handy, Status, OTA, Werksreset), parallel BLE-GATT mit Passkey.
- Wöchentlicher NTP-Sync (optional), Watchdog, Brownout-Schutz, Werksreset per 10-s-Tastendruck.
- Android-App (Angular, zenit-ui, Capacitor) mit Mock-Gerät und Kontrakttests.
- Parametrisches OpenSCAD-Gehäuse (7,5″/4,2″) mit STL/3MF und Druckanleitung.
- Host-Renderer für Display-Vorschauen, Portal-Mock, CI, Release-Workflow, Web-Flasher.
