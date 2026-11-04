// SPDX-License-Identifier: GPL-3.0-or-later
#include <filesystem>
#include <fstream>

#include "app/launchersettings.hpp"
#include "test.hpp"

using namespace rah;

TEST(launcher_settings_roundtrip) {
    const auto dir = std::filesystem::temp_directory_path() / "rah_test_launcher";
    std::filesystem::remove_all(dir);
    const auto file = dir / "sub" / "launcher.ini";
    LauncherSettings s;
    CHECK(!loadLauncherSettings(file, s));  // fehlt → false, Standardwerte
    CHECK(s.musicVol == 100 && !s.skipIntro && !s.fixWalkJump && s.sources.empty());

    s.sources = {"/media/Rent a Hero CD1.cue", "D:\\", "/pfad/mit Ümläut"};
    s.common = "C:\\Programme\\Rent-A-Hero\\Common";
    s.saveDir = "";
    s.subtitles = true;
    s.audio3d = true;
    s.musicVol = 40;
    s.sfxVol = 0;
    s.speechVol = 77;
    s.saveOnClose = true;
    s.fullscreen = true;
    s.skipIntro = true;
    s.fixWalkJump = true;
    REQUIRE(saveLauncherSettings(file, s));

    LauncherSettings r;
    REQUIRE(loadLauncherSettings(file, r));
    CHECK(r.sources == s.sources);
    CHECK(r.common == s.common);
    CHECK(r.saveDir.empty());
    CHECK(r.subtitles && r.audio3d && !r.mute && r.saveOnClose && r.fullscreen && r.skipIntro && r.fixWalkJump);
    CHECK(r.musicVol == 40 && r.sfxVol == 0 && r.speechVol == 77);

    // Werte außerhalb 0…100 werden begrenzt
    {
        std::ofstream o(file, std::ios::binary | std::ios::trunc);
        o << "[Launcher]\nMusicVolume=250\nSfxVolume=-3\n";
    }
    REQUIRE(loadLauncherSettings(file, r));
    CHECK(r.musicVol == 100 && r.sfxVol == 0 && r.speechVol == 100);
    std::filesystem::remove_all(dir);
}

TEST(launcher_check_empty_and_bad_sources) {
    DataCheck c = checkDataSources({});
    CHECK(c.errors.empty() && c.found == 0 && c.expected > 0 && !c.hasCab);
    CHECK(!c.ok(true));
    c = checkDataSources({"/gibt/es/nicht/rah"});
    CHECK(c.errors.size() == 1);
}

TEST(launcher_check_original_data) {
    const auto d = rahtest::dataDir();
    SKIP_IF(d.empty() || !std::filesystem::is_directory(d / "RentAHero"));
    DataCheck c = checkDataSources({(d / "RentAHero").string()});
    CHECK(c.errors.empty());
    CHECK(c.missing.empty());
    CHECK(c.found == c.expected);
    CHECK(c.hasCab);
    CHECK(c.ok(false));
    // Beide CD-Abbilder zusammen ergeben dasselbe
    const auto cd1 = d / "Rent a Hero CD1.cue", cd2 = d / "Rent a Hero CD2.cue";
    if (std::filesystem::exists(cd1) && std::filesystem::exists(cd2)) {
        DataCheck one = checkDataSources({cd1.string()});
        CHECK(one.errors.empty() && one.hasCab && !one.missing.empty());
        DataCheck both = checkDataSources({cd1.string(), cd2.string()});
        CHECK(both.missing.empty());
        CHECK(both.ok(false));
    }
}
