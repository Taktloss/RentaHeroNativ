// SPDX-License-Identifier: GPL-3.0-or-later
#include <functional>
#include <map>

#include "core/strings.hpp"
#include "audio/mixer.hpp"
#include "game/loc01.hpp"
#include "game/loc02.hpp"
#include "game/loc03.hpp"
#include "game/loc10.hpp"
#include "game/loc12.hpp"
#include "game/loc31.hpp"
#include "game/loc04.hpp"
#include "game/loc05.hpp"
#include "game/loc06.hpp"
#include "game/loc35.hpp"
#include "game/loc37.hpp"
#include "game/loc40.hpp"
#include "test.hpp"

using namespace rah;

namespace {

class FakeVideo final : public VideoDecoder {
public:
    explicit FakeVideo(int n) : n_(n) {}
    int width() const override { return 4; }
    int height() const override { return 2; }
    std::int64_t startTime() const override { return 0; }
    std::int64_t duration() const override { return std::int64_t{n_} * 100000; }
    void seek(std::int64_t t) override { pos_ = static_cast<int>(t / 100000); }
    Result next(std::vector<std::uint16_t>& f) override {
        if (pos_ >= n_) return Result::End;
        f.assign(8, static_cast<std::uint16_t>(++pos_));
        return Result::Ok;
    }

private:
    int n_, pos_ = 0;
};

class MemFs final : public FileSystem {
public:
    std::map<std::string, Bytes> files;
    void put(const std::string& p, std::string_view s) { files[toUpperAscii(p)] = Bytes(s.begin(), s.end()); }
    bool exists(std::string_view p) const override { return files.count(toUpperAscii(std::string(p))) != 0; }
    std::optional<Bytes> read(std::string_view p) const override {
        auto it = files.find(toUpperAscii(std::string(p)));
        if (it == files.end()) return std::nullopt;
        return it->second;
    }
    std::vector<std::string> list(std::string_view) const override { return {}; }
};

struct Rig {
    MemFs fs;
    DataPaths paths{&fs, nullptr};
    EngineGlobals g;
    std::uint32_t clock = 1000;
    Surface565 surf{640, 420}, screen{640, 480};
    std::unique_ptr<Scene> scene;
    std::unique_ptr<GameContext> ctx;
    std::vector<std::string> status;
    explicit Rig(int frames) {
        g.now = [this] { return clock; };
        g.openVideo = [frames](const FoundFile&, std::string*) -> std::unique_ptr<VideoDecoder> {
            return std::make_unique<FakeVideo>(frames);
        };
        g.screen = &screen;
        g.drawStatusText = [this](const Rect&, const std::string& t) { status.push_back(t); };
        scene = std::make_unique<Scene>(paths, g, surf);
        ctx = std::make_unique<GameContext>(g, paths, *scene);
        ctx->pumpMessage = [this] { return pending.empty() ? GameContext::kNoMessage : take(); };
        ctx->idle = [this] { frame(); };
    }
    // ein Game_Idle: Timer, Update, Compose, Ereignisse, Schauplatzwechsel
    void frame(std::uint32_t ms = 100) {
        ++g.idleDepth;
        clock += ms;
        ++frames;
        if (onFrame) onFrame(frames);
        pollTimers(g);
        scene->update(true);
        scene->compose(false);
        scene->clearOutput();
        scene->dispatchEvents();
        processLocationChange(*ctx);
        --g.idleDepth;
        ctx->releaseRetired();
    }
    int take() {
        const int m = pending.front();
        pending.erase(pending.begin());
        return m;
    }
    std::vector<int> pending;  // Nachrichten für pumpMessage
    int frames = 0;
    std::function<void(int)> onFrame;
};

}  // namespace

