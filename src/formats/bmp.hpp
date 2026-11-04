// SPDX-License-Identifier: GPL-3.0-or-later
// BMP (BITMAPINFOHEADER, 24 Bit, unkomprimiert) – einzige Variante in den Daten
// (docs/formats/media.md). Ergebnis direkt im Szenenformat RGB565.
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace rah {

struct Image565 {
    int width = 0, height = 0;
    std::vector<std::uint16_t> pixels;  // oben links zuerst
};

// orig: Rent-A-Hero.exe 0x004382C0 CImageBmp_Load (LoadImageA) und 0x00438330 (Übertragen in
// die 16-Bit-Oberfläche per GDI).
// GDI-BitBlt (DDCopyBitmap 0x0043D130) wandelt 24 → 16 Bit durch Abschneiden (wie pack565), ohne
// Dithering – Wine-Messung: 1024/1024 Werte abgeschnitten, einfarbige Fläche gleichmäßig (WINEPROBE.md).
bool loadBmp565(std::span<const std::uint8_t> file, Image565& out, std::string* error);

}  // namespace rah
