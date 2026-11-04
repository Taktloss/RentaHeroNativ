// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/present/filters.hpp"

#include <algorithm>

#include "app/present/embedded.hpp"

namespace rah {
namespace {

constexpr FilterInfo kScalers[] = {
    {"original", "Original (Pixel)", nullptr, false, "Unverändert, nächster Nachbar"},
    {"smooth", "Weich (bilinear)", nullptr, true, "Einfache Glättung, leicht unscharf"},
    {"sharp", "Sharp-Bilinear", "interpolation/sharp-bilinear-simple.glslp", false,
     "Scharfe Pixel ohne ungleiche Breiten, auch bei krummen Faktoren"},
    {"fsr", "AMD FSR 1", "fsr/fsr.glslp", false, "Hochskalieren mit Kantenrekonstruktion und Nachschärfen"},
    {"xbr", "xBR (lv2)", "xbr/xbr-lv2.glslp", false, "Kantenglättung für Pixelgrafik (Hyllian)"},
    {"xbrz", "xBRZ (freescale)", "xbrz/xbrz-freescale.glslp", false, "xBRZ-Kantenglättung (Zenju/Hyllian)"},
    {"hq2x", "HQ2x", "hqx/hq2x.glslp", false, "Klassischer hqx-Filter, Faktor 2"},
    {"hq4x", "HQ4x", "hqx/hq4x.glslp", false, "Klassischer hqx-Filter, Faktor 4"},
    {"scalefx", "ScaleFX", "scalefx/scalefx.glslp", false, "Mehrstufige Kantenglättung (Sp00kyFox), Faktor 3"},
};

constexpr FilterInfo kCrt[] = {
    {"off", "Aus", nullptr, false, ""},
    {"lottes", "CRT-Lottes", "crt/crt-lottes.glslp", false, "Scanlines, Lochmaske, leichte Wölbung, Bloom"},
    {"geom", "CRT-Geom", "crt/crt-geom.glslp", false, "Gewölbte Röhre mit Scanlines und Maske"},
};

std::optional<PresetDesc> loadPreset(const char* path, std::string* err) {
    const auto text = embeddedFile(path);
    if (!text) {
        if (err) *err = std::string(path) + " nicht eingebettet";
        return std::nullopt;
    }
    const std::string p(path);
    const auto slash = p.rfind('/');
    return parsePreset(*text, slash == std::string::npos ? std::string() : p.substr(0, slash), err);
}

}  // namespace

std::span<const FilterInfo> scalerFilters() { return kScalers; }
std::span<const FilterInfo> crtFilters() { return kCrt; }

const FilterInfo* findFilter(std::span<const FilterInfo> list, std::string_view id) {
    for (const auto& f : list)
        if (id == f.id) return &f;
    return nullptr;
}

std::string nextFilterId(std::span<const FilterInfo> list, std::string_view id, int dir) {
    const int n = static_cast<int>(list.size());
    int i = 0;
    for (int k = 0; k < n; ++k)
        if (id == list[k].id) i = k;
    return list[((i + dir) % n + n) % n].id;
}

std::optional<PresetDesc> buildChain(const DisplaySettings& d, bool* linear, std::string* err) {
    const FilterInfo* crt = findFilter(kCrt, d.crt);
    const FilterInfo* sc = findFilter(kScalers, d.filter);
    if (!sc) sc = &kScalers[0];
    const FilterInfo* main = (crt && crt->preset) ? crt : sc;
    if (linear) *linear = main->linear;
    if (!d.deband && !main->preset) return std::nullopt;
    PresetDesc chain;
    if (d.deband) {
        auto p = loadPreset("misc/deband.glslp", err);
        if (!p) return std::nullopt;
        appendPreset(chain, *p);
    }
    if (main->preset) {
        auto p = loadPreset(main->preset, err);
        if (!p) return std::nullopt;
        appendPreset(chain, *p);
    } else if (!chain.passes.empty()) {
        // Nur Entbänderung: in Originalgröße rechnen, der Ausgabedurchgang des Ports streckt dann
        // mit nächstem Nachbarn („Original“) bzw. bilinear („Weich“)
        chain.passes.back().typeX = chain.passes.back().typeY = ScaleType::Source;
    }
    return chain;
}

std::string displayLabel(const DisplaySettings& d) {
    const FilterInfo* crt = findFilter(kCrt, d.crt);
    const FilterInfo* sc = findFilter(kScalers, d.filter);
    std::string s = (crt && crt->preset) ? crt->name : (sc ? sc->name : kScalers[0].name);
    if (d.deband) s += " + Entbänderung";
    return s;
}

OutRect computeOutRect(int dw, int dh, int fw, int fh, bool integerScale) {
    OutRect r;
    if (dw <= 0 || dh <= 0 || fw <= 0 || fh <= 0) return r;
    const int k = std::min(dw / fw, dh / fh);
    if (integerScale && k >= 1) {
        r.w = fw * k;
        r.h = fh * k;
    } else if (static_cast<long long>(dw) * fh > static_cast<long long>(dh) * fw) {
        r.h = dh;
        r.w = static_cast<int>(static_cast<long long>(dh) * fw / fh);
    } else {
        r.w = dw;
        r.h = static_cast<int>(static_cast<long long>(dw) * fh / fw);
    }
    r.x = (dw - r.w) / 2;
    r.y = (dh - r.h) / 2;
    return r;
}

}  // namespace rah
