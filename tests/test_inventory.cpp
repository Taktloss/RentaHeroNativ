// SPDX-License-Identifier: GPL-3.0-or-later
#include <map>
#include <vector>

#include "core/strings.hpp"
#include "engine/scene.hpp"
#include "test.hpp"

using namespace rah;

namespace {

class MemFs final : public FileSystem {
public:
    std::map<std::string, Bytes> files;
    void put(const std::string& p, Bytes b) { files[toUpperAscii(p)] = std::move(b); }
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

// RLE: Lauf (1, n, v) = n× v (tcc.md); 3600 Pixel = 60 Läufe à 60
Bytes plane(std::uint8_t v) {
    Bytes p;
    for (int i = 0; i < 60; ++i) {
        p.push_back(1);
        p.push_back(60);
        p.push_back(v);
    }
    return p;
}

Bytes itemFile(std::uint8_t red) {
    Bytes f;
    for (int i = 0; i < 2; ++i) {
        put32(f, 0);
        for (int c = 0; c < 4; ++c) put32(f, 180);
        put32(f, 60);
        put32(f, 60);
        for (std::uint8_t v : {red, std::uint8_t{0}, std::uint8_t{0}, std::uint8_t{255}}) {
            const Bytes p = plane(v);
            f.insert(f.end(), p.begin(), p.end());
        }
    }
    return f;
}

struct Rig {
    MemFs common;
    DataPaths paths{nullptr, &common};
    EngineGlobals g;
    Surface565 surf{640, 420};
    std::unique_ptr<Scene> scene;
    std::uint32_t clock = 1000;
    std::vector<std::pair<int, int>> fired;  // (Handler, Typ)
    std::vector<Rect> outputs;
    std::vector<std::string> texts;
    std::vector<int> described;

