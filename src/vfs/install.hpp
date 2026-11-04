// SPDX-License-Identifier: GPL-3.0-or-later
// Installation ohne Original-Setup: SETUP/DATA1.CAB (InstallShield 5, CD1) mit libunshield
// entpacken – Rent-A-Hero.exe und der Common-Ordner (docs/DATAPATHS.md, Entscheidung Nutzer).
#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace rah {

class FileSystem;

// Ziel enthält nach Erfolg <dest>/Rent-A-Hero.exe, <dest>/Common/… und die Markierung kInstallDone.
inline constexpr const char* kInstallDone = ".rah-install-ok";

bool installComplete(const std::filesystem::path& dest);

// Kabinettpfad innerhalb der Datenquelle: "SETUP/DATA1.CAB".
// Das Kabinett wird nach <dest>/.cab/DATA1.CAB kopiert (auch aus ISO, BIN/CUE) und danach gelöscht.
// progress(n, gesamt) nach jeder entpackten Datei.
bool installFromCab(const FileSystem& cd, const std::filesystem::path& dest, std::string* err,
                    const std::function<void(int, int)>& progress = {});

// Abbildung des CAB-Verzeichnisses auf den Zielordner: letzte Komponente „Common“ → "Common",
// sonst Hauptordner.
std::string installSubdir(const std::string& cabDir);

}  // namespace rah
