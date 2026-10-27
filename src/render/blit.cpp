// SPDX-License-Identifier: GPL-3.0-or-later
#include "render/blit.hpp"

#include <vector>

namespace rah {

void Surface565::fill(int x0, int y0, int x1, int y1, std::uint16_t c) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > kWidth) x1 = kWidth;
    if (y1 > kHeight) y1 = kHeight;
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) row(y)[x] = c;
    }
}

namespace {

inline bool inside(int x, int y) {
    return x >= 0 && y >= 0 && x < Surface565::kWidth && y < Surface565::kHeight;
}

// orig: Rent-A-Hero.exe 0x00439A80 – Mischung je Kanal, Division durch 256
inline std::uint16_t blend565(std::uint16_t d, unsigned r, unsigned g, unsigned b, unsigned a) {
    if (a == 255) return pack565(r, g, b);
    const int dR = (d >> 8) & 0xF8;
    const int dG = (d & 0x7E0) >> 3;
    const int dB = (d & 0x1F) << 3;
    const int ia = static_cast<int>(a);
    const unsigned cR = static_cast<unsigned>(dR + (((static_cast<int>(r) - dR) * ia) >> 8)) & 0xFF;
    const unsigned cG = static_cast<unsigned>(dG + (((static_cast<int>(g) - dG) * ia) >> 8)) & 0xFF;
    const unsigned cB = static_cast<unsigned>(dB + (((static_cast<int>(b) - dB) * ia) >> 8)) & 0xFF;
    return pack565(cR, cG, cB);
}

inline bool depthPass(const DepthTest* t, int screenX, int screenY) {
    if (!t || !t->map) return true;
    const int sy = screenY - t->sceneTop;
    if (sy < 0 || sy >= 360 || screenX < 0 || screenX >= 640) return true;  // UNVERIFIED: außerhalb der Karte
    return t->map[static_cast<std::size_t>(sy) * 640 + static_cast<std::size_t>(screenX)] > t->z;
}

}  // namespace

void blitOpaque565(Surface565& s, const TccPlanes& p, int srcX, int srcY, const Rect& dst) {
    for (int j = 0; j < dst.height(); ++j) {
        const std::size_t srow = static_cast<std::size_t>(srcY + j) * p.width;
        for (int i = 0; i < dst.width(); ++i) {
            const int x = dst.left + i, y = dst.top + j;
            if (!inside(x, y)) continue;
            const std::size_t k = srow + static_cast<std::size_t>(srcX + i);
            s.row(y)[x] = pack565(p.rgb[0][k], p.rgb[1][k], p.rgb[2][k]);
        }
    }
}

void blitAlpha565(Surface565& s, const TccPlanes& p, int srcX, int srcY, const Rect& dst,
                  const DepthTest* depth) {
    for (int j = 0; j < dst.height(); ++j) {
        const std::size_t srow = static_cast<std::size_t>(srcY + j) * p.width;
        for (int i = 0; i < dst.width(); ++i) {
            const int x = dst.left + i, y = dst.top + j;
            if (!inside(x, y)) continue;
            const std::size_t k = srow + static_cast<std::size_t>(srcX + i);
            const unsigned a = p.alpha[k];
            if (a == 0 || !depthPass(depth, x, y)) continue;
            std::uint16_t& d = s.row(y)[x];
            d = blend565(d, p.rgb[0][k], p.rgb[1][k], p.rgb[2][k], a);
        }
    }
}

void blitAlphaZoom565(Surface565& s, const TccPlanes& p, const Rect& full, const Rect& dst,
                      int clipL, int clipT, const DepthTest* depth) {
    if (full.width() <= 0 || full.height() <= 0) return;
    const unsigned stepX = (p.width << 7) / static_cast<unsigned>(full.width());
    const unsigned stepY = (p.height << 7) / static_cast<unsigned>(full.height());
    // Ursprung und Zeile getrennt abgerundet (Original-Eigenheit, docs/RENDERER.md)
    const std::size_t origin = ((static_cast<unsigned>(clipL) * stepX) >> 7) +
                               ((static_cast<unsigned>(clipT) * stepY) >> 7) * std::size_t{p.width};
    std::vector<unsigned> col(static_cast<std::size_t>(dst.width()));
    for (int i = 0; i < dst.width(); ++i) col[static_cast<std::size_t>(i)] = (static_cast<unsigned>(i) * stepX) >> 7;
    const std::size_t total = std::size_t{p.width} * p.height;
    for (int j = 0; j < dst.height(); ++j) {
        const std::size_t srow = origin + ((static_cast<unsigned>(j) * stepY) >> 7) * std::size_t{p.width};
        for (int i = 0; i < dst.width(); ++i) {
            const int x = dst.left + i, y = dst.top + j;
            const std::size_t k = srow + col[static_cast<std::size_t>(i)];
            if (!inside(x, y) || k >= total) continue;
            const unsigned a = p.alpha[k];
            if (a == 0 || !depthPass(depth, x, y)) continue;
            std::uint16_t& d = s.row(y)[x];
            d = blend565(d, p.rgb[0][k], p.rgb[1][k], p.rgb[2][k], a);
        }
    }
}

}  // namespace rah
