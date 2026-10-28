// SPDX-License-Identifier: GPL-3.0-or-later
#include <climits>
#include <vector>

#include "engine/actor.hpp"
#include "engine/pathfinder.hpp"
#include "test.hpp"

using namespace rah;

namespace {

class OneFs final : public FileSystem {
public:
    Bytes data;
    bool exists(std::string_view) const override { return true; }
    std::optional<Bytes> read(std::string_view) const override { return data; }
    std::vector<std::string> list(std::string_view) const override { return {}; }
};

void put32(Bytes& b, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) b.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
}

// n Schlüsselbilder 2×2 mit Alpha
Bytes a16(int n) {
    Bytes f;
    for (int i = 0; i < n; ++i) {
        put32(f, 0);
        for (int c = 0; c < 4; ++c) put32(f, 3);
        put32(f, 2);
        put32(f, 2);
        for (std::uint8_t v : {static_cast<std::uint8_t>(i), std::uint8_t{0}, std::uint8_t{0}, std::uint8_t{255}}) {
            f.push_back(1);
            f.push_back(4);
            f.push_back(v);
        }
    }
    return f;
}

ZMap openMap(std::uint8_t v = 100) {
    ZMap m;
    m.data.assign(static_cast<std::size_t>(ZMap::kWidth) * ZMap::kHeight, v);
    return m;
}

void wall(ZMap& m, int x0, int x1, int y0, int y1) {
    for (int y = y0; y <= y1; ++y)
        for (int x = x0; x <= x1; ++x) m.data[static_cast<std::size_t>(y) * ZMap::kWidth + static_cast<std::size_t>(x)] = 0;
}

std::vector<std::array<int, 2>> drain(PathFinder& p) {
    std::vector<std::array<int, 2>> out;
    int x = 0, y = 0;
    while (p.next(x, y)) out.push_back({x, y});
    return out;
}

}  // namespace

TEST(path_grid_borders_and_blocks) {
    PathGrid grid;
    ZMap m = openMap();
    wall(m, 100, 100, 100, 100);  // ein Pixel 0 sperrt die ganze Zelle (25, 25)
    PathFinder p(grid, m);
    CHECK(grid.at(0, 10) == 0x80 && grid.at(159, 10) == 0x80 && grid.at(10, 0) == 0x80 && grid.at(10, 89) == 0x80);
    CHECK(grid.at(25, 25) == 0x80 && grid.at(26, 25) == 0);
    CHECK(pathCellFree(grid, 107, 100) && !pathCellFree(grid, 103, 101));
    pathBlockRect(grid, 200, 200, 207, 203);  // Zellen 50…51 × 50
    CHECK(grid.at(50, 50) == 0x40 && grid.at(51, 50) == 0x40 && grid.at(52, 50) == 0);
    pathUnblockRect(grid, 200, 200, 207, 203);
    CHECK(grid.at(50, 50) == 0 && grid.at(51, 50) == 0);
}

TEST(path_straight_and_around_wall) {
    PathGrid grid;
    ZMap m = openMap();
    PathFinder p(grid, m);
    REQUIRE(p.find(100, 100, 500, 100));
    auto w = drain(p);
    // freie Sicht: Start, genaues Ziel
    REQUIRE(w.size() == 2);
    CHECK(w[0] == (std::array<int, 2>{100, 100}) && w[1] == (std::array<int, 2>{500, 100}));

    ZMap m2 = openMap();
    wall(m2, 280, 291, 0, 300);  // Wand mit Lücke unten
    PathFinder q(grid, m2);
    REQUIRE(q.find(100, 100, 500, 100));
    w = drain(q);
    REQUIRE(w.size() >= 3);
    CHECK(w.front() == (std::array<int, 2>{100, 100}) && w.back() == (std::array<int, 2>{500, 100}));
    bool below = false;
    for (const auto& pt : w) below = below || pt[1] > 300;  // Umweg unter der Wand hindurch
    CHECK(below);
    for (std::uint8_t c : grid.cells) CHECK((c & 1) == 0);  // Besucht-Bits gelöscht
}

TEST(path_nearest_free_west_quirk) {
    PathGrid grid;
    ZMap m = openMap();
    PathFinder p(grid, m);
    // Startzelle (50, 50) und alle Nachbarn gesperrt außer West (49, 50); geprüft wird (48, 50)
    pathBlockRect(grid, 192, 192, 207, 207);  // Zellen 48…51 × 48…51
    pathUnblockRect(grid, 196, 200, 199, 203);  // (49, 50) frei
    CHECK(pathCellFree(grid, 196, 200));
    CHECK(!p.find(200, 200, 500, 300));  // Original-Fehler: (−2, 0) statt (−1, 0)
}

TEST(actor_face_towards_codes) {
    EngineGlobals g;
    Actor a(g);
    a.faceTowards(100, 100, 200, 105);  // fast waagerecht rechts
    CHECK(a.dir == 6 && a.walkMode == 1);
    a.faceTowards(100, 100, 0, 100);
    CHECK(a.dir == 2 && a.walkMode == 1);
    a.faceTowards(100, 100, 102, 200);  // senkrecht unten
    CHECK(a.dir == 0 && a.walkMode == 2);
    a.faceTowards(100, 100, 100, 0);
    CHECK(a.dir == 4 && a.walkMode == 2);
    a.faceTowards(100, 100, 150, 150);  // diagonal rechts unten
    CHECK(a.dir == 7 && a.walkMode == 3);
    a.faceTowards(100, 100, 50, 40);
    CHECK(a.dir == 3 && a.walkMode == 3);
    a.dir = 5;
    a.faceTowards(100, 100, 100, 100);  // kein Code → unverändert
    CHECK(a.dir == 5);
}

