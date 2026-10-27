// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/layer.hpp"

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
    isVideo = kind == LayerKind::Avi;

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
        tcc.planes = std::move(img.planes());
        tcc.afterDecode();
        width = tcc.width();
        height = tcc.height();
        alpha = tcc.hasAlpha();
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

void Layer::setPos(int nx, int ny) {
    x = nx;
    y = ny;
    if (active) dirty = true;
    // UNVERIFIED/offen: Nachführen des verknüpften Bereichs (+0x1C0, 0x004360A0) folgt mit CArea
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

    if (!alpha) {
        // DirectDraw-Oberfläche der Ebene
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
        if (!tcc.draw(s, r, flag, zBuffered ? zmap : nullptr, ftol(z), ftol(zoom))) return false;
    }
    dirty = false;
    return true;
}

}  // namespace rah
