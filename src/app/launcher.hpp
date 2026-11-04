// SPDX-License-Identifier: GPL-3.0-or-later
// Startmenü/Setup des Ports mit Dear ImGui (kein Original-Gegenstück; docs/LAUNCHER.md)
#pragma once

#include <filesystem>
#include <string>

#include <SDL3/SDL.h>

#include "app/launchersettings.hpp"
#include "app/present/presenter.hpp"

namespace rah {

struct LaunchChoice {
    enum Kind { Quit, Continue, NewGame, Load } kind = Quit;
    std::string savePath;  // Load/Continue: Spielstanddatei
    std::string common;    // Common-Ordner (angegeben oder entpackt)
};

struct LauncherEnv {
    std::filesystem::path iniFile;      // <Benutzer-Datenordner>/launcher.ini
    std::filesystem::path installDir;   // <Benutzer-Datenordner>/Install (Ziel für DATA1.CAB)
    std::string defaultSaveDir;         // Benutzer-Datenordner
    bool canExtract = false;            // mit libunshield gebaut
};

// Zeigt das Menü im vorhandenen Fenster, bis gestartet oder beendet wird. Speichert die
// Einstellungen bei jedem Start in env.iniFile. Danach ist das Fenster wieder frei für das Spiel.
// ImGui-Kontext (uiCreate) und presenter.imguiInit() muss der Aufrufer schon eingerichtet haben.
LaunchChoice runLauncher(SDL_Window* win, Presenter& p, LauncherSettings& s, const LauncherEnv& env);

}  // namespace rah
