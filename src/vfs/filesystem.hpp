// SPDX-License-Identifier: GPL-3.0-or-later
// Lesende Dateiquelle mit Windows-Pfadsemantik (Groß-/Kleinschreibung egal, '\\' oder '/').
#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "core/bytes.hpp"

namespace rah {

class FileSystem {
public:
    virtual ~FileSystem() = default;
    virtual bool exists(std::string_view path) const = 0;
    virtual std::optional<Bytes> read(std::string_view path) const = 0;
    // Einträge eines Verzeichnisses (Namen wie auf dem Datenträger); leer, wenn unbekannt.
    virtual std::vector<std::string> list(std::string_view dir) const = 0;
};

}  // namespace rah
