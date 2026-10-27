// SPDX-License-Identifier: GPL-3.0-or-later
#include "formats/bmp.hpp"

#include "core/bytes.hpp"
#include "render/surface.hpp"

namespace rah {

bool loadBmp565(std::span<const std::uint8_t> f, Image565& out, std::string* error) {
    auto fail = [&](const char* why) {
        if (error) *error = why;
        return false;
    };
    if (f.size() < 54 || f[0] != 'B' || f[1] != 'M') return fail("kein BMP");
    const std::uint32_t dataOff = le32(f, 10);
    const std::uint32_t hdr = le32(f, 14);
    const auto w = static_cast<std::int32_t>(le32(f, 18));
    const auto h = static_cast<std::int32_t>(le32(f, 22));
    const unsigned bpp = le16(f, 28);
    const std::uint32_t comp = le32(f, 30);
    if (hdr != 40 || bpp != 24 || comp != 0 || w <= 0 || h == 0) return fail("BMP-Variante nicht unterstützt");
    const bool bottomUp = h > 0;
    const int ah = bottomUp ? h : -h;
    const std::size_t stride = (static_cast<std::size_t>(w) * 3 + 3) & ~std::size_t{3};
    if (dataOff + stride * static_cast<std::size_t>(ah) > f.size()) return fail("BMP zu kurz");
    out.width = w;
    out.height = ah;
    out.pixels.assign(static_cast<std::size_t>(w) * static_cast<std::size_t>(ah), 0);
    for (int y = 0; y < ah; ++y) {
        const std::size_t srcRow = dataOff + stride * static_cast<std::size_t>(bottomUp ? ah - 1 - y : y);
        for (int x = 0; x < w; ++x) {
            const std::size_t p = srcRow + static_cast<std::size_t>(x) * 3;  // B, G, R
            out.pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)] =
                pack565(f[p + 2], f[p + 1], f[p]);
        }
    }
    return true;
}

}  // namespace rah
