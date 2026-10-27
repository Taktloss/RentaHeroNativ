// SPDX-License-Identifier: GPL-3.0-or-later
// Tiefen- und Wegkarten (.ZBF), 640 × 360 Bytes mit optionalem Skalenkopf (docs/formats/zbf.md).
#pragma once

#include <span>
#include <string>
#include <vector>

#include "core/bytes.hpp"

namespace rah {

struct ZMap {
    static constexpr int kWidth = 640;
    static constexpr int kHeight = 360;
    std::vector<std::uint8_t> data;  // kWidth * kHeight
    float sMin = 0, sMax = 0;        // +0x110 / +0x114
    int vMin = 0, vMax = 0;          // +0x118 / +0x11C
    int dv = 0;                      // +0x120
    float ds = 0;                    // +0x124
    double k = 0, kInv = 0;          // +0x128 / +0x130

    std::uint8_t at(int x, int y) const { return data[static_cast<std::size_t>(y) * kWidth + static_cast<std::size_t>(x)]; }
};

// orig: Rent-A-Hero.exe 0x0043D710 CZMap_Load (+ 0x0043D6C0 CZMap_CalcScale)
bool loadZMap(std::span<const std::uint8_t> file, ZMap& out, std::string* error);

}  // namespace rah
