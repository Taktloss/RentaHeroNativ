// SPDX-License-Identifier: GPL-3.0-or-later
#include "vfs/dirfs.hpp"

#include "core/strings.hpp"

namespace rah {

DirFileSystem::DirFileSystem(std::filesystem::path root) : root_(std::move(root)) {}

std::filesystem::path DirFileSystem::resolve(std::string_view path) const {
    std::lock_guard lock(mutex_);
    std::filesystem::path cur = root_;
    for (const auto& part : splitPath(path)) {
        auto [it, inserted] = cache_.try_emplace(cur);
        if (inserted) {
            std::error_code ec;
            for (const auto& e : std::filesystem::directory_iterator(cur, ec)) {
                const std::string name = e.path().filename().string();
                it->second.emplace(toUpperAscii(name), name);
            }
        }
        const auto found = it->second.find(toUpperAscii(part));
        if (found == it->second.end()) return {};
        cur /= found->second;
    }
    return cur;
}

bool DirFileSystem::exists(std::string_view path) const {
    return !resolve(path).empty();
}

std::optional<Bytes> DirFileSystem::read(std::string_view path) const {
    const auto p = resolve(path);
    if (p.empty() || std::filesystem::is_directory(p)) return std::nullopt;
    return readFile(p);
}

std::vector<std::string> DirFileSystem::list(std::string_view dir) const {
    std::vector<std::string> names;
    const auto p = resolve(dir);
    std::error_code ec;
    if (p.empty()) return names;
    for (const auto& e : std::filesystem::directory_iterator(p, ec)) {
        names.push_back(e.path().filename().string());
    }
    return names;
}

}  // namespace rah
