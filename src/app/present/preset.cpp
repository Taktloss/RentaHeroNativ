// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/present/preset.hpp"

#include <cstdlib>
#include <sstream>

namespace rah {
namespace {

std::string trim(std::string_view s) {
    std::size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
    return std::string(s.substr(a, b - a));
}

std::string unquote(std::string v) {
    if (v.size() >= 2 && v.front() == '"' && v.back() == '"') v = v.substr(1, v.size() - 2);
    return v;
}

bool parseBool(const std::string& v) { return v == "true" || v == "1"; }

std::optional<ScaleType> parseScaleType(const std::string& v) {
    if (v == "source") return ScaleType::Source;
    if (v == "viewport") return ScaleType::Viewport;
    if (v == "absolute") return ScaleType::Absolute;
    return std::nullopt;
}

}  // namespace

std::string normalizePath(std::string_view p) {
    std::vector<std::string> parts;
    std::string cur;
    auto flush = [&] {
        if (cur == "..") {
            if (!parts.empty()) parts.pop_back();
        } else if (!cur.empty() && cur != ".") {
            parts.push_back(cur);
        }
        cur.clear();
    };
    for (char c : p) {
        if (c == '/' || c == '\\') flush();
        else cur.push_back(c);
    }
    flush();
    std::string out;
    for (const auto& s : parts) out += (out.empty() ? "" : "/") + s;
    return out;
}

std::optional<PresetDesc> parsePreset(std::string_view text, std::string_view dir, std::string* err) {
    std::map<std::string, std::string> kv;
    std::istringstream in{std::string(text)};
    std::string line;
    while (std::getline(in, line)) {
        const auto hash = line.find('#');
        if (hash != std::string::npos) line.resize(hash);
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        kv[trim(std::string_view(line).substr(0, eq))] = unquote(trim(std::string_view(line).substr(eq + 1)));
    }
    auto get = [&](const std::string& k) -> const std::string* {
        const auto it = kv.find(k);
        return it == kv.end() ? nullptr : &it->second;
    };
    const std::string base = std::string(dir).empty() ? std::string() : std::string(dir) + "/";
    PresetDesc p;
    const std::string* n = get("shaders");
    if (!n) {
        if (err) *err = "shaders fehlt";
        return std::nullopt;
    }
    const int count = std::atoi(n->c_str());
    if (count <= 0 || count > 16) {
        if (err) *err = "ungültige Anzahl Durchgänge";
        return std::nullopt;
    }
    for (int i = 0; i < count; ++i) {
        const std::string s = std::to_string(i);
        PassDesc d;
        const std::string* sh = get("shader" + s);
        if (!sh) {
            if (err) *err = "shader" + s + " fehlt";
            return std::nullopt;
        }
        d.shader = normalizePath(base + *sh);
        if (auto* v = get("filter_linear" + s)) d.filterLinear = parseBool(*v);
        if (auto* v = get("float_framebuffer" + s)) d.floatFb = parseBool(*v);
        if (auto* v = get("wrap_mode" + s)) d.clampToBorder = *v == "clamp_to_border";
        if (auto* v = get("scale_type" + s)) {
            if (auto t = parseScaleType(*v)) d.typeX = d.typeY = *t;
        }
        if (auto* v = get("scale_type_x" + s)) {
            if (auto t = parseScaleType(*v)) d.typeX = *t;
        }
        if (auto* v = get("scale_type_y" + s)) {
            if (auto t = parseScaleType(*v)) d.typeY = *t;
        }
        if (auto* v = get("scale" + s)) d.scaleX = d.scaleY = std::strtof(v->c_str(), nullptr);
        if (auto* v = get("scale_x" + s)) d.scaleX = std::strtof(v->c_str(), nullptr);
        if (auto* v = get("scale_y" + s)) d.scaleY = std::strtof(v->c_str(), nullptr);
        p.passes.push_back(std::move(d));
    }
    if (auto* t = get("textures")) {
        std::string name;
        std::istringstream names(*t);
        while (std::getline(names, name, ';')) {
            name = trim(name);
            if (name.empty()) continue;
            LutDesc l;
            l.name = name;
            if (auto* v = get(name)) l.path = normalizePath(base + *v);
            if (auto* v = get(name + "_linear")) l.linear = parseBool(*v);
            p.luts.push_back(std::move(l));
        }
    }
    if (auto* ps = get("parameters")) {
        std::string name;
        std::istringstream names(*ps);
        while (std::getline(names, name, ';')) {
            name = trim(name);
            if (auto* v = get(name)) p.params[name] = std::strtof(v->c_str(), nullptr);
        }
    }
    return p;
}

void appendPreset(PresetDesc& a, const PresetDesc& b) {
    a.passes.insert(a.passes.end(), b.passes.begin(), b.passes.end());
    a.luts.insert(a.luts.end(), b.luts.begin(), b.luts.end());
    for (const auto& [k, v] : b.params) a.params[k] = v;
}

ShaderSource prepareShader(std::string_view src) {
    ShaderSource out;
    std::string version = "#version 130";
    std::string body;
    std::istringstream in{std::string(src)};
    std::string line;
    bool versionTaken = false;
    while (std::getline(in, line)) {
        const std::string t = trim(line);
        if (!versionTaken && t.rfind("#version", 0) == 0) {
            version = t;
            versionTaken = true;
            body += "\n";  // Zeilennummern der Fehlermeldungen bleiben gleich
            continue;
        }
        if (t.rfind("#pragma parameter", 0) == 0) {
            // #pragma parameter NAME "Beschreibung" Standard Min Max [Schritt]
            std::string rest = trim(std::string_view(t).substr(17));
            const auto sp = rest.find_first_of(" \t");
            ShaderParam p;
            p.name = rest.substr(0, sp);
            const auto q2 = rest.find('"', rest.find('"') + 1);
            if (q2 != std::string::npos) p.def = std::strtof(rest.c_str() + q2 + 1, nullptr);
            bool dup = false;
            for (const auto& e : out.params) dup |= e.name == p.name;
            if (!dup && !p.name.empty()) out.params.push_back(p);
        }
        body += line;
        body += '\n';
    }
    out.vertex = version + "\n#define VERTEX\n#define PARAMETER_UNIFORM\n#line 1\n" + body;
    out.fragment = version + "\n#define FRAGMENT\n#define PARAMETER_UNIFORM\n#line 1\n" + body;
    return out;
}

}  // namespace rah
