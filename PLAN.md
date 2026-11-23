# since – Phasenplan

Jede Phase beginnt erst, wenn die Akzeptanzkriterien der vorherigen erfüllt sind.
Schritte mit echter Hardware sind als **MANUELL PRÜFEN** markiert.

## Phase 0 – Repo-Setup
- [x] Struktur wie in CLAUDE.md, .gitignore, .editorconfig, README-Gerüst
- [x] Lizenzdateien: GPL-3.0-or-later (Root), CERN-OHL-S-2.0 (hardware/), CC BY-SA 4.0 (enclosure/)
- [x] platformio.ini mit ee04 (XIAO ESP32-S3, Arduino) und native (Unity)
- [x] Hello-World mit serieller Ausgabe, ein Dummy-Test

Akzeptanz: `pio run -e ee04` und `pio test -e native` laufen grün.

## Phase 1 – Zeitrechnung (since_core)
- [x] Kalenderdifferenz Ereignis → jetzt: Jahre, Monate, Tage, Stunden, Minuten + Gesamttage/-wochen
- [x] Randfälle: Monatsende (31.01. + 1 Monat), Geburt am 29.02., Sommer-/Winterzeit-Wechsel, Ereignis in der Zukunft (Countdown)
- [x] Deutsche Formatierung mit Singular/Plural („1 Jahr“, „2 Jahre“), Nullwerte ausblenden
- [x] Meilensteine erkennen: Geburtstag, 100./1000. Tag, Monatstag

Akzeptanz: mindestens 30 Unit-Tests grün, inkl. aller Randfälle.

## Phase 2 – Konfiguration
- [ ] Config: Name, Anlass-Text, Ereignis (lokales Datum + Uhrzeit), TZ, Layout, Update-Intervall, optional WLAN, NTP an/aus, Schema-Version
- [ ] JSON (de)serialisieren + validieren in since_core, Persistenz via Preferences in src/
- [ ] Sinnvolle Defaults und Migration bei neuer Schema-Version

Akzeptanz: Tests für Validierung und Round-Trip grün.

## Phase 3 – Hardware-Bring-up
- [ ] board_ee04.h: Display-SPI, Taster, Akku-ADC, I²C für RTC, RTC-fähiger GPIO für DS3231-INT (mit Quellenangabe)
- [ ] Display „Hallo since“, DS3231 lesen/stellen, Akkuspannung messen

Akzeptanz: **MANUELL PRÜFEN** – Text sichtbar, RTC läuft nach Stromtrennung weiter, Spannung plausibel.

## Phase 4 – Darstellung
- [ ] Layouts: Standard (Name groß, „3 Jahre · 2 Monate · 14 Tage“, „seit 12.03.2026, 08:15“, Gesamttage), Minimal, Geburtstag, Akku leer, Setup-Modus
- [ ] Fonts mit Umlauten, Akku-Symbol
- [ ] Optional: Host-Renderer, der Layouts als PNG/PBM nach docs/previews/ exportiert

Akzeptanz: **MANUELL PRÜFEN** – alle Screens auf dem Gerät ok.

## Phase 5 – Strom & Deep Sleep
- [ ] Ablauf: Aufwachen → Grund prüfen (RTC-Alarm/Timer oder Taster) → RTC lesen → nur bei Änderung neu zeichnen → Display schlafen legen → nächsten Alarm setzen → Deep Sleep
- [ ] DS3231-Alarm über INT als ext0/ext1-Weckquelle, ESP-Timer als Fallback
- [ ] Akkuschutz: Warnung bei niedrigem Stand, unter Grenzwert „Bitte laden“ anzeigen und lange schlafen

Akzeptanz: **MANUELL PRÜFEN** – Ruhestrom gemessen und in docs/ dokumentiert (Ziel: < 50 µA gesamt).

## Phase 6 – Webportal
- [ ] Taster → Hotspot since-XXXX (MAC-Suffix), optional WPA2-Passwort, Captive Portal
- [ ] Display zeigt im Setup SSID + QR-Code zum Verbinden
- [ ] Seiten: Einstellungen, Uhrzeit vom Handy übernehmen (Browser-Zeit per JS, kein Internet nötig), WLAN für NTP, Status (Akku, FW-Version, letzte Sync), Firmware-Update (OTA-Upload), Werksreset
- [ ] API: GET/POST /api/config, POST /api/time, GET /api/status, POST /api/ota
- [ ] Nach 10 min Inaktivität: Portal beenden → Deep Sleep

Akzeptanz: **MANUELL PRÜFEN** – komplette Einrichtung per Handy, überlebt Neustart.

## Phase 7 – Zeit-Sync & Robustheit
- [ ] NTP-Sync wöchentlich, falls WLAN konfiguriert (kurz verbinden, dann WLAN aus)
- [ ] RTC-„Lost Power“-Flag → Hinweis „Uhrzeit einstellen“
- [ ] Watchdog, Brownout-Handling, Werksreset per 10 s langem Tastendruck

## Phase 8 – Gehäuse
- [ ] OpenSCAD, parametrisch (Displaygröße, Akku-/Board-Maße, Wandstärke)
- [ ] Frontrahmen mit optionalem Namens-Relief, Rückseite mit USB-C- und Taster-Zugang, Aufsteller + Schlüssellochaufhängung
- [ ] Export 3MF, Druckhinweise für Bambu (Material, Multicolor, Supports)

## Phase 9 – Doku & Release
- [ ] README mit Fotos, BOM mit Bezugsquellen, Bauanleitung, Strommessungen
- [ ] CHANGELOG, Tag v1.0.0, Release-Artefakte (firmware.bin, littlefs.bin)
- [ ] Optional: Browser-Flasher mit ESP Web Tools

## Offene Entscheidungen
Werden erfragt, sobald sie relevant werden.
- [ ] Lizenz final: GPL-3.0 oder MIT? (vorläufig GPL-3.0-or-later)
- [ ] Hosting: GitHub öffentlich oder eigenes GitLab, welcher Namespace?
- [ ] Display: 7,5″ (Bilderrahmen) oder 4,2″ (Tischaufsteller)?
- [ ] Default-Intervall: stündlich oder alle 10 Minuten?
- [ ] v2-Ideen: mehrere Ereignisse (Geschwister), Farb-E-Paper, Home-Assistant-Anbindung
