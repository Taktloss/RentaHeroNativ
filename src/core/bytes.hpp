// SPDX-License-Identifier: GPL-3.0-or-later
// Little-Endian-Zugriffe auf Byte-Puffer und Dateien.
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <vector>

namespace rah {

using Bytes = std::vector<std::uint8_t>;

inline std::uint16_t le16(std::span<const std::uint8_t> b, std::size_t off) {
    return static_cast<std::uint16_t>(b[off] | (b[off + 1] << 8));
}

inline std::uint32_t le32(std::span<const std::uint8_t> b, std::size_t off) {
    return static_cast<std::uint32_t>(b[off]) | (static_cast<std::uint32_t>(b[off + 1]) << 8) |
           (static_cast<std::uint32_t>(b[off + 2]) << 16) |
           (static_cast<std::uint32_t>(b[off + 3]) << 24);
}

// Ganze Datei lesen; nullopt bei Fehler.
std::optional<Bytes> readFile(const std::filesystem::path& path);

}  // namespace rah
