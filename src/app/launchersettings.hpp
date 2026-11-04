// SPDX-License-Identifier: GPL-3.0-or-later
// Einstellungen des Startmenüs (Port, kein Original-Gegenstück; docs/LAUNCHER.md)
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace rah {

struct LauncherSettings {
    std::vector<std::string> sources;  // --cd: Ordner, .iso oder .cue (CD1, CD2, …)
    std::string common;                // --common (leer: aus SETUP/DATA1.CAB entpacken)
    std::string saveDir;               // --save-dir (leer: Benutzer-Datenordner)
    bool subtitles = false;
    bool mute = false;
    bool audio3d = false;
    int musicVol = 100, sfxVol = 100, speechVol = 100;
    bool saveOnClose = false;
    bool fullscreen = false;
    bool skipIntro = false;  // Port-Option, Standard aus (originalgetreu: Intro bei Neues Spiel)
};

// launcher.ini (UTF-8, Abschnitt [Launcher]); fehlende Datei → Standardwerte, false
bool loadLauncherSettings(const std::filesystem::path& file, LauncherSettings& s);
bool saveLauncherSettings(const std::filesystem::path& file, const LauncherSettings& s);

// Prüfung der Datenquellen, bevor gestartet wird
struct DataCheck {
    std::vector<std::string> errors;   // Quelle nicht lesbar
    std::vector<std::string> missing;  // fehlende Schauplatzordner GAME/LOCnn, MENU, COMMONCD
    int found = 0, expected = 0;
    bool hasCab = false;               // SETUP/DATA1.CAB (nötig ohne Common-Ordner)
    bool ok(bool haveCommon) const { return errors.empty() && missing.empty() && (haveCommon || hasCab); }
};
DataCheck checkDataSources(const std::vector<std::string>& sources);

}  // namespace rah
