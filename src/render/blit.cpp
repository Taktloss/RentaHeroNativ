// SPDX-License-Identifier: GPL-3.0-or-later
#include "render/blit.hpp"

#include <vector>

namespace rah {

void Surface565::fill(int x0, int y0, int x1, int y1, std::uint16_t c) {
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > width) x1 = width;
    if (y1 > height) y1 = height;
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) row(y)[x] = c;
    }
}

namespace {

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

inline bool depthPass(const DepthTest* t, long long index) {
    if (!t || !t->map) return true;
    if (index < 0 || index >= 640LL * 360) return false;  // UNVERIFIED: Lesen außerhalb der Karte
    return t->map[static_cast<std::size_t>(index)] > t->z;
}

inline long long lin(const Surface565& s, int x, int y) {
    return static_cast<long long>(y) * s.width + x;
}

}  // namespace

void blitOpaque565(Surface565& s, const TccPlanes& p, int dstX, int dstY) {
    const int w = static_cast<int>(p.width), h = static_cast<int>(p.height);
    std::size_t k = 0;
    for (int j = 0; j < h; ++j) {
        const long long base = lin(s, dstX, dstY + j);
        for (int i = 0; i < w; ++i, ++k) {
            if (std::uint16_t* d = s.at(base + i)) *d = pack565(p.rgb[0][k], p.rgb[1][k], p.rgb[2][k]);
        }
    }
}

void blitAlpha565(Surface565& s, const TccPlanes& p, std::size_t srcOff, int width, int height, int srcSkip,
                  int dstX, int dstY, const DepthTest* depth) {
    const std::size_t total = std::size_t{p.width} * p.height;
    std::size_t k = srcOff;
    for (int j = 0; j < height; ++j) {
        const long long base = lin(s, dstX, dstY + j);
        const long long dbase = static_cast<long long>(dstY + j) * 640 + dstX;
        for (int i = 0; i < width; ++i, ++k) {
            if (k >= total) return;  // Original liest hier über das Bild hinaus (UNVERIFIED, kommt nicht vor)
            const unsigned a = p.alpha[k];
            if (a == 0 || !depthPass(depth, dbase + i)) continue;
            if (std::uint16_t* d = s.at(base + i)) *d = blend565(*d, p.rgb[0][k], p.rgb[1][k], p.rgb[2][k], a);
        }
        k += static_cast<std::size_t>(srcSkip);
    }
}

void blitAlphaZoom565(Surface565& s, const TccPlanes& p, const Rect& full, const Rect& dst, int clipL, int clipT,
                      const DepthTest* depth) {
    if (full.width() <= 0 || full.height() <= 0) return;
    const unsigned stepX = (p.width << 7) / static_cast<unsigned>(full.width());
    const unsigned stepY = (p.height << 7) / static_cast<unsigned>(full.height());
    // Ursprung und Zeile getrennt abgerundet (Original-Eigenheit, docs/RENDERER.md)
    const std::size_t origin = ((static_cast<unsigned>(clipL) * stepX) >> 7) +
                               ((static_cast<unsigned>(clipT) * stepY) >> 7) * std::size_t{p.width};
    std::vector<unsigned> col(static_cast<std::size_t>(dst.width() > 0 ? dst.width() : 0));
    for (std::size_t i = 0; i < col.size(); ++i) col[i] = (static_cast<unsigned>(i) * stepX) >> 7;
    const std::size_t total = std::size_t{p.width} * p.height;
    for (int j = 0; j < dst.height(); ++j) {
        const std::size_t srow = origin + ((static_cast<unsigned>(j) * stepY) >> 7) * std::size_t{p.width};
        const long long base = lin(s, dst.left, dst.top + j);
        const long long dbase = static_cast<long long>(dst.top + j) * 640 + dst.left;
        for (int i = 0; i < dst.width(); ++i) {
            const std::size_t k = srow + col[static_cast<std::size_t>(i)];
            if (k >= total) continue;
            const unsigned a = p.alpha[k];
            if (a == 0 || !depthPass(depth, dbase + i)) continue;
            if (std::uint16_t* d = s.at(base + i)) *d = blend565(*d, p.rgb[0][k], p.rgb[1][k], p.rgb[2][k], a);
        }
    }
}

}  // namespace rah
