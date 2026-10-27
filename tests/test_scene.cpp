// SPDX-License-Identifier: GPL-3.0-or-later
#include <fstream>
#include <tuple>
#include <map>

#include "core/bytes.hpp"
#include "core/strings.hpp"
#include "engine/scene.hpp"
#include "test.hpp"
#include "vfs/dirfs.hpp"

using namespace rah;

namespace {

class MemFs final : public FileSystem {
public:
    std::map<std::string, Bytes> files;  // Schlüssel groß, '/' als Trenner
    void put(const std::string& p, Bytes b) { files[toUpperAscii(p)] = std::move(b); }
    void put(const std::string& p, std::string_view s) { put(p, Bytes(s.begin(), s.end())); }
    bool exists(std::string_view p) const override { return files.count(toUpperAscii(std::string(p))) != 0; }
    std::optional<Bytes> read(std::string_view p) const override {
        auto it = files.find(toUpperAscii(std::string(p)));
        if (it == files.end()) return std::nullopt;
        return it->second;
    }
    std::vector<std::string> list(std::string_view) const override { return {}; }
};

void put32(Bytes& b, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) b.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
}

Bytes bmp24(int w, int h, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    const int stride = (w * 3 + 3) & ~3;
    Bytes f = {'B', 'M'};
    put32(f, static_cast<std::uint32_t>(54 + stride * h));
    put32(f, 0);
    put32(f, 54);
    put32(f, 40);
    put32(f, static_cast<std::uint32_t>(w));
    put32(f, static_cast<std::uint32_t>(h));
    f.push_back(1); f.push_back(0); f.push_back(24); f.push_back(0);
    for (int i = 0; i < 6; ++i) put32(f, 0);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) { f.push_back(b); f.push_back(g); f.push_back(r); }
        for (int p = w * 3; p < stride; ++p) f.push_back(0);
    }
    return f;
}

// Einfarbiges TCC (w·h ≤ 255): je Ebene ein Füll-Befehl
Bytes tccSolid(int w, int h, std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    Bytes f;
    const auto n = static_cast<std::uint8_t>(w * h);
    put32(f, 0);
    for (int i = 0; i < 4; ++i) put32(f, 3);
    put32(f, static_cast<std::uint32_t>(w));
    put32(f, static_cast<std::uint32_t>(h));
    for (std::uint8_t v : {r, g, b, a}) { f.push_back(1); f.push_back(n); f.push_back(v); }
    return f;
}

void writePpm(const Surface565& s, const std::filesystem::path& p) {
    std::ofstream o(p, std::ios::binary);
    o << "P6\n" << s.width << " " << s.height << "\n255\n";
    for (auto px : s.pixels) {
        std::uint8_t c[3];
        unpack565To888(px, c[0], c[1], c[2]);
        o.write(reinterpret_cast<const char*>(c), 3);
    }
}

struct Fixture {
    MemFs fs;
    DataPaths paths{&fs, nullptr};
    EngineGlobals g;
    Surface565 surf{640, 420};
    Scene scene{paths, g, surf};
    Fixture() {
        fs.put("GAME/MENU/BG.bmp", bmp24(640, 360, 0x40, 0x40, 0x40));
        fs.put("GAME/MENU/RED.TCC", tccSolid(4, 4, 0xF8, 0, 0, 255));
        fs.put("GAME/MENU/GREEN.TCC", tccSolid(4, 4, 0, 0xFC, 0, 255));
        fs.put("GAME/MENU/HALF.TCC", tccSolid(4, 4, 0, 0, 0xF8, 128));
        fs.put("GAME/MENU/T1.szn",
               "[GENERAL]\r\nBackground1=Bg\r\nLayer1=Red\r\nLayer2=Green\r\nLayer3=Half\r\n"
               "[Bg]\r\nFilename=BG.bmp\r\n"
               "[Red]\r\nFilename=RED.TCC\r\nX=10\r\nY=10\r\nZ=10\r\n"
               "[Green]\r\nFilename=GREEN.TCC\r\nX=12\r\nY=12\r\nZ=5\r\n"
               "[Half]\r\nFilename=HALF.TCC\r\nX=100\r\nY=100\r\nZ=255\r\n");
    }
    void enableAll() {
        scene.background()->enable();
        for (int i = 0; scene.layer(i); ++i) scene.layer(i)->enable();
    }
};

const std::uint16_t kBg = pack565(0x40, 0x40, 0x40);
const std::uint16_t kRed = pack565(0xF8, 0, 0);
const std::uint16_t kGreen = pack565(0, 0xFC, 0);

}  // namespace

