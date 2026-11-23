# Changelog

Format nach [Keep a Changelog](https://keepachangelog.com/de/1.1.0/),
Versionierung nach [Semantic Versioning](https://semver.org/lang/de/).

## [Unreleased]

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