TEST(loc40_intro_s114_speech_chain_and_click_skip) {
    Rig r(200);
    r.g.subtitles = true;
    r.fs.put("GAME/LOC40/S114_IntroPart1.avi", "x");
    r.fs.put("GAME/LOC40/Loc40.ger", "[SPEECH]\r\n016_SPR_01=Erste Zeile\r\n");
    requestLocation(r.ctx->request, 40, 114, 0, false);
    processLocationChange(*r.ctx);
    auto* loc = dynamic_cast<Loc40*>(r.ctx->current.get());
    REQUIRE(loc && loc->anim());
    CHECK(r.scene->modal() == loc->anim() && r.scene->sceneNumber() == 114);
    CHECK(r.scene->clicksDuringFull && !r.scene->globalClick && r.scene->clicksEnabled);
    for (int i = 0; i < 12 && r.status.empty(); ++i) r.frame();
    // Bild 10 → 016_SPR_01 als Untertitel; nächstes Zielbild 150
    REQUIRE(!r.status.empty());
    CHECK(r.status.back() == "Erste Zeile");
    CHECK(r.g.speechRunning && loc->anim()->frameEventFrame == 0x96);
    // Sprache ohne WAV: mindestens 1000 ms, dann Ende
    for (int i = 0; i < 11; ++i) r.frame();
    CHECK(!r.g.speechRunning);
    // Klick ins Bild → Ereignis 2 → Loc 35 / Szene 1 (Loc35 setzt CGame+0x28 = 1)
    r.scene->onClick(300, 200);
    r.frame();
    auto* glider = dynamic_cast<Loc35*>(r.ctx->current.get());
    REQUIRE(glider);
    CHECK(glider->sceneNumber() == 1 && glider->prevScene() == 114 && glider->location() == 1);
    CHECK(r.ctx->request.nextLocation == 35 && r.ctx->request.prevLocation == 40);
}

TEST(loc40_intro_s116_prev_scene_quirk) {
    Rig r(140);
    r.fs.put("GAME/LOC40/S116_IntroPart3.avi", "x");
    requestLocation(r.ctx->request, 40, 116, 115, false);
    processLocationChange(*r.ctx);
    REQUIRE(r.ctx->current);
    r.scene->onClick(300, 200);
    r.clock += 100;
    r.scene->dispatchEvents();
    // Wechsel nach Loc 1 / Szene 7 mit vorheriger Szene 5 (Original-Verhalten)
    CHECK(r.ctx->request.pending && r.ctx->request.nextLocation == 1);
    CHECK(r.ctx->request.nextScene == 7 && r.ctx->request.prevScene == 5);
    CHECK(r.ctx->request.prevLocation == 40);
}

TEST(loc40_intro_cues_and_music) {
    for (const auto& [scene, n] : {std::pair{114, 45}, std::pair{115, 52}, std::pair{116, 4}}) {
        Rig r(10);
        Mixer mixer;
        AudioManager mgr(r.g, &mixer);
        r.g.audio = &mgr;
        r.fs.put("GAME/LOC40/S114_IntroPart1.avi", "x");
        r.fs.put("GAME/LOC40/S115_IntroPart2.avi", "x");
        r.fs.put("GAME/LOC40/S116_IntroPart3.avi", "x");
        requestLocation(r.ctx->request, 40, scene, 0, false);
        processLocationChange(*r.ctx);
        auto* loc = dynamic_cast<Loc40*>(r.ctx->current.get());
        REQUIRE(loc && loc->anim());
        int cues = 0;
        for (const auto& c : loc->anim()->cues) cues += c.type != 0;
        CHECK(cues == n);
        CHECK(r.ctx->music != nullptr);  // Musikobjekt auch ohne WAV-Datei (Open scheitert still)
        r.ctx->current.reset();
        r.ctx->music.reset();
        r.g.audio = nullptr;
    }
}

