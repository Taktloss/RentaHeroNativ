// SPDX-License-Identifier: GPL-3.0-or-later
#include "formats/ini.hpp"

#include "core/strings.hpp"

namespace rah {

namespace {

std::string_view trim(std::string_view s) {
    const auto ws = [](char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; };
    while (!s.empty() && ws(s.front())) s.remove_prefix(1);
    while (!s.empty() && ws(s.back())) s.remove_suffix(1);
    return s;
}

}  // namespace

void IniFile::parse(std::span<const std::uint8_t> text) {
    sections_.clear();
    sectionIndex_.clear();
    dupSections_ = dupKeys_ = 0;
    Section* cur = nullptr;
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t end = pos;
        while (end < text.size() && text[end] != '\n') ++end;
        const std::string_view line =
            trim(std::string_view(reinterpret_cast<const char*>(text.data()) + pos, end - pos));
        pos = end + 1;
        if (line.empty()) continue;
        if (line.front() == '[') {
            const auto close = line.find(']');
            const std::string name(trim(line.substr(1, close == std::string_view::npos ? line.size() - 1 : close - 1)));
            sections_.push_back({name, {}, {}});
            cur = &sections_.back();
            if (!sectionIndex_.emplace(toUpperAscii(name), sections_.size() - 1).second) ++dupSections_;
            continue;
        }
        if (!cur || line.front() == ';') continue;
        const auto eq = line.find('=');
        const std::string key(trim(line.substr(0, eq)));
        const std::string value(eq == std::string_view::npos ? std::string_view() : trim(line.substr(eq + 1)));
        cur->entries.emplace_back(key, value);
        if (!cur->index.emplace(toUpperAscii(key), cur->entries.size() - 1).second) ++dupKeys_;
    }
}

const IniFile::Section* IniFile::section(std::string_view name) const {
    const auto it = sectionIndex_.find(toUpperAscii(name));
    return it == sectionIndex_.end() ? nullptr : &sections_[it->second];
}

bool IniFile::hasSection(std::string_view name) const {
    return section(name) != nullptr;
}

std::optional<std::string> IniFile::find(std::string_view sec, std::string_view key) const {
    const Section* s = section(sec);
    if (!s) return std::nullopt;
    const auto it = s->index.find(toUpperAscii(key));
    if (it == s->index.end()) return std::nullopt;
    return s->entries[it->second].second;
}

std::string IniFile::getString(std::string_view sec, std::string_view key, std::string_view def,
                               std::size_t maxLen) const {
    std::string v = find(sec, key).value_or(std::string(def));
    if (v.size() >= 2 && (v.front() == '"' || v.front() == '\'') && v.back() == v.front()) {
        v = v.substr(1, v.size() - 2);
    }
    if (maxLen > 0 && v.size() > maxLen - 1) v.resize(maxLen - 1);
    return v;
}

int IniFile::getInt(std::string_view sec, std::string_view key, int def) const {
    const auto v = find(sec, key);
    if (!v) return def;
    std::size_t i = 0;
    while (i < v->size() && ((*v)[i] == ' ' || (*v)[i] == '\t')) ++i;
    bool neg = false;
    if (i < v->size() && ((*v)[i] == '-' || (*v)[i] == '+')) neg = (*v)[i++] == '-';
    long long r = 0;
    while (i < v->size() && (*v)[i] >= '0' && (*v)[i] <= '9') {
        r = r * 10 + ((*v)[i++] - '0');
        if (r > 0xFFFFFFFFLL) r &= 0xFFFFFFFFLL;  // 32-Bit-Überlauf wie im Original-API
    }
    return static_cast<int>(static_cast<std::uint32_t>(neg ? -r : r));
}

std::vector<std::string> IniFile::sections() const {
    std::vector<std::string> out;
    for (const auto& s : sections_) out.push_back(s.name);
    return out;
}

std::vector<std::string> IniFile::keys(std::string_view sec) const {
    std::vector<std::string> out;
    if (const Section* s = section(sec)) {
        for (const auto& e : s->entries) out.push_back(e.first);
    }
    return out;
}

}  // namespace rah
