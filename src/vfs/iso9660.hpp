// SPDX-License-Identifier: GPL-3.0-or-later
// Leser für ISO-9660-Dateisysteme mit Joliet (Namen aus der Joliet-Ergänzung, sonst
// Primärverzeichnis ohne ";1"). Quelle: .iso (2048-Byte-Sektoren) oder Spur einer CUE.
#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "vfs/filesystem.hpp"

namespace rah {

class IsoFileSystem final : public FileSystem {
public:
    // .iso oder .cue öffnen; nullptr bei Fehler (Grund in *error).
    static std::unique_ptr<IsoFileSystem> open(const std::filesystem::path& path, std::string* error);

    bool exists(std::string_view path) const override;
    std::optional<Bytes> read(std::string_view path) const override;
    std::vector<std::string> list(std::string_view dir) const override;

    bool joliet() const { return joliet_; }

private:
    struct Entry {
        std::string name;
        std::uint32_t extent = 0;
        std::uint32_t size = 0;
        bool dir = false;
    };
    struct Dir {
        std::map<std::string, Entry> byUpper;  // Schlüssel: Name in Großbuchstaben
        bool loaded = false;
        std::uint32_t extent = 0, size = 0;
    };

    IsoFileSystem() = default;
    bool readSectors(std::uint32_t lba, std::uint32_t bytes, Bytes& out) const;
    const Entry* find(std::string_view path) const;
    Dir* loadDir(const std::string& key, std::uint32_t extent, std::uint32_t size) const;

    mutable std::ifstream file_;
    mutable std::mutex mutex_;
    unsigned sectorSize_ = 2048;
    unsigned dataOffset_ = 0;
    bool joliet_ = false;
    Entry root_;
    mutable std::map<std::string, Dir> dirs_;  // Schlüssel: Pfad in Großbuchstaben
};

}  // namespace rah
