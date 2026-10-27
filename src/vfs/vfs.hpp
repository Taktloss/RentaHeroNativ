// SPDX-License-Identifier: GPL-3.0-or-later
// Mehrere Datenquellen (CD-Ordner, CD-Images, Common-Ordner) zu einer Sicht vereinigt.
// Die erste Quelle, die eine Datei hat, gewinnt. Die eigentliche Suchreihenfolge des
// Originals über Szenen-, Common- und CD-Ordner (docs/DATAPATHS.md) baut darauf auf.
#pragma once

#include <memory>

#include "vfs/filesystem.hpp"

namespace rah {

class Vfs final : public FileSystem {
public:
    void mount(std::unique_ptr<FileSystem> fs, std::string name);
    // Pfad (Ordner, .iso oder .cue) erkennen und einhängen; false + Grund bei Fehler.
    bool mountPath(const std::filesystem::path& path, std::string* error);

    bool exists(std::string_view path) const override;
    std::optional<Bytes> read(std::string_view path) const override;
    std::vector<std::string> list(std::string_view dir) const override;

    std::size_t size() const { return mounts_.size(); }
    const std::string& name(std::size_t i) const { return mounts_[i].name; }

private:
    struct Mount {
        std::unique_ptr<FileSystem> fs;
        std::string name;
    };
    std::vector<Mount> mounts_;
};

}  // namespace rah
