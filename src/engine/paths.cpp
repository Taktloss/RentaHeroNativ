// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/paths.hpp"

#include <cstdio>

namespace rah {

DataPaths::DataPaths(const FileSystem* cd, const FileSystem* common)
    : common_{common, ""}, commonCd_{cd, "GAME/COMMONCD/"} {
    scene_.fs = sceneCd_.fs = cd;
    setLocation(0);
}

void DataPaths::setLocation(int location) {
    char buf[32];
    if (location == 0) std::snprintf(buf, sizeof buf, "GAME/MENU/");
    else std::snprintf(buf, sizeof buf, "GAME/LOC%02d/", location);
    scene_.prefix = sceneCd_.prefix = buf;
}

std::optional<FoundFile> DataPaths::search(const std::string& name) const {
    // Reihenfolge wie im Original: Szene, Common, CommonCD, Szene CD; erster Treffer (_access)
    for (const Dir* d : {&scene_, &common_, &commonCd_, &sceneCd_}) {
        if (!d->fs) continue;
        std::string p = d->prefix + name;
        if (d->fs->exists(p)) return FoundFile{d->fs, std::move(p)};
    }
    return std::nullopt;
}

std::optional<FoundFile> DataPaths::findPath(std::string_view name) const {
    return search(std::string(name));
}

std::optional<FoundFile> DataPaths::findFile(std::string_view name, std::string_view ext) const {
    return search(makeFileName(name, ext));
}

std::string makeFileName(std::string_view name, std::string_view ext) {
    std::string r(name);
    if (r.size() > 4 && r[r.size() - 4] == '.') return r;
    r += '.';
    r += ext;
    return r;
}

}  // namespace rah
