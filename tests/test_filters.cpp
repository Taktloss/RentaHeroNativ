// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/present/embedded.hpp"
#include "app/present/filters.hpp"
#include "app/present/preset.hpp"
#include "test.hpp"

using namespace rah;

TEST(filters_normalize_and_parse_preset) {
    CHECK(normalizePath("hqx/./shader-files/../resources/hq4x.png") == "hqx/resources/hq4x.png");
    CHECK(normalizePath("a\\b/c") == "a/b/c");
    std::string err;
    const auto p = parsePreset("shaders = \"2\"\n# Kommentar\nshader0 = a.glsl\nfilter_linear0 = true\n"
                               "scale_type0 = source\nscale0 = 2.0\nfloat_framebuffer0 = true\n"
                               "shader1 = ../x/b.glsl\nscale_type_x1 = viewport\nwrap_mode1 = \"clamp_to_border\"\n"
                               "textures = LUT\nLUT = res/l.png\nLUT_linear = true\nparameters = \"S;T\"\nS = 4.0\n",
                               "dir/sub", &err);
    REQUIRE(p);
    REQUIRE(p->passes.size() == 2);
    CHECK(p->passes[0].shader == "dir/sub/a.glsl");
    CHECK(p->passes[0].filterLinear && p->passes[0].floatFb);
    CHECK(p->passes[0].typeX == ScaleType::Source && p->passes[0].scaleX == 2.0f);
    CHECK(p->passes[1].shader == "dir/x/b.glsl");
    CHECK(p->passes[1].typeX == ScaleType::Viewport && p->passes[1].typeY == ScaleType::Unset);
    CHECK(p->passes[1].clampToBorder && !p->passes[1].filterLinear);
    REQUIRE(p->luts.size() == 1);
    CHECK(p->luts[0].name == "LUT" && p->luts[0].path == "dir/sub/res/l.png" && p->luts[0].linear);
    CHECK(p->params.size() == 1 && p->params.at("S") == 4.0f);
    CHECK(!parsePreset("shader0 = a.glsl\n", "", &err));
}

TEST(filters_prepare_shader) {
    const auto s = prepareShader("// Kopf\n#version 130\n#pragma parameter SHARP \"Schärfe\" 0.5 0.0 1.0 0.05\n"
                                 "#pragma parameter SHARP \"doppelt\" 9.0 0.0 1.0 0.05\nvoid main(){}\n");
    CHECK(s.vertex.rfind("#version 130\n#define VERTEX\n#define PARAMETER_UNIFORM\n", 0) == 0);
    CHECK(s.fragment.rfind("#version 130\n#define FRAGMENT\n", 0) == 0);
    CHECK(s.vertex.find("\n#version") == std::string::npos);
    REQUIRE(s.params.size() == 1);
    CHECK(s.params[0].name == "SHARP" && s.params[0].def == 0.5f);
}

// Alle Filter: Preset eingebettet, jeder Durchgang und jede LUT vorhanden, Shader mit VERTEX/FRAGMENT
TEST(filters_all_presets_embedded) {
    auto checkList = [](std::span<const FilterInfo> list) {
        for (const auto& f : list) {
            if (!f.preset) continue;
            DisplaySettings d;
            if (list.data() == crtFilters().data()) d.crt = f.id;
            else d.filter = f.id;
            for (int deband = 0; deband < 2; ++deband) {
                d.deband = deband != 0;
                std::string err;
                bool linear = true;
                const auto c = buildChain(d, &linear, &err);
                CHECK(c);
                if (!c) {
                    std::fprintf(stderr, "  %s: %s\n", f.id, err.c_str());
                    continue;
                }
                CHECK(!c->passes.empty());
                CHECK(!linear);
                for (const auto& p : c->passes) {
                    const auto src = embeddedFile(p.shader);
                    CHECK(src);
                    if (src) {
                        CHECK(src->find("defined(VERTEX)") != std::string_view::npos);
                        CHECK(src->find("FRAGMENT") != std::string_view::npos);
                    }
                }
                for (const auto& l : c->luts) CHECK(embeddedFile(l.path));
            }
        }
    };
    checkList(scalerFilters());
    checkList(crtFilters());
    CHECK(buildChain(DisplaySettings{}, nullptr, nullptr) == std::nullopt);  // Original: direkt
    bool linear = false;
    DisplaySettings soft;
    soft.filter = "smooth";
    std::string err;
    CHECK(!buildChain(soft, &linear, &err) && linear && err.empty());
    // CRT ersetzt den Filter
    DisplaySettings both;
    both.filter = "fsr";
    both.crt = "lottes";
    const auto c = buildChain(both, nullptr, nullptr);
    REQUIRE(c);
    CHECK(c->passes.size() == 1 && c->passes[0].shader == "crt/shaders/crt-lottes.glsl");
    CHECK(displayLabel(both) == "CRT-Lottes");
}

TEST(filters_cycle_and_outrect) {
    CHECK(nextFilterId(scalerFilters(), "original") == "smooth");
    CHECK(nextFilterId(scalerFilters(), "original", -1) == "scalefx");
    CHECK(nextFilterId(scalerFilters(), "scalefx") == "original");
    CHECK(nextFilterId(crtFilters(), "geom") == "off");
    CHECK(nextFilterId(crtFilters(), "unbekannt") == "lottes");
    // ganzzahlig: 1920×1080 → 2×(1280×960), zentriert
    OutRect r = computeOutRect(1920, 1080, 640, 480, true);
    CHECK(r.w == 1280 && r.h == 960 && r.x == 320 && r.y == 60);
    // füllen: 4:3 in 1920×1080 → 1440×1080
    r = computeOutRect(1920, 1080, 640, 480, false);
    CHECK(r.w == 1440 && r.h == 1080 && r.x == 240 && r.y == 0);
    // kleiner als 640×480: einpassen statt Faktor 0
    r = computeOutRect(320, 240, 640, 480, true);
    CHECK(r.w == 320 && r.h == 240);
}
