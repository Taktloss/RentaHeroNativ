// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/area.hpp"

#include <algorithm>

#include "engine/layer.hpp"

namespace rah {

void Area::markGrid(const Rect& r) const {
    if (!grid) return;
    Rect c = r;
    if (!clipToScene(c)) return;
    if (name.compare(0, 7, "Rodrigo") == 0) return;  // strncmp(name, "Rodrigo", 7)
    for (int y = c.top; y < c.bottom; y += 8) {
        for (int x = c.left; x < c.right; x += 8) {
            const int i = HitGrid::index(x, y);
            if (static_cast<unsigned>(i) < static_cast<unsigned>(HitGrid::kMarkLimit)) {
                grid->cells[static_cast<std::size_t>(i)] = 1;
            }
        }
    }
}

void Area::attach(Layer& layer) {
    if (mask) mask->area = nullptr;  // 0x00436170
    mask = &layer;
    layer.area = this;
    rect = layer.getRect();
}

}  // namespace rah
