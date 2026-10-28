// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc40.hpp"

namespace rah {

namespace {
constexpr int kFrameEvent = 1;  // EventValue(1): Zielbild erreicht
constexpr int kDoneEvent = 2;   // EventValue(2): Animationsende oder Klick
constexpr int kSpeechEvent = 3; // EventValue(3): Sprache fertig (S116)
}  // namespace

Loc40::Loc40(GameContext& ctx, int scene, int prevScene) : Game(ctx, 40, scene, prevScene) {
    scene_.load("xxx", 0xFFFF);  // CSzene_LoadAll: SZN fehlt → leere Szene
    getViewObjects();
    // Musik, Effekte (CGame_LoadSfx…) und Ton-Cues der Animation folgen mit dem Audio-Port
    if (sceneNr_ == 114) {
        anim_ = loadAnim("S114_IntroPart1");
        playMusic("Loc40_IntroPart1");
    } else if (sceneNr_ == 115) {
        anim_ = loadAnim("S115_IntroPart2");
        playMusic("Loc36_IntroDragonAttack");
    } else if (sceneNr_ == 116) {
        anim_ = loadAnim("S116_IntroPart3");
        playMusic("Loc36_IntroDragonAttack");
    }
}

Loc40::~Loc40() {
    anim_.reset();  // +0x7C … +0xFC löschen, danach CGame_dtor (Szene hält +0x5DC noch)
}

void Loc40::start() {
    // UNVERIFIED (Port): ohne AVI fehlt +0x7C; das Original würde hier abstürzen
    if (!anim_) return;
    if (sceneNr_ == 114) {
        audio("Musik abspielen, Puffer-Eigenschaften");
        setEventProc(&Loc40::onEvent114);
        scene_.setClickEvent(kDoneEvent);
        scene_.setModal(anim_.get());
        anim_->setFrameEvent(kFrameEvent, 10);
        anim_->playEndEvent(10, kDoneEvent);
        audio("Sfx_Ocean abspielen, Lautstärke 30 über 5000 ms");
        scene_.clicksDuringFull = true;
    } else if (sceneNr_ == 115) {
        audio("Musik abspielen, Puffer-Eigenschaften");
        setEventProc(&Loc40::onEvent115);
        scene_.setClickEvent(kDoneEvent);
        scene_.setModal(anim_.get());
        anim_->playEndEvent(10, kDoneEvent);
        audio("Effekt +0xD0 abspielen, Lautstärke 75 / Frequenz 50 über 1200 ms");
        scene_.clicksDuringFull = true;
    } else if (sceneNr_ == 116) {
        audio("Musik einblenden, Puffer-Eigenschaften");
        setEventProc(&Loc40::onEvent116);
        scene_.setClickEvent(kDoneEvent);
        scene_.setModal(anim_.get());
        anim_->setFrameEvent(kFrameEvent, 1);
        anim_->playRangeNoEvent(11, 0, 120);
        scene_.clicksDuringFull = true;
    }
    scene_.clearFlag510();
    scene_.enableInput();
}

void Loc40::onEvent114(Event& e, int) {
    if (e.param == kFrameEvent) {
        int next = 0;
        switch (anim_->frame) {
        case 10: say("Loc40", "016_SPR_01"); next = 0x96; break;
        case 0x96:
            say("Loc40", "016_SPR_02");
            audio("Musik-Puffer-Eigenschaften");
            next = 0x212;
            break;
        case 0x212: say("Loc40", "016_SPR_03"); next = 0x2E4; break;
        case 0x2E4: say("Loc40", "021_SPR_01"); next = 0x352; break;
        case 0x352: say("Loc40", "021_SPR_02"); next = 0x3C0; break;
        case 0x3C0:
            say("Loc40", "023_SPR_01");
            audio("Musik-Puffer-Eigenschaften");
            next = 0x435;
            break;
        case 0x435:
            playMusic("Loc09_SmashvilleAttack");
            audio("Musik-Puffer-Eigenschaften, abspielen");
            next = 0x4E0;
            break;
        case 0x4E0: say("Loc40", "024_ST2_01"); return;
        default: return;
        }
        anim_->setFrameEvent(kFrameEvent, next);
    } else if (e.param == kDoneEvent) {
        scene_.clicksDuringFull = false;
        changeScene(1, 35);
    }
}

void Loc40::onEvent115(Event& e, int) {
    if (e.param == kDoneEvent) {
        scene_.clicksDuringFull = false;
        changeScene(3, 37);
    }
}

void Loc40::onEvent116(Event& e, int) {
    if (e.param == kFrameEvent) {
        int next = 0;
        switch (anim_->frame) {
        case 1: say("Loc40", "003_PRI_05"); next = 0x15; break;
        case 0x15: say("Loc40", "003_PRI_06"); next = 0x57; break;
        case 0x57: speak("Loc40", "004_PRI_01", kSpeechEvent); next = 0x62; break;
        case 0x62:
            playMusic("Loc35_RodrigoOnGlider");
            audio("Musik einblenden 100 über 2000 ms; Effekt +0xD0 Lautstärke 1 / Frequenz 50 über 5000 ms");
            return;
        default: return;
        }
        anim_->setFrameEvent(kFrameEvent, next);
    } else if (e.param == kDoneEvent) {
        scene_.clicksDuringFull = false;
        sceneNr_ = 5;  // Original: vorherige Szene wird 5 (loc40.md)
        changeScene(7, 1);
    } else if (e.param == kSpeechEvent) {
        anim_->playRange(10, kDoneEvent, 0x79, -1);
    }
}

std::unique_ptr<Game> createLocation(GameContext& ctx, int location, int scene, int prevScene) {
    switch (location) {
    case 40: return std::make_unique<Loc40>(ctx, scene, prevScene);
    default: return nullptr;  // weitere Schauplätze folgen
    }
}

}  // namespace rah
