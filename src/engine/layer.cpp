// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/layer.hpp"

#include "engine/area.hpp"

#include <cmath>

namespace rah {

namespace {

// __ftol: Abschneiden Richtung 0
inline int ftol(double v) {
    return static_cast<int>(std::trunc(v));
}

}  // namespace

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
        still = std::move(img.planes());
        tcc.afterDecode(still);
        width = static_cast<int>(still.width);
        height = static_cast<int>(still.height);
        alpha = still.hasAlpha();
        hasImage = true;
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

bool Layer::update(EngineGlobals&) {
    if (tweenSteps == 0) return true;
    // Noch nicht portiert: Interpolation von X/Y/Z/Zoom/Farbe und Ereignis Typ 2 (0x00447890);
    // folgt mit den Setzern der Zielwerte (+0x228…, +0x278…), die +0x50 setzen.
    return true;
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
    if (zoom != 100.0) {
        zoomed = full;
        const int zi = ftol(zoom);
        zoomed.right = full.right * zi / 100;
        zoomed.bottom = full.bottom * zi / 100;
    }
    refreshArea();  // 0x00447060 aktualisiert den Bereich auch bei Zoom 100
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
            return false;  // UNVERIFIED: gestrecktes Blt (Verfahren des Treibers) noch nicht nachgebildet
        }
        // BltFast(r.left + src.left, r.top + src.top, src), mit Farbschlüssel bei flag != 0.
        // UNVERIFIED: BltFast ohne Clipper lehnt Ziele außerhalb der Oberfläche ab.
        const int dx = r.left + src.left, dy = r.top + src.top;
        if (src.left < 0 || src.top < 0 || src.right > width || src.bottom > height || dx < 0 || dy < 0 ||
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
