// SPDX-License-Identifier: GPL-3.0-or-later
// Minimaler CUE-Leser: erste Datenspur eines CD-Images (BIN/CUE).
#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace rah {

struct CueTrack {
    std::filesystem::path binFile;  // absolut bzw. relativ zur CUE-Datei aufgelöst
    std::string mode;               // z. B. "MODE2/2352", "MODE1/2352", "MODE1/2048"
    unsigned sectorSize = 0;        // Bytes je Sektor in der BIN-Datei
    unsigned dataOffset = 0;        // Offset der 2048 Nutzdaten-Bytes im Sektor
};

// Erste Spur im Modus MODE1/MODE2; nullopt, wenn keine passende Spur gefunden wird.
std::optional<CueTrack> parseCueFile(const std::filesystem::path& cuePath);
std::optional<CueTrack> parseCue(const std::string& text, const std::filesystem::path& baseDir);

}  // namespace rah
