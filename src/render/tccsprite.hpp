// SPDX-License-Identifier: GPL-3.0-or-later
// Zeichenzustand eines TCC-Bilds wie CImageTcc (0x3A4 Bytes): Ebenen, Alpha-Rechteck,
// Clipping-Ränder und das Zeichnen nach CImageTcc_Draw (docs/COMPOSITOR.md, RENDERER.md).
#pragma once

#include "core/rect.hpp"
#include "formats/tcc.hpp"
#include "formats/zbf.hpp"
#include "render/surface.hpp"

namespace rah {

struct TccSprite {
    TccPlanes planes;
    bool dirtyAlpha = false;  // +0x37C: Alpha-Rechteck nach jedem Dekodieren neu berechnen
    Rect alphaRect;           // +0x35C, rechts/unten einschließend (Pixel mit Alpha ≠ 0)
    Rect clip;                // +0x34C: Ränder links/oben/rechts/unten in Zielpixeln
    Rect lastRect;            // +0x24: übergebenes Zielrechteck vor dem Clippen

    bool hasAlpha() const { return !planes.alpha.empty(); }  // +0x334
    int width() const { return static_cast<int>(planes.width); }
    int height() const { return static_cast<int>(planes.height); }

    // Nach dem Dekodieren (orig: 0x0043B8A7 in CImageTcc_Decode)
    void afterDecode() {
        if (dirtyAlpha) calcAlphaBounds();
    }
    // orig: Rent-A-Hero.exe 0x004386E0 CImage_CalcAlphaBounds
    void calcAlphaBounds();
    // orig: Rent-A-Hero.exe 0x00438040 – Clipping-Ränder; bei top < 0 wird unten nicht geprüft
    void setClip(const Rect& r);
    // orig: Rent-A-Hero.exe 0x0043B900 CImageTcc_Draw(surface, rect, flag, zmap, z, zoom)
    bool draw(Surface565& s, Rect rect, int flag, const ZMap* zmap, int z, int zoom);
};

}  // namespace rah
