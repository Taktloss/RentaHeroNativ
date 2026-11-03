// SPDX-License-Identifier: GPL-3.0-or-later
// Spielstände *.rah und Aufträge (docs/formats/savegame.md, docs/MENU.md)
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace rah {

struct GameContext;

// Globale Variablen rund um Spielstände
struct SaveGlobals {
    // Auftrag g_SaveRequest 0x0047D68C: 1 Laden, 2 Speichern, 3 Neues Spiel, 4 Beenden mit
    // Autosave, 5 Beenden
    int request = 0;
    std::string path;                 // g_SavePath 0x0047C128
    std::array<char, 30> name{};      // g_SaveName 0x00470810 (Reste früherer Namen bleiben)
    std::int32_t version = 0;         // 0x0047080C
    int slot = 0;                     // aktueller Platz 0x0047D690 (0…8)
    bool canSave = false;             // 0x0047D694: Held vorhanden und aktiv (GameMenu_Open)
    bool restorePos = false;          // g_RestorePos 0x0047D764
    int x = 0, y = 0, dir = 0;        // 0x0047D758 / 0x0047D75C / 0x0047D760
    // Spielstandordner mit abschließendem '/': Original g_CommonPath 0x0047D278; Port: --save-dir
    std::string dir_;
};

// strcpy in den 30-Byte-Namenspuffer (NUL mitkopiert, Rest bleibt)
void copySaveName(std::array<char, 30>& buf, std::string_view s);
// Name im Puffer bis zum ersten NUL
std::string saveName(const std::array<char, 30>& buf);
// "%sSaveGame%02d.rah" (0x00476A28)
std::string slotPath(const SaveGlobals& s, int slot);

// orig: Rent-A-Hero.exe 0x00404780 Save_Read – Datei g_SavePath; Kopf in die Globals
bool saveRead(GameContext& ctx);
// orig: Rent-A-Hero.exe 0x00404930 Save_Write – nur mit laufendem Schauplatz und aktivem Helden
bool saveWrite(GameContext& ctx);
// orig: Rent-A-Hero.exe 0x004045C0 Save_Process (Ende jedes Game_Idle, nicht bei offenem Menü);
// quit = PostQuitMessage(0)
void saveProcess(GameContext& ctx);

// Kopf eines Spielstands für die Liste im Menü (0x004446C0): Version und Name (30 Bytes roh)
struct SaveHeader {
    std::int32_t version = 0;
    std::array<char, 30> name{};
    std::size_t nameBytes = 0;  // tatsächlich gelesene Namensbytes (ReadFile bei kurzer Datei)
    bool versionRead = false;
};
std::optional<SaveHeader> readSaveHeader(const std::string& path);

}  // namespace rah