TEST(speech_lipsync_frames_follow_audio_position) {
    // ACS mit Kopf: Offset 6, Rate 10 → Bilder 3, 4, 5, Ende
    const Bytes acs{0xFF, 0xFF, 6, 0, 10, 0, 3, 0, 4, 0, 5, 0, 0xFF, 0xFF};
    std::vector<std::uint16_t> data;
    int rate = 0;
    REQUIRE(parseLipSync(acs, data, rate));
    CHECK(rate == 10 && data[0] == 3 && data[2] == 5 && data[3] == 0xFFFF);
    const Bytes plain{7, 0, 8, 0};
    REQUIRE(parseLipSync(plain, data, rate));
    CHECK(rate == 20 && data[0] == 7 && data.size() == 52);
}

namespace {

// Schauplatz für Tests: macht die Warte-API zugänglich
class WaitLoc final : public Game {
public:
    using Game::Game;
    using Game::beginWait;
    using Game::changeScene;
    using Game::delay;
    using Game::wait;
    using Game::random;
};

}  // namespace

TEST(game_wait_slots_and_done_callback) {
    Rig r(10);
    WaitLoc w(*r.ctx, 50, 1, 0);
    CHECK(w.beginWait() == 0x0043F910 && w.beginWait() == 0x0043F940);
    CHECK(w.waitSlots()[0] == 1 && w.waitSlots()[1] == 1);
    // Ereignis mit Warte-Rückruf: 0x29 → Platz fertig (2), 0x2A → abgebrochen (3)
    Event a, b;
    a.setHandler(0x0043F910);
    b.setHandler(0x0043F940);
    a.enabled = b.enabled = true;
    a.mark(r.g, 0x29);
    b.mark(r.g, 0x2A);
    fireEvent(r.g, a, 0x29);
    fireEvent(r.g, b, 0x2A);
    CHECK(w.waitSlots()[0] == 2 && w.waitSlots()[1] == 3);
    CHECK(a.handlerId == 0 && b.handlerId == 0);  // Rückruf löscht den Handler
    // Wait liest den zuletzt belegten Platz (1): abgebrochen → false, Platz frei
    CHECK(!w.wait());
    CHECK(w.waitSlots()[1] == 0 && w.waitSlots()[0] == 2);
    // alle fünf belegt → 0, Wait bricht Platz 0 ab, wenn er wartet
    for (int i = 0; i < 4; ++i) w.beginWait();
    CHECK(w.beginWait() == 0);
    CHECK(!w.wait());
}

TEST(game_delay_runs_nested_idle_until_timer) {
    Rig r(10);
    WaitLoc w(*r.ctx, 50, 1, 0);
    const std::uint32_t t0 = r.clock;
    int depth = 0;
    r.onFrame = [&](int) { depth = std::max(depth, r.g.idleDepth); };
    w.delay(300);
    CHECK(r.clock - t0 >= 300 && r.clock - t0 < 500);  // Timer fällig, Ereignis in DispatchEvents
    CHECK(depth == 1);  // Game_Idle aus der Wartestelle (Hauptschleife hier nicht beteiligt)
    CHECK(w.waitSlots()[0] == 0);
}

TEST(game_wait_quit_message_aborts) {
    Rig r(10);
    WaitLoc w(*r.ctx, 50, 1, 0);
    r.pending = {GameContext::kMessage, GameContext::kCommand};
    w.beginWait();
    CHECK(!w.wait());  // WM_COMMAND während des Wartens: Platz 0, abgebrochen
    CHECK(w.waitSlots()[0] == 0 && r.frames == 0);
}

TEST(game_nested_wait_aborts_outer) {
    Rig r(10);
    WaitLoc w(*r.ctx, 50, 1, 0);
    // äußere Wartestelle (Platz 0); im ersten Bild startet eine innere (Delay, Platz 1).
    // CGame_Wait: ein wartender Platz mit kleinerem Index → auf 3 (abgebrochen).
    w.beginWait();
    bool inner = false;
    r.onFrame = [&](int n) {
        if (n != 1) return;
        w.delay(200);
        inner = true;
    };
    CHECK(!w.wait());  // innere Wartestelle hat Platz 0 auf 3 gesetzt
    CHECK(inner && w.waitSlots()[0] == 0 && w.waitSlots()[1] == 0);
}

