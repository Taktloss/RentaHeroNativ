// SPDX-License-Identifier: GPL-3.0-or-later
// RECT mit Win32-Semantik (IsRectEmpty, IntersectRect, UnionRect), wie sie das Original nutzt.
#pragma once

namespace rah {

struct Rect {
    int left = 0, top = 0, right = 0, bottom = 0;
    int width() const { return right - left; }
    int height() const { return bottom - top; }
    bool operator==(const Rect&) const = default;
};

inline bool isRectEmpty(const Rect& r) {
    return r.right <= r.left || r.bottom <= r.top;
}

// IntersectRect: leerer Schnitt → out = (0,0,0,0), Rückgabe false
inline bool intersectRect(Rect& out, const Rect& a, const Rect& b) {
    Rect r{a.left > b.left ? a.left : b.left, a.top > b.top ? a.top : b.top,
           a.right < b.right ? a.right : b.right, a.bottom < b.bottom ? a.bottom : b.bottom};
    if (isRectEmpty(r)) {
        out = {};
        return false;
    }
    out = r;
    return true;
}

// UnionRect: leere Rechtecke werden ignoriert; beide leer → (0,0,0,0), false
inline bool unionRect(Rect& out, const Rect& a, const Rect& b) {
    const bool ea = isRectEmpty(a), eb = isRectEmpty(b);
    if (ea && eb) {
        out = {};
        return false;
    }
    if (ea) {
        out = b;
        return true;
    }
    if (eb) {
        out = a;
        return true;
    }
    out = {a.left < b.left ? a.left : b.left, a.top < b.top ? a.top : b.top,
           a.right > b.right ? a.right : b.right, a.bottom > b.bottom ? a.bottom : b.bottom};
    return true;
}

// orig: Rent-A-Hero.exe 0x004481E0 ClipToScene – false, wenn außerhalb; schneidet in jedem Fall
// mit (0, 0, 640, 360).
// PtInRect: left ≤ x < right, top ≤ y < bottom
inline bool ptInRect(const Rect& r, int x, int y) {
    return x >= r.left && x < r.right && y >= r.top && y < r.bottom;
}

inline bool clipToScene(Rect& r) {
    bool visible = !(r.right < 1 || r.bottom < 1 || r.top > 359 || r.left > 639);
    intersectRect(r, r, Rect{0, 0, 640, 360});
    return visible;
}

}  // namespace rah
