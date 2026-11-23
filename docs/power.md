# Stromverbrauch

Ziel: **< 50 µA** im Deep Sleep (gesamtes Gerät, am Akku gemessen).

> Status: **MANUELL PRÜFEN – noch nicht gemessen.** Ergebnisse unten eintragen.

## Was im Schlaf noch Strom zieht
| Verbraucher | Zustand im Schlaf | Erwartung |
|---|---|---|
| ESP32-S3 Deep Sleep, RTC-Peripherie an (ext1 + Pull-up) | an | ~10–20 µA |
| VCC_3V3 (Panel, Onboard-Flash, DS3231-VCC) | **aus** (GPIO43 LOW, 100k Pull-down) | ~0 |
| Akku-Teiler | **aus** (ADC_EN LOW) | ~0 |
| PMIC SY6974B + Schutzschaltung des Akkus | an | Datenblatt/Messung |
| DS3231 | aus der LIR2032 versorgt | belastet den LiPo nicht |
| E-Paper | Hibernate, Bild bleibt stehen | ~0 |

## Messung
1. Strommessgerät mit µA-Bereich (oder einen Power-Profiler wie Nordic PPK2) **in Reihe zum Akku** schalten: Akku → Messgerät → JST-Stecker.
2. USB abziehen (sonst versorgt der Laderegler).
3. Gerät starten. Es zeichnet und geht schlafen, die serielle Ausgabe meldet „sleep until …“.
4. 30 s warten und den Ruhestrom ablesen. Zusätzlich die Ladung eines Weck-Zyklus messen (Display-Refresh) in mAs.
5. Hochrechnen: `Laufzeit [h] = 2000 mAh / (I_sleep + Q_wake × Wakes/h / 3600)`.

## Ergebnisse
| Datum | Firmware | Zustand | Strom | Messgerät | Notiz |
|---|---|---|---|---|---|
| | | Deep Sleep | | | |
| | | Refresh (Peak / Dauer) | | | |
