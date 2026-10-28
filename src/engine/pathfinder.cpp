// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/pathfinder.hpp"

#include <cmath>
#include <cstdlib>

#include "core/ftol.hpp"

namespace rah {

namespace {

constexpr int kCellsY = PathGrid::kCellsY;

// orig: Rent-A-Hero.exe 0x004454F0 PathHeuristic – ftol(sqrt(dx² + dy²) · 1000.0)
int heuristic(int x0, int y0, int x1, int y1) {
    const double dx = x1 - x0, dy = y1 - y0;
    return ftol(std::sqrt(dy * dy + dx * dx) * 1000.0);
}

}  // namespace

bool pathCellFree(const PathGrid& grid, int x, int y) {
    return grid.at(x / 4, y / 4) == 0;
}

void pathBlockRect(PathGrid& grid, int x1, int y1, int x2, int y2) {
    for (int cy = y1 / 4; cy <= y2 / 4; ++cy)
        for (int cx = x1 / 4; cx <= x2 / 4; ++cx)
            if (auto* p = grid.ptr(cx, cy)) *p |= 0x40;
}

void pathUnblockRect(PathGrid& grid, int x1, int y1, int x2, int y2) {
    for (int cy = y1 / 4; cy <= y2 / 4; ++cy)
        for (int cx = x1 / 4; cx <= x2 / 4; ++cx)
            if (auto* p = grid.ptr(cx, cy)) *p &= 0xBF;
}

PathFinder::PathFinder(PathGrid& grid, const ZMap& map)
    : grid_(grid), map_(map), pool_(kPool), list_(kPool) {
    // Knoten 0 wird nie beschrieben; malloc-Inhalt laut PATHFINDING.md 0xCD (UNVERIFIED)
    pool_[0].g = static_cast<std::int32_t>(0xCDCDCDCDu);
    cellsX_ = ZMap::kWidth / 4;
    cellsY_ = ZMap::kHeight / 4;
    grid_.cells.fill(0);
    for (int cy = 0; cy < cellsY_; ++cy)
        for (int cx = 0; cx < cellsX_; ++cx)
            if (!blockSet(cx * 4, cy * 4, 4, 4)) *grid_.ptr(cx, cy) = 0x80;
    for (int cx = 0; cx < cellsX_; ++cx) {
        *grid_.ptr(cx, 0) = 0x80;
        *grid_.ptr(cx, cellsY_ - 1) = 0x80;
    }
    for (int cy = 0; cy < cellsY_; ++cy) {
        *grid_.ptr(0, cy) = 0x80;
        *grid_.ptr(cellsX_ - 1, cy) = 0x80;
    }
}

bool PathFinder::blockSet(int x, int y, int w, int h) const {
    for (int yy = y; yy < y + h; ++yy)
        for (int xx = x; xx < x + w; ++xx)
            if (map_.at(xx, yy) == 0) return false;  // 0x004457A0 ZMap_PixelSet
    return true;
}

bool PathFinder::find(int sx, int sy, int tx, int ty) {
    last_ = 0;
    waypoints_.assign(20, {0, 0});
    read_ = 0;
    sx_ = sx;
    sy_ = sy;
    tx_ = tx;
    ty_ = ty;
    int csx = sx / 4, csy = sy / 4, ctx = tx / 4, cty = ty / 4;
    for (int cy = 0; cy < cellsY_; ++cy)
        for (int cx = 0; cx < cellsX_; ++cx) *grid_.ptr(cx, cy) &= 0xFE;
    if (grid_.at(csx, csy) != 0 && !nearestFree(csx, csy)) return false;
    if (grid_.at(ctx, cty) != 0 && !nearestFree(ctx, cty)) return false;
    if (csx == ctx && csy == cty) {
        waypoints_[0] = {sx_, sy_};
        waypoints_[1] = {tx_, ty_};
        waypoints_[2] = {0, 0};
        return true;
    }
    if (!aStar(ctx, cty, csx, csy)) return false;
    buildWaypoints();
    return true;
}

bool PathFinder::next(int& x, int& y) {
    if (read_ >= static_cast<int>(waypoints_.size())) waypoints_.resize(static_cast<std::size_t>(read_) + 1, {0, 0});
    x = waypoints_[static_cast<std::size_t>(read_)][0];
    y = waypoints_[static_cast<std::size_t>(read_)][1];
    ++read_;
    return x != 0;
}

bool PathFinder::nearestFree(int& cx, int& cy) const {
    // Westnachbar (−2, 0): Original-Fehler, am Disassembly 0x00445703 belegt
    static constexpr int kDx[8] = {0, 1, 1, 1, 0, -1, -2, -1};
    static constexpr int kDy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
    for (int i = 0; i < 8; ++i) {
        const int x = kDx[i] + cx, y = kDy[i] + cy;
        if (0 < x && x < cellsX_ - 1 && 0 < y && y < cellsY_ - 1 && grid_.at(x, y) == 0) {
            cx = x;
            cy = y;
            return true;
        }
    }
    return false;
}

bool PathFinder::lineFree(int x0, int y0, int x1, int y1) const {
    const int dx = x1 - x0, dy = y1 - y0;
    int n = std::abs(dy);
    if (n < std::abs(dx)) n = std::abs(dx);
    if (n == 0) return true;
    int fx = x0 << 10, fy = y0 << 10;
    for (int i = 1; i <= n; ++i) {
        if (grid_.at(fx >> 10, fy >> 10) != 0) return false;  // SAR: Abrunden
        fx += (dx * 0x400) / n;
        fy += (dy * 0x400) / n;
    }
    return true;
}

std::uint16_t PathFinder::newNode(int x, int y, int g, int h, std::uint16_t parent) {
    const int n = count_;
    if (n >= kPool) return 0;  // UNVERIFIED: Original schreibt vorher noch in den Pool
    Node& k = pool_[static_cast<std::size_t>(n)];
    k.x = static_cast<std::uint8_t>(x);
    k.y = static_cast<std::uint8_t>(y);
    k.parent = parent;
    k.g = g;
    k.h = h;
    ++count_;
    return count_ >= kPool ? 0 : static_cast<std::uint16_t>(n);
}

void PathFinder::updateNode(std::uint16_t n, int g, int h, std::uint16_t parent) {
    Node& k = pool_[n];
    k.parent = parent;
    k.g = g;
    k.h = h;
}

std::uint16_t PathFinder::popOpen() {
    const std::uint16_t n = list_[static_cast<std::size_t>(openBegin_)];
    for (int i = openBegin_; i + 1 < openEnd_; ++i) list_[static_cast<std::size_t>(i)] = list_[static_cast<std::size_t>(i) + 1];
    --openEnd_;
    return n;
}

void PathFinder::removeClosed(std::uint16_t n) {
    // Behandelt den Knotenindex wie eine Listenposition (Original). Nie erreicht, solange
    // Knoten 0 den Wert 0xCDCDCDCD hat (PATHFINDING.md).
    const int e = closedEnd_;
    if (static_cast<unsigned>(n - e) >= 2) {
        const int cnt = n - (e + 1);
        for (int i = cnt - 1; i >= 0; --i) {
            const std::size_t dst = static_cast<std::size_t>(e + 2 + i), src = static_cast<std::size_t>(e + 1 + i);
            if (dst < list_.size() && src < list_.size()) list_[dst] = list_[src];
        }
    }
    ++closedEnd_;
}

void PathFinder::pushOpen(std::uint16_t n) {
    const Node& k = pool_[n];
    int pos = openBegin_;
    while (pos < openEnd_) {
        const Node& o = pool_[list_[static_cast<std::size_t>(pos)]];
        if (k.h + k.g <= o.h + o.g) break;  // gleiches f: neuer Knoten zuerst
        ++pos;
    }
    for (int i = openEnd_; i > pos; --i) list_[static_cast<std::size_t>(i)] = list_[static_cast<std::size_t>(i) - 1];
    list_[static_cast<std::size_t>(pos)] = n;
    ++openEnd_;
}

void PathFinder::pushClosed(std::uint16_t n) {
    list_[static_cast<std::size_t>(closedEnd_)] = n;
    --closedEnd_;
}

std::uint16_t PathFinder::findOpen(int x, int y) const {
    for (int i = openBegin_; i < openEnd_; ++i) {
        const std::uint16_t n = list_[static_cast<std::size_t>(i)];
        if (pool_[n].x == x && pool_[n].y == y) return n;
    }
    return 0;
}

std::uint16_t PathFinder::findClosed(int x, int y) const {
    for (int i = closedTop_; closedEnd_ < i; --i) {
        const std::uint16_t n = list_[static_cast<std::size_t>(i)];
        if (pool_[n].x == x && pool_[n].y == y) return n;
    }
    return 0;
}

bool PathFinder::aStar(int fx, int fy, int tx, int ty) {
    static constexpr int kCost[8] = {1000, 1414, 1000, 1414, 1000, 1414, 1000, 1414};
    static constexpr int kDx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
    static constexpr int kDy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
    bool found = false;
    openEnd_ = openBegin_;
    closedEnd_ = closedTop_;
    count_ = 1;
    std::uint16_t fresh = newNode(fx, fy, 0, heuristic(fx, fy, tx, ty), 0);
    *grid_.ptr(fx, fy) |= 1;
    pushOpen(fresh);
    std::uint16_t cur = 0;
    while (openBegin_ < openEnd_) {
        cur = popOpen();
        const Node k = pool_[cur];
        if (k.x == tx && k.y == ty) {
            found = true;
            break;
        }
        for (int i = 0; i < 8; ++i) {
            const int nx = pool_[cur].x + kDx[i], ny = pool_[cur].y + kDy[i];
            std::uint8_t* cell = grid_.ptr(nx, ny);
            if (!cell || (*cell & 0xC0) != 0) continue;
            const int g = pool_[cur].g + kCost[i];
            const std::uint16_t o = findOpen(nx, ny);
            const std::uint16_t c = findClosed(nx, ny);
            // nur geschlossen: vergleicht mit Knoten o = 0 (Original-Fehler)
            if (!((o == 0 && c == 0) || g < pool_[o].g)) continue;
            if ((*cell & 1) == 0) {
                fresh = newNode(nx, ny, g, heuristic(nx, ny, tx, ty), cur);
                *cell |= 1;
            } else if (o != 0) {
                updateNode(o, g, heuristic(nx, ny, tx, ty), cur);  // ohne Neusortierung
            } else if (c != 0) {
                updateNode(c, g, heuristic(nx, ny, tx, ty), cur);
            } else {
                pushOpen(fresh);  // veralteter Knoten (Original; praktisch unerreichbar)
                continue;
            }
            if (c != 0) removeClosed(c);
            if (o != 0) continue;
            pushOpen(fresh);
        }
        pushClosed(cur);
    }
    last_ = cur;
    for (int cy = 0; cy < cellsY_; ++cy)
        for (int cx = 0; cx < cellsX_; ++cx) *grid_.ptr(cx, cy) &= 0xFE;
    return found;
}

void PathFinder::buildWaypoints() {
    auto put = [&](int i, int x, int y) {
        if (i >= static_cast<int>(waypoints_.size())) waypoints_.resize(static_cast<std::size_t>(i) + 1, {0, 0});
        waypoints_[static_cast<std::size_t>(i)] = {x, y};
    };
    read_ = 0;
    put(0, sx_, sy_);
    std::uint16_t n = pool_[static_cast<std::size_t>(last_)].parent;
    read_ = 1;
    int candX = sx_, candY = sy_;   // iVar2/iVar3
    int lastX = candX, lastY = candY;  // iVar7/local_14
    for (;;) {
        if (n == 0) {
            put(read_, 0, 0);
            read_ = 0;
            return;
        }
        const int ax = lastX / 4, ay = lastY / 4;  // Anker
        lastX = candX;
        lastY = candY;
        bool reachedEnd = false;
        for (;;) {
            const Node& k = pool_[n];
            candX = k.x * 4 + 2;
            candY = k.y * 4 + 2;
            if (!lineFree(ax, ay, candX / 4, candY / 4)) break;
            n = pool_[n].parent;
            if (n == 0) {
                reachedEnd = true;
                break;
            }
            lastX = candX;
            lastY = candY;
        }
        if (reachedEnd) put(read_, tx_, ty_);
        else put(read_, lastX, lastY);
        ++read_;
    }
}

}  // namespace rah
