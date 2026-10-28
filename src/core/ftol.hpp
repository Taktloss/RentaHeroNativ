// SPDX-License-Identifier: GPL-3.0-or-later
// __ftol (MSVC-Laufzeit 0x0044F08C): double → int mit Abschneiden Richtung 0
#pragma once

#include <climits>
#include <cmath>

namespace rah {

// Außerhalb des int-Bereichs und NaN: „integer indefinite“ 0x80000000 (FISTP)
inline int ftol(double v) {
    const double t = std::trunc(v);
    if (!(t >= -2147483648.0 && t <= 2147483647.0)) return INT_MIN;
    return static_cast<int>(t);
}

// Betrag wie CDQ/XOR/SUB: abs(INT_MIN) bleibt INT_MIN
inline int iabs(int v) {
    return v < 0 ? static_cast<int>(0u - static_cast<unsigned>(v)) : v;
}

}  // namespace rah
