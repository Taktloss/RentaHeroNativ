// SPDX-License-Identifier: GPL-3.0-or-later
// Statische Szenenebene (CLayer, 0x2B0 Bytes, vtable 0x0046B918) mit BMP- oder TCC-Bild.
// Feldnamen mit Offsets des Originals; Verhalten: docs/COMPOSITOR.md, docs/STRUCTURES.md.
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "core/rect.hpp"
#include "engine/globals.hpp"
#include "engine/paths.hpp"
#include "formats/bmp.hpp"
#include "formats/szn.hpp"
#include "formats/zbf.hpp"
#include "render/surface.hpp"
#include "render/tccsprite.hpp"

namespace rah {

class Layer {
public:
    virtual ~Layer() = default;

    // Werte aus dem Konstruktor 0x004466A0
    std::string name;            // +0x15C (Sektionsname)
    std::string path;            // +0x5C
    LayerKind kind = LayerKind::Bmp;  // +0x220
    double x = 0, y = 0;         // +0x10, +0x18
    double z = 1.0;              // +0x20
    double zoom = 100.0;         // +0x28
    double prevZ = -1.0;         // +0x38 altes Z nach einer Z-Änderung (−1.0 = keine), CSzene_Update
    bool zBuffered = true;       // +0x40
    int tweenSteps = 0;          // +0x50 verbleibende Interpolationsschritte (0x00447890)
    int fps = 10;                // +0x54
    bool alwaysUpdate = false;   // +0x58
    bool alpha = false;          // +0x198
    bool dirtyAlpha = false;     // +0x19C
    bool dirty = false;          // +0x1A0
    bool active = false;         // +0x1A4
    // Abspielzustand; im CLayer angelegt, genutzt von CAnimLayer und CSzene_DispatchEvents
    bool running = false;        // +0x1A8
    bool loop = false;           // +0x1AC
    int endMode = 0;             // +0x1B0 (1 normal, 2 = am Ende auf den Bereichsanfang)
    bool hideAtEnd = false;      // +0x1B4
    bool ended = false;          // +0x1B8
    bool isAnim = false;         // +0x1BC: CAnimLayer (CLayer_Load setzt es für AVI)
    std::unique_ptr<Event> endEvent;    // +0x1C4 (Typ 1)
    std::unique_ptr<Event> frameEvent;  // +0x1CC (Typ 3)
    std::unique_ptr<Event> countEvent;  // +0x1D0 (Typ 4)
    std::unique_ptr<Event> tweenEvent;  // +0x1D4 (Interpolation fertig, CLayer_Update)
    bool loaded = false;         // +0x1D8
    std::uint16_t colorKey = 0;  // +0x1E4
    int width = 0, height = 0;   // +0x1E8, +0x1EC
    Rect full;                   // +0x1F0
    Rect zoomed;                 // +0x200
    Rect old;                    // +0x210

    Image565 bmp;           // Oberfläche +0x08 bei Ebenen ohne Alpha
    TccPlanes still;        // Bild +0x0C bei TCC
    TccSprite tcc;          // Zeichenzustand des Bilds +0x0C
    bool hasImage = false;  // +0x0C != 0 (BMP-Bild wird nach dem Anlegen der Oberfläche freigegeben)

    // orig: Rent-A-Hero.exe 0x00446940 CLayer_Load + vtable +0x0C (0x00446AC0 CLayer_CreateImage)
    virtual bool load(const FoundFile& file, std::uint16_t colorKey, std::string* error);
    // vtable +0x18 – orig: Rent-A-Hero.exe 0x00447890 CLayer_Update (Interpolation, +0x50)
    virtual bool update(EngineGlobals& g);
    // Ebenen des aktuellen Bilds für CImageTcc_Draw
    virtual const TccPlanes& planes() const { return still; }
    // orig: Rent-A-Hero.exe 0x00446E70 (vtable +0x14 SetPos)
    void setPos(int nx, int ny);
    // orig: Rent-A-Hero.exe 0x00447470 CLayer_Enable
    void enable();
    // orig: Rent-A-Hero.exe 0x00447490 CLayer_Disable (Bereich +0x1C0 folgt mit CArea)
    void disable() {
        if (!active) return;
        active = false;
        dirty = true;
    }
    // orig: Rent-A-Hero.exe 0x00447030 CLayer_SetZoom + 0x00447060
    void setZoom(int zoomVal);
    // orig: Rent-A-Hero.exe 0x00447B80 CLayer_GetRect
    Rect getRect() const;
    // orig: Rent-A-Hero.exe 0x004471C0 CLayer_Draw(surface, flag, srcRect, zmap)
    bool draw(Surface565& s, int flag, const Rect& src, const ZMap* zmap);
    bool draw(Surface565& s, int flag, const ZMap* zmap) { return draw(s, flag, full, zmap); }
};

}  // namespace rah