TEST(scene_load_inactive_and_z) {
    Fixture f;
    REQUIRE(f.scene.load("T1", 0));
    REQUIRE(f.scene.layer(2) && !f.scene.layer(3));
    CHECK(!f.scene.layer(0)->active && !f.scene.background()->active);  // Schauplatz aktiviert
    CHECK(f.scene.zMax() == 255 && f.scene.zMin() == 5);
    CHECK(f.scene.zCount(10) == 1 && f.scene.zCount(5) == 1);
    CHECK(f.scene.getObject("green") == f.scene.layer(1));
    CHECK(f.scene.getObject("Bg") == nullptr);  // Hintergrund ist kein Objekt
}

TEST(scene_compose_order_and_restore) {
    Fixture f;
    REQUIRE(f.scene.load("T1", 0));
    f.enableAll();
    CHECK(f.scene.compose(false));
    CHECK(f.surf.row(10)[10] == kRed);
    CHECK(f.surf.row(13)[13] == kGreen);  // Z 5 vor Z 10
    CHECK(f.surf.row(200)[200] == kBg);
    CHECK(f.surf.row(101)[101] == pack565(0, 0, 0xF8));  // Z = 255: deckend trotz Alpha 128
    CHECK(f.scene.output().first() && *f.scene.output().first() == (Rect{0, 0, 640, 360}));
    f.scene.clearOutput();

    CHECK(!f.scene.compose(false));  // nichts geändert

    f.scene.layer(0)->setPos(30, 30);  // rot verschieben
    f.g.sceneChanged = true;            // setzt im Original CSzene_Update
    CHECK(f.scene.compose(false));
    CHECK(f.surf.row(10)[10] == kBg);      // alter Platz wiederhergestellt
    CHECK(f.surf.row(13)[13] == kGreen);   // überlappende Ebene neu gezeichnet
    CHECK(f.surf.row(31)[31] == kRed);
    const auto out = f.scene.output().first();
    REQUIRE(out.has_value());
    // altes Rechteck (+1 wegen DirtyAlpha) ∪ neues Alpha-Rechteck (rechts/unten einschließend,
    // also ohne letzte Spalte/Zeile – Original-Eigenheit) ∪ grün
    CHECK(*out == (Rect{10, 10, 33, 33}));
}

// Eine Originalszene laden, alles aktivieren und als Bild ablegen (RAH_DUMP_DIR).
TEST(scene_original_dump) {
    const auto dir = rahtest::dataDir();
    SKIP_IF(dir.empty());
    DirFileSystem cd(dir / "RentAHero");
    DirFileSystem common(dir / "wineprefix/drive_c/Program Files (x86)/neoSoftware/Rent-A-Hero/Common");
    DataPaths paths(&cd, &common);
    paths.setLocation(1);
    EngineGlobals g;
    Surface565 surf(640, 420);
    Scene scene(paths, g, surf);
    scene.setLocationNumber(1);
    REQUIRE(scene.load("7A", 0xFFFF));
    REQUIRE(scene.background());
    scene.background()->enable();
    int n = 0;
    for (int i = 0; scene.layer(i); ++i, ++n) scene.layer(i)->enable();
    CHECK(scene.compose(false));
    int areas = 0, texts = 0, hits = 0, free = 0;
    for (; areas < Scene::kMaxAreas && scene.area(areas); ++areas) {
        scene.area(areas)->setEvent(areas + 1);
        if (!scene.area(areas)->text.empty()) ++texts;
    }
    for (int i = 0; i < areas; ++i) {
        const Area* a = scene.area(i);
        if (a->mask) continue;
        ++free;
        const int cx = (a->rect.left + a->rect.right) / 2, cy = (a->rect.top + a->rect.bottom) / 2;
        const Area* h = scene.hitTest(cx, cy);
        if (h) ++hits;
        std::cout << "     Bereich " << a->name << " -> " << (h ? h->name : std::string("-")) << "\n";
    }
    CHECK(texts > 0);
    std::cout << "     LOC01/7A: " << areas << " Bereiche, " << texts << " mit Text, " << hits << "/" << free
              << " freie in der Mitte getroffen\n";
    std::cout << "     LOC01/7A: " << n << " Ebenen, " << scene.unsupportedLayers()
              << " nicht unterstützt (AVI)\n";
    if (const char* d = std::getenv("RAH_DUMP_DIR")) writePpm(surf, std::filesystem::path(d) / "loc01_7a.ppm");
}

