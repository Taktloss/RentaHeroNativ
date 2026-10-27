// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/strings.hpp"
#include "formats/tcc.hpp"
#include "test.hpp"
#include "vfs/dirfs.hpp"

using namespace rah;

TEST(rle_ops) {
    // Füllen 3×7, Überspringen 2, Kopieren 2 Bytes
    const std::uint8_t src[] = {1, 3, 7, 2, 2, 0, 2, 9, 8};
    std::vector<std::uint8_t> dst(8, 0xEE);
    REQUIRE(tccRleDecode(src, dst));
    const std::vector<std::uint8_t> want = {7, 7, 7, 0xEE, 0xEE, 9, 8, 0xEE};
    CHECK(dst == want);
}

TEST(rle_overflow_rejected) {
    const std::uint8_t src[] = {1, 9, 7};
    std::vector<std::uint8_t> dst(4);
    CHECK(!tccRleDecode(src, dst));
}

TEST(expand4_keeps_first_pair) {
    std::vector<std::uint8_t> p = {0xAB, 0xCD, 0xEF, 0x12, 0, 0};
    tccExpand4(p, 6);
    // Paare (4,5) aus p[2], (2,3) aus p[1]; Paar (0,1) bleibt wie im Original unverändert
    const std::vector<std::uint8_t> want = {0xAB, 0xCD, 0xC0, 0xD0, 0xE0, 0xF0};
    CHECK(p == want);
}

TEST(color_add_clamps) {
    TccPlanes p;
    p.width = 2;
    p.height = 1;
    p.rgb = {std::vector<std::uint8_t>{250, 10}, std::vector<std::uint8_t>{0, 0}, std::vector<std::uint8_t>{5, 200}};
    TccColorAdd{10, 0, -20}.apply(p);
    CHECK(p.rgb[0][0] == 255 && p.rgb[0][1] == 20);
    CHECK(p.rgb[2][0] == 0 && p.rgb[2][1] == 180);
}

// Alle TCC/A16-Dateien der Originaldaten dekodieren (docs/formats/tcc.md: 102 TCC, 219 A16).
TEST(decode_all_original_images) {
    const auto dir = rahtest::dataDir();
    SKIP_IF(dir.empty());
    int tcc = 0, a16 = 0, bad = 0, frames = 0;
    for (const auto& e : std::filesystem::recursive_directory_iterator(dir / "RentAHero")) {
        const std::string ext = toUpperAscii(e.path().extension().string());
        if (ext != ".TCC" && ext != ".A16") continue;
        auto data = readFile(e.path());
        std::string err;
        if (ext == ".TCC") {
            TccImage img;
            if (img.load(*data, &err)) ++tcc;
            else if (e.path().filename() != "S26_RING.TCC") { ++bad; std::cerr << e.path() << ": " << err << "\n"; }
        } else {
            A16Anim anim;
            if (!anim.load(std::move(*data), &err)) { ++bad; std::cerr << e.path() << ": " << err << "\n"; continue; }
            int n = 1;
            anim.next();
            while (anim.next()) ++n;
            if (n != anim.frameCount()) ++bad;
            frames += n;
            ++a16;
        }
    }
    std::cout << "     " << tcc << " TCC, " << a16 << " A16 (" << frames << " Bilder), " << bad << " Fehler\n";
    CHECK(tcc == 102);  // 103 Dateien, S26_RING.TCC ist ungültig
    CHECK(a16 == 219);
    CHECK(bad == 0);
}

#include "formats/zbf.hpp"

// docs/formats/zbf.md: 119 Dateien, Köpfe mit 20 oder 48 Bytes.
TEST(zbf_all_original_maps) {
    const auto dir = rahtest::dataDir();
    SKIP_IF(dir.empty());
    int ok = 0, bad = 0;
    for (const auto& e : std::filesystem::recursive_directory_iterator(dir / "RentAHero")) {
        if (toUpperAscii(e.path().extension().string()) != ".ZBF") continue;
        ZMap m;
        std::string err;
        if (loadZMap(*readFile(e.path()), m, &err) && m.dv != 0) ++ok;
        else { ++bad; std::cerr << e.path() << ": " << err << "\n"; }
    }
    std::cout << "     " << ok << " ZBF, " << bad << " Fehler\n";
    CHECK(ok == 119);
    CHECK(bad == 0);
}

namespace {

void put32le(Bytes& b, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) b.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
}

// A16 aus 2×2-Bildern, Rotwert = 10·(i+1); `key[i]` = Schlüsselbild
Bytes makeA16(const std::vector<bool>& key) {
    Bytes f;
    for (std::size_t i = 0; i < key.size(); ++i) {
        put32le(f, key[i] ? 0 : 0x10);
        for (int c = 0; c < 3; ++c) put32le(f, 3);
        put32le(f, 0);
        put32le(f, 2);
        put32le(f, 2);
        const auto r = static_cast<std::uint8_t>(10 * (i + 1));
        for (std::uint8_t v : {r, std::uint8_t{0}, std::uint8_t{0}}) {
            f.push_back(1);
            f.push_back(4);
            f.push_back(v);
        }
    }
    return f;
}

int shown(const A16Anim& a) {
    return a.planes().rgb[0][0] / 10 - 1;
}

}  // namespace

TEST(a16_next_hold_and_end) {
    A16Anim a;
    REQUIRE(a.load(makeA16({true, false, false}), nullptr));
    CHECK(shown(a) == 0 && a.counter() == 0 && a.hold());
    CHECK(a.next() && shown(a) == 0);  // erstes Next nach dem Laden bleibt stehen
    CHECK(a.next() && shown(a) == 1 && a.counter() == 1);
    CHECK(a.next() && shown(a) == 2);
    CHECK(!a.next());
    a.rewind();
    CHECK(shown(a) == 0 && a.counter() == 0 && !a.hold());
    CHECK(a.next() && shown(a) == 1);  // Rewind setzt das Halte-Flag nicht
}

TEST(a16_seek_semantics) {
    A16Anim a;
    REQUIRE(a.load(makeA16({true, false, false, true, false}), nullptr));
    a.next();
    a.next();  // Bild 1
    CHECK(a.seek(3) && shown(a) == 1 && a.counter() == 2);  // Schlüsselbild: noch nicht dekodiert
    CHECK(a.next() && shown(a) == 3 && a.counter() == 3);
    CHECK(a.seek(0) && a.counter() == -1);
    CHECK(a.next() && shown(a) == 0 && a.counter() == 0);
    CHECK(a.seek(2) && shown(a) == 1 && a.counter() == 1);  // Deltabild: bis n−1 dekodiert
    CHECK(a.next() && shown(a) == 2);
    CHECK(a.seek(2) && a.hold());  // n == Zähler → nächstes Next bleibt stehen
    CHECK(a.next() && shown(a) == 2);
    // Original-Eigenheit: rückwärts auf ein Deltabild, dessen Schlüsselbild älter als der
    // Zähler ist → es wird vom aktuellen Bild aus weiterdekodiert
    a.next();
    a.next();  // Bild 4, Zähler 4
    CHECK(a.seek(1) && shown(a) == 4 && a.counter() == 0);
}
