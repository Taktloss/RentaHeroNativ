// SPDX-License-Identifier: GPL-3.0-or-later
// Wegsuche (CPathFinder, 0xE8 Bytes) über das globale Raster g_PathGrid 0x0047E64C
// (docs/PATHFINDING.md). Datenstrukturen wörtlich nachgebildet: sortierte offene Liste mit
// linearem Einfügen, geschlossene Liste als Stapel, Knoten-Pool ab Index 1.
#pragma once

#include <array>
#include <cstdint>
#include <vector>

#include "formats/zbf.hpp"

namespace rah {

// g_PathGrid: 160 × 90 Zellen (4 × 4 Pixel), Index (x/4) · 90 + (y/4)
struct PathGrid {
    static constexpr int kCellsX = 160, kCellsY = 90;
    std::array<std::uint8_t, kCellsX * kCellsY> cells{};
    // Port: Zugriffe außerhalb lesen im Original fremden Speicher – hier gesperrt (UNVERIFIED)
    std::uint8_t at(int cx, int cy) const {
        const int i = cx * kCellsY + cy;
        return i >= 0 && i < kCellsX * kCellsY ? cells[static_cast<std::size_t>(i)] : 0x80;
    }
    std::uint8_t* ptr(int cx, int cy) {
        const int i = cx * kCellsY + cy;
        return i >= 0 && i < kCellsX * kCellsY ? &cells[static_cast<std::size_t>(i)] : nullptr;
    }
};

// orig: Rent-A-Hero.exe 0x00445020 PathCellFree(x, y) – Pixelkoordinaten, ganzes Byte == 0
bool pathCellFree(const PathGrid& grid, int x, int y);
// orig: Rent-A-Hero.exe 0x00445060 PathBlockRect / 0x004450D0 PathUnblockRect (Bit 0x40)
void pathBlockRect(PathGrid& grid, int x1, int y1, int x2, int y2);
void pathUnblockRect(PathGrid& grid, int x1, int y1, int x2, int y2);

class PathFinder {
public:
    static constexpr int kPool = 72000;  // +0xCC / +0xD0

    // orig: Rent-A-Hero.exe 0x00444C60 CPathFinder_ctor(zmap) – baut das globale Raster neu
    PathFinder(PathGrid& grid, const ZMap& map);

    // orig: Rent-A-Hero.exe 0x00444E20 CPathFinder_Find(sx, sy, tx, ty) – Pixel
    bool find(int sx, int sy, int tx, int ty);
    // orig: Rent-A-Hero.exe 0x00444FE0 CPathFinder_Next – nächster Wegpunkt, false bei (0, …)
    bool next(int& x, int& y);

private:
    struct Node {  // 12 Bytes: u8 x, u8 y, u16 parent, i32 g, i32 h
        std::uint8_t x = 0, y = 0;
        std::uint16_t parent = 0;
        std::int32_t g = 0, h = 0;
    };
    // orig: Rent-A-Hero.exe 0x00445140 CPathFinder_AStar(von, nach) – sucht vom Ziel zum Start
    bool aStar(int fx, int fy, int tx, int ty);
    // orig: Rent-A-Hero.exe 0x00445540 CPathFinder_BuildWaypoints
    void buildWaypoints();
    // orig: Rent-A-Hero.exe 0x004456B0 CPathFinder_NearestFree (Zellen)
    bool nearestFree(int& cx, int& cy) const;
    // orig: Rent-A-Hero.exe 0x004457C0 ZMap_BlockSet – alle Pixel des Blocks ≠ 0
    bool blockSet(int x, int y, int w, int h) const;
    // orig: Rent-A-Hero.exe 0x00445820 PathLineFree (Zellen, Endzelle nicht geprüft)
    bool lineFree(int x0, int y0, int x1, int y1) const;
    // orig: Rent-A-Hero.exe 0x004458C0 / 0x00445970 / 0x004459B0 / 0x00445A10 / 0x00445A80 /
    // 0x00445B40 / 0x00445B70 / 0x00445BE0
    std::uint16_t newNode(int x, int y, int g, int h, std::uint16_t parent);
    void updateNode(std::uint16_t n, int g, int h, std::uint16_t parent);
    std::uint16_t popOpen();
    void removeClosed(std::uint16_t n);
    void pushOpen(std::uint16_t n);
    void pushClosed(std::uint16_t n);
    std::uint16_t findOpen(int x, int y) const;
    std::uint16_t findClosed(int x, int y) const;

    PathGrid& grid_;
    const ZMap& map_;          // +0x00 Daten, +0x04 Breite, +0x08 Höhe
    int cellsX_ = 0, cellsY_ = 0;
    int last_ = 0;             // +0x0C zuletzt entnommener Knoten
    int sx_ = 0, sy_ = 0, tx_ = 0, ty_ = 0;  // +0x10…+0x1C Pixel
    // +0x20: 20 Paare; das Original schreibt ohne Grenze weiter (überschreibt +0xC0 ff.) –
    // Port: wachsende Liste (Abweichung, UNVERIFIED ob im Spiel je > 19 Wegpunkte)
    std::vector<std::array<int, 2>> waypoints_ = std::vector<std::array<int, 2>>(20, {0, 0});
    int read_ = 0;             // +0xC0
    std::vector<Node> pool_;   // +0xC4
    std::vector<std::uint16_t> list_;  // +0xC8
    int count_ = 1;            // +0xD4
    int openBegin_ = 1, openEnd_ = 1;  // +0xD8, +0xDC
    int closedTop_ = kPool - 1, closedEnd_ = kPool - 1;  // +0xE0, +0xE4
};

}  // namespace rah
