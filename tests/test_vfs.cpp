// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/strings.hpp"
#include "test.hpp"
#include "vfs/cue.hpp"
#include "vfs/dirfs.hpp"
#include "vfs/iso9660.hpp"
#include "vfs/vfs.hpp"

using namespace rah;

TEST(cue_mode2) {
    const auto t = parseCue("FILE \"Rent a Hero CD1.bin\" BINARY\r\n  TRACK 01 MODE2/2352\r\n    INDEX 01 00:00:00\r\n", "/x");
    REQUIRE(t.has_value());
    CHECK(t->binFile == std::filesystem::path("/x/Rent a Hero CD1.bin"));
    CHECK(t->sectorSize == 2352);
    CHECK(t->dataOffset == 24);
}

TEST(cue_audio_only) {
    CHECK(!parseCue("FILE \"a.bin\" BINARY\nTRACK 01 AUDIO\n", "/x").has_value());
}

TEST(split_path) {
    const auto p = splitPath("GAME\\Loc01//./S7.SZN");
    REQUIRE(p.size() == 3);
    CHECK(p[0] == "GAME" && p[2] == "S7.SZN");
    CHECK(equalsNoCase("Loc01", "LOC01"));
}

// Jede Datei des zusammengeführten Ordners muss aus einem der beiden Images byte-gleich lesbar sein.
TEST(iso_matches_merged_folder) {
    const auto dir = rahtest::dataDir();
    SKIP_IF(dir.empty());
    Vfs vfs;
    std::string err;
    REQUIRE(vfs.mountPath(dir / "Rent a Hero CD1.cue", &err));
    REQUIRE(vfs.mountPath(dir / "Rent a Hero CD2.cue", &err));
    DirFileSystem merged(dir / "RentAHero");
    int checked = 0, missing = 0, diff = 0;
    for (const auto& e : std::filesystem::recursive_directory_iterator(dir / "RentAHero")) {
        if (!e.is_regular_file()) continue;
        const auto rel = std::filesystem::relative(e.path(), dir / "RentAHero").generic_string();
        const auto a = vfs.read(rel);
        if (!a) { ++missing; continue; }
        if (*a != *readFile(e.path())) ++diff;
        ++checked;
    }
    std::cout << "     " << checked << " Dateien verglichen, " << missing << " fehlen, " << diff << " abweichend\n";
    CHECK(checked > 1000);
    CHECK(missing == 0);
    CHECK(diff == 0);
}
