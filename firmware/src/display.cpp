// SPDX-License-Identifier: GPL-3.0-or-later
#include "display.h"

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <SPI.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <qrcode.h>

#include "board_ee04.h"

namespace display {
namespace {

using Panel = GxEPD2_750_GDEY075T7;  // 7,5″ 800×480, UC8179
constexpr int kW = 800;
constexpr int kH = 480;
constexpr int kMargin = 24;

GxEPD2_BW<Panel, Panel::HEIGHT>* g_epd = nullptr;
U8G2_FOR_ADAFRUIT_GFX g_u8g2;

void begin() {
  if (g_epd != nullptr) return;
  SPI.begin(board::kEpdSck, board::kEpdMiso, board::kEpdMosi, board::kEpdCs);
  g_epd = new GxEPD2_BW<Panel, Panel::HEIGHT>(
      Panel(board::kEpdCs, board::kEpdDc, board::kEpdRst, board::kEpdBusy));
  g_epd->epd2.selectSPI(SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));
  g_epd->init(0);  // keine Debug-Ausgabe
  g_epd->setRotation(0);
  g_u8g2.begin(*g_epd);
  g_u8g2.setFontMode(1);
  g_u8g2.setFontDirection(0);
}

int textWidth(const std::string& s) { return g_u8g2.getUTF8Width(s.c_str()); }

// Wählt den größten Font, in dem s in maxWidth passt.
void fitFont(const std::string& s, const uint8_t* const* fonts, int count, int maxWidth) {
  for (int i = 0; i < count; ++i) {
    g_u8g2.setFont(fonts[i]);
    if (textWidth(s) <= maxWidth) return;
  }
}

void centered(const std::string& s, int baselineY, uint16_t color = GxEPD_BLACK) {
  if (s.empty()) return;
  g_u8g2.setForegroundColor(color);
  g_u8g2.setCursor((kW - textWidth(s)) / 2, baselineY);
  g_u8g2.print(s.c_str());
}

void left(const std::string& s, int x, int baselineY) {
  if (s.empty()) return;
  g_u8g2.setForegroundColor(GxEPD_BLACK);
  g_u8g2.setCursor(x, baselineY);
  g_u8g2.print(s.c_str());
}

const uint8_t* const kTitleFonts[] = {u8g2_font_logisoso58_tf, u8g2_font_logisoso46_tf,
                                      u8g2_font_logisoso38_tf, u8g2_font_logisoso30_tf};
const uint8_t* const kHeadFonts[] = {u8g2_font_logisoso34_tf, u8g2_font_logisoso28_tf,
                                     u8g2_font_logisoso24_tf, u8g2_font_helvB18_tf};
const uint8_t* const kBigHeadFonts[] = {u8g2_font_logisoso50_tf, u8g2_font_logisoso42_tf,
                                        u8g2_font_logisoso34_tf, u8g2_font_logisoso28_tf};
const uint8_t* const kTextFonts[] = {u8g2_font_helvR24_tf, u8g2_font_helvR18_tf,
                                     u8g2_font_helvR14_tf};

void battery(const since::ScreenModel& m) {
  if (m.batteryPercent < 0) return;
  const int x = kW - kMargin - 44, y = 14, w = 40, h = 18;
  g_epd->drawRect(x, y, w, h, GxEPD_BLACK);
  g_epd->fillRect(x + w, y + 5, 4, h - 10, GxEPD_BLACK);
  const int fill = (w - 4) * m.batteryPercent / 100;
  g_epd->fillRect(x + 2, y + 2, fill, h - 4, GxEPD_BLACK);
  if (m.batteryWarning) {
    g_u8g2.setFont(u8g2_font_helvB14_tf);
    left("!", x - 14, y + h - 2);
  }
}

void banner(const std::string& text) {
  if (text.empty()) return;
  const int y = kH - 84;
  g_epd->fillRect(0, y, kW, 84, GxEPD_BLACK);
  const uint8_t* const fonts[] = {u8g2_font_helvB24_tf, u8g2_font_helvB18_tf};
  fitFont(text, fonts, 2, kW - 2 * kMargin);
  g_u8g2.setBackgroundColor(GxEPD_BLACK);
  centered(text, y + 54, GxEPD_WHITE);
  g_u8g2.setBackgroundColor(GxEPD_WHITE);
}

void qr(const std::string& payload, int x, int y, int size) {
  QRCode code;
  const int version = 6;  // 41×41 Module, reicht für WIFI-Payloads bis ~100 Zeichen
  std::vector<uint8_t> buf(qrcode_getBufferSize(version));
  if (qrcode_initText(&code, buf.data(), version, ECC_MEDIUM, payload.c_str()) != 0) return;
  const int scale = size / (code.size + 8);
  const int offset = (size - code.size * scale) / 2;
  for (int r = 0; r < code.size; ++r) {
    for (int c = 0; c < code.size; ++c) {
      if (qrcode_getModule(&code, c, r)) {
        g_epd->fillRect(x + offset + c * scale, y + offset + r * scale, scale, scale,
                        GxEPD_BLACK);
      }
    }
  }
}

void drawStandard(const since::ScreenModel& m) {
  const int maxW = kW - 2 * kMargin;
  fitFont(m.subtitle, kTextFonts, 3, maxW);
  centered(m.subtitle, 64);
  fitFont(m.title, kTitleFonts, 4, maxW);
  centered(m.title, 160);
  fitFont(m.headline, kHeadFonts, 4, maxW);
  centered(m.headline, 250);
  g_u8g2.setFont(u8g2_font_helvR18_tf);
  centered(m.line1, 310);
  g_u8g2.setFont(u8g2_font_helvR14_tf);
  centered(m.line2, 354);
  banner(m.banner);
}

void drawMinimal(const since::ScreenModel& m) {
  const int maxW = kW - 2 * kMargin;
  fitFont(m.title, kHeadFonts, 4, maxW);
  centered(m.title, 110);
  fitFont(m.headline, kBigHeadFonts, 4, maxW);
  centered(m.headline, 270);
  g_u8g2.setFont(u8g2_font_helvR14_tf);
  centered(m.line1, kH - 40);
  banner(m.banner);
}

void drawMessage(const since::ScreenModel& m) {
  const int maxW = kW - 2 * kMargin;
  fitFont(m.title, kTitleFonts, 4, maxW);
  centered(m.title, 190);
  fitFont(m.subtitle, kTextFonts, 3, maxW);
  centered(m.subtitle, 260);
  g_u8g2.setFont(u8g2_font_helvR18_tf);
  centered(m.line1, 330);
  centered(m.line2, 370);
}

void drawSetup(const since::ScreenModel& m) {
  const int qrSize = 360;
  const int qrX = kW - kMargin - qrSize, qrY = (kH - qrSize) / 2;
  qr(m.qr, qrX, qrY, qrSize);
  const int x = kMargin + 8;
  g_u8g2.setFont(u8g2_font_logisoso34_tf);
  left(m.title, x, 90);
  g_u8g2.setFont(u8g2_font_helvB18_tf);
  left("1. QR-Code scannen oder", x, 160);
  left(m.subtitle, x, 196);
  g_u8g2.setFont(u8g2_font_helvR18_tf);
  left(m.headline, x, 232);
  left("2. " + m.line1, x, 290);
  g_u8g2.setFont(u8g2_font_helvR14_tf);
  left(m.line2, x, 350);
  left("Ende nach 10 min ohne Aktivität.", x, kH - 40);
}

}  // namespace

void show(const since::ScreenModel& m) {
  begin();
  g_epd->setFullWindow();
  g_epd->firstPage();
  do {
    g_epd->fillScreen(GxEPD_WHITE);
    g_u8g2.setForegroundColor(GxEPD_BLACK);
    g_u8g2.setBackgroundColor(GxEPD_WHITE);
    switch (m.kind) {
      case since::ScreenKind::kStandard:
      case since::ScreenKind::kMilestone:
        drawStandard(m);
        break;
      case since::ScreenKind::kMinimal:
        drawMinimal(m);
        break;
      case since::ScreenKind::kSetup:
        drawSetup(m);
        break;
      default:
        drawMessage(m);
        break;
    }
    if (m.kind != since::ScreenKind::kSetup) battery(m);
  } while (g_epd->nextPage());
  g_epd->hibernate();
}

}  // namespace display
