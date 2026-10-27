// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/bytes.hpp"
#include "core/strings.hpp"
#include "engine/paths.hpp"
#include "formats/szn.hpp"
#include "test.hpp"
#include "vfs/dirfs.hpp"

using namespace rah;

namespace {

IniFile ini(std::string_view s) {
    return IniFile(std::span(reinterpret_cast<const std::uint8_t*>(s.data()), s.size()));
}

}  // namespace

TEST(make_file_name) {
    CHECK(makeFileName("S1_Back", "bmp") == "S1_Back.bmp");
    CHECK(makeFileName("S1_Back.bmp", "bmp") == "S1_Back.bmp");
    CHECK(makeFileName("a.b", "hlp") == "a.b.hlp");      // kurze Endung: angehängt
    CHECK(makeFileName(".wav", "wav") == ".wav.wav");    // genau 4 Zeichen: angehängt
}

TEST(layer_kind_case_sensitive) {
    CHECK(layerKindFromName("a.TCC") == LayerKind::Tcc);
    CHECK(layerKindFromName("a.bmp") == LayerKind::Bmp);
    CHECK(layerKindFromName("a.Bmp") == LayerKind::Unknown);
}

TEST(szn_loops_and_loadcond) {
    const auto d = readSceneDesc(ini("[GENERAL]\nLayer1=A\nLayer2=B\nLayer4=D\nArea2=X\n"
                                     "[A]\nFilename=a.TCC\nLoadCond=1\n[B]\nFilename=b.a16\nLoadCond=3\nfps=10\n"),
                                 1);
    REQUIRE(d.layers.size() == 1);  // B: 3 & 1 != 3; D nach Lücke nicht gelesen
    CHECK(d.layers[0].section == "A" && d.layers[0].z == 1 && d.layers[0].zoomVal == 100);
    CHECK(d.skippedByLoadCond == 1);
    CHECK(d.areas.empty());  // Area1 fehlt → Abbruch
    CHECK(!d.background && !d.characters[0]);
}

TEST(szn_character_sfx_rule) {
    const auto d = readSceneDesc(ini("[GENERAL]\nCharacter2=R\n[R]\nFilename=r.a16\nSFXRight=x.wav\n"
                                     "RedAdd10=7\nZpos=40\n"), 0);
    REQUIRE(d.characters[1].has_value());
    const auto& c = *d.characters[1];
    CHECK(!c.sfxLeft && !c.sfxRight);  // SFXRight nur mit SFXLeft
    CHECK(c.colorAdd[9].red == 7 && c.zpos == 40 && c.zoomVal == 0 && c.perCor == 100);
}

// Alle 77 SZN lesen und jede referenzierte Datei mit der Suchreihenfolge des Originals auflösen.
// Ohne Common-Ordner fehlen nur Dateien aus DATA1.CAB (26 Namen, alle im Manifest install.sha256).
TEST(szn_all_original_resolve) {
    const auto dir = rahtest::dataDir();
    SKIP_IF(dir.empty());
    DirFileSystem cd(dir / "RentAHero");
    // installierter Common-Ordner (Wine-Prefix), falls vorhanden
    const auto commonDir = dir / "wineprefix/drive_c/Program Files (x86)/neoSoftware/Rent-A-Hero/Common";
    const bool haveCommon = std::filesystem::is_directory(commonDir);
    DirFileSystem common(commonDir);
    DataPaths paths(&cd, haveCommon ? &common : nullptr);
    int files = 0, layers = 0, areas = 0, chars = 0, refs = 0, missing = 0;
    for (const auto& e : std::filesystem::recursive_directory_iterator(dir / "RentAHero" / "GAME")) {
        if (toUpperAscii(e.path().extension().string()) != ".SZN") continue;
        const std::string folder = toUpperAscii(e.path().parent_path().filename().string());
        paths.setLocation(folder == "MENU" ? 0 : std::stoi(folder.substr(3)));
        const IniFile f(*readFile(e.path()));
        const auto d = readSceneDesc(f, 0xFFFFFFFFu);
        ++files;
        auto need = [&](const std::string& name) {
            ++refs;
            if (!paths.findPath(name)) {
                ++missing;
                std::cout << "     fehlt: " << folder << "/" << e.path().filename().string() << " → " << name << "\n";
            }
        };
        if (d.background) {
            need(d.background->filename);
            if (d.background->zBuf) need(*d.background->zBuf);
            if (d.background->wBuf) need(*d.background->wBuf);
        }
        for (const auto& l : d.layers) {
            need(l.filename);
            CHECK(layerKindFromName(l.filename) != LayerKind::Unknown);
        }
        for (const auto& c : d.characters) {
            if (!c) continue;
            ++chars;
            need(c->filename);
            for (const auto& s : {c->sfxLeft, c->sfxRight, c->sfxAlt}) if (s) need(*s);
        }
        layers += static_cast<int>(d.layers.size());
        areas += static_cast<int>(d.areas.size());
    }
    std::cout << "     " << files << " SZN, " << layers << " Ebenen, " << areas << " Bereiche, " << chars
              << " Figuren, " << refs << " Dateiverweise, " << missing << " nicht gefunden\n";
    CHECK(files == 77);
    if (haveCommon) CHECK(missing == 0);
}
