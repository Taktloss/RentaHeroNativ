// SPDX-License-Identifier: GPL-3.0-or-later
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>

#include "test.hpp"
#include "video/iv5mask.hpp"

using namespace rah;

namespace {

// LE-Bitschreiber zum Bauen von Testbildern
struct BitWriter {
    std::vector<std::uint8_t> d;
    std::size_t p = 0;
    void put(std::uint32_t v, int n) {
        for (int i = 0; i < n; ++i, ++p) {
            if ((p >> 3) >= d.size()) d.push_back(0);
            if (v >> i & 1) d[p >> 3] |= static_cast<std::uint8_t>(1u << (p & 7));
        }
    }
    void align() {
        while (p & 7) put(0, 1);
    }
};

// Schlüsselbild 8×3 ohne Bilddaten (3 leere Bänder) mit Maske aus Lauflängen
std::vector<std::uint8_t> keyFrame(bool delta, const std::vector<std::uint32_t>& runs, bool first) {
    BitWriter w;
    w.put(0x1F, 5);
    w.put(0, 3);  // Schlüsselbild
    w.put(0, 8);
    w.put(0x08, 8);       // GOP: Transparenz
    w.put(0, 3);          // 1 Luma-Band, je 1 Chroma-Band
    w.put(15, 4);
    w.put((8u << 13) | 3u, 26);  // 8 × 3
    w.put(0, 6);          // Bandbeschreibung Luma
    w.put(0, 6);          // Chroma
    w.put(0, 4);          // Transparenz ohne Füllfarbe
    w.align();
    w.put(0, 24);         // Version
    w.align();
    w.put(0x80, 8);       // Bild-Flags: Bandlängen
    w.put(0, 3);
    w.align();
    for (int i = 0; i < 3; ++i) w.put(1, 8);  // leere Bänder
    w.put(0, 3);          // keine Rechtecke
    w.put(delta ? 1 : 0, 1);
    w.put(1, 1);          // eigenes Codebuch: 1 Zeile mit 5 Bit → Symbole 0…31, Länge 5
    w.put(1, 4);
    w.put(5, 4);
    w.align();
    w.put(0, 1);          // Kachel nicht gefüllt
    w.put(0, 1);          // ohne Größe
    w.align();
    w.put(first ? 1 : 0, 1);  // Startwert
    for (std::uint32_t s : runs) {
        std::uint32_t rev = 0;
        for (int k = 0; k < 5; ++k)
            if (s & (1u << k)) rev |= 1u << (4 - k);
        w.put(rev, 5);
    }
    w.put(0, 32);
    return w.d;
}

}  // namespace

TEST(iv5mask_runs_and_row_delta) {
    Indeo5Mask m;
    // Zeilen sind 32 breit (aufgerundet): 2 aus, 3 an, 27 aus | 31 an, 1 aus | 31 an, 1 aus
    auto f = keyFrame(false, {2, 3, 27, 31, 1, 31, 1}, false);
    REQUIRE(m.decode(f.data(), f.size()) == Indeo5Mask::Result::Mask);
    REQUIRE(m.width() == 8 && m.height() == 3);
    const auto& a = m.mask();
    CHECK(a[0] == 0 && a[1] == 0 && a[2] == 1 && a[4] == 1 && a[5] == 0);
    CHECK(a[8] == 1 && a[15] == 1 && a[16] == 1 && a[23] == 1);
    // Delta: jede Zeile XOR mit der fertigen Zeile darüber
    // roh: [1,1,0…] | [1×31,0] | [1×31,0] → [1,1,0…] | [0,0,1…] | [1,1,0…]
    auto g = keyFrame(true, {2, 30, 31, 1, 31, 1}, true);
    REQUIRE(m.decode(g.data(), g.size()) == Indeo5Mask::Result::Mask);
    const auto& b = m.mask();
    CHECK(b[0] == 1 && b[1] == 1 && b[2] == 0);
    CHECK(b[8] == 0 && b[9] == 0 && b[10] == 1);
    CHECK(b[16] == 1 && b[17] == 1 && b[18] == 0);
}

TEST(iv5mask_zero_symbol_is_255_without_toggle) {
    Indeo5Mask m;
    // Symbol 0 = 255 Pixel ohne Wechsel: 96 Pixel gesamt → alles gleich, nur Startwert zählt
    auto f = keyFrame(false, {0}, true);
    REQUIRE(m.decode(f.data(), f.size()) == Indeo5Mask::Result::Mask);
    for (auto v : m.mask()) CHECK(v == 1);
}

TEST(iv5mask_original_glider) {
    // installierter Common-Ordner (die Gleiter-Videos liegen nur dort)
    const char* root = std::getenv("RAH_TEST_COMMON");
    SKIP_IF(!root);
    // Werte aus dem Prototyp, der pixelgenau mit ir50_32.dll übereinstimmt (tools/iv5ref)
    std::ifstream in(std::string(root) + "/S1_RodOnGlider.avi", std::ios::binary);
    REQUIRE(in);
    const std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const auto* bytes = &data;
    // Videobilder („00db“/„00dc“) der Reihe nach
    std::vector<std::pair<std::size_t, std::size_t>> fr;
    for (std::size_t a = 0; a + 8 <= bytes->size();) {
        const auto* p = bytes->data() + a;
        const std::size_t n = std::size_t{p[4]} | std::size_t{p[5]} << 8 | std::size_t{p[6]} << 16 | std::size_t{p[7]} << 24;
        if (std::string(reinterpret_cast<const char*>(p), 4) == "RIFF" || std::string(reinterpret_cast<const char*>(p), 4) == "LIST") {
            a += 12;
            continue;
        }
        if (p[0] == '0' && p[1] == '0' && p[2] == 'd' && (p[3] == 'b' || p[3] == 'c') && n > 0) fr.emplace_back(a + 8, n);
        a += 8 + n + (n & 1);
    }
    REQUIRE(fr.size() >= 4);
    Indeo5Mask m;
    const int expect[4] = {0, 35758, 35614, 36094};
    for (int i = 0; i < 4; ++i) {
        const auto r = m.decode(bytes->data() + fr[static_cast<std::size_t>(i)].first, fr[static_cast<std::size_t>(i)].second);
        if (r != Indeo5Mask::Result::Mask) std::fprintf(stderr, "Bild %d: Ergebnis %d\n", i, static_cast<int>(r));
        REQUIRE(r == Indeo5Mask::Result::Mask);
        int ones = 0;
        for (auto v : m.mask()) ones += v;
        CHECK(ones == expect[i]);
    }
}
