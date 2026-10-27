// SPDX-License-Identifier: GPL-3.0-or-later
// Dirty-Rechteckliste der Szene. Das Original (CRectList, 0x16C Bytes) hat einen Listenmodus
// mit 20 Einträgen, benutzt für die Szenenlisten aber nur Modus 0 = ein Vereinigungsrechteck
// (docs/COMPOSITOR.md). Nur dieser Modus ist nachgebildet.
#pragma once

#include <optional>

#include "core/rect.hpp"

namespace rah {

class DirtyRegion {
public:
    // orig: Rent-A-Hero.exe 0x00445D20 CRectList_Add – begrenzt `r` im Speicher des Aufrufers
    // (left/top ≥ 0, right ≤ 640, bottom ≤ 480) und vereinigt.
    void add(Rect& r) {
        if (r.left < 0) r.left = 0;
        if (r.top < 0) r.top = 0;
        if (r.right > 640) r.right = 640;
        if (r.bottom > 480) r.bottom = 480;
        unionRect(u_, u_, r);
    }
    // Add(NULL): ganze Szene (0, 0, 640, 360)
    void addAll() {
        Rect r{0, 0, 640, 360};
        add(r);
    }
    // orig: 0x00445EA0 CRectList_First (Modus 0: nur das Vereinigungsrechteck, wenn nicht leer)
    std::optional<Rect> first() const {
        if (isRectEmpty(u_)) return std::nullopt;
        return u_;
    }
    // orig: 0x00445C90 CRectList_Intersects
    bool intersects(const Rect& r) const {
        Rect t;
        return intersectRect(t, u_, r);
    }
    // orig: 0x00445C70 CRectList_Clear
    void clear() { u_ = {}; }
    const Rect& rect() const { return u_; }

private:
    Rect u_;
};

}  // namespace rah
