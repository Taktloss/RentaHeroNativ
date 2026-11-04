// SPDX-License-Identifier: GPL-3.0-or-later
#include "render/tccsprite.hpp"

#include "render/blit.hpp"

namespace rah {

void TccSprite::calcAlphaBounds(const TccPlanes& planes) {
    if (!planes.hasAlpha()) {
        alphaRect = {0, 0, static_cast<int>(planes.width), static_cast<int>(planes.height)};
        return;
    }
    unsigned minX = 10000, minY = 10000, maxX = 0, maxY = 0;
    std::size_t k = 0;
    for (unsigned y = 0; y < planes.height; ++y) {
        for (unsigned x = 0; x < planes.width; ++x, ++k) {
            if (planes.alpha[k] == 0) continue;
            if (x < minX) minX = x;
            if (x > maxX) maxX = x;
            if (y < minY) minY = y;
            if (y > maxY) maxY = y;
        }
    }
    alphaRect = {static_cast<int>(minX), static_cast<int>(minY), static_cast<int>(maxX), static_cast<int>(maxY)};
}

void TccSprite::setClip(const Rect& r) {
    clip = {};
    if (r.left < 0) clip.left = -r.left;
    if (r.right > 640) clip.right = r.right - 640;
    if (r.top < 0) {
        clip.top = -r.top;
        return;
    }
    if (r.bottom > 360) clip.bottom = r.bottom - 360;
}

bool TccSprite::draw(Surface565& s, const TccPlanes& planes, Rect rect, int flag, const ZMap* zmap, int z,
                     int zoom) {
    if (planes.rgb[0].empty()) return false;  // +0x390: kein Bild
    setClip(rect);
    lastRect = rect;
    if (!(rect.top == 360 && rect.bottom == 420) && !clipToScene(rect)) return true;
    const Rect visible = rect;  // +0x20
    if (flag == 0) s.fill(rect.left, rect.top, rect.right, rect.bottom, 0);
    // Lock(rect): außerhalb oder umgedreht → DDERR_INVALIDPARAMS, leer erlaubt (Wine-Messung, WINEPROBE.md)
    if (rect.left < 0 || rect.top < 0 || rect.right > s.width || rect.bottom > s.height ||
        rect.right < rect.left || rect.bottom < rect.top)
        return false;

    // orig: 0x0043B9ED–0x0043BA1C – Z wird vorzeichenlos gegen 255.0 verglichen
    if (!planes.hasAlpha() || flag == 0 || static_cast<unsigned>(z) >= 255u) {
        blitOpaque565(s, planes, rect.left, rect.top);
        return true;
    }
    // UNVERIFIED (RENDERER.md): Tiefentest nur mit Tiefenkarte und Z ≥ 4
    DepthTest dt{zmap ? zmap->data.data() : nullptr, z};
    const DepthTest* depth = (zmap && !zmap->data.empty() && z >= 4) ? &dt : nullptr;

    if (zoom != 100) {
        blitAlphaZoom565(s, planes, lastRect, visible, clip.left, clip.top, depth);
        return true;
    }
    const int w = static_cast<int>(planes.width), h = static_cast<int>(planes.height);
    int left0, top0, width0, height0;
    if (dirtyAlpha) {
        // orig: 0x0043AF02–0x0043AFCF (Eigenheiten: gespeicherter unterer Rand schrumpft beim
        // unteren Clipping, rechter Rand nur für diesen Aufruf)
        top0 = alphaRect.top + clip.top;
        left0 = alphaRect.left + clip.left;
        const int bottomEff = alphaRect.bottom - clip.bottom;
        if (alphaRect.bottom != 0 && clip.bottom != 0) --alphaRect.bottom;
        int rightEff = alphaRect.right - clip.right;
        if (alphaRect.right != 0 && clip.right != 0) --rightEff;
        width0 = rightEff - left0 + 1;
        height0 = bottomEff - top0 + 1;
    } else {
        // orig: 0x0043AFDB–0x0043B040
        top0 = clip.top;
        left0 = clip.left;
        width0 = w - clip.right - clip.left;
        height0 = h - clip.bottom - clip.top;
    }
    if (width0 <= 0 || height0 <= 0) return true;  // UNVERIFIED: Original rechnet vorzeichenlos
    const std::size_t origin = static_cast<std::size_t>(w) * static_cast<std::size_t>(top0) +
                               static_cast<std::size_t>(left0);
    blitAlpha565(s, planes, origin, width0, height0, w - width0, rect.left, rect.top, depth);
    return true;
}

}  // namespace rah