TEST(game_change_scene_aborts_waits_until_top_level) {
    Rig r(10);
    WaitLoc w(*r.ctx, 50, 1, 0);
    w.beginWait();
    w.changeScene(2, 50);
    CHECK(w.waitSlots()[0] == 3 && r.ctx->request.pending && r.ctx->request.abort);
    // verschachteltes Game_Idle (Tiefe 2): Wechsel wartet
    ++r.g.idleDepth;
    r.frame();
    --r.g.idleDepth;
    CHECK(r.ctx->request.pending && r.ctx->request.abort);
    // oberste Ebene: Abbruch-Flag gelöscht, Wechsel (Schauplatz 50 fehlt → Szene leer)
    r.frame();
    CHECK(!r.ctx->request.pending && !r.ctx->request.abort);
    CHECK(!w.wait());
}

TEST(loc35_timer_stages_and_item_use) {
    Rig r(10);
    r.ctx->current = std::make_unique<Loc35>(*r.ctx, 1, 0);
    auto& loc = static_cast<Loc35&>(*r.ctx->current);
    CHECK(loc.location() == 1 && loc.sceneNumber() == 1);  // Ctor setzt CGame+0x28 = 1
    loc.start();
    CHECK(loc.timeStage() == 1 && loc.nextAction() == 1);
    for (int i = 0; i < 21; ++i) r.frame();  // 2000 ms → 001_ROD_01, Timer 5000 ms
    CHECK(loc.timeStage() == 2);
    for (int i = 0; i < 50; ++i) r.frame();  // 5000 ms → Aktion 2, Timer 20 000 ms
    CHECK(loc.timeStage() == 3 && loc.nextAction() == 2);
    for (int i = 0; i < 200; ++i) r.frame();  // 20 000 ms → Aktion 5 (Nahaufnahme), kein Timer
    CHECK(loc.timeStage() == 4 && loc.nextAction() == 5);
    // Gegenstand 27 benutzt → Aktion 10 bleibt gegen spätere Zeitstufen bestehen
    Event e;
    e.setHandler(3);
    e.enabled = true;
    e.mark(r.g, 0x0B);
    fireEvent(r.g, e, 0x0B);
    CHECK(loc.nextAction() == 10);
}

TEST(game_random_matches_original_lcg) {
    Rig r(10);
    WaitLoc w(*r.ctx, 50, 1, 0);  // Startwert = timeGetTime() = 1000
    // Folge nachgerechnet mit 32-Bit-Überlauf (Schritt 3 läuft über) und IDIV
    const int lo[8] = {3, 1, -100, 50, 3, 1, -100, 50}, hi[8] = {10, 4, 100, 100, 10, 4, 100, 100};
    const int expect[8] = {10, 1, 60, 98, 3, 4, 52, 83};
    for (int i = 0; i < 8; ++i) CHECK(w.random(lo[i], hi[i]) == expect[i]);
}

TEST(loc37_nag_flag_and_cave_walk_abort) {
    Rig r(10);
    r.ctx->current = std::make_unique<Loc37>(*r.ctx, 3, 115);
    auto& loc = *r.ctx->current;
    CHECK(loc.location() == 37 && loc.sceneNumber() == 3);
    CHECK(r.ctx->state.at(0x410C) == 1);
    // Landung: drei Wartestellen (ohne Ebenen/Figur) per WM_QUIT abbrechen
    r.pending = {GameContext::kQuit, GameContext::kQuit, GameContext::kQuit};
    loc.start();
    CHECK(r.pending.empty());
    for (int i = 0; i < 301; ++i) r.frame();  // 30 000 ms → 003_ROD_02, Flag 2, Timer 10 000 ms
    CHECK(r.ctx->state.at(0x410C) == 2);
    // 10 000 ms → 003_ROD_03, weiter wie Höhle (fehlendes break); Gang abgebrochen → kein Wechsel
    r.pending = {GameContext::kQuit};
    for (int i = 0; i < 101; ++i) r.frame();
    CHECK(r.pending.empty() && !r.ctx->request.pending);
    CHECK(r.ctx->state.at(0x410C) == 2);  // nie zurückgesetzt
}

