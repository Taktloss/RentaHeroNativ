// SPDX-License-Identifier: GPL-3.0-or-later
// Suchordner einer Szene und Dateisuche wie im Original (docs/DATAPATHS.md).
#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "vfs/filesystem.hpp"

namespace rah {

struct FoundFile {
    const FileSystem* fs = nullptr;
    std::string path;
    std::optional<Bytes> read() const { return fs ? fs->read(path) : std::nullopt; }
};

class DataPaths {
public:
    struct Dir {
        const FileSystem* fs = nullptr;  // nullptr = Ordner nicht vorhanden
        std::string prefix;              // mit abschließendem '/'
    };

    // Abbildung (DATAPATHS.md „Port“): ScenePath = ScenePathCd = <cd>/GAME/,
    // CommonPathCd = <cd>/GAME/COMMONCD/, CommonPath = eigener Common-Ordner.
    DataPaths(const FileSystem* cd, const FileSystem* common);

    // orig: Rent-A-Hero.exe 0x004036E0 Game_SwitchLocation – Szenenordner LocNN bzw. menu (0)
    void setLocation(int location);

    // orig: Rent-A-Hero.exe 0x00448A70 CSzene_FindPath
    std::optional<FoundFile> findPath(std::string_view name) const;
    // orig: Rent-A-Hero.exe 0x00448B30 CSzene_FindFile
    std::optional<FoundFile> findFile(std::string_view name, std::string_view ext) const;

    const Dir& sceneDir() const { return scene_; }
    const Dir& commonDir() const { return common_; }
    const Dir& sceneCdDir() const { return sceneCd_; }

private:
    std::optional<FoundFile> search(const std::string& name) const;

    Dir scene_, common_, commonCd_, sceneCd_;
};

// orig: Rent-A-Hero.exe 0x00447F90 MakeFileName (ohne Ordner): Endung nur anhängen, wenn
// der Name nicht länger als 4 Zeichen ist oder an Position len−4 kein '.' steht.
std::string makeFileName(std::string_view name, std::string_view ext);

}  // namespace rah