TEST(actor_type_tables) {
    EngineGlobals g;
    Actor a(g);
    CHECK(a.headHeight == 0 && !a.head && a.standFrame[0] == 0x62 && a.running && a.loop);
    a.setType(1);
    CHECK(a.footOffX == 108 && a.footOffY == 318 && a.stepX[0] == 15 && a.stepY[0] == 5);
    CHECK(a.stepX[36] == 18 && a.stepX[84] == 18 && a.stepY[45] == 2 && a.headHeight == 100 && a.type == 1);
    a.setType(12);
    CHECK(a.footOffX == 42 && a.standFrame[0] == 13 && a.standFrame[7] == 1 && a.headHeight == 100);
    a.setType(2);
    CHECK(a.footOffX == 55 && a.stepX[36] == 9 && a.headHeight == 50);
}

TEST(actor_position_zoom_and_tint) {
    EngineGlobals g;
    Actor a(g);
    a.setType(2);  // Fuß 55/162
    ZMap m = openMap(0);
    wall(m, 0, 0, 0, 0);
    for (int x = 0; x < 640; ++x) m.data[200 * 640 + static_cast<std::size_t>(x)] = 50;  // Zeile 200: Wert 50
    m.sMax = 0.0f;
    a.pathMap = &m;
    a.zpos = 100;
    a.zoomVal = 100;
    a.setPosition(300, 200, false);
    CHECK(a.z == 50.0 && a.zoom == 200.0);  // Zpos / v · ZoomVal
    CHECK(a.footX == 300 && a.footY == 200 && a.x == 300 - 110 && a.y == 200 - 324);
    a.setPosition(10, 10, false);  // Wegkarte 0 → bleibt stehen
    CHECK(a.footX == 300 && a.footY == 200);
    a.setPosition(10, 10, true);  // force: alter Zoom, Position trotzdem
    CHECK(a.footX == 10 && a.footY == 10 && a.zoom == 200.0);
    a.setPosition(-5, 900, true);  // Kartenzugriff begrenzt, Position unbegrenzt
    CHECK(a.footX == -5 && a.footY == 900);
}

TEST(actor_walk_straight_fires_done_event) {
    OneFs fs;
    fs.data = a16(105);
    EngineGlobals g;
    std::uint32_t clock = 1000;
    g.now = [&] { return clock; };
    std::vector<std::pair<int, int>> fired;
    g.eventProc = [&](Event& e, int t) { fired.emplace_back(e.param, t); };
    Actor a(g);
    std::string err;
    REQUIRE(a.load(FoundFile{&fs, "R.a16"}, 0, &err));
    a.setType(2);
    ZMap m = openMap();
    a.setPathMap(&m);
    a.zpos = 100;  // Wegkarte 100, sMax 0 → Zoom = Zpos / v · ZoomVal = 100
    a.zoomVal = 100;
    a.setPos(100, 200);
    a.setDirection(6);
    a.walkTo(160, 200, 7, -1);
    CHECK(a.active && a.targetX == 160 && a.targetY == 200 && !a.walkDone);
    int steps = 0;
    while (a.targetX != 0 && steps < 100) {
        clock += 100;
        a.update(g);
        if (a.walkEvent) fireEvent(g, *a.walkEvent, a.walkEvent->type);
        ++steps;
    }
    CHECK(a.footX == 160 && a.footY == 200 && a.walkDone);
    CHECK(steps > 3 && steps < 20);  // 9 bzw. 10 Pixel je Bild
    CHECK(fired.size() == 1 && fired[0] == (std::pair<int, int>{7, 0x29}));  // (Ereignis-ID, Typ)
    // erneuter Lauf ohne Ereignis bricht nichts ab
    a.walkToClick(160, 200);
    CHECK(fired.size() == 1);
}

TEST(actor_walk_cancel_moves_event) {
    OneFs fs;
    fs.data = a16(105);
    EngineGlobals g;
    std::vector<std::pair<int, int>> fired;
    g.eventProc = [&](Event& e, int t) { fired.emplace_back(e.param, t); };
    Actor a(g);
    std::string err;
    REQUIRE(a.load(FoundFile{&fs, "R.a16"}, 0, &err));
    a.setType(2);
    ZMap m = openMap();
    a.setPathMap(&m);
    a.setPos(100, 200);
    a.walkTo(300, 200, 7, -1);
    a.walkToClick(50, 200);  // Klick bricht die Aktion ab: Typ 0x2A, Ereignis nach +0x16F0
    CHECK(!a.walkEvent && a.oldWalkEvent && a.oldWalkEvent->pending && a.oldWalkEvent->type == 0x2A);
    fireEvent(g, *a.oldWalkEvent, a.oldWalkEvent->type);
    CHECK(fired.size() == 1 && fired[0].second == 0x2A);
}

TEST(actor_walk_without_zpos_divides_by_zero) {
    // Zpos 0: Ziel direkt, ohne FaceTowards → Laufmodus 0 (senkrecht); rein waagerechter Weg
    // teilt durch 0, __ftol(∞) = 0x80000000 (Original-Verhalten)
    OneFs fs;
    fs.data = a16(105);
    EngineGlobals g;
    std::uint32_t clock = 1000;
    g.now = [&] { return clock; };
    Actor a(g);
    std::string err;
    REQUIRE(a.load(FoundFile{&fs, "R.a16"}, 0, &err));
    a.setType(2);
    ZMap m = openMap();
    a.setPathMap(&m);
    a.setPos(100, 200);
    a.walkTo(160, 200, 0, -1);
    CHECK(a.walkMode == 0 && a.targetX == 160);
    clock += 100;
    a.update(g);
    CHECK(a.footX == 100 + INT_MIN && a.footY == 200);
}