namespace {

// A16 aus n Schlüsselbildern 2×2 mit Alpha
Bytes a16Solid(int n) {
    Bytes f;
    for (int i = 0; i < n; ++i) {
        put32(f, 0);
        for (int c = 0; c < 4; ++c) put32(f, 3);
        put32(f, 2);
        put32(f, 2);
        for (std::uint8_t v : {std::uint8_t{0xF8}, std::uint8_t{0}, std::uint8_t{0}, std::uint8_t{255}}) {
            f.push_back(1);
            f.push_back(4);
            f.push_back(v);
        }
    }
    return f;
}

}  // namespace

TEST(scene_update_stage_ticks) {
    Fixture f;
    f.fs.put("GAME/MENU/A.A16", a16Solid(8));
    f.fs.put("GAME/MENU/T2.szn",
             "[GENERAL]\r\nBackground1=Bg\r\nLayer1=S0\r\nLayer2=S1\r\nLayer3=S2\r\nLayer4=S3\r\n"
             "[Bg]\r\nFilename=BG.bmp\r\n"
             "[S0]\r\nFilename=A.A16\r\nfps=5\r\nX=0\r\n"
             "[S1]\r\nFilename=A.A16\r\nfps=010\r\nX=20\r\n"
             "[S2]\r\nFilename=A.A16\r\nfps=15\r\nX=40\r\n"
             "[S3]\r\nFilename=A.A16\r\nfps=20\r\nX=60\r\n");
    std::uint32_t clock = 0;
    f.g.now = [&] { return clock; };
    REQUIRE(f.scene.load("T2", 0));
    std::array<AnimLayer*, 4> a{};
    for (int i = 0; i < 4; ++i) {
        a[static_cast<std::size_t>(i)] = dynamic_cast<AnimLayer*>(f.scene.layer(i));
        REQUIRE(a[static_cast<std::size_t>(i)]);
        a[static_cast<std::size_t>(i)]->start(13);
    }
    f.scene.background()->enable();
    f.scene.compose(false);  // Start setzt dirty; ein Frame zeichnet es ab
    for (clock = 0; clock < 400; ++clock) {
        f.scene.update(true);
        f.scene.compose(false);
        f.scene.clearOutput();
    }
    // Erstes Fenster (Zähler aus dem Ctor = 0): jede Zwischenstufe tickt in zwei Frames
    // hintereinander (z. B. Stufe 3 bei 50 und 51 ms), Stufe 0 gar nicht → 0/2/3/4.
    // Zweites Fenster ab 200 ms regulär: 1/2/3/4.
    CHECK(a[0]->stepCount == 1);
    CHECK(a[1]->stepCount == 4);
    CHECK(a[2]->stepCount == 6);
    CHECK(a[3]->stepCount == 8);
}

TEST(scene_update_skips_after_gap) {
    Fixture f;
    std::uint32_t clock = 0;
    f.g.now = [&] { return clock; };
    REQUIRE(f.scene.load("T1", 0));
    clock = 1000;
    f.scene.update(true);  // weit hinter dem Fenster: kein Nachholen, Start = jetzt
    CHECK(f.scene.tickWindowStart() == 1000);
    clock = 1199;
    f.scene.update(true);
    CHECK(f.scene.tickWindowStart() == 1000);
    clock = 1250;
    f.scene.update(true);
    CHECK(f.scene.tickWindowStart() == 1250);  // start + 200 < now → start = now
}

TEST(scene_dispatch_hides_and_fires_end) {
    Fixture f;
    f.fs.put("GAME/MENU/A.A16", a16Solid(2));
    f.fs.put("GAME/MENU/T3.szn", "[GENERAL]\r\nLayer1=Anim\r\n[Anim]\r\nFilename=A.A16\r\nfps=10\r\n");
    std::uint32_t clock = 0;
    f.g.now = [&] { return clock; };
    std::vector<std::pair<int, int>> fired;
    f.g.eventProc = [&](Event& e, int t) { fired.emplace_back(e.param, t); };
    REQUIRE(f.scene.load("T3", 0));
    auto* a = dynamic_cast<AnimLayer*>(f.scene.layer(0));
    REQUIRE(a);
    a->playEndEvent(0, 42);  // Modus 0: am Ende ausblenden
    int frames = 0;
    for (clock = 0; clock < 2000 && fired.empty(); clock += 10, ++frames) {
        f.scene.update(true);
        f.scene.compose(false);
        f.scene.clearOutput();
        if (fired.empty()) CHECK(a->active);
        f.scene.dispatchEvents();
    }
    REQUIRE(fired.size() == 1);
    CHECK(fired[0] == std::make_pair(42, 1));
    CHECK(!a->active && a->dirty);  // ausgeblendet, Bereich wird im nächsten Frame restauriert
}

