// SPDX-License-Identifier: GPL-3.0-or-later
// Zeichenketten-Hilfen mit Windows-Semantik (Dateinamen ohne Groß-/Kleinschreibung).
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace rah {

// ASCII-Großbuchstaben (das Original vergleicht Pfade mit lstrcmpi, die Daten sind ASCII).
std::string toUpperAscii(std::string_view s);
bool equalsNoCase(std::string_view a, std::string_view b);

// Pfad an '/' und '\\' zerlegen, leere Teile und "." verwerfen.
std::vector<std::string> splitPath(std::string_view path);

// _splitpath(path, 0, 0, fname, 0) – orig: Rent-A-Hero.exe 0x00447F00 SplitFileName
std::string fileStem(const std::string& path);

}  // namespace rah
