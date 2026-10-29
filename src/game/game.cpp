// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/game.hpp"

#include <cstdio>

#include "core/strings.hpp"
#include "engine/help.hpp"
#include "engine/inventory.hpp"

namespace rah {

Game::Game(GameContext& ctx, int location, int scene, int prevScene)
    : ctx_(ctx), g_(ctx.g), scene_(ctx.scene), location_(location), sceneNr_(scene), prevScene_(prevScene),
      seed_(ctx.g.now ? ctx.g.now() : 0) {
    setEventProc(nullptr);
    for (int i = 0; i < 5; ++i)
        g_.fnHandlers[kWaitDone[static_cast<std::size_t>(i)]] = {this, [this, i](Event& e, int t) { waitDone(i, e, t); }};
}

Game::~Game() {
    shutdown();
    for (int id : kWaitDone) {
        const auto it = g_.fnHandlers.find(id);
        if (it != g_.fnHandlers.end() && it->second.owner == this) g_.fnHandlers.erase(it);
    }
}

void Game::shutdown() {
    if (shutDown_) return;
    shutDown_ = true;
    destroyLocal();
    g_.eventVariant = 0;  // SetEventVariant(0)
    // belegte Warteplätze auf 0, je Platz 3 Nachrichten verarbeiten (ohne Game_Idle)
    for (int& w : waits_) {
        if (w == 0) continue;
        w = 0;
        for (int k = 0; k < 3; ++k)
            if (ctx_.pumpMessage) ctx_.pumpMessage();
    }
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
    second_ = scene_.actor(1);  // +0x58 = CSzene+0x5E4
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

int Game::beginWait() {
    int i = 0;
    for (; i < 5; ++i) {
        if (waits_[static_cast<std::size_t>(i)] == 0) {
            waits_[static_cast<std::size_t>(i)] = 1;
            lastWait_ = i;
            break;
        }
    }
    if (i == 5) {
        lastWait_ = 5;
        return 0;
    }
    return kWaitDone[static_cast<std::size_t>(i)];  // FUN_0043FA10: Funktionszeiger als Wert
}

void Game::waitDone(int i, Event& e, int type) {
    e.setHandler(0);
    int& w = waits_[static_cast<std::size_t>(i)];
    if (w == 1) w = type == 0x2A ? 3 : 2;
}

bool Game::wait() {
    const int i = lastWait_;
    if (i == 5) {
        if (waits_[0] == 1) waits_[0] = 3;
        return false;
    }
    auto& slot = waits_[static_cast<std::size_t>(i)];
    while (slot == 1) {
        int j = 0;
        while (j < 5 && waits_[static_cast<std::size_t>(j)] != 1) ++j;
        if (j < i) {
            // äußere Wartestelle(n) 0…j abbrechen; eigener Platz bleibt 1
            for (int k = j; k >= 0; --k)
                if (waits_[static_cast<std::size_t>(k)] == 1) waits_[static_cast<std::size_t>(k)] = 3;
            slot = 1;
            continue;
        }
        const int m = ctx_.pumpMessage ? ctx_.pumpMessage() : GameContext::kQuit;
        if (m == GameContext::kNoMessage) {
            if (ctx_.idle) ctx_.idle();
        } else if (m == GameContext::kQuit || m == GameContext::kCommand) {
            // WM_QUIT wird an das Fenster weitergeschickt; WM_COMMAND geht verloren
            dbg(g_, "Wartestelle %d beendet: %s", i, m == GameContext::kQuit ? "Quit" : "Command");
            slot = 0;
            return false;
        }
    }
    const int r = slot;
    slot = 0;
    if (r == 3) dbg(g_, "Wartestelle %d abgebrochen (Schauplatz %d)", i, location_);
    return r != 3;  // 0x0047D130 = 0 (immer 0)
}

void Game::setTimerHandler(Event& e, std::uint32_t ms, unsigned flags, int handler) {
    // 0x0043F280: Handler, nicht ausstehend, aktiv; dann CEvent_SetTimer
    e.setHandler(handler);
    e.enabled = true;
    setEventTimer(g_, e, ms, flags);
}

void Game::delay(std::uint32_t ms) {
    Event e;
    const int cb = beginWait();
    setTimerHandler(e, ms, 0, cb);
    wait();
}

void Game::walkAndComment(int x, int y, int dir, std::string_view key) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // UNVERIFIED: Original ohne Prüfung auf Held
    if (wait()) comment(key);
}

void Game::comment(std::string_view key) {
    const int oldDir = hero_ ? hero_->dir : 0;
    const bool input = scene_.clicksEnabled;
    scene_.disableInput();
    delay(300);
    int cb = beginWait();
    if (hero_) hero_->turn(0, cb);
    wait();
    scene_.enableInput();
    heroSay(key);
    scene_.disableInput();
    delay(300);
    cb = beginWait();
    if (hero_) hero_->turn(oldDir, cb);
    wait();
    scene_.clicksEnabled = input;
}

void Game::heroSay(std::string_view key) {
    char sec[16];
    std::snprintf(sec, sizeof sec, "Loc%02d", location_);
    const int cb = beginWait();
    speak(sec, key, hero_, 0x0C, -1, -1, cb);  // 0x00440400(ger "LocNN", schlüssel, Held, cb): Modus 12
    wait();
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

void Game::faceAndSay(std::string_view key) {
    const int cb = beginWait();
    if (hero_) hero_->turn(0, cb);
    if (wait()) heroSay(key);
}

bool Game::playAnimWait(AnimLayer* a) {
    scene_.setModal(a);
    const int cb = beginWait();
    if (a) a->playEndEvent(10, cb);
    return wait();
}

void Game::describeItem(int id) {
    char key[16];
    if (!hero_ || !hero_->active) {
        std::snprintf(key, sizeof key, "INVENT%03dH", id);
        speak("DefInvent", key, 0);  // CGame_SayNoActor
        return;
    }
    hero_->stop(0);  // vtable +0x24
    const int cb = beginWait();
    hero_->turn(0, cb);
    if (!wait()) return;
    std::snprintf(key, sizeof key, "INVENT%03d", id);
    const int cb2 = beginWait();
    sayActor("DefInvent", key, hero_, cb2);
    wait();
}

int Game::random(int lo, int hi) {
    const auto s = static_cast<std::int32_t>(seed_ * 0x7F05u) / 0xB2;
    seed_ = static_cast<std::uint32_t>(s);
    const auto a = static_cast<std::int32_t>((static_cast<std::uint32_t>(s) ^ static_cast<std::uint32_t>(s >> 31)) -
                                             static_cast<std::uint32_t>(s >> 31));  // CDQ/XOR/SUB
    const int r = a % (hi - lo + 1) + lo;
    return r > hi ? hi : r;
}

void Game::playSfxRandom3D(AudioBuffer& b) {
    const float z = static_cast<float>(random(-100, 100)) * 0.01f;  // 0x0046B740
    const float x = static_cast<float>(random(-100, 100)) * 0.01f;
    b.setPos3D(x, 0.0f, z);
    b.play();
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
    if (!ctx.current) {
        std::fprintf(stderr, "Schauplatz %d / Szene %d noch nicht portiert\n", r.nextLocation, r.nextScene);
        return false;
    }
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
    // Abbruch-Flag (Warteplätze waren belegt) gilt nur bis zur obersten Game_Idle-Ebene
    if (r.abort && ctx.g.idleDepth == 1) r.abort = false;
    if (!r.pending || r.abort) return;
    r.pending = false;  // = 0x0047D74C (0)
    if (ctx.current) {
        ctx.current->shutdown();  // Löschen des alten CGame (Speicher siehe GameContext::retired)
        ctx.retired.push_back(std::move(ctx.current));
    }
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
