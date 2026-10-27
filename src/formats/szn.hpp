// SPDX-License-Identifier: GPL-3.0-or-later
// Inhalt einer SZN-Szenenbeschreibung mit den Leseregeln von CSzene_LoadSzn
// (docs/formats/szn.md). Erzeugt keine Objekte; das übernimmt die Szene.
#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include "formats/ini.hpp"

namespace rah {

struct SznBackground {
    std::string section, filename;
    std::optional<std::string> zBuf, wBuf;  // fehlt ⇔ "NULL"
    int fps = 0;
};

struct SznLayer {
    std::string section, filename;
    int loadCond = 0, x = 0, y = 0, fps = 0;
    int z = 1, zBuffered = 1, dirtyAlpha = 0, zoomVal = 100;
};

struct SznArea {
    std::string section;
    int x1 = 0, x2 = 0, y1 = 0, y2 = 0;  // RECT{left=X1, top=Y1, right=X2, bottom=Y2}
    int cursorType = 0;
};

struct SznColorAdd {
    int red = 0, green = 0, blue = 0, x = 0, y = 0, range = 0;
};

struct SznCharacter {
    std::string section, filename;
    int type = 0;
    std::optional<std::string> sfxLeft, sfxRight, sfxAlt;  // Right/Alt nur gelesen, wenn Left gesetzt
    int perCor = 100, redAdd = 0, greenAdd = 0, blueAdd = 0;
    std::array<SznColorAdd, 10> colorAdd{};  // RedAdd1 … RedAdd10 usw.
    int x = 0, y = 0, zpos = 0, zBuffered = 1, zoomVal = 0, direction = 0;
};

struct SceneDesc {
    std::optional<SznBackground> background;
    std::vector<SznLayer> layers;  // nur Ebenen, deren LoadCond zur LoadMask passt
    int skippedByLoadCond = 0;
    std::vector<SznArea> areas;
    std::array<std::optional<SznCharacter>, 2> characters;
};

// Bildart einer Ebene aus den letzten drei Zeichen des Dateinamens, case-sensitiv
// (nur "BMP"/"bmp" usw.; "Bmp" → Unknown und scheitert später, docs/STRUCTURES.md).
enum class LayerKind { Bmp = 0, Tcc = 1, A16 = 2, Avi = 3, Unknown = 4 };
// orig: Rent-A-Hero.exe 0x00446940 CLayer_Load (Typbestimmung +0x220)
LayerKind layerKindFromName(std::string_view filename);

// orig: Rent-A-Hero.exe 0x00449380 CSzene_LoadSzn (nur das Lesen der Werte)
SceneDesc readSceneDesc(const IniFile& szn, unsigned loadMask);

}  // namespace rah
