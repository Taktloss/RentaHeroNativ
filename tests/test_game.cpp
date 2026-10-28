// SPDX-License-Identifier: GPL-3.0-or-later
#include <map>

#include "core/strings.hpp"
#include "audio/mixer.hpp"
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
    }
    // ein Game_Idle: Timer, Update, Compose, Ereignisse, Schauplatzwechsel
    void frame(std::uint32_t ms = 100) {
        clock += ms;
        pollTimers(g);
        scene->update(true);
        scene->compose(false);
        scene->clearOutput();
        scene->dispatchEvents();
        processLocationChange(*ctx);
    }
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
    // Klick ins Bild → Ereignis 2 → Loc 35 / Szene 1. Fehlt der Schauplatz, nimmt Game_Idle die
    // vertauschten Werte: zurück nach Loc 40 / Szene 114 (Intro beginnt neu)
    r.scene->onClick(300, 200);
    r.frame();
    auto* again = dynamic_cast<Loc40*>(r.ctx->current.get());
    REQUIRE(again);
    CHECK(again->sceneNumber() == 114 && again->prevScene() == 1);
    CHECK(r.ctx->request.nextLocation == 40 && r.ctx->request.prevLocation == 40);
    CHECK(r.scene->modal() == again->anim() && again->anim()->frame <= 1);
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
