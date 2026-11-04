// SPDX-License-Identifier: GPL-3.0-or-later
// libretro-Shader-Presets (.glslp) und GLSL-Quellen für den Bildfilter (Port; docs/FILTERS.md)
#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace rah {

enum class ScaleType { Unset, Source, Viewport, Absolute };

struct PassDesc {
    std::string shader;           // Pfad relativ zum Shader-Wurzelordner
    bool filterLinear = false;    // Abtastung der Eingabe dieses Durchgangs
    ScaleType typeX = ScaleType::Unset, typeY = ScaleType::Unset;
    float scaleX = 1.0f, scaleY = 1.0f;
    bool floatFb = false;
    bool clampToBorder = false;   // wrap_mode clamp_to_border (sonst clamp_to_edge)
};

struct LutDesc {
    std::string name, path;
    bool linear = false;
};

struct PresetDesc {
    std::vector<PassDesc> passes;
    std::vector<LutDesc> luts;
    std::map<std::string, float> params;  // Überschreibungen der #pragma-parameter-Werte
};

// "a/b/../c" → "a/c"
std::string normalizePath(std::string_view p);

// .glslp lesen; Pfade werden relativ zu dir (Ordner des Presets) aufgelöst
std::optional<PresetDesc> parsePreset(std::string_view text, std::string_view dir, std::string* err);

// Hängt b an a an (Durchgänge, LUTs, Parameter) – z. B. Entbänderung vor einem Filter
void appendPreset(PresetDesc& a, const PresetDesc& b);

struct ShaderParam {
    std::string name;
    float def = 0;
};

// libretro-GLSL: eine Datei mit #if defined(VERTEX) / defined(FRAGMENT). Die #version-Zeile wird
// nach vorn gezogen, danach VERTEX/FRAGMENT und PARAMETER_UNIFORM definiert.
struct ShaderSource {
    std::string vertex, fragment;
    std::vector<ShaderParam> params;  // #pragma parameter NAME "Text" Standard Min Max Schritt
};
ShaderSource prepareShader(std::string_view src);

}  // namespace rah
