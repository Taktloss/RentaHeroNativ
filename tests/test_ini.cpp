// SPDX-License-Identifier: GPL-3.0-or-later
#include <cctype>

#include "core/bytes.hpp"
#include "core/strings.hpp"
#include "formats/ini.hpp"
#include "test.hpp"

using namespace rah;

namespace {

IniFile ini(std::string_view s) {
    return IniFile(std::span(reinterpret_cast<const std::uint8_t*>(s.data()), s.size()));
}

}  // namespace

TEST(ini_case_insensitive_and_trim) {
    const auto f = ini("[General]\r\n  Layer1 = S7_Back \r\nZPos=010\r\n[layer]\r\nName=\"x y\"\r\n");
    CHECK(f.getString("GENERAL", "layer1", "NULL") == "S7_Back");
    CHECK(f.getInt("general", "Zpos", 0) == 10);
    CHECK(f.getString("Layer", "name", "") == "x y");
    CHECK(f.getString("Layer", "fehlt", "NULL") == "NULL");
    CHECK(f.getInt("Layer", "fehlt", -1) == -1);
}

TEST(ini_int_like_atoi) {
    const auto f = ini("[a]\nx=-12abc\ny=abc\nz=\n");
    CHECK(f.getInt("a", "x", 5) == -12);
    CHECK(f.getInt("a", "y", 5) == 0);
    CHECK(f.getInt("a", "z", 5) == 0);
}

TEST(ini_first_wins_and_truncate) {
    const auto f = ini("[a]\nk=1\nk=2\n[A]\nk=3\nlang=abcdef\n");
    CHECK(f.getInt("a", "k", 0) == 1);
    CHECK(f.duplicateKeys() == 1 && f.duplicateSections() == 1);
    CHECK(f.getString("a", "lang", "", 4).empty());  // nur in der zweiten Sektion
    const auto g = ini("[a]\nlang=abcdef\n");
    CHECK(g.getString("a", "lang", "", 4) == "abc");
}

// Alle INI-artigen Originaldateien: Doppelte prüfen und Integer-Werte auf Sonderformen.
// Befund: Doppelte (S31.SZN [S31_Back], LOC19/21/41.GER) haben identische Werte.
TEST(ini_original_files) {
    const auto dir = rahtest::dataDir();
    SKIP_IF(dir.empty());
    int files = 0, dupS = 0, dupK = 0, hexLike = 0;
    for (const auto& e : std::filesystem::recursive_directory_iterator(dir / "RentAHero")) {
        const std::string ext = toUpperAscii(e.path().extension().string());
        if (ext != ".SZN" && ext != ".GER" && ext != ".INI") continue;
        const auto data = readFile(e.path());
        const IniFile f(*data);
        ++files;
        if (f.duplicateSections() || f.duplicateKeys()) {
            std::cout << "     doppelt: " << e.path().filename().string() << " Sektionen " << f.duplicateSections()
                      << ", Schlüssel " << f.duplicateKeys() << "\n";
        }
        dupS += f.duplicateSections();
        dupK += f.duplicateKeys();
        for (const auto& s : f.sections()) {
            for (const auto& k : f.keys(s)) {
                const auto v = f.find(s, k).value_or("");
                // nur Spieldateien; die Treiber-INIs (Indeo/DirectX) enthalten Hex-Werte
                if (ext != ".INI" && v.size() > 2 && v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) ++hexLike;
            }
        }
    }
    std::cout << "     " << files << " Dateien, " << dupS << " doppelte Sektionen, " << dupK
              << " doppelte Schlüssel, " << hexLike << " Hex-artige Werte\n";
    CHECK(files > 100);
    CHECK(hexLike == 0);
}
