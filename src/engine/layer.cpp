// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/ftol.hpp"
#include "engine/layer.hpp"

#include "engine/area.hpp"

#include <cmath>

namespace rah {

bool Layer::load(const FoundFile& file, std::uint16_t key, std::string* error) {
    path = file.path;
    const auto data = file.read();
    if (!data) {  // orig: _access-Prüfung in CLayer_Load
        if (error) *error = "Datei fehlt: " + file.path;
        return false;
    }
    kind = layerKindFromName(path);
    loaded = true;
    colorKey = key;
    if (active) dirty = true;
    if (kind == LayerKind::Avi) isAnim = true;

    // CLayer_CreateImage (vtable +0x0C)
    switch (kind) {
    case LayerKind::Bmp:
        if (!loadBmp565(*data, bmp, error)) return false;
        width = bmp.width;
        height = bmp.height;
        alpha = false;
        hasImage = false;  // 0x00446DE0: Bild ohne Alpha nach dem Anlegen der Oberfläche freigegeben
        break;
    case LayerKind::Tcc: {
        // Typ 1 setzt DirtyAlpha zwingend, damit auch das Bild-Flag +0x37C vor dem Laden
        dirtyAlpha = true;
        tcc.dirtyAlpha = true;
        TccImage img;
        if (!img.load(*data, error)) return false;
        tccData = *data;
        still = std::move(img.planes());
        tcc.afterDecode(still);
        width = static_cast<int>(still.width);
        height = static_cast<int>(still.height);
        alpha = still.hasAlpha();
        hasImage = true;
        if (!alpha) {
            // 0x00446C90: Ebenenoberfläche anlegen und per Bild-Slot 3 (0x0043B8C0 → CImageTcc_Draw
            // an (0, 0), flag 0) deckend füllen; gezeichnet wird dann wie bei BMP
            Surface565 tmp(width, height);
            tcc.draw(tmp, still, Rect{0, 0, width, height}, 0, nullptr, 0, 100);
            bmp.width = width;
            bmp.height = height;
            bmp.pixels = std::move(tmp.pixels);
        }
        break;
    }
    default:
        // A16 (CAnimLayer) und AVI werden als eigene Klassen portiert
        if (error) *error = "Ebenenart noch nicht portiert: " + path;
        return false;
    }
    full = {0, 0, width, height};
    zoomed = full;
    return true;
}

// Zielwert „keiner“: Konstante 0x0046B568 = 0.0
constexpr double kNoTarget = 0.0;

bool Layer::update(EngineGlobals& g) {
    if (tweenSteps == 0) return true;
    if (--tweenSteps == 0) {
        // letzter Schritt: Zielwerte direkt übernehmen
        if (tweenX != kNoTarget) x = tweenX;
        if (tweenY != kNoTarget) y = tweenY;
        if (tweenZ != kNoTarget) z = tweenZ;  // ohne +0x38; CSzene_Update vergleicht ftol(Z)
        if (tweenZoom != kNoTarget) zoom = tweenZoom;
        // Eigenheit: ist nur ein Kanal ≠ 0, werden die übrigen am Ende auf 0 gesetzt
        if (tweenR != kNoTarget || tweenG != kNoTarget || tweenB != kNoTarget)
            setColor(ftol(tweenR), ftol(tweenG), ftol(tweenB));
        if (tweenEvent) tweenEvent->mark(g, 2);
    } else {
        x += stepX;
        y += stepY;
        z += stepZ;
        zoom += stepZoom;
        if (tweenR != kNoTarget || tweenG != kNoTarget || tweenB != kNoTarget) {
            curR += stepR;
            curG += stepG;
            curB += stepB;
            setColor(ftol(curR), ftol(curG), ftol(curB));
        }
    }
    if (tweenZoom != kNoTarget) updateZoomRect();  // aktualisiert auch den Bereich
    else refreshArea();
    dirty = true;
    return true;
}

void Layer::tweenTo(double tx, double ty, double tz, double tzoom, int r, int g, int b, int steps,
                    int handler) {
    if (steps == 0) return;
    tweenSteps = steps;
    tweenX = tx;
    tweenY = ty;
    tweenZ = tz;
    tweenZoom = tzoom;
    tweenR = r;
    tweenG = g;
    tweenB = b;
    if (handler != 0) {
        // ohne Handler bleibt ein vorhandenes Ereignis unverändert (auch aktiviert)
        if (!tweenEvent) tweenEvent = std::make_unique<Event>();
        tweenEvent->setHandler(handler);
        tweenEvent->enabled = true;
    }
    const double n = steps;
    stepX = tweenX == kNoTarget ? 0 : (tweenX - x) / n;
    stepY = tweenY == kNoTarget ? 0 : (tweenY - y) / n;
    stepZ = tweenZ == kNoTarget ? 0 : (tweenZ - z) / n;
    stepZoom = tweenZoom == kNoTarget ? 0 : (tweenZoom - zoom) / n;
    curR = colorR;
    curG = colorG;
    curB = colorB;
    stepR = tweenR == kNoTarget ? 0 : (tweenR - colorR) / n;
    stepG = tweenG == kNoTarget ? 0 : (tweenG - colorG) / n;
    stepB = tweenB == kNoTarget ? 0 : (tweenB - colorB) / n;
}

void Layer::setColor(int r, int g, int b) {
    colorR = r;
    colorG = g;
    colorB = b;
    applyImageColor(r, g, b);
}

void Layer::applyImageColor(int r, int g, int b) {
    // UNVERIFIED: BMP gibt das Bild +0x0C nach dem Anlegen der Oberfläche frei; ob der Slot-Aufruf
    // dann ins Leere geht, ist nicht geprüft (kommt in den Daten bisher nicht vor)
    if (kind != LayerKind::Tcc || !hasImage) return;
    tccColor = {r, g, b};
    if (tccRedecode(tccData, still, tccColor)) tcc.afterDecode(still);
}

void Layer::setPos(int nx, int ny) {
    x = nx;
    y = ny;
    if (active) dirty = true;
    refreshArea();
}

void Layer::refreshArea() {
    if (area && area->mask) {
        area->rect = area->mask->getRect();  // CLayer_GetRect der Masken-Ebene
        area->markGrid(area->rect);
    }
}

void Layer::deactivateArea() {
    if (area) area->active = false;
}

void Layer::releaseImage(bool force) {
    if (hasImage && (!alpha || force)) hasImage = false;
}

bool Layer::isTransparentAt(int px, int py) const {
    if (!hasImage) return false;
    int dx = px - ftol(x);
    int dy = py - ftol(y);
    if (zoom != 100.0) {
        const int zi = ftol(zoom);
        dx = dx * 100 / zi;
        dy = dy * 100 / zi;
    }
    // orig: Rent-A-Hero.exe 0x00438680 CImage_IsTransparent – Index ohne Bereichsprüfung
    const TccPlanes& p = planes();
    const long long i = static_cast<long long>(p.width) * dy + dx;
    auto at = [i](const std::vector<std::uint8_t>& v) -> std::uint8_t {
        // UNVERIFIED: außerhalb des Puffers liest das Original fremden Speicher; hier 0
        return i >= 0 && static_cast<std::size_t>(i) < v.size() ? v[static_cast<std::size_t>(i)] : 0;
    };
    if (p.hasAlpha()) return ((~at(p.alpha) & 0xFF) >> 7) != 0;
    return at(p.rgb[0]) == 0 && at(p.rgb[1]) == 0 && at(p.rgb[2]) == 0;
}

void Layer::enable() {
    if (!active) {
        active = true;
        dirty = true;
    }
}

void Layer::setZoom(int zoomVal) {
    zoom = zoomVal;
    updateZoomRect();
}

void Layer::updateZoomRect() {
    if (zoom != 100.0) {
        zoomed = full;
        const int zi = ftol(zoom);
        zoomed.right = full.right * zi / 100;
        zoomed.bottom = full.bottom * zi / 100;
    }
    refreshArea();  // auch bei Zoom 100
}

Rect Layer::getRect() const {
    Rect r;
    if (dirtyAlpha && hasImage && alpha && zoom == 100.0) r = tcc.alphaRect;
    else if (zoom == 100.0) r = full;
    else r = zoomed;
    const int ix = ftol(x), iy = ftol(y);
    r.left += ix;
    r.right += ix;
    r.top += iy;
    r.bottom += iy;
    return r;
}

bool Layer::draw(Surface565& s, int flag, const Rect& src, const ZMap* zmap) {
    if (!loaded) return false;
    if (!active) {
        dirty = false;
        return true;
    }
    if (src.right < src.left) return false;
    const Rect r = getRect();
    old = r;
    if (dirtyAlpha) {
        ++old.right;
        ++old.bottom;
    }
    if (!(y == 360.0 && height == 60) && !clipToScene(old)) {
        dirty = false;
        return true;
    }
    if (zmap && zmap->data.empty()) zmap = nullptr;

    if (kind == LayerKind::Avi) {
        // Typ 3 (0x0044723B): direkt nur bei Zoom 100, flag 0 und +0x30; Wechsel gibt die Oberfläche frei
        const bool wantDirect = zoom == 100.0 && flag == 0 && direct;
        if (wantDirect != alpha) {
            alpha = wantDirect;
            releaseSurface();
        }
        if (alpha) {
            // Slot 1 → CImageAvi_Draw mit dem ungeclippten Ebenenrechteck
            if (!drawImageDirect(s, r)) return false;
            dirty = false;
            return true;
        }
    }
    if (!alpha) {
        // DirectDraw-Oberfläche der Ebene (fehlt sie: 0x00446C90)
        if (!hasSurface && !renderToSurface()) return false;
        if (zoom != 100.0) {
            // Gestrecktes Blt nicht nachgebildet – unerreichbar: Zoom ≠ 100 haben in den Daten nur TCC mit
            // Alpha (eigener Blitter) und A16; BMP/AVI immer 100 (WINEPROBE.md). Wine streckt
            // nächster-Nachbar mit (x · ((s << 16) / d)) >> 16.
            return false;
        }
        // BltFast(r.left + src.left, r.top + src.top, src), mit Farbschlüssel bei flag != 0.
        // BltFast ohne Clipper: Quelle oder Ziel nur teilweise außerhalb, leere Quelle → DDERR_INVALIDRECT,
        // nichts gezeichnet (Wine-Messung tools/wineprobe, WINEPROBE.md)
        const int dx = r.left + src.left, dy = r.top + src.top;
        if (src.width() <= 0 || src.height() <= 0 || src.left < 0 || src.top < 0 || src.right > width || src.bottom > height || dx < 0 || dy < 0 ||
            dx + src.width() > s.width || dy + src.height() > s.height) {
            return false;  // dirty bleibt gesetzt
        }
        for (int j = 0; j < src.height(); ++j) {
            const std::uint16_t* sp = bmp.pixels.data() +
                                      static_cast<std::size_t>(src.top + j) * static_cast<std::size_t>(width) +
                                      static_cast<std::size_t>(src.left);
            std::uint16_t* dp = s.row(dy + j) + dx;
            for (int i = 0; i < src.width(); ++i) {
                if (flag != 0 && sp[i] == colorKey) continue;
                dp[i] = sp[i];
            }
        }
    } else {
        if (!tcc.draw(s, planes(), r, flag, zBuffered ? zmap : nullptr, ftol(z), ftol(zoom))) return false;
    }
    dirty = false;
    return true;
}

}  // namespace rah