TEST(loc01_scene_numbers_and_canal_flag) {
    Rig r(10);
    {
        Loc01 a(*r.ctx, 5, 0);  // unbekannte Szene → 7
        CHECK(a.location() == 1 && a.sceneNumber() == 7);
        CHECK(r.g.fnHandlers.count(0x00407DC0) == 1);  // Rückruf der Gleiterbewegung
    }
    CHECK(r.g.fnHandlers.count(0x00407DC0) == 0);
    CHECK(r.ctx->state.at(0x3E94) == 1);
    {
        Loc01 b(*r.ctx, 46, 47);  // aus dem Kanal (Szene 47): Kanalwesen weg
        CHECK(b.sceneNumber() == 46);
    }
    CHECK(r.ctx->state.at(0x3E94) == 0);
    Loc01 c(*r.ctx, 1007, 9);
    CHECK(c.sceneNumber() == 1007);
}

TEST(loc01_start_scene9_and_ambient_timer) {
    Rig r(10);
    r.ctx->current = std::make_unique<Loc01>(*r.ctx, 9, 10);
    auto& loc = *r.ctx->current;
    loc.start();  // ohne Wartestellen: Eingabe an, Umgebungstimer 5–15 s
    CHECK(r.scene->clicksEnabled);
    for (int i = 0; i < 400; ++i) r.frame();  // Umgebungsklänge ohne Tonsystem: nur Timer
    CHECK(!r.ctx->request.pending);
}

TEST(loc06_visit_counter_and_entry_flags) {
    Rig r(10);
    auto& st = r.ctx->state;
    CHECK(st.at(0x3EE8) == 0 && st.at(0x3EEC) == 1 && st.at(0x3EF0) == 1 && st.at(0x3E8C) == 0);
    {
        Loc06 a(*r.ctx, 99, 1);  // unbekannte Szene → 26, erster Besuch
        CHECK(a.location() == 6 && a.sceneNumber() == 26 && st.at(0x3EE8) == 1);
        CHECK(r.g.fnHandlers.count(0x0040F4F0) == 1 && r.g.fnHandlers.count(0x0040F4E0) == 1);
    }
    CHECK(r.g.fnHandlers.count(0x0040F4F0) == 0);
    { Loc06 b(*r.ctx, 26, 1); }
    CHECK(st.at(0x3EE8) == 2);
    { Loc06 c(*r.ctx, 27, 26); }  // Gespräch: Besuchszähler bleibt 2
    CHECK(st.at(0x3EE8) == 2);
    { Loc06 d(*r.ctx, 27, 2003); }  // Sprungziel: Ring, Gesprächsstufe 3
    CHECK(st.at(0x3EE8) == 2 && st.at(0x3E8C) == 1 && st.at(0x3EEC) == 3);
    { Loc06 e(*r.ctx, 26, 1001); }  // 0 → +1
    CHECK(st.at(0x3EE8) == 1);
}

