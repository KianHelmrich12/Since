// SPDX-License-Identifier: GPL-3.0-or-later
// Pinbelegung des Seeed XIAO ePaper Display Board EE04 (XIAO ESP32-S3 Plus).
//
// Quellen (Details und Abwägungen: hardware/ee04-pinout.md):
//   [SCH]  XIAO_ePaper_Display_Board_EE04_SCH_V1.2.pdf, Rev. v1.2 (2025-10-20),
//          https://files.seeedstudio.com/wiki/Epaper/EE04/XIAO_ePaper_Display_Board_EE04_SCH_V1.2.pdf
//   [WIKI] https://wiki.seeedstudio.com/epaper_ee04/
//   [GFX]  Seeed_GFX User_Setups/EPaper_Board_Pins_Setups.h, USE_XIAO_EPAPER_DISPLAY_BOARD_EE04
//   [GFX2] Seeed_GFX2 src/board/configs/XIAO_ePaper_Board_Configs.h, Config_XIAO_ePaper_EE04_Board
// Alle Werte sind rohe ESP32-S3-GPIO-Nummern; die D-Aliase unterscheiden sich zwischen
// XIAO ESP32-S3 und S3 Plus.
#pragma once

#include <cstdint>

namespace board {

// --- E-Paper (SPI0) ------------------------------------------------- [SCH S.5, GFX, GFX2]
constexpr int kEpdSck = 7;    // D8, SPI0_CLK
constexpr int kEpdMosi = 9;   // D10, SPI0_MOSI
constexpr int kEpdMiso = -1;  // nicht verbunden
constexpr int kEpdCs = 44;    // D7, SPI0_CS
constexpr int kEpdDc = 10;    // D16 (Plus), EDP_DC
constexpr int kEpdRst = 38;   // D11 (Plus), EDP_RES
constexpr int kEpdBusy = 4;   // D3, EDP_BUSY
// Lastschalter U16 (TPS22916) für VCC_3V3 → Panel + Onboard-Flash (+ DS3231, siehe unten).
// Aktiv HIGH, 100k Pull-down → im Deep Sleep aus.                     [SCH S.4/5/6, GFX]
constexpr int kEpdPowerEnable = 43;  // D6

// --- Taster (aktiv LOW, 10k Pull-up an XIAO_3V3, 100 nF) -------------- [SCH S.5, WIKI]
// Alle drei sind RTC-GPIOs → Wecken per ext1 möglich.
constexpr int kKey0 = 2;  // D1 – Setup-Modus; 10 s halten = Werksreset
constexpr int kKey1 = 3;  // D2 – sofort neu zeichnen (Strapping-Pin, extern hochgezogen)
constexpr int kKey2 = 5;  // D4 – Layout wechseln

// --- Akku ------------------------------------------------------------- [SCH S.4/5, WIKI]
constexpr int kBatteryAdc = 1;        // D0/A0, Teiler 10k/10k
constexpr int kBatteryAdcEnable = 6;  // D5, Lastschalter U17, aktiv HIGH, 100k Pull-down
constexpr int kBatteryDividerNum = 2;  // V_bat = 2 × V_adc
constexpr int kBatteryDividerDen = 1;

// --- DS3231 (extern verdrahtet) --------------------------------------- [hardware/wiring.md]
// Die Standard-XIAO-I²C-Pins (D4/D5) sind auf dem EE04 belegt (KEY2, ADC_EN). Verwendet
// werden die unbestückten NFC-Pads J4 (je 0R in Serie + 300 pF gegen GND → 100 kHz).
constexpr int kI2cSda = 41;  // D14, Pad J4  [SCH S.5]
constexpr int kI2cScl = 42;  // D15, Pad J4  [SCH S.5]
constexpr uint32_t kI2cHz = 100000;
// DS3231 INT/SQW (Open-Drain, aktiv LOW) → einziger freier RTC-GPIO auf dem EE04. [SCH S.5]
constexpr int kRtcInt = 8;  // D9

// --- Sonstiges --------------------------------------------------------------------- [S3+]
constexpr int kUserLed = 21;  // auf dem XIAO-Modul (Polarität: nicht verifiziert)

// Weckmaske für ext1 (ANY_LOW): drei Taster + RTC-Alarm
constexpr uint64_t kWakeMask =
    (1ULL << kKey0) | (1ULL << kKey1) | (1ULL << kKey2) | (1ULL << kRtcInt);

}  // namespace board
