// SPDX-License-Identifier: GPL-3.0-or-later
#include "display.h"

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <SPI.h>

#include "board_ee04.h"
#include "render.h"

namespace display {
namespace {

using Panel = GxEPD2_750_GDEY075T7;  // 7,5″ 800×480, UC8179
GxEPD2_BW<Panel, Panel::HEIGHT>* g_epd = nullptr;

void begin() {
  if (g_epd != nullptr) return;
  SPI.begin(board::kEpdSck, board::kEpdMiso, board::kEpdMosi, board::kEpdCs);
  g_epd = new GxEPD2_BW<Panel, Panel::HEIGHT>(
      Panel(board::kEpdCs, board::kEpdDc, board::kEpdRst, board::kEpdBusy));
  g_epd->epd2.selectSPI(SPI, SPISettings(4000000, MSBFIRST, SPI_MODE0));
  g_epd->init(0);  // keine Debug-Ausgabe
  g_epd->setRotation(0);
}

}  // namespace

void show(const since::ScreenModel& m) {
  begin();
  g_epd->setFullWindow();
  g_epd->firstPage();
  do {
    render::draw(*g_epd, m);  // gleicher Code wie im Emulator
  } while (g_epd->nextPage());
  g_epd->hibernate();
}

}  // namespace display