TEST(loc04_entry_targets_and_visit_counter) {
    Rig r(10);
    auto& st = r.ctx->state;
    {
        Loc04 a(*r.ctx, 6, 1004);  // Sprungziel: Ramil kommt, dritter Besuch
        CHECK(a.location() == 4 && a.sceneNumber() == 6);
        CHECK(st.at(0x3E84) == 1 && st.at(0x3ED4) == 0 && st.at(0x3ED0) == 3);
        CHECK(r.g.fnHandlers.count(0x0040CF50) == 1 && !a.atDesk() && a.ramTalkStage() == 0);
    }
    CHECK(r.g.fnHandlers.count(0x0040CF50) == 0);
    { Loc04 b(*r.ctx, 6, 1003); }  // Geldsack in der Kiste; 0x3E84 bleibt
    CHECK(st.at(0x3ED4) == 1 && st.at(0x3E84) == 1 && st.at(0x3ED0) == 2);
    { Loc04 c(*r.ctx, 6, 1001); }
    CHECK(st.at(0x3ED4) == 0 && st.at(0x3E84) == 0 && st.at(0x3ED0) == 0);
    { Loc04 d(*r.ctx, 6, 7); }  // normaler Zugang ändert nichts im Konstruktor
    CHECK(st.at(0x3ED0) == 0);
}

TEST(loc05_constructs_and_keeps_talk_state) {
    Rig r(10);
    auto& st = r.ctx->state;
    const auto talk = st.at(0x3EDC), flowers = st.at(0x3EE0);
    {
        Loc05 a(*r.ctx, 10, 9);
        CHECK(a.location() == 5 && a.louiTalk() == nullptr);  // ohne Spieldaten keine Ebenen
    }
    // Konstruktor und Abbau ändern keine Flags (Gesprächsstufe, Blumen)
    CHECK(st.at(0x3EDC) == talk && st.at(0x3EE0) == flowers);
}

TEST(loc03_guards_only_until_sabrina_met) {
    Rig r(10);
    auto& st = r.ctx->state;
    CHECK(st.at(0x3ECC) == 0);
    {
        Loc03 a(*r.ctx, 12, 8);
        CHECK(a.location() == 3 && a.guardsLoaded());  // Wachen-Timer +0xB8 angelegt
    }
    st.at(0x3ECC) = 1;
    {
        Loc03 b(*r.ctx, 12, 13);
        CHECK(!b.guardsLoaded());
    }
    CHECK(st.at(0x3EC8) == 0);  // nur der Piratenangriff (von Szene 15) setzt 0x3EC8
}

TEST(loc02_all_scenes_construct_without_changing_flags) {
    Rig r(10);
    auto& st = r.ctx->state;
    const auto before = st.at(0x3EA8) + st.at(0x3EB4) + st.at(0x3EC0) + st.at(0x3EA0);
    for (const auto& [scene, prev] : {std::pair{13, 12}, {1013, 13}, {14, 1013}, {2013, 13}, {15, 2013}}) {
        Loc02 a(*r.ctx, scene, prev);
        CHECK(a.location() == 2 && a.sceneNumber() == scene);
    }
    CHECK(st.at(0x3EA8) + st.at(0x3EB4) + st.at(0x3EC0) + st.at(0x3EA0) == before);
}

TEST(loc31_scene_number_is_fixed_to_113) {
    Rig r(10);
    Loc31 a(*r.ctx, 0, 7);
    CHECK(a.location() == 31 && a.sceneNumber() == 113);
}

TEST(loc10_constructor_sets_glider_and_night_flags) {
    Rig r(10);
    auto& st = r.ctx->state;
    st.at(0x3F24) = 0;
    st.at(0x3E80) = 0;
    for (int scene : {38, 39, 40, 41, 42, 43, 44}) {
        Loc10 a(*r.ctx, scene, 37);
        CHECK(a.location() == 10 && a.sceneNumber() == scene);
    }
    CHECK(st.at(0x3F24) == 0 && st.at(0x3E80) == 0);
    { Loc10 a(*r.ctx, 37, 113); }
    CHECK(st.at(0x3F24) == 1);
    { Loc10 a(*r.ctx, 37, 1001); }
    CHECK(st.at(0x3E80) == 1);
    st.at(0x3F24) = 0;
    st.at(0x3E80) = 0;
}

TEST(loc12_scene_number_is_fixed_to_91) {
    Rig r(10);
    Loc12 a(*r.ctx, 0, 42);
    CHECK(a.location() == 12 && a.sceneNumber() == 91);
}
