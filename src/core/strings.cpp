// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/strings.hpp"

namespace rah {

std::string toUpperAscii(std::string_view s) {
    std::string out(s);
    for (char& c : out) {
        if (c >= 'a' && c <= 'z') {
            c = static_cast<char>(c - 'a' + 'A');
        }
    }
    return out;
}

bool equalsNoCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        char x = a[i];
        char y = b[i];
        if (x >= 'a' && x <= 'z') x = static_cast<char>(x - 'a' + 'A');
        if (y >= 'a' && y <= 'z') y = static_cast<char>(y - 'a' + 'A');
        if (x != y) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> splitPath(std::string_view path) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : path) {
        if (c == '/' || c == '\\') {
            if (!cur.empty() && cur != ".") parts.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    if (!cur.empty() && cur != ".") parts.push_back(cur);
    return parts;
}

std::string fileStem(const std::string& path) {
    const auto slash = path.find_last_of("/\\:");
    std::string f = slash == std::string::npos ? path : path.substr(slash + 1);
    const auto dot = f.rfind('.');
    if (dot != std::string::npos) f.resize(dot);
    return f;
}

}  // namespace rah
