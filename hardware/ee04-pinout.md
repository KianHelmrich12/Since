# EE04 – Pinbelegung (Recherche für Phase 3)

Grundlage für `firmware/include/board_ee04.h`. Jeder Wert ist belegt; was nicht verifiziert
ist, steht unten unter „Offen“.

## Quellen
- **[SCH]** Schaltplan `XIAO_ePaper_Display_Board_EE04_SCH_V1.2.pdf` (Rev. v1.2, 2025-10-20),
  <https://files.seeedstudio.com/wiki/Epaper/EE04/XIAO_ePaper_Display_Board_EE04_SCH_V1.2.pdf>.
  Die Pins stehen auf Blatt 4 „Power“, Blatt 5 „XIAO“ und Blatt 6 „E-ink“.
- **[WIKI]** <https://wiki.seeedstudio.com/epaper_ee04/>, Abschnitte „Buttons“,
  „User Battery“ und „Jumper Cap Selection“
- **[GFX]** Seeed_GFX `User_Setups/EPaper_Board_Pins_Setups.h`, Block
  `USE_XIAO_EPAPER_DISPLAY_BOARD_EE04` (Commit 0dfdd71)
- **[GFX2]** Seeed_GFX2 `src/board/configs/XIAO_ePaper_Board_Configs.h`,
  `Config_XIAO_ePaper_EE04_Board` (Commit 5ddb925)
- **[S3+]** <https://wiki.seeedstudio.com/xiao_esp32s3_getting_started/> (Pin Map „XIAO ESP32-S3 Plus“),
  arduino-esp32 `variants/XIAO_ESP32S3_Plus/pins_arduino.h`

## Belegung
| Funktion | GPIO | XIAO | Hinweise | Quelle |
|---|---|---|---|---|
| EPD SCK | 7 | D8 | SPI0_CLK | SCH S.5, GFX, GFX2 |
| EPD MOSI | 9 | D10 | SPI0_MOSI | SCH S.5, GFX, GFX2 |
| EPD MISO | – | – | nicht verbunden | SCH S.5, GFX (`-1`) |
| EPD CS | 44 | D7 | SPI0_CS | SCH S.5, GFX, GFX2 |
| EPD DC | 10 | D16 | EDP_DC | SCH S.5, GFX, GFX2 |
| EPD RST | 38 | D11 | EDP_RES | SCH S.5, GFX, GFX2 |
| EPD BUSY | 4 | D3 | EDP_BUSY | SCH S.5, GFX, GFX2 |
| Panel-Versorgung EN | 43 | D6 | aktiv HIGH, Lastschalter TPS22916 (U16), 100k Pull-down, versorgt Panel + Onboard-Flash | SCH S.4/5/6, GFX `TFT_ENABLE` |
| KEY0 | 2 | D1 | aktiv LOW, 10k Pull-up an XIAO_3V3, 100 nF | SCH S.5, WIKI |
| KEY1 | 3 | D2 | aktiv LOW, 10k Pull-up, 100 nF; Strapping-Pin | SCH S.5, WIKI |
| KEY2 | 5 | D4 | aktiv LOW, 10k Pull-up, 100 nF | SCH S.5, WIKI |
| Akku-ADC | 1 | D0 | Teiler 10k/10k → V_bat = 2 × V_adc | SCH S.4, WIKI |
| Akku-ADC EN | 6 | D5 | aktiv HIGH, TPS22916 (U17), 100k Pull-down | SCH S.4/5, WIKI |
| Onboard-Flash U5 | 13/12/11/40 | D17/D18/D19/D13 | belegt | SCH S.5 |
| Lade-LED | – | – | am PMIC (SY6974B), kein GPIO | SCH S.4 |
| User-LED (XIAO) | 21 | – | auf dem XIAO-Modul | S3+ |

Der Jumper J5 muss für das 7,5″-Panel (24-Pin) auf „24 Pin“ stehen (WIKI, „Jumper Cap Selection“).

## Konsequenzen für die DS3231
- **Die Standard-I²C-Pins sind belegt:** D4 = GPIO5 ist KEY2 und D5 = GPIO6 ist ADC_EN.
- **Frei auf dem EE04:**
  - GPIO8 (D9) ist der einzige freie RTC-fähige GPIO (0–21).
  - GPIO39 (D12) ist frei, aber nur mit unbestückten Pads verbunden.
  - GPIO41/42 (D14/D15) liegen auf den unbestückten NFC-Pads J4. Jede der beiden Leitungen hat 300 pF gegen GND (C64/C65 sind bestückt).
- **Vorschlag, noch nicht entschieden und nicht getestet:**
  - SDA/SCL auf GPIO41/42, mit 100 kHz und kräftigen Pull-ups. Alternativ C64/C65 entfernen.
  - DS3231 INT/SQW auf GPIO8 als ext1-Weckquelle. INT ist Open-Drain, braucht also einen Pull-up.
- **Wecken per Taster:** GPIO2, 3 und 5 sind RTC-GPIOs, also geht ext1 mit der Maske {2, 3, 5, 8}.
- **DS3231 an XIAO_3V3 hängen,** nicht an VCC_3V3: VCC_3V3 ist abgeschaltet, solange GPIO43 LOW ist.

## Offen (MANUELL PRÜFEN / nicht verifiziert)
- Ist GPIO8/D9 bei aufgestecktem XIAO physisch erreichbar? Das Layout wurde nicht geprüft, und das EE04 hat keinen Erweiterungs-Header.
- Physische Lage der J5-Positionen und Beschriftung auf der Platine.
- Panel-Controller: UC8179 (Seeed_GFX Combo 502) oder JD79686B. Steht auf dem Panel bzw. dem Kaufbeleg.
- Polarität der User-LED an GPIO21.

## Widersprüche
- Akku-Faktor: Der Wiki-Code nutzt `adc/4096*7.16`, der Schaltplan zeigt einen Teiler 1:2. Plan: `analogReadMilliVolts(1) * 2`, auf dem Gerät kalibrieren.
- D-Nummern unterscheiden sich zwischen XIAO S3 und S3 Plus. In der Firmware deshalb nur rohe GPIO-Nummern verwenden.
