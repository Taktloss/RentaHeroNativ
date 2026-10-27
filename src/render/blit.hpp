// SPDX-License-Identifier: GPL-3.0-or-later
// Blitter-Kerne für TCC-Ebenen auf RGB565 (Formeln und Eigenheiten: docs/RENDERER.md).
// Die Aufrufer übergeben das schon geclippte Rechteck wie das Original (CImageTcc_Draw
// 0x0043B900); die Kerne selbst prüfen nur Puffergrenzen.
#pragma once

#include <cstdint>

#include "formats/tcc.hpp"
#include "render/surface.hpp"

namespace rah {

struct Rect {
    int left = 0, top = 0, right = 0, bottom = 0;
    int width() const { return right - left; }
    int height() const { return bottom - top; }
};

// Tiefenkarte (640 × 360, Zeilenlänge 640), adressiert über das sichtbare Zielrechteck:
// depth[(y − sceneTop)][x] (orig: 0x00439EC0 / 0x0043A0B0, Zeiger map + top·640 + left).
struct DepthTest {
    const std::uint8_t* map = nullptr;
    int z = 0;          // Ebenen-/Figurentiefe; gezeichnet nur bei depth > z
    // UNVERIFIED: Koordinatensystem des Rechtecks im Original (docs/RENDERER.md); 0 = das
    // Ziel ist eine eigene Szenenoberfläche, 60 = Bildschirm mit Szene ab Zeile 60.
    int sceneTop = 0;
};

// Ohne Zoom: Quelle ab (srcX, srcY), Ziel `dst` (sichtbarer Ausschnitt in Bildschirmkoordinaten).
// orig: Rent-A-Hero.exe 0x004399E0 (deckend), 0x00439A80 (Alpha), 0x00439EC0 (Alpha + Tiefe)
void blitOpaque565(Surface565& s, const TccPlanes& p, int srcX, int srcY, const Rect& dst);
void blitAlpha565(Surface565& s, const TccPlanes& p, int srcX, int srcY, const Rect& dst,
                  const DepthTest* depth);

// Mit Zoom: `full` = gesamtes Zielrechteck (bestimmt die Schrittweite), `dst` = sichtbarer
// Ausschnitt, clipL/clipT = Ränder in Zielpixeln (+0x34C/+0x350).
// orig: Rent-A-Hero.exe 0x00439C30 (Alpha + Zoom), 0x0043A0B0 (Alpha + Zoom + Tiefe)
void blitAlphaZoom565(Surface565& s, const TccPlanes& p, const Rect& full, const Rect& dst,
                      int clipL, int clipT, const DepthTest* depth);

}  // namespace rah
