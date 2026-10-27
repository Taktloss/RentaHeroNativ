// SPDX-License-Identifier: GPL-3.0-or-later
#include <map>
#include <vector>

#include "core/strings.hpp"
#include "engine/animlayer.hpp"
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

// n Schlüsselbilder 2×2 mit Alpha, Rotwert = 10·(i+1)
Bytes a16(int n) {
    Bytes f;
    for (int i = 0; i < n; ++i) {
        put32(f, 0);
        for (int c = 0; c < 4; ++c) put32(f, 3);
        put32(f, 2);
        put32(f, 2);
        for (std::uint8_t v : {static_cast<std::uint8_t>(10 * (i + 1)), std::uint8_t{0}, std::uint8_t{0},
                               std::uint8_t{255}}) {
            f.push_back(1);
            f.push_back(4);
            f.push_back(v);
        }
    }
    return f;
}

struct Rig {
    OneFs fs;
    EngineGlobals g;
    std::uint32_t clock = 1000;
    std::vector<std::pair<int, int>> fired;  // (Ereignis-ID, Typ)
    std::vector<int> cueFrames;
    std::unique_ptr<AnimLayer> L;

    explicit Rig(int frames) {
        fs.data = a16(frames);
        g.now = [this] { return clock; };
        g.eventProc = [this](Event& e, int t) { fired.emplace_back(e.param, t); };
        g.runCue = [this](AnimLayer& a, const AnimCue&) { cueFrames.push_back(a.frame); };
        L = std::make_unique<AnimLayer>(g);
        std::string err;
        REQUIRE(L->load(FoundFile{&fs, "T.A16"}, 0, &err));
    }
    int shown() const { return L->planes().rgb[0][0] / 10 - 1; }
    void tick() {
        L->update(g);
        if (L->endEvent) fireEvent(g, *L->endEvent, L->endEvent->type);
        if (L->frameEvent) fireEvent(g, *L->frameEvent, L->frameEvent->type);
    }
};

}  // namespace

TEST(anim_load_defaults) {
    Rig r(3);
    CHECK(r.L->frame == -1 && r.L->period == 100 && r.L->nextTime == 1100);
    CHECK(r.L->alpha && r.L->isAnim && !r.L->active && !r.L->running);
}

TEST(anim_play_once_fires_end_next_tick) {
    Rig r(3);
    r.L->playEndEvent(1, 7);
    CHECK(r.L->active && r.L->running && !r.L->hideAtEnd);
    r.tick();  // erstes Next nach dem Laden hält Bild 0
    CHECK(r.L->frame == 0 && r.shown() == 0);
    r.tick();
    r.tick();
    CHECK(r.L->frame == 2 && r.shown() == 2 && r.fired.empty());
    r.tick();  // Dateiende → OnEnd: angehalten, Ende ausstehend
    CHECK(!r.L->running && r.L->endPending && r.fired.empty());
    r.tick();  // erst im nächsten Update Typ 1
    CHECK(r.L->ended && r.fired.size() == 1 && r.fired[0] == std::make_pair(7, 1));
}

TEST(anim_loop_wraps) {
    Rig r(2);
    r.L->start(13);
    CHECK(r.L->loop);
    for (int i = 0; i < 3; ++i) r.tick();  // 0, 1, Ende → Rewind (frame 0)
    CHECK(r.L->running && r.L->frame == 0 && r.shown() == 0 && !r.L->endPending);
}

TEST(anim_mode_table) {
    Rig r(2);
    r.L->start(0);
    CHECK(r.L->hideAtEnd);
    r.L->start(10);
    CHECK(r.L->hideAtEnd);
    r.L->start(11);
    CHECK(!r.L->hideAtEnd && r.L->endMode == 1);
    r.L->start(12);
    CHECK(r.L->endMode == 2);
    r.L->hideAtEnd = true;
    r.L->start(5);  // Fall 5 der Tabelle: nichts ändern
    CHECK(r.L->hideAtEnd && r.L->endMode == 1);
}

TEST(anim_range_and_frame_event) {
    Rig r(6);
    r.L->setFrameEvent(9, 3);
    r.L->playRange(1, 4, 2, 4);
    CHECK(r.L->frame == 1 && r.L->rangeTo == 4);
    r.tick();
    CHECK(r.L->frame == 2 && r.shown() == 2);
    r.tick();
    CHECK(r.L->frame == 3 && r.fired.size() == 1 && r.fired[0] == std::make_pair(9, 3));
    r.tick();  // Bild 4 = Bereichsende
    CHECK(r.L->frame == 4 && !r.L->running && r.L->rangeTo == -1);
}

TEST(anim_catch_up_reruns_cues_without_advancing) {
    Rig r(5);
    AnimCue c;
    c.type = 1;
    c.frame = 1;
    r.L->addCue(c);
    r.L->start(1);
    r.L->nextTime = 1000;
    r.tick();  // Bild 0; 1100 > 1000 → kein Nachholen
    r.clock = 1500;
    r.tick();  // Bild 1; 1200 < 1500 → Nachholschritt: Cue noch einmal, kein Bildwechsel
    CHECK(r.L->frame == 1 && r.shown() == 1);
    CHECK(r.cueFrames == std::vector<int>({1, 1}));
    CHECK(r.L->nextTime == 1600);
}

TEST(anim_reverse_skips_frame0) {
    Rig r(5);
    r.L->playRangeNoEvent(1, 4, 1);
    CHECK(r.L->reverse && r.L->frame == 3);
    r.tick();
    CHECK(r.L->frame == 2 && r.shown() == 2);
    r.tick();  // Bild 1 = Bereichsende
    CHECK(r.L->frame == 1 && r.shown() == 1 && !r.L->running);
}

TEST(anim_stop_fires_end) {
    Rig r(3);
    r.L->playEndEvent(1, 5);
    r.L->stop(0);
    CHECK(!r.L->running && r.L->ended);
    fireEvent(r.g, *r.L->endEvent, 1);
    CHECK(r.fired.size() == 1 && r.fired[0].first == 5);
}
