// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc37.hpp"

#include "engine/area.hpp"

namespace rah {

namespace {
constexpr int kCaveEvent = 1;     // Höhle angeklickt
constexpr int kGliderEvent = 2;   // Gleiter angeklickt
constexpr int kNagEvent = 3;      // Mahnung
constexpr int kAmbientEvent = 4;  // Umgebungsklang
constexpr std::size_t kNagFlag = 0x410C;  // CGameState, Start 1
}  // namespace

Loc37::Loc37(GameContext& ctx, int scene, int prevScene) : Game(ctx, 37, scene, prevScene) {
    getViewObjects();
    scene_.load("S3", 0xFFFF);  // CSzene_LoadAll(DAT_00476274)
    getViewObjects();
    // UNVERIFIED: das Original prüft weder auf fehlende noch auf statische Ebenen
    landing_ = dynamic_cast<AnimLayer*>(scene_.getObject("S3_Landen"));
    pointing_ = dynamic_cast<AnimLayer*>(scene_.getObject("S3_Zeigen"));
    glider_ = scene_.getObject("S3_Gleiter");
    passing_ = loadSfx("Sfx_Glider_PassingBy");
    bird_ = loadSfx("Sfx_Bird");
    bird2_ = loadSfx("Sfx_Bird2");
    bird3_ = loadSfx("Sfx_Bird3");
    chirp_ = loadSfx("Sfx_Chirp");
    cave_ = scene_.getArea("S3_Hoehle");
    nagTimer_ = std::make_unique<Event>();
    ambientTimer_ = std::make_unique<Event>();
    music_ = playMusic("Loc36_IntroDragonAttack");
    if (music_) {
        music_->setReverb(0x10, 0.111f, 2.767f, 0.224f);  // 0x3DE353F8, 0x40311687, 0x3E656042
        music_->setReverbMix(0.1f);                       // 0x3DCCCCCD
    }
}

void Loc37::destroyLocal() {
    // Ereignisse: 0x0043F160 (austragen) + Löschen
    nagTimer_.reset();
    ambientTimer_.reset();
    passing_.reset();
    bird_.reset();
    bird2_.reset();
    bird3_.reset();
    chirp_.reset();
}

void Loc37::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    setEventProc(&Loc37::onEvent);
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(3, 10) * 1000), 0, kAmbientEvent);
    if (restored_ == 0) {
        // Landung (nur ohne Position aus dem Spielstand)
        if (passing_) {
            passing_->setFreq(100);
            passing_->play();
            passing_->fadeFreq(25, 2000);
        }
        int cb = beginWait();
        if (landing_) landing_->playRange(10, cb, 0, -1);
        wait();
        if (glider_) glider_->enable();
        if (hero_) hero_->setPosDir(280, 264, 6);
        cb = beginWait();
        if (hero_) hero_->walkTo(340, 283, cb, 5);  // CActor_WalkTo2
        wait();
        if (hero_) hero_->hide(scene_.clicksEnabled);
        speak("Loc37", "003_ROD_01", 0);  // CGame_SayNoActor
        cb = beginWait();
        if (pointing_) pointing_->playRange(10, cb, 0, -1);
        wait();
    }
    if (hero_) hero_->show(scene_.clicksEnabled);
    if (glider_) {
        if (glider_->area) glider_->area->setEvent(kGliderEvent);
        glider_->enable();
    }
    setTimerHandler(*nagTimer_, 30000, 0, kNagEvent);
    if (cave_) cave_->setEvent(kCaveEvent);
    scene_.enableInput();
}

void Loc37::onEvent(Event& e, int) {
    switch (e.param) {
    case kGliderEvent:
        scene_.disableInput();
        commenting_ = true;
        walkAndComment(289, 266, 5, "003_ROD_04");
        commenting_ = false;
        scene_.enableInput();
        return;
    case kNagEvent: {
        if (commenting_) {
            setTimerHandler(*nagTimer_, 1000, 0, kNagEvent);
            return;
        }
        std::int32_t& flag = ctx_.state.at(kNagFlag);
        if (flag == 1) {
            speak("Loc37", "003_ROD_02", hero_, 0x0C, -1, -1, 0);  // CGame_SayActor
            setTimerHandler(*nagTimer_, 10000, 0, kNagEvent);
            flag = 2;
            return;
        }
        if (flag != 2) return;
        scene_.disableInput();
        speak("Loc37", "003_ROD_03", hero_, 0x0C, -1, -1, 0);
        [[fallthrough]];  // Original: fehlendes break, der Held geht selbst in die Höhle
    }
    case kCaveEvent: {
        const int cb = beginWait();
        if (hero_) hero_->walkTo(482, 207, cb, 6);
        if (wait()) changeScene(116, 40);
        return;
    }
    case kAmbientEvent: {
        AudioBuffer* b = nullptr;
        switch (random(1, 4)) {
        case 1: b = bird_.get(); break;
        case 2: b = bird2_.get(); break;
        case 3: b = bird3_.get(); break;
        case 4: b = chirp_.get(); break;
        default: break;
        }
        if (b) {
            b->setVolume(random(50, 100));
            playSfxRandom3D(*b);
        }
        setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(3, 10) * 1000), 0, kAmbientEvent);
        return;
    }
    default: return;
    }
}

}  // namespace rah