    Rig() {
        for (int id = 1; id <= 12; ++id) {
            char n[16];
            std::snprintf(n, sizeof n, "Item%03d.a16", id);
            common.put(n, itemFile(static_cast<std::uint8_t>(10 * id)));
        }
        const std::string ger = "[SPEECH]\r\nTEXT001=Schl\xFCssel\r\n";
        common.put("DefInvent.GER", Bytes(ger.begin(), ger.end()));
        g.now = [this] { return clock; };
        g.eventProc = [this](Event& e, int t) { fired.emplace_back(e.handlerId, t); };
        g.outputSceneRect = [this](const Rect& r) { outputs.push_back(r); };
        g.drawStatusText = [this](const Rect&, const std::string& t) { texts.push_back(t); };
        scene = std::make_unique<Scene>(paths, g, surf);
        scene->setBarShown(true);
    }
    InventoryBar& bar() { return *scene->inventory(); }
};

}  // namespace

TEST(inventory_add_remove_and_rects) {
    Rig r;
    InventoryBar& b = r.bar();
    CHECK(b.takeOutputRect() == Rect{});
    CHECK(!b.add(99));  // Datei fehlt
    REQUIRE(b.add(3));
    CHECK(b.add(3) && b.count() == 1);  // schon vorhanden
    REQUIRE(b.add(5));
    CHECK(b.has(5) && !b.has(4));
    CHECK(b.entry(0).layer->name == "Item003" && b.entry(0).layer->fps == 10);
    CHECK(b.takeOutputRect() == (Rect{0, 360, 128, 420}));
    CHECK(b.draw());
    CHECK(!b.draw());  // nichts mehr zu tun
    // dunkel: Rot 30 − 128 → 0
    CHECK(r.surf.row(370)[10] == 0 && b.entry(1).layer->x == 64);
    r.surf.fill(0, 360, 640, 420, 0xFFFF);
    CHECK(b.remove(3) && b.count() == 1 && b.entry(0).id == 5);
    CHECK(r.surf.row(400)[600] == 0);  // Leiste schwarz gefüllt
    CHECK(b.takeOutputRect() == (Rect{0, 360, 640, 420}));  // einmal ganz
    CHECK(b.takeOutputRect() == (Rect{0, 360, 64, 420}));
    b.clear();
    CHECK(b.count() == 0 && !b.has(5));
}

TEST(inventory_hover_click_and_scroll) {
    Rig r;
    InventoryBar& b = r.bar();
    for (int id = 1; id <= 12; ++id) REQUIRE(b.add(id));
    CHECK(b.takeOutputRect() == (Rect{0, 360, 640, 420}));
    b.draw();
    // Maus über Platz 0 (Bildschirm y 430 = Szene 370): Typ 0x0D, Text aus DefInvent
    r.scene->onMouseMove(20, 430);
    r.scene->dispatchEvents();
    CHECK(b.bright() && b.hovered() == 0);
    REQUIRE(!r.texts.empty());
    CHECK(r.texts.back() == "Schl\xFCssel");
    CHECK(b.entry(0).layer->loop);
    // Platz 1: Schleife von Platz 0 endet, fehlender Text → [TEXT002]
    r.scene->onMouseMove(70, 430);
    r.scene->dispatchEvents();
    CHECK(b.hovered() == 1 && !b.entry(0).layer->running && r.texts.back() == "[TEXT002]");
    // Rand rechts: blättert, solange mehr als 10 übrig
    r.scene->onMouseMove(636, 430);
    r.scene->dispatchEvents();
    r.scene->onMouseMove(637, 430);
    r.scene->dispatchEvents();
    r.scene->onMouseMove(638, 430);
    r.scene->dispatchEvents();
    CHECK(b.first() == 2);
    // Maus verlässt die Leiste: Text gelöscht (Rechteck −60 neu ausgegeben), dunkel
    r.outputs.clear();
    r.scene->onMouseMove(300, 200);
    CHECK(!b.bright() && b.hovered() == -1 && r.g.statusText.empty());
    REQUIRE(!r.outputs.empty());
    CHECK(r.outputs.back() == (Rect{40, 364, 600, 415}));
    // Klick ohne Benutzungs-Ereignis → Beschreibung (CGame_DescribeItem)
    b.setDescribe(0, [&r](int id) { r.described.push_back(id); });
    r.scene->onClick(10, 430);  // Platz 0 + 2 = ID 3
    r.scene->dispatchEvents();
    CHECK(r.described == std::vector<int>{3} && b.lastClicked() == 3);
    // Benutzungs-Ereignis, Gegenstands-Ereignisse aus → Typ = ID
    b.setUseHandler(500);
    r.scene->onClick(10, 430);
    r.scene->dispatchEvents();
    CHECK(!r.fired.empty() && r.fired.back() == (std::pair<int, int>{500, 3}));
    // wie CGame_SetEventProc: +0x41C = Handler; Gegenstands-Ereignis 77 an ID 3
    b.setDescribe(500, [&r](int id) { r.described.push_back(id); });
    b.setItemEvent(3, 77);
    r.scene->onClick(10, 430);
    r.scene->dispatchEvents();
    CHECK(r.fired.back() == (std::pair<int, int>{500, 77}));
    // ID 4 ohne eigenes Ereignis → Beschreibung
    r.scene->onClick(70, 430);
    r.scene->dispatchEvents();
    CHECK(r.described.back() == 4);
    // Schauplatzwechsel (CGame_dtor): 0x0043FA60(0), 0x00442280 → nichts mehr auslösen/beschreiben
    b.setUseHandler(0);
    b.clearItemEvents();
    const auto nFired = r.fired.size();
    const auto nDescribed = r.described.size();
    r.scene->onClick(10, 430);
    r.scene->dispatchEvents();
    CHECK(r.fired.size() == nFired && r.described.size() == nDescribed && b.entry(2).eventId == 0);
}

TEST(inventory_hit_needs_bar_shown) {
    Rig r;
    CHECK(r.scene->hitTest(10, 370) == &r.bar().area());
    r.scene->setBarShown(false);
    CHECK(r.scene->hitTest(10, 370) == nullptr);
}
