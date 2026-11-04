// SPDX-License-Identifier: GPL-3.0-or-later
// Vergleicht die Schriftdarstellung des Ports mit der GDI-Messung unter Wine
// (tools/wineprobe/fontprobe.c → font.out). Gleiche Fälle, Texte und Position.
// Aufruf: rah_fontcmp <font.out> [<ttf für alle Fälle>]    docs/WINEPROBE.md
// Mit TTF: dieselbe Datei wie Wine (z. B. Wines arial.ttf) – vergleicht nur die Darstellungsregeln.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "core/bytes.hpp"
#include "render/font.hpp"
#include "render/surface.hpp"

using namespace rah;

namespace {
constexpr int kW = 200, kH = 28, kX = 10, kBase = 20;
struct Case { int h; bool bold, italic; const char* name; };
constexpr Case kCases[] = {{15, true, true, "Bereichsname 15 fett kursiv"}, {15, false, false, "Hilfetext 15"},
                           {12, false, false, "Renderer 12"}};
const char* const kTexts[] = {
    "Rodrigo", "Hallo Welt", "B\xFCro", "Wagenrad", "iiiii", "WWWW", "Kneipe zum Ochsen",
    "\xC4rger?! 1234567890", "Megophias", "Jasmin & Ramil", "A", "Il1|", "Tr\xE4nke",
};
std::uint32_t rd(const std::uint8_t* p) { return p[0] | p[1] << 8 | p[2] << 16 | static_cast<std::uint32_t>(p[3]) << 24; }
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Aufruf: rah_fontcmp <font.out>\n";
        return 2;
    }
    const auto data = readFile(argv[1]);
    if (!data) return 1;
    const std::size_t rec = 16 + kW * kH;
    std::size_t off = 0;
    int totalDiff = 0, widthDiff = 0, n = 0;
    for (const Case& c : kCases) {
        Font f;
        std::string err;
        const std::string file = argc > 2 ? std::string(argv[2]) : Font::findSystemFont(c.bold, c.italic);
        if (!f.open(file, c.h, &err)) {
            std::cerr << file << ": " << err << "\n";
            return 1;
        }
        std::cout << c.name << " (" << file.substr(file.find_last_of('/') + 1) << "): Zelle " << f.cellHeight()
                  << ", Oberlänge " << f.ascent() << "\n";
        for (const char* t : kTexts) {
            if (off + rec > data->size()) return 1;
            const std::uint8_t* p = data->data() + off;
            off += rec;
            const int cx = static_cast<int>(rd(p + 8));
            Surface565 s(kW, kH);
            f.textOut(s, kX, kBase, t, TextAlign::Left, 0xFFFF);
            int diff = 0, onlyGdi = 0, onlyPort = 0;
            for (int i = 0; i < kW * kH; ++i) {
                const bool g = p[16 + i] != 0, q = s.pixels[static_cast<std::size_t>(i)] != 0;
                if (g != q) {
                    ++diff;
                    (g ? onlyGdi : onlyPort)++;
                }
            }
            const int pw = f.textWidth(t);
            ++n;
            totalDiff += diff;
            widthDiff += pw != cx;
            if (diff || pw != cx)
                std::cout << "  \"" << t << "\": Breite GDI " << cx << " Port " << pw << ", Pixel verschieden "
                          << diff << " (nur GDI " << onlyGdi << ", nur Port " << onlyPort << ")\n";
        }
    }
    std::cout << n << " Texte: " << widthDiff << " mit anderer Breite, " << totalDiff << " Pixel verschieden\n";
    return 0;
}