TEST(area_grid_mark_quirk) {
    HitGrid grid;
    Area a;
    a.grid = &grid;
    a.setRect(Rect{5, 0, 12, 4}, 0);
    CHECK(grid.test(0, 0));    // Zelle 0
    CHECK(!grid.test(8, 0));   // x = 13 erreicht right = 12 nicht → Zelle 1 bleibt leer
    a.name = "RodrigoSmall";
    a.markGrid(Rect{100, 100, 108, 108});
    CHECK(!grid.test(100, 100));  // „Rodrigo“-Ausnahme
    a.name.clear();
    a.markGrid(Rect{630, 350, 700, 400});  // auf die Szene beschnitten
    CHECK(grid.test(639, 359));
    grid.clear();
    CHECK(!grid.test(0, 0));
}

TEST(area_hit_priority_and_activation) {
    Fixture f;
    f.fs.put("GAME/MENU/T4.szn",
             "[GENERAL]\r\nBackground1=Bg\r\nLayer1=Red\r\nLayer2=Green\r\nLayer3=Half\r\n"
             "Area1=Door\r\n"
             "[Bg]\r\nFilename=BG.bmp\r\n"
             "[Red]\r\nFilename=RED.TCC\r\nX=10\r\nY=10\r\nZ=10\r\n"
             "[Green]\r\nFilename=GREEN.TCC\r\nX=12\r\nY=12\r\nZ=5\r\n"
             "[Half]\r\nFilename=HALF.TCC\r\nX=100\r\nY=100\r\nZ=255\r\n"
             "[Door]\r\nX1=300\r\nX2=200\r\nY1=50\r\nY2=80\r\nCursorType=3\r\n");
    REQUIRE(f.scene.load("T4", 0));
    // Ebenenbereiche zuerst (Dateiname ohne Endung), dann die SZN-Bereiche
    REQUIRE(f.scene.area(0) && f.scene.area(0)->name == "RED" && f.scene.area(0)->mask == f.scene.layer(0));
    CHECK(f.scene.area(0)->cursor == 10);
    Area* door = f.scene.getArea("door");
    REQUIRE(door == f.scene.area(3));
    CHECK(door->rect == (Rect{200, 50, 300, 80}) && door->cursor == 3);  // normalisiert
    // TCC-Ebenen: DirtyAlpha, Alpha-Rechteck mit einschließendem Maximum → letzte Spalte/Zeile
    // des Sprites gehört nicht zum Bereich (Original-Eigenheit)
    CHECK(f.scene.layer(0)->area->rect == (Rect{10, 10, 13, 13}));

    f.enableAll();
    CHECK(f.scene.hitTest(13, 13) == nullptr);  // Enable aktiviert den Bereich nicht
    for (int i = 0; i < 4; ++i) f.scene.area(i)->setEvent(100 + i);
    CHECK(f.scene.hitTest(13, 13) == f.scene.area(1));  // grün (Z 5) vor rot (Z 10)
    CHECK(f.scene.hitTest(10, 10) == f.scene.area(0));
    CHECK(f.scene.hitTest(250, 60) == door);
    CHECK(f.scene.hitTest(101, 101) == f.scene.area(2));  // Z = 255, Alpha 128 ≥ 0x80 → Treffer
    f.scene.layer(1)->disable();
    CHECK(f.scene.hitTest(12, 12) == f.scene.area(0));
    CHECK(f.scene.hitTest(13, 13) == nullptr);  // rechts/unten außerhalb des Bereichs
}

TEST(area_hit_transparency_and_click_events) {
    Fixture f;
    f.fs.put("GAME/MENU/SEMI.TCC", tccSolid(4, 4, 0xF8, 0, 0, 0x7F));
    f.fs.put("GAME/MENU/T5.szn",
             "[GENERAL]\r\nBackground1=Bg\r\nLayer1=Semi\r\nArea1=Spot\r\n"
             "[Bg]\r\nFilename=BG.bmp\r\n"
             "[Semi]\r\nFilename=SEMI.TCC\r\nX=10\r\nY=10\r\n"
             "[Spot]\r\nX1=0\r\nX2=40\r\nY1=0\r\nY2=40\r\n");
    std::vector<std::pair<int, int>> fired;
    f.g.eventProc = [&](Event& e, int t) { fired.emplace_back(e.param, t); };
    REQUIRE(f.scene.load("T5", 0));
    f.enableAll();
    f.scene.area(0)->setEvent(7);
    f.scene.area(1)->setEvent(8);
    // Alpha 0x7F < 0x80 → transparent; der freie Bereich darunter wird getroffen
    CHECK(f.scene.hitTest(11, 11) == f.scene.area(1));

    f.scene.onClick(11, 71);  // Bildschirm-y = Szenen-y + 60
    CHECK(fired.empty());     // nur markiert
    f.scene.dispatchEvents();
    REQUIRE(fired.size() == 1);
    CHECK(fired[0] == std::make_pair(8, 0x0B));

    f.scene.onClick(500, 300);  // ins Leere → Szenen-Ereignis 0x16
    CHECK(f.scene.sceneEvent().pending && f.scene.sceneEvent().type == 0x16);

    f.scene.clicksEnabled = false;
    f.scene.onClick(11, 71);
    f.scene.dispatchEvents();
    CHECK(fired.size() == 1);
}

