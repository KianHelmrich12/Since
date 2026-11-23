// SPDX-License-Identifier: GPL-3.0-or-later
// Gemeinsames Rendering für Gerät und Emulator.
#pragma once

#include <Adafruit_GFX.h>

#include <string>

#include "screen.h"

namespace render {

constexpr int kWidth = 800;
constexpr int kHeight = 480;

// Zeichnet das komplette Bild (inklusive Hintergrund) auf gfx.
void draw(Adafruit_GFX& gfx, const since::ScreenModel& m);

// Prüft, ob alle verwendeten Fonts jedes Zeichen von utf8 enthalten. Gibt die Anzahl
// fehlender (Font, Zeichen)-Paare zurück; report beschreibt sie. Für Emulator und Tests.
int missingGlyphs(Adafruit_GFX& gfx, const char* utf8, std::string* report);

}  // namespace render
