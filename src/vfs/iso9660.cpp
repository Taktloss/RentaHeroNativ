// SPDX-License-Identifier: GPL-3.0-or-later
#include "vfs/iso9660.hpp"

#include "core/strings.hpp"
#include "vfs/cue.hpp"

namespace rah {

namespace {

constexpr std::uint32_t kSector = 2048;

std::string decodeName(const std::uint8_t* p, std::size_t len, bool joliet) {
    std::string name;
    if (joliet) {
        // UCS-2 Big-Endian → UTF-8
        for (std::size_t i = 0; i + 1 < len; i += 2) {
            const unsigned c = (p[i] << 8) | p[i + 1];
            if (c < 0x80) {
                name += static_cast<char>(c);
            } else if (c < 0x800) {
                name += static_cast<char>(0xC0 | (c >> 6));
                name += static_cast<char>(0x80 | (c & 0x3F));
            } else {
                name += static_cast<char>(0xE0 | (c >> 12));
                name += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
                name += static_cast<char>(0x80 | (c & 0x3F));
            }
        }
    } else {
        name.assign(reinterpret_cast<const char*>(p), len);
    }
    if (const auto semi = name.find(';'); semi != std::string::npos) name.resize(semi);
    if (!name.empty() && name.back() == '.') name.pop_back();  // "NAME." ohne Endung
    return name;
}

std::string joinKey(const std::vector<std::string>& parts, std::size_t count) {
    std::string key;
    for (std::size_t i = 0; i < count; ++i) {
        key += '/';
        key += fileNameKey(parts[i]);
    }
    return key;
}

}  // namespace

std::unique_ptr<IsoFileSystem> IsoFileSystem::open(const std::filesystem::path& path,
                                                   std::string* error) {
    auto fail = [&](const std::string& why) -> std::unique_ptr<IsoFileSystem> {
        if (error) *error = why;
        return nullptr;
    };
    std::unique_ptr<IsoFileSystem> fs(new IsoFileSystem());
    std::filesystem::path image = path;
    if (equalsNoCase(path.extension().string(), ".cue")) {
        const auto track = parseCueFile(path);
        if (!track) return fail("CUE-Datei ohne lesbare Datenspur: " + path.string());
        image = track->binFile;
        fs->sectorSize_ = track->sectorSize;
        fs->dataOffset_ = track->dataOffset;
    }
    fs->file_.open(image, std::ios::binary);
    if (!fs->file_) return fail("Image nicht lesbar: " + image.string());

    // Datenträgerbeschreibungen ab Sektor 16: Primär (1), Joliet = Ergänzung (2) mit
    // Escape-Sequenz %/@, %/C oder %/E.
    Bytes pvd, svd, d;
    for (std::uint32_t lba = 16; lba < 64; ++lba) {
        if (!fs->readSectors(lba, kSector, d)) break;
        if (d[1] != 'C' || d[2] != 'D' || d[3] != '0' || d[4] != '0' || d[5] != '1') break;
        if (d[0] == 1 && pvd.empty()) pvd = d;
        if (d[0] == 2 && d[88] == 0x25 && d[89] == 0x2F &&
            (d[90] == 0x40 || d[90] == 0x43 || d[90] == 0x45)) svd = d;
        if (d[0] == 255) break;
    }
    const Bytes& vd = svd.empty() ? pvd : svd;
    if (vd.empty()) return fail("kein ISO-9660-Dateisystem: " + image.string());
    fs->joliet_ = !svd.empty();
    fs->root_.dir = true;
    fs->root_.extent = le32(vd, 156 + 2);
    fs->root_.size = le32(vd, 156 + 10);
    return fs;
}

bool IsoFileSystem::readSectors(std::uint32_t lba, std::uint32_t bytes, Bytes& out) const {
    out.resize(bytes);
    std::uint32_t done = 0;
    while (done < bytes) {
        const std::uint32_t n = std::min(kSector, bytes - done);
        const auto pos = static_cast<std::streamoff>(lba + done / kSector) * sectorSize_ + dataOffset_;
        file_.clear();
        file_.seekg(pos);
        if (!file_.read(reinterpret_cast<char*>(out.data() + done), n)) return false;
        done += n;
    }
    return true;
}

IsoFileSystem::Dir* IsoFileSystem::loadDir(const std::string& key, std::uint32_t extent,
                                           std::uint32_t size) const {
    Dir& dir = dirs_[key];
    if (dir.loaded) return &dir;
    dir.loaded = true;
    Bytes data;
    if (!readSectors(extent, size, data)) return &dir;
    std::size_t pos = 0;
    while (pos < data.size()) {
        const std::uint8_t len = data[pos];
        if (len == 0) {  // Rest des Sektors leer
            pos = (pos / kSector + 1) * kSector;
            continue;
        }
        if (pos + len > data.size() || len < 34) break;
        const std::uint8_t nameLen = data[pos + 32];
        const std::uint8_t* namePtr = &data[pos + 33];
        if (!(nameLen == 1 && (namePtr[0] == 0 || namePtr[0] == 1))) {
            Entry e;
            e.name = decodeName(namePtr, nameLen, joliet_);
            e.extent = le32(data, pos + 2);
            e.size = le32(data, pos + 10);
            e.dir = (data[pos + 25] & 0x02) != 0;
            dir.byUpper.emplace(fileNameKey(e.name), e);
        }
        pos += len;
    }
    return &dir;
}

const IsoFileSystem::Entry* IsoFileSystem::find(std::string_view path) const {
    const auto parts = splitPath(path);
    const Entry* cur = &root_;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        if (!cur->dir) return nullptr;
        Dir* dir = loadDir(joinKey(parts, i), cur->extent, cur->size);
        const auto it = dir->byUpper.find(fileNameKey(parts[i]));
        if (it == dir->byUpper.end()) return nullptr;
        cur = &it->second;
    }
    return cur;
}

bool IsoFileSystem::exists(std::string_view path) const {
    std::lock_guard lock(mutex_);
    return find(path) != nullptr;
}

std::optional<Bytes> IsoFileSystem::read(std::string_view path) const {
    std::lock_guard lock(mutex_);
    const Entry* e = find(path);
    if (!e || e->dir) return std::nullopt;
    Bytes data;
    if (!readSectors(e->extent, e->size, data)) return std::nullopt;
    return data;
}

std::vector<std::string> IsoFileSystem::list(std::string_view path) const {
    std::lock_guard lock(mutex_);
    const Entry* e = find(path);
    std::vector<std::string> names;
    if (!e || !e->dir) return names;
    const auto parts = splitPath(path);
    for (const auto& [upper, entry] : loadDir(joinKey(parts, parts.size()), e->extent, e->size)->byUpper) {
        names.push_back(entry.name);
    }
    return names;
}

}  // namespace rah
