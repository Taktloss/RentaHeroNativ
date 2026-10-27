// SPDX-License-Identifier: GPL-3.0-or-later
#include "formats/pe.hpp"
#include "test.hpp"

using namespace rah;

namespace {

void put16(Bytes& b, std::size_t o, unsigned v) {
    if (b.size() < o + 2) b.resize(o + 2);
    b[o] = static_cast<std::uint8_t>(v);
    b[o + 1] = static_cast<std::uint8_t>(v >> 8);
}
void put32(Bytes& b, std::size_t o, std::uint32_t v) {
    put16(b, o, v & 0xFFFF);
    put16(b, o + 2, v >> 16);
}

// Kleinste PE32 mit einem Abschnitt (VA 0x1000, Datei 0x200) und einer Zeigergruppe 0x95 → Zeiger 2
// (8×2 Pixel, Hotspot (3,1)): obere Zeile Schwarz|Weiß|durchsichtig|invertiert je 2 Pixel.
Bytes miniPe() {
    Bytes f(0x200, 0);
    put16(f, 0, 0x5A4D);
    put32(f, 0x3C, 0x40);
    put32(f, 0x40, 0x00004550);
    put16(f, 0x46, 1);        // Abschnitte
    put16(f, 0x54, 224);      // Größe des optionalen Kopfs
    put16(f, 0x58, 0x10B);    // PE32
    put32(f, 0x58 + 96 + 16, 0x1000);  // Datenverzeichnis 2: Ressourcen
    const std::size_t s = 0x58 + 224;
    put32(f, s + 8, 0x200);
    put32(f, s + 12, 0x1000);
    put32(f, s + 16, 0x200);
    put32(f, s + 20, 0x200);

    Bytes r(0x200, 0);  // Ressourcenabschnitt, Versätze relativ zum Anfang
    auto dir = [&](std::size_t o, std::uint32_t id, std::uint32_t ptr) {
        put16(r, o + 14, 1);
        put32(r, o + 16, id);
        put32(r, o + 20, ptr);
    };
    dir(0x00, 1, 0x80000000u | 0x30);     // RT_CURSOR
    put16(r, 0x0E, 2);
    put32(r, 0x18, 12);                   // RT_GROUP_CURSOR
    put32(r, 0x1C, 0x80000000u | 0x48);
    dir(0x30, 2, 0x80000000u | 0x60);
    dir(0x48, 0x95, 0x80000000u | 0x78);
    dir(0x60, 0x407, 0x90);
    dir(0x78, 0x407, 0xA0);
    // Datenelemente: RVA, Größe
    const std::uint32_t cur = 0x100, grp = 0xC0;
    put32(r, 0x90, 0x1000 + cur);
    put32(r, 0x94, 4 + 40 + 8 + 2 * 4 * 2);
    put32(r, 0xA0, 0x1000 + grp);
    put32(r, 0xA4, 6 + 14);
    // Gruppe: reserviert, Typ 2, 1 Eintrag; Eintrag-ID 2
    put16(r, grp + 2, 2);
    put16(r, grp + 4, 1);
    put16(r, grp + 6 + 12, 2);
    // Zeiger: Hotspot, BITMAPINFOHEADER (8 × 4 = 2 Zeilen doppelt), Palette schwarz/weiß
    put16(r, cur, 3);
    put16(r, cur + 2, 1);
    put32(r, cur + 4, 40);
    put32(r, cur + 8, 8);
    put32(r, cur + 12, 4);
    put16(r, cur + 16, 1);
    put16(r, cur + 18, 1);
    put32(r, cur + 4 + 40 + 4, 0x00FFFFFF);
    const std::size_t x = cur + 4 + 40 + 8, a = x + 8;
    // von unten nach oben: Zeile 1 (unten) zuerst. Obere Zeile: XOR 00 11 00 11, AND 00 00 11 11
    r[x + 4] = 0b00110011;
    r[a + 4] = 0b00001111;
    r[a + 0] = 0xFF;  // untere Zeile ganz durchsichtig
    f.insert(f.end(), r.begin(), r.end());
    return f;
}

}  // namespace

TEST(pe_cursor_synthetic) {
    PeResources pe;
    REQUIRE(pe.parse(miniPe()));
    CHECK(!pe.find(12, 0x96));
    auto c = loadCursor(pe, 0x95);
    REQUIRE(c);
    CHECK(c->width == 8 && c->height == 2 && c->hotX == 3 && c->hotY == 1);
    using P = MonoCursor;
    const P::Pixel top[8] = {P::Black, P::Black, P::White, P::White, P::Transparent, P::Transparent, P::Invert, P::Invert};
    for (int i = 0; i < 8; ++i) CHECK(c->pixels[static_cast<std::size_t>(i)] == top[i]);
    for (int i = 8; i < 16; ++i) CHECK(c->pixels[static_cast<std::size_t>(i)] == P::Transparent);
    CHECK(!pe.parse(Bytes(10, 0)));
}

TEST(pe_original_cursors) {
    const auto dir = rahtest::dataDir();
    SKIP_IF(dir.empty());
    auto f = readFile(dir / "wineprefix/drive_c/Program Files (x86)/neoSoftware/Rent-A-Hero/Rent-A-Hero.exe");
    SKIP_IF(!f);
    PeResources pe;
    REQUIRE(pe.parse(std::move(*f)));
    // Hotspots laut tools/pe_cursors.py (MOUSE.md)
    const int hot[11][2] = {{2, 2}, {6, 5}, {10, 12}, {12, 10}, {14, 13}, {13, 11},
                            {10, 11}, {9, 9}, {11, 11}, {14, 9}, {10, 8}};
    for (std::uint32_t g = 0x95; g <= 0x9F; ++g) {
        auto c = loadCursor(pe, g);
        REQUIRE(c);
        CHECK(c->width == 32 && c->height == 32);
        CHECK(c->hotX == hot[g - 0x95][0] && c->hotY == hot[g - 0x95][1]);
        int opaque = 0;
        for (auto p : c->pixels) opaque += p == MonoCursor::Black || p == MonoCursor::White;
        CHECK(opaque > 20);
    }
}
