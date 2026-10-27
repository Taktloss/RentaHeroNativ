// SPDX-License-Identifier: GPL-3.0-or-later
// Bildschirmpuffer im Referenzformat des Originals: 640×480, 16 Bit RGB565 (docs/RENDERER.md).
#pragma once

#include <cstdint>
#include <vector>

namespace rah {

struct Surface565 {
    static constexpr int kWidth = 640;
    static constexpr int kHeight = 480;
    std::vector<std::uint16_t> pixels = std::vector<std::uint16_t>(kWidth * kHeight, 0);

    std::uint16_t* row(int y) { return pixels.data() + static_cast<std::size_t>(y) * kWidth; }
    const std::uint16_t* row(int y) const { return pixels.data() + static_cast<std::size_t>(y) * kWidth; }
    void fill(int x0, int y0, int x1, int y1, std::uint16_t c);  // [x0,x1) × [y0,y1), geclippt
};

// orig: Rent-A-Hero.exe 0x004399E0 – Packen ohne Runden/Dithering
inline std::uint16_t pack565(unsigned r, unsigned g, unsigned b) {
    return static_cast<std::uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// RGB565 → 8 Bit je Kanal für die Ausgabe (Bitwiederholung, verlustfrei umkehrbar).
inline void unpack565To888(std::uint16_t p, std::uint8_t& r, std::uint8_t& g, std::uint8_t& b) {
    const unsigned r5 = p >> 11, g6 = (p >> 5) & 0x3F, b5 = p & 0x1F;
    r = static_cast<std::uint8_t>((r5 << 3) | (r5 >> 2));
    g = static_cast<std::uint8_t>((g6 << 2) | (g6 >> 4));
    b = static_cast<std::uint8_t>((b5 << 3) | (b5 >> 2));
}

}  // namespace rah
