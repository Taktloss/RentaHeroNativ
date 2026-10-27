// SPDX-License-Identifier: GPL-3.0-or-later
#include "render/blit.hpp"
#include "test.hpp"

using namespace rah;

namespace {

TccPlanes solid(unsigned w, unsigned h, std::uint8_t r, std::uint8_t g, std::uint8_t b, int alpha) {
    TccPlanes p;
    p.width = w;
    p.height = h;
    const std::size_t n = std::size_t{w} * h;
    p.rgb = {std::vector<std::uint8_t>(n, r), std::vector<std::uint8_t>(n, g), std::vector<std::uint8_t>(n, b)};
    if (alpha >= 0) p.alpha.assign(n, static_cast<std::uint8_t>(alpha));
    return p;
}

}  // namespace

TEST(pack565_truncates) {
    CHECK(pack565(0xFF, 0xFF, 0xFF) == 0xFFFF);
    CHECK(pack565(0x07, 0x03, 0x07) == 0x0000);  // abschneiden, nicht runden
    CHECK(pack565(0x08, 0x04, 0x08) == ((1 << 11) | (1 << 5) | 1));
}

TEST(alpha_divides_by_256) {
    Surface565 s;
    s.row(0)[0] = 0x0000;  // schwarz
    const auto p = solid(1, 1, 0xF8, 0, 0, 254);
    blitAlpha565(s, p, 0, 0, {0, 0, 1, 1}, nullptr);
    // 0 + (248·254 >> 8) = 246 → & 0xF8 = 240 → 30 << 11: weiß bleibt nicht ganz erreicht
    CHECK((s.row(0)[0] >> 11) == 30);
}

TEST(alpha_zero_keeps_255_replaces) {
    Surface565 s;
    s.row(0)[0] = 0x1234;
    s.row(0)[1] = 0x1234;
    TccPlanes p = solid(2, 1, 0xFF, 0xFF, 0xFF, 0);
    p.alpha[1] = 255;
    blitAlpha565(s, p, 0, 0, {0, 0, 2, 1}, nullptr);
    CHECK(s.row(0)[0] == 0x1234);
    CHECK(s.row(0)[1] == 0xFFFF);
}

TEST(depth_strictly_greater) {
    Surface565 s;
    std::vector<std::uint8_t> map(640 * 360, 50);
    map[1] = 51;
    const auto p = solid(2, 1, 0xFF, 0xFF, 0xFF, 255);
    DepthTest t{map.data(), 50, 0};
    blitAlpha565(s, p, 0, 0, {0, 0, 2, 1}, &t);
    CHECK(s.row(0)[0] == 0);       // depth == Z → nicht gezeichnet
    CHECK(s.row(0)[1] == 0xFFFF);  // depth > Z
}

TEST(zoom_nearest_7bit) {
    // 3 Spalten auf 2 skaliert: step = (3<<7)/2 = 192 → Spalten 0, 1
    Surface565 s;
    TccPlanes p = solid(3, 1, 0, 0, 0, 255);
    p.rgb[0] = {0x00, 0xF8, 0x80};
    blitAlphaZoom565(s, p, {0, 0, 2, 1}, {0, 0, 2, 1}, 0, 0, nullptr);
    CHECK(s.row(0)[0] == 0x0000);
    CHECK(s.row(0)[1] == pack565(0xF8, 0, 0));
}
