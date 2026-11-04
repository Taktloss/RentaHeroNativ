// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/platform.hpp"

#ifdef _WIN32
#include <SDL3/SDL.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#define WIN32_LEAN_AND_MEAN
#include <io.h>
#include <windows.h>

namespace rah {
namespace {
bool g_console = false;
std::string g_log;
}  // namespace

void platformInit() {
    // Ausgabe schon umgeleitet (Datei/Pipe, auch unter Wine geerbt) → so lassen
    const HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (out && out != INVALID_HANDLE_VALUE && GetFileType(out) != FILE_TYPE_UNKNOWN) {
        g_console = true;
        return;
    }
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        std::freopen("CONOUT$", "w", stdout);
        std::freopen("CONOUT$", "w", stderr);
        g_console = true;
        return;
    }
    // Doppelklick: Ausgabe in <Benutzer-Datenordner>/rentahero-log.txt (UTF-8 dank Manifest)
    if (char* pref = SDL_GetPrefPath(nullptr, "RentaHeroNativ")) {
        g_log = std::string(pref) + "rentahero-log.txt";
        SDL_free(pref);
        if (std::freopen(g_log.c_str(), "w", stdout)) {
            _dup2(_fileno(stdout), _fileno(stderr));
            std::setvbuf(stdout, nullptr, _IONBF, 0);
            std::setvbuf(stderr, nullptr, _IONBF, 0);
        } else {
            g_log.clear();
        }
    }
}

void platformExit(int rc) {
    if (g_console || rc == 0) return;
    std::string text = "Rent-A-Hero (Port) wurde mit Fehler beendet.";
    if (!g_log.empty()) {
        std::fflush(stdout);
        std::ifstream in(g_log, std::ios::binary);
        std::stringstream ss;
        ss << in.rdbuf();
        std::string body = ss.str();
        if (body.size() > 3000) body = body.substr(body.size() - 3000);
        text += "\n\n" + body + "\nProtokoll: " + g_log;
    }
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Rent-A-Hero", text.c_str(), nullptr);
}

}  // namespace rah

#else

namespace rah {
void platformInit() {}
void platformExit(int) {}
}  // namespace rah

#endif
