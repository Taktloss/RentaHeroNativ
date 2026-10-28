// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/area.hpp"
#include "render/font.hpp"
#include "test.hpp"

using namespace rah;

TEST(font_cp1252) {
    CHECK(cp1252ToUnicode('A') == U'A');
    CHECK(cp1252ToUnicode(0xFC) == U'ü');  // ü
    CHECK(cp1252ToUnicode(0x80) == U'€');  // €
    CHECK(cp1252ToUnicode(0x84) == U'„');  // „
}

TEST(font_area_text_align) {
    // w = len · 8 / 2 = 20 bei 5 Zeichen
    CHECK(areaTextAlign(5, 19) == TextAlign::Left);
    CHECK(areaTextAlign(5, 20) == TextAlign::Center);
    CHECK(areaTextAlign(5, 620) == TextAlign::Center);
    CHECK(areaTextAlign(5, 621) == TextAlign::Right);
}

TEST(font_draw_area_text_outline) {
    const std::string file = Font::findSystemFont(true, true);
    SKIP_IF(file.empty());
    Font font;
    REQUIRE(font.open(file, 15));
    CHECK(font.textWidth("T\xFCr") > 0);
    CHECK(font.textWidth("WW") > font.textWidth("ii"));

    Surface565 s(200, 40);
    s.fill(0, 0, 200, 40, 0x1234);
    Area a;
    a.name = "007a_Tuer";  // ohne Anzeigetext → interner Name
    drawAreaText(font, s, a, 100, 25);
    int white = 0, black = 0, minX = 200, maxX = -1, maxY = -1;
    for (int y = 0; y < s.height; ++y)
        for (int x = 0; x < s.width; ++x) {
            const auto p = s.row(y)[x];
            if (p == 0xFFFF) {
                ++white;
                minX = std::min(minX, x);
                maxX = std::max(maxX, x);
                maxY = std::max(maxY, y);
            }
            if (p == 0x0000) ++black;
        }
    CHECK(white > 20 && black > white);
    CHECK(maxY <= 25 + 4);  // Grundlinie y = 25 (nur „_“ reicht darunter)
    const int w = font.textWidth("007a_Tuer");
    CHECK(minX >= 100 - w / 2 - 2 && maxX <= 100 + w / 2 + 2);  // zentriert

    Surface565 e(10, 10);
    Area none;
    drawAreaText(font, e, none, 5, 5);  // Text und Name leer → nichts
    for (auto p : e.pixels) CHECK(p == 0);
}

TEST(font_wrap_text_lines) {
    Font f;
    const std::string file = Font::findSystemFont(false, false);
    if (file.empty() || !f.open(file, 15)) throw rahtest::Skip{};
    const int wA = f.textWidth("aaaa");
    // Breite für genau zwei Wörter
    const auto lines = wrapText(f, "aaaa aaaa aaaa\r\n\r\nbb", f.textWidth("aaaa aaaa"));
    REQUIRE(lines.size() == 4);
    CHECK(lines[0] == "aaaa aaaa" && lines[1] == "aaaa" && lines[2].empty() && lines[3] == "bb");
    // zu langes Wort bleibt ungebrochen
    const auto one = wrapText(f, "aaaaaaaa", wA);
    CHECK(one.size() == 1 && one[0] == "aaaaaaaa");
}
