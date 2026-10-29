// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/strings.hpp"

namespace rah {

std::string fileNameKey(std::string_view s) {
    const auto validUtf8 = [](std::string_view v) {
        for (std::size_t i = 0; i < v.size();) {
            const auto c = static_cast<unsigned char>(v[i]);
            const std::size_t n = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 0;
            if (n == 0 || i + n > v.size()) return false;
            for (std::size_t k = 1; k < n; ++k)
                if ((static_cast<unsigned char>(v[i + k]) & 0xC0) != 0x80) return false;
            i += n;
        }
        return true;
    };
    if (validUtf8(s)) return toUpperAscii(s);
    std::string u;
    for (const char ch : s) {
        const auto c = static_cast<unsigned char>(ch);
        if (c < 0x80) {
            u += ch;
        } else {
            u += static_cast<char>(0xC0 | (c >> 6));
            u += static_cast<char>(0x80 | (c & 0x3F));
        }
    }
    return toUpperAscii(u);
}

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
