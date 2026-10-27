// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/bytes.hpp"

#include <fstream>

namespace rah {

std::optional<Bytes> readFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) {
        return std::nullopt;
    }
    const auto size = static_cast<std::streamsize>(in.tellg());
    Bytes data(static_cast<std::size_t>(size));
    in.seekg(0);
    if (!in.read(reinterpret_cast<char*>(data.data()), size)) {
        return std::nullopt;
    }
    return data;
}

}  // namespace rah
