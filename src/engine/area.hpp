// SPDX-License-Identifier: GPL-3.0-or-later
// Klickbereich (CArea, 0xBC Bytes, Ctor 0x00435F80) und globales Treffer-Raster
// 0x0047D77C (docs/AREAS.md).
#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>

#include "core/rect.hpp"
#include "engine/event.hpp"

namespace rah {

class Layer;

// 8×8-Pixel-Zellen, 80 je Zeile
struct HitGrid {
    static constexpr int kMarkLimit = 0xE74;  // Grenze beim Markieren (0x004360A0)
    static constexpr int kClearSize = 0xE10;  // gelöschte Bytes (0x00436150)
    std::array<std::uint8_t, kMarkLimit> cells{};

    static int index(int x, int y) { return x / 8 + (y / 8) * 80; }  // Abschneiden zur 0
    // orig: Rent-A-Hero.exe 0x00436150
    void clear() { std::fill(cells.begin(), cells.begin() + kClearSize, std::uint8_t{0}); }
    // orig: Rent-A-Hero.exe 0x004364C0 (ohne Bereichsprüfung; y < 360 durch HitTest)
    bool test(int x, int y) const {
        const int i = index(x, y);
        return i >= 0 && i < kMarkLimit && cells[static_cast<std::size_t>(i)] != 0;  // UNVERIFIED: außerhalb
    }
};

struct Area {
    Rect rect;                   // +0x00
    std::string name;            // +0x10
    std::string text;            // +0x42
    int cursor = 10;             // +0xA8
    Layer* mask = nullptr;       // +0xAC
    std::unique_ptr<Event> event = std::make_unique<Event>();  // +0xB0
    int rectParam = 0;           // +0xB4
    bool active = false;         // +0xB8
    HitGrid* grid = nullptr;

    // orig: Rent-A-Hero.exe 0x00436190 – Länge > 50 → bei 49 abgeschnitten
    void setName(std::string n) {
        if (n.size() > 50) n.resize(49);
        name = std::move(n);
    }
    // orig: Rent-A-Hero.exe 0x004361F0 CArea_SetText – Länge > 100 → bei 99 abgeschnitten
    void setText(std::string t) {
        if (t.size() > 100) t.resize(99);
        text = std::move(t);
    }
    // orig: Rent-A-Hero.exe 0x004361C0
    void setCursor(int c) { cursor = c == 0 ? 10 : c; }
    // orig: Rent-A-Hero.exe 0x004363C0
    void setRect(const Rect& r, int param) {
        rect = r;
        markGrid(r);
        rectParam = param;
    }
    // orig: Rent-A-Hero.exe 0x004360A0
    void markGrid(const Rect& r) const;
    // orig: Rent-A-Hero.exe 0x00436440 CArea_SetEvent
    void setEvent(int id) {
        if (id == 0) return;
        event->setHandler(id);
        event->enabled = true;
        active = true;
    }
    // orig: Rent-A-Hero.exe 0x00436060 – Ebene als Maske anhängen
    void attach(Layer& layer);
};

}  // namespace rah
