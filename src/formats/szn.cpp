// SPDX-License-Identifier: GPL-3.0-or-later
#include "formats/szn.hpp"

#include <cstdio>

namespace rah {

namespace {

constexpr const char* kNull = "NULL";  // 0x004707D4
// Puffergrößen im Original: Sektionsnamen 100, Dateinamen 0x104
constexpr std::size_t kNameBuf = 100, kPathBuf = 0x104;

std::optional<std::string> optString(const IniFile& f, const std::string& sec, const char* key) {
    std::string v = f.getString(sec, key, kNull, kPathBuf);
    if (v == kNull) return std::nullopt;
    return v;
}

}  // namespace

LayerKind layerKindFromName(std::string_view n) {
    if (n.size() < 3) return LayerKind::Unknown;  // Port-Schutz: kein Dateiname < 3 Zeichen in den Daten (tools/data_scan.py)
    const std::string_view e = n.substr(n.size() - 3);
    if (e == "BMP" || e == "bmp") return LayerKind::Bmp;
    if (e == "TCC" || e == "tcc") return LayerKind::Tcc;
    if (e == "A16" || e == "a16") return LayerKind::A16;
    if (e == "AVI" || e == "avi") return LayerKind::Avi;
    return LayerKind::Unknown;
}

SceneDesc readSceneDesc(const IniFile& f, unsigned loadMask) {
    SceneDesc d;
    const std::string bg = f.getString("GENERAL", "Background1", kNull, kNameBuf);
    if (bg != kNull) {
        SznBackground b;
        b.section = bg;
        b.filename = f.getString(bg, "Filename", kNull, kPathBuf);
        b.zBuf = optString(f, bg, "zBuf");
        b.wBuf = optString(f, bg, "wBuf");
        b.fps = f.getInt(bg, "fps", 0);
        d.background = std::move(b);
    }

    char key[32];
    for (int i = 1; i <= 40; ++i) {  // Abbruch beim ersten fehlenden LayerN
        std::snprintf(key, sizeof key, "Layer%d", i);
        const std::string sec = f.getString("GENERAL", key, kNull, kNameBuf);
        if (sec == kNull) break;
        SznLayer l;
        l.section = sec;
        l.loadCond = f.getInt(sec, "LoadCond", 0);
        const unsigned lc = static_cast<unsigned>(l.loadCond);
        if (lc != 0 && (lc & loadMask) != lc) {
            ++d.skippedByLoadCond;
            continue;
        }
        l.filename = f.getString(sec, "Filename", kNull, kPathBuf);
        l.x = f.getInt(sec, "X", 0);
        l.y = f.getInt(sec, "Y", 0);
        l.fps = f.getInt(sec, "fps", 0);
        // Die folgenden Werte liest das Original erst nach erfolgreichem CSzene_CreateLayer;
        // ohne Ebene sind sie bedeutungslos.
        l.dirtyAlpha = f.getInt(sec, "DirtyAlpha", 0);
        l.z = f.getInt(sec, "Z", 1);
        l.zBuffered = f.getInt(sec, "ZBuffered", 1);
        l.zoomVal = f.getInt(sec, "ZoomVal", 100);
        d.layers.push_back(std::move(l));
    }

    for (int i = 1; i <= 60; ++i) {  // Abbruch beim ersten fehlenden AreaN
        std::snprintf(key, sizeof key, "Area%d", i);
        const std::string sec = f.getString("GENERAL", key, kNull, kNameBuf);
        if (sec == kNull) break;
        SznArea a;
        a.section = sec;
        a.x1 = f.getInt(sec, "X1", 0);
        a.x2 = f.getInt(sec, "X2", 0);
        a.y1 = f.getInt(sec, "Y1", 0);
        a.y2 = f.getInt(sec, "Y2", 0);
        a.cursorType = f.getInt(sec, "CursorType", 0);
        d.areas.push_back(std::move(a));
    }

    for (int n = 0; n < 2; ++n) {
        const std::string sec = f.getString("GENERAL", n == 0 ? "Character1" : "Character2", kNull, kNameBuf);
        if (sec == kNull) continue;
        SznCharacter c;
        c.section = sec;
        c.filename = f.getString(sec, "Filename", kNull, kPathBuf);
        c.type = f.getInt(sec, "Type", 0);
        c.sfxLeft = optString(f, sec, "SFXLeft");
        if (c.sfxLeft) {
            c.sfxRight = optString(f, sec, "SFXRight");
            c.sfxAlt = optString(f, sec, "SFXAlt");
        }
        c.perCor = f.getInt(sec, "PerCor", 100);
        c.redAdd = f.getInt(sec, "RedAdd", 0);
        c.greenAdd = f.getInt(sec, "GreenAdd", 0);
        c.blueAdd = f.getInt(sec, "BlueAdd", 0);
        for (int i = 0; i < 10; ++i) {
            auto get = [&](const char* base) {
                std::snprintf(key, sizeof key, "%s%1d", base, i + 1);
                return f.getInt(sec, key, 0);
            };
            auto& ca = c.colorAdd[static_cast<std::size_t>(i)];
            ca.red = get("RedAdd");
            ca.green = get("GreenAdd");
            ca.blue = get("BlueAdd");
            ca.x = get("ColorAddX");
            ca.y = get("ColorAddY");
            ca.range = get("ColorAddRange");
        }
        c.x = f.getInt(sec, "X", 0);
        c.y = f.getInt(sec, "Y", 0);
        c.zpos = f.getInt(sec, "Zpos", 0);
        c.zBuffered = f.getInt(sec, "ZBuffered", 1);
        c.zoomVal = f.getInt(sec, "ZoomVal", 0);
        c.direction = f.getInt(sec, "Direction", 0);
        d.characters[static_cast<std::size_t>(n)] = std::move(c);
    }
    return d;
}

}  // namespace rah
