// SPDX-License-Identifier: GPL-3.0-or-later
#include "formats/zbf.hpp"

#include <cstring>

namespace rah {

namespace {

float f32(std::span<const std::uint8_t> b, std::size_t off) {
    const std::uint32_t u = le32(b, off);
    float f;
    std::memcpy(&f, &u, sizeof f);
    return f;
}

}  // namespace

bool loadZMap(std::span<const std::uint8_t> file, ZMap& out, std::string* error) {
    constexpr std::size_t kData = std::size_t{ZMap::kWidth} * ZMap::kHeight;
    auto fail = [&](const char* why) {
        if (error) *error = why;
        return false;
    };
    out = ZMap{};
    std::size_t off = 0;
    if (file.size() != kData) {  // genau 230 400 Bytes = ohne Kopf, Skalenwerte bleiben 0
        if (file.size() < 20) return fail("ZBF zu kurz");
        const std::uint32_t hs = le32(file, 0);
        if (hs < 20) return fail("ZBF-Kopf kleiner als 20");
        out.sMin = f32(file, 4);
        out.vMin = static_cast<int>(le32(file, 8));
        out.sMax = f32(file, 12);
        out.vMax = static_cast<int>(le32(file, 16));
        off = hs;  // Zusatzbytes des Kopfs werden im Original gelesen und überschrieben
        // orig: 0x0043D6C0 – float-Rechnung wie im Original (k in double gespeichert)
        out.dv = out.vMax - out.vMin;
        out.ds = out.sMax - out.sMin;
        out.k = static_cast<double>(out.ds / static_cast<float>(out.dv));
        out.kInv = static_cast<double>(static_cast<float>(out.dv) / out.ds);
    }
    if (off + kData > file.size()) return fail("ZBF-Daten zu kurz");
    out.data.assign(file.begin() + static_cast<std::ptrdiff_t>(off),
                    file.begin() + static_cast<std::ptrdiff_t>(off + kData));
    return true;
}

}  // namespace rah
