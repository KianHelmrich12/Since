# Stückliste (BOM)

| # | Teil | Menge | Hinweise | Bezug (Beispiele) |
|---|---|---|---|---|
| 1 | Seeed XIAO ePaper Display Board EE04 (mit XIAO ESP32-S3 Plus) | 1 | Ladeschaltung, JST-Akkuanschluss mit Schalter, 3 User-Taster | Seeed Studio Shop, Distributoren (Mouser, Digi-Key, Reichelt) |
| 2 | 7,5″ E-Paper, monochrom, 800×480, 24-Pin-FPC (UC8179, z. B. GDEY075T7) | 1 | Jumper J5 auf „24 Pin“ | Seeed Studio, Good Display, Waveshare |
| 3 | DS3231-RTC-Modul (ZS042) | 1 | Umbauten siehe [wiring.md](wiring.md) | Elektronik-Versand, Marktplätze |
| 4 | LIR2032-Akku-Knopfzelle | 1 | wiederaufladbar; für CR2032 Ladediode entfernen | Elektronik-Versand |
| 5 | LiPo 1S 3,7 V, ~2000 mAh, JST-PH 2.0, mit Schutzschaltung | 1 | **Polarität vor dem Einstecken prüfen** (keine einheitliche Belegung bei JST-PH) | Akku-Fachhandel |
| 6 | Litze 0,14 mm², 4 Adern | ~20 cm | DS3231 ↔ EE04 | |
| 7 | Gewindeeinsätze / Schrauben | siehe [enclosure/README.md](../enclosure/README.md) | | |
| 8 | Filament PLA oder PETG | ~150 g | Gehäuse, optional 2. Farbe für das Namens-Relief | |

Optional: Widerstand 100 kΩ (INT-Pull-up an XIAO_3V3), 2× 4,7 kΩ (stärkere I²C-Pull-ups).
