// SPDX-License-Identifier: GPL-3.0-or-later
// Zeichnet ein Bildschirm-Modell auf eine beliebige Adafruit-GFX-Fläche (800×480).
// Gerät: GxEPD2-Display (src/display.cpp). Emulator: GFXcanvas1 (firmware/emu).
#include "render.h"

#include <U8g2_for_Adafruit_GFX.h>
#include <qrcode.h>

#include <vector>

namespace render {
namespace {

constexpr uint16_t kBlack = 0x0000;  // = GxEPD_BLACK
constexpr uint16_t kWhite = 0xFFFF;  // = GxEPD_WHITE
constexpr int kW = 800;
constexpr int kH = 480;
constexpr int kMargin = 24;

Adafruit_GFX* g_gfx = nullptr;
U8G2_FOR_ADAFRUIT_GFX g_u8g2;

int textWidth(const std::string& s) { return g_u8g2.getUTF8Width(s.c_str()); }

// Wählt den größten Font, in dem s in maxWidth passt.
void fitFont(const std::string& s, const uint8_t* const* fonts, int count, int maxWidth) {
  for (int i = 0; i < count; ++i) {
    g_u8g2.setFont(fonts[i]);
    if (textWidth(s) <= maxWidth) return;
  }
}

void centered(const std::string& s, int baselineY, uint16_t color = kBlack) {
  if (s.empty()) return;
  g_u8g2.setForegroundColor(color);
  g_u8g2.setCursor((kW - textWidth(s)) / 2, baselineY);
  g_u8g2.print(s.c_str());
}

void left(const std::string& s, int x, int baselineY) {
  if (s.empty()) return;
  g_u8g2.setForegroundColor(kBlack);
  g_u8g2.setCursor(x, baselineY);
  g_u8g2.print(s.c_str());
}

// FreeUniversal (fub/fur) statt logisoso: enthält den vollen Latin-1-Satz inklusive „·“
// (U+00B7) und Umlauten. Gefunden mit dem Emulator (logisoso hat kein „·“).
const uint8_t* const kTitleFonts[] = {u8g2_font_fub42_tf, u8g2_font_fub35_tf,
                                      u8g2_font_fub30_tf, u8g2_font_fub25_tf};
const uint8_t* const kHeadFonts[] = {u8g2_font_fur30_tf, u8g2_font_fur25_tf,
                                     u8g2_font_fur20_tf, u8g2_font_helvB18_tf};
const uint8_t* const kBigHeadFonts[] = {u8g2_font_fur42_tf, u8g2_font_fur35_tf,
                                        u8g2_font_fur30_tf, u8g2_font_fur25_tf};
const uint8_t* const kTextFonts[] = {u8g2_font_helvR24_tf, u8g2_font_helvR18_tf,
                                     u8g2_font_helvR14_tf};

void battery(const since::ScreenModel& m) {
  if (m.batteryPercent < 0) return;
  const int x = kW - kMargin - 44, y = 14, w = 40, h = 18;
  g_gfx->drawRect(x, y, w, h, kBlack);
  g_gfx->fillRect(x + w, y + 5, 4, h - 10, kBlack);
  const int fill = (w - 4) * m.batteryPercent / 100;
  g_gfx->fillRect(x + 2, y + 2, fill, h - 4, kBlack);
  if (m.batteryWarning) {
    g_u8g2.setFont(u8g2_font_helvB14_tf);
    left("!", x - 14, y + h - 2);
  }
}

void banner(const std::string& text) {
  if (text.empty()) return;
  const int y = kH - 84;
  g_gfx->fillRect(0, y, kW, 84, kBlack);
  const uint8_t* const fonts[] = {u8g2_font_helvB24_tf, u8g2_font_helvB18_tf};
  fitFont(text, fonts, 2, kW - 2 * kMargin);
  g_u8g2.setBackgroundColor(kBlack);
  centered(text, y + 54, kWhite);
  g_u8g2.setBackgroundColor(kWhite);
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
        g_gfx->fillRect(x + offset + c * scale, y + offset + r * scale, scale, scale,
                        kBlack);
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
  g_u8g2.setFont(u8g2_font_fub30_tf);
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

void draw(Adafruit_GFX& gfx, const since::ScreenModel& m) {
  g_gfx = &gfx;
  g_u8g2.begin(gfx);
  g_u8g2.setFontMode(1);
  g_u8g2.setFontDirection(0);
  gfx.fillScreen(kWhite);
  g_u8g2.setForegroundColor(kBlack);
  g_u8g2.setBackgroundColor(kWhite);
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
}

int missingGlyphs(Adafruit_GFX& gfx, const char* utf8, std::string* report) {
  struct Named {
    const uint8_t* font;
    const char* name;
  };
  const Named fonts[] = {
      {u8g2_font_fub42_tf, "fub42"}, {u8g2_font_fub35_tf, "fub35"}, {u8g2_font_fub30_tf, "fub30"},
      {u8g2_font_fub25_tf, "fub25"}, {u8g2_font_fur42_tf, "fur42"}, {u8g2_font_fur35_tf, "fur35"},
      {u8g2_font_fur30_tf, "fur30"}, {u8g2_font_fur25_tf, "fur25"}, {u8g2_font_fur20_tf, "fur20"},
      {u8g2_font_helvB24_tf, "helvB24"}, {u8g2_font_helvB18_tf, "helvB18"},
      {u8g2_font_helvB14_tf, "helvB14"}, {u8g2_font_helvR24_tf, "helvR24"},
      {u8g2_font_helvR18_tf, "helvR18"}, {u8g2_font_helvR14_tf, "helvR14"},
  };
  g_u8g2.begin(gfx);
  int missing = 0;
  for (const auto& f : fonts) {
    g_u8g2.setFont(f.font);
    // jedes UTF-8-Zeichen einzeln messen; fehlende Glyphen haben die Breite 0
    for (const char* p = utf8; *p;) {
      int len = 1;
      const unsigned char c = static_cast<unsigned char>(*p);
      if (c >= 0xF0) len = 4;
      else if (c >= 0xE0) len = 3;
      else if (c >= 0xC0) len = 2;
      const std::string ch(p, len);
      p += len;
      if (ch == " ") continue;
      if (g_u8g2.getUTF8Width(ch.c_str()) <= 0) {
        ++missing;
        if (report != nullptr) *report += std::string(f.name) + ": '" + ch + "'\n";
      }
    }
  }
  return missing;
}

}  // namespace render