TEST(area_text_from_ger) {
    Fixture f;
    f.fs.put("GAME/MENU/LOC01.GER", "[AREA_TEXT]\r\nDoor=Tür zum Hof\r\nRED=Rote Kiste\r\n");
    f.fs.put("GAME/MENU/T6.szn",
             "[GENERAL]\r\nLayer1=Red\r\nArea1=Door\r\nArea2=Win\r\n"
             "[Red]\r\nFilename=RED.TCC\r\n[Door]\r\nX2=5\r\nY2=5\r\n[Win]\r\nX2=5\r\nY2=5\r\n");
    f.scene.setLocationNumber(1);
    REQUIRE(f.scene.load("T6", 0));
    CHECK(f.scene.area(0)->text == "Rote Kiste");  // Ebenenbereich: Name = Dateiname
    CHECK(f.scene.area(1)->text == "Tür zum Hof" || f.scene.area(1)->text == "T\xFCr zum Hof");
    CHECK(f.scene.area(2)->text.empty());
}

TEST(mouse_hover_text_timing_and_cursor) {
    Fixture f;
    f.fs.put("GAME/MENU/T7.szn",
             "[GENERAL]\r\nBackground1=Bg\r\nArea1=Door\r\nArea2=Win\r\n[Bg]\r\nFilename=BG.bmp\r\n"
             "[Door]\r\nX1=200\r\nX2=300\r\nY1=50\r\nY2=80\r\nCursorType=3\r\n"
             "[Win]\r\nX1=400\r\nX2=450\r\nY1=330\r\nY2=360\r\n");
    std::uint32_t clock = 5000;
    f.g.now = [&] { return clock; };
    std::vector<std::tuple<std::string, int, int>> drawn;
    f.g.drawAreaText = [&](Surface565&, const Area& a, int x, int y) { drawn.emplace_back(a.name, x, y); };
    REQUIRE(f.scene.load("T7", 0));
    f.scene.background()->enable();
    f.scene.area(0)->setEvent(1);
    f.scene.area(1)->setEvent(2);
    auto frame = [&] {
        pollTimers(f.g);
        f.scene.update(true);
        f.scene.compose(false);
        f.scene.clearOutput();
        f.scene.dispatchEvents();
    };
    frame();
    CHECK(f.scene.onMouseMove(10, 300) == 0);
    CHECK(f.scene.onMouseMove(250, 120) == 3);  // Szenen-y 60 → Tür
    CHECK(f.g.toggleEvent->enabled && f.g.toggleEvent->timerRunning);
    clock += 249;
    frame();
    CHECK(!f.g.textVisible && drawn.empty());
    clock += 1;
    frame();  // Timer 250 ms → sichtbar, nächster Frame zeichnet
    CHECK(f.g.textVisible);
    frame();
    REQUIRE(!drawn.empty());
    CHECK(drawn.back() == std::make_tuple(std::string("Door"), 250, 145 - 60));  // y + 25, dann − 60
    CHECK(f.scene.onMouseMove(260, 125) == 3);  // gleicher Bereich: Text bleibt
    CHECK(f.g.textVisible);
    clock += 2000;
    frame();  // nach 2000 ms ausgeblendet
    CHECK(!f.g.textVisible && f.scene.lastHit() == nullptr);
    const auto n = drawn.size();
    frame();
    CHECK(drawn.size() == n);
    CHECK(f.scene.onMouseMove(420, 400) == 10);  // Fenster (Szenen-y 340): Zeiger 0 → 10; y ≥ 390 → Text bei y − 20
    clock += 250;
    frame();
    frame();
    CHECK(std::get<2>(drawn.back()) == 400 - 20 - 60);
}
