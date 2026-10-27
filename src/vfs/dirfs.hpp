// SPDX-License-Identifier: GPL-3.0-or-later
// Ordner auf der Festplatte als Datenquelle; Pfade ohne Groß-/Kleinschreibung aufgelöst
// (unter Linux haben die Originaldaten oft andere Schreibweisen als im Programm).
#pragma once

#include <filesystem>
#include <map>
#include <mutex>

#include "vfs/filesystem.hpp"

namespace rah {

class DirFileSystem final : public FileSystem {
public:
    explicit DirFileSystem(std::filesystem::path root);

    bool exists(std::string_view path) const override;
    std::optional<Bytes> read(std::string_view path) const override;
    std::vector<std::string> list(std::string_view dir) const override;

    // Tatsächlicher Pfad auf der Platte oder leer.
    std::filesystem::path resolve(std::string_view path) const;

private:
    std::filesystem::path root_;
    mutable std::mutex mutex_;
    // Verzeichnis (tatsächlicher Pfad) → Name in Großbuchstaben → tatsächlicher Name
    mutable std::map<std::filesystem::path, std::map<std::string, std::string>> cache_;
};

}  // namespace rah
