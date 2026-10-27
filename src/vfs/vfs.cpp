// SPDX-License-Identifier: GPL-3.0-or-later
#include "vfs/vfs.hpp"

#include <set>

#include "core/strings.hpp"
#include "vfs/dirfs.hpp"
#include "vfs/iso9660.hpp"

namespace rah {

void Vfs::mount(std::unique_ptr<FileSystem> fs, std::string name) {
    mounts_.push_back({std::move(fs), std::move(name)});
}

bool Vfs::mountPath(const std::filesystem::path& path, std::string* error) {
    std::error_code ec;
    if (std::filesystem::is_directory(path, ec)) {
        mount(std::make_unique<DirFileSystem>(path), path.string());
        return true;
    }
    const std::string ext = toUpperAscii(path.extension().string());
    if (ext == ".ISO" || ext == ".CUE") {
        auto fs = IsoFileSystem::open(path, error);
        if (!fs) return false;
        mount(std::move(fs), path.string());
        return true;
    }
    if (error) *error = "unbekannte Datenquelle (Ordner, .iso oder .cue erwartet): " + path.string();
    return false;
}

bool Vfs::exists(std::string_view path) const {
    for (const auto& m : mounts_) {
        if (m.fs->exists(path)) return true;
    }
    return false;
}

std::optional<Bytes> Vfs::read(std::string_view path) const {
    for (const auto& m : mounts_) {
        if (auto data = m.fs->read(path)) return data;
    }
    return std::nullopt;
}

std::vector<std::string> Vfs::list(std::string_view dir) const {
    std::vector<std::string> names;
    std::set<std::string> seen;
    for (const auto& m : mounts_) {
        for (auto& n : m.fs->list(dir)) {
            if (seen.insert(toUpperAscii(n)).second) names.push_back(std::move(n));
        }
    }
    return names;
}

}  // namespace rah
