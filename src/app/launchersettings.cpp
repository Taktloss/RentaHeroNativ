// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/launchersettings.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iterator>

#include "formats/ini.hpp"
#include "vfs/vfs.hpp"

namespace rah {
namespace {

constexpr const char* kSection = "Launcher";
constexpr std::size_t kMaxPath = 4096;

// Ordner unter GAME/ der zusammengeführten CDs 1+2 (tools/data_scan.py, DATAPATHS.md)
constexpr const char* kGameDirs[] = {
    "COMMONCD", "MENU",  "LOC01", "LOC02", "LOC03", "LOC04", "LOC05", "LOC06", "LOC07",
    "LOC08",    "LOC09", "LOC10", "LOC12", "LOC14", "LOC15", "LOC16", "LOC17", "LOC18",
    "LOC19",    "LOC20", "LOC21", "LOC22", "LOC23", "LOC24", "LOC25", "LOC26", "LOC27",
    "LOC28",    "LOC29", "LOC30", "LOC31", "LOC35", "LOC37", "LOC40", "LOC41",
};

int clampVol(int v) { return std::clamp(v, 0, 100); }

}  // namespace

bool loadLauncherSettings(const std::filesystem::path& file, LauncherSettings& s) {
    std::ifstream in(file, std::ios::binary);
    if (!in) return false;
    const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const IniFile ini(std::span(reinterpret_cast<const std::uint8_t*>(text.data()), text.size()));
    s = LauncherSettings{};
    for (int i = 1; i <= 16; ++i) {
        const std::string v = ini.getString(kSection, "Source" + std::to_string(i), "", kMaxPath);
        if (!v.empty()) s.sources.push_back(v);
    }
    s.common = ini.getString(kSection, "Common", "", kMaxPath);
    s.saveDir = ini.getString(kSection, "SaveDir", "", kMaxPath);
    s.subtitles = ini.getInt(kSection, "SubTitles", 0) != 0;
    s.mute = ini.getInt(kSection, "Mute", 0) != 0;
    s.audio3d = ini.getInt(kSection, "Audio3D", 0) != 0;
    s.musicVol = clampVol(ini.getInt(kSection, "MusicVolume", 100));
    s.sfxVol = clampVol(ini.getInt(kSection, "SfxVolume", 100));
    s.speechVol = clampVol(ini.getInt(kSection, "SpeechVolume", 100));
    s.saveOnClose = ini.getInt(kSection, "SaveOnClose", 0) != 0;
    s.fullscreen = ini.getInt(kSection, "Fullscreen", 0) != 0;
    s.skipIntro = ini.getInt(kSection, "SkipIntro", 0) != 0;
    s.fixWalkJump = ini.getInt(kSection, "FixWalkJump", 0) != 0;
    s.display.filter = ini.getString(kSection, "Filter", "original");
    s.display.crt = ini.getString(kSection, "Crt", "off");
    s.display.deband = ini.getInt(kSection, "Deband", 0) != 0;
    s.display.integerScale = ini.getInt(kSection, "IntegerScale", 1) != 0;
    if (!findFilter(scalerFilters(), s.display.filter)) s.display.filter = "original";
    if (!findFilter(crtFilters(), s.display.crt)) s.display.crt = "off";
    return true;
}

bool saveLauncherSettings(const std::filesystem::path& file, const LauncherSettings& s) {
    std::error_code ec;
    if (file.has_parent_path()) std::filesystem::create_directories(file.parent_path(), ec);
    std::ofstream out(file, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out << "; Rent-A-Hero (Port) – Startmenü (docs/LAUNCHER.md)\n[" << kSection << "]\n";
    for (std::size_t i = 0; i < s.sources.size() && i < 16; ++i) out << "Source" << i + 1 << '=' << s.sources[i] << '\n';
    out << "Common=" << s.common << '\n'
        << "SaveDir=" << s.saveDir << '\n'
        << "SubTitles=" << int(s.subtitles) << '\n'
        << "Mute=" << int(s.mute) << '\n'
        << "Audio3D=" << int(s.audio3d) << '\n'
        << "MusicVolume=" << s.musicVol << '\n'
        << "SfxVolume=" << s.sfxVol << '\n'
        << "SpeechVolume=" << s.speechVol << '\n'
        << "SaveOnClose=" << int(s.saveOnClose) << '\n'
        << "Fullscreen=" << int(s.fullscreen) << '\n'
        << "SkipIntro=" << int(s.skipIntro) << '\n'
        << "FixWalkJump=" << int(s.fixWalkJump) << '\n'
        << "Filter=" << s.display.filter << '\n'
        << "Crt=" << s.display.crt << '\n'
        << "Deband=" << int(s.display.deband) << '\n'
        << "IntegerScale=" << int(s.display.integerScale) << '\n';
    return static_cast<bool>(out);
}

DataCheck checkDataSources(const std::vector<std::string>& sources) {
    DataCheck c;
    Vfs vfs;
    for (const auto& p : sources) {
        std::string err;
        if (!vfs.mountPath(p, &err)) c.errors.push_back(p + ": " + err);
    }
    c.expected = static_cast<int>(std::size(kGameDirs));
    for (const char* d : kGameDirs) {
        if (!vfs.list(std::string("GAME/") + d).empty()) ++c.found;
        else c.missing.push_back(d);
    }
    c.hasCab = vfs.exists("SETUP/DATA1.CAB");
    return c;
}

}  // namespace rah
