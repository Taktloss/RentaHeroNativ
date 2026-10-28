// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/game.hpp"

#include "core/strings.hpp"
#include "engine/help.hpp"
#include "engine/inventory.hpp"

namespace rah {

Game::Game(GameContext& ctx, int location, int scene, int prevScene)
    : ctx_(ctx), g_(ctx.g), scene_(ctx.scene), location_(location), sceneNr_(scene), prevScene_(prevScene),
      seed_(ctx.g.now ? ctx.g.now() : 0) {
    setEventProc(nullptr);
}

Game::~Game() {
    // Warteplätze ≠ 0 freigeben (mit je 3 Nachrichten) – Wartestellen folgen
    waits_.fill(0);
    setEventProc(nullptr);
    if (auto* bar = scene_.inventory()) bar->setUseHandler(0);  // 0x0043FA60(0)
    // +0x74 (Leiste): 0x00442280, 0x00441E60(0, 0); +0x60…+0x6C Ton – folgen mit dem Ton
    stopSpeech();
    if (ctx_.help) ctx_.help->iconOff();  // 0x004414B0
    scene_.reset();
    scene_.setBarShown(false);  // +0x5BC = 0
}

void Game::setEventProc(Handler h) {
    if (h) g_.eventProc = [this, h](Event& e, int t) { (this->*h)(e, t); };
    else g_.eventProc = nullptr;
    // 0x00441E60(leiste, proc, this): Beschreibung über CGame_DescribeItem – folgt mit dem Inventar
}

void Game::getViewObjects() {
    // Hilfe, Tiefen-/Wegekarte, Bild-Hinweistabelle folgen
    music_ = ctx_.music.get();  // +0x08 = CSzene+0x1A8
    hero_ = scene_.actor(0);    // +0x1C = CSzene+0x5E0
    scene_.setSceneNumber(sceneNr_);  // CSzene+0x5D4 = +0x2C
}

void Game::changeScene(int scene, int location) {
    bool abort = false;
    for (int& w : waits_) {
        if (w != 0) {
            abort = true;
            w = 3;
        }
    }
    requestLocation(ctx_.request, location, scene, sceneNr_, abort);
}

void Game::stopSpeech() {
    if (!speech_) return;
    speechStop(g_);  // 0x00446030
    speech_.reset();
}

bool Game::say(std::string_view ger, std::string_view name) {
    stopSpeech();
    const auto file = scene_.paths().findFile(ger, "GER");
    speech_ = std::make_unique<Speech>(g_);
    speech_->load(scene_.paths(), file, name, true);
    speech_->play(0);
    return true;
}

bool Game::speak(std::string_view ger, std::string_view name, AnimLayer* speaker, int mode, int from, int to,
                 int doneEvent) {
    stopSpeech();
    const auto file = scene_.paths().findFile(ger, "GER");  // 0x004405D0
    speech_ = std::make_unique<Speech>(g_);
    speech_->load(scene_.paths(), file, name, false);
    // Port: 0x004405D0 liefert nur bei Speichermangel 0 (dann Ereignis-Rückruf (0, 0))
    if (speaker == hero_) {
        mode = -1;
        from = -1;
        to = -1;
        speaker = hero_;
    }
    speechSetSpeaker(g_, speaker, mode, from, to);  // 0x00446660
    speech_->play(doneEvent);
    return true;
}

AudioStream* Game::playMusic(std::string_view name) {
    if (music_) {
        if (equalsNoCase(name, ctx_.musicName)) return music_;
        // 0x00438000: +0x12C = 1, FadeVolume(0, 2000) – FadeTick löscht sie bei Lautstärke 0
        music_->deleteWhenSilent = true;
        music_->fadeVolume(0, 2000);
        if (ctx_.music.get() == music_ && g_.audio) g_.audio->adopt(std::move(ctx_.music));
        music_ = nullptr;
    }
    const auto file = scene_.paths().findFile(name, "WAV");
    if (!g_.audio) return nullptr;  // CSzene+0x5C4 == 0
    auto m = std::make_unique<AudioStream>();
    m->open(file ? &*file : nullptr, g_.audio, kAudioMusic);
    m->setPos3D(0.0f, 0.0f, -0.1f);
    music_ = m.get();
    ctx_.music = std::move(m);  // CSzene+0x1A8
    ctx_.musicName = std::string(name);  // CSzene+0x1AC
    return music_;
}

std::unique_ptr<AudioBuffer> Game::loadSfx(std::string_view name) {
    const auto file = scene_.paths().findFile(name, "WAV");
    if (!g_.audio) return nullptr;
    auto b = std::make_unique<AudioBuffer>();
    // UNVERIFIED: ohne Treffer öffnet das Original den zuletzt probierten Pfad – scheitert ebenso
    b->open(file ? &*file : nullptr, g_.audio, kAudioSfx);
    return b;
}

std::unique_ptr<AudioBuffer> Game::loadSfxVol(std::string_view name, int vol) {
    auto b = loadSfx(name);
    if (b) b->setVolume(vol);
    return b;
}

void requestLocation(LocationRequest& r, int location, int scene, int prevScene, bool abort) {
    r.prevLocation = r.nextLocation;
    r.nextLocation = location;
    r.pending = true;
    r.prevScene = prevScene;
    r.nextScene = scene;
    r.abort = abort;
}

bool switchLocation(GameContext& ctx) {
    const LocationRequest& r = ctx.request;
    // Szenenordner "menu\" bzw. "LocNN\", CSzene+0x5D0/+0x5D4
    ctx.paths.setLocation(r.nextLocation);
    ctx.scene.setLocationNumber(r.nextLocation);
    ctx.scene.setSceneNumber(r.nextScene);
    // Config_ReadCdMap 0x00404360 (CD-Abfrage) entfällt: alle Daten sind gemeinsam eingebunden.
    // Renderer 0x0043E960(0x0B) … (0): Bedeutung offen
    ctx.current = createLocation(ctx, r.nextLocation, r.nextScene, r.prevScene);
    if (!ctx.current) return false;
    ctx.scene.setBarShown(true);  // +0x5BC = g_CurGame
    // Spielstand-Position (0x0047D764) folgt; ausstehende Maus-/Tastennachrichten verwerfen: Plattform
    ctx.current->start();
    if (ctx.help) ctx.help->setup(&ctx.scene, nullptr, false);  // 0x004410C0(hwnd, 0, szene, game)
    rendererClearStatusText(ctx.g);                             // 0x0043E7C0
    if (auto* bar = ctx.scene.inventory()) bar->invalidate();   // 0x00441F50
    return true;
}

void processLocationChange(GameContext& ctx) {
    LocationRequest& r = ctx.request;
    if (!r.pending || r.abort) return;
    r.pending = false;  // = 0x0047D74C (0)
    ctx.current.reset();
    if (switchLocation(ctx)) return;
    // mit vertauschten Zielwerten erneut (zurück zum vorigen Schauplatz)
    const int scene = r.nextScene;
    r.nextLocation = r.prevLocation;
    r.nextScene = r.prevScene;
    r.prevScene = scene;
    if (switchLocation(ctx)) return;
    // orig: 0x004487A0 CSzene_Clear, 0x0043EC60 – Port: Szene leeren
    ctx.scene.reset();
}

}  // namespace rah
