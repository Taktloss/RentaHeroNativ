// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc26.hpp"

#include <bit>
#include <cstdlib>

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
float f32(std::uint32_t bits) { return std::bit_cast<float>(bits); }
constexpr std::size_t k3F18 = 0x3F18;       // Loc08-Zustand; ≠ 0 nötig für die Fahne
constexpr std::size_t kPitSeen = 0x4094;    // Grube kommentiert (liest Loc27)
constexpr std::size_t kTo98Open = 0x4098;   // Weg nach 98 frei (Loc27); sonst Wald kommentierbar
constexpr std::size_t kFirstFlight = 0x409C;  // erste Ankunft: 093_ROD_01/093_DRA_01
constexpr std::size_t kFlagThere = 0x40A0;  // Fahne steckt noch (Gegenstand 2)
constexpr std::size_t kFlagStage = 0x40A4;  // 1: erst Kommentar, 2: aufheben
constexpr int kScene58 = 0x3A, kScene93 = 0x5D, kScene95 = 0x5F, kScene98 = 0x62;
constexpr int kAmbient = 1000;
}  // namespace

Loc26::Loc26(GameContext& ctx, int scene, int prevScene) : Game(ctx, 26, scene, prevScene) {
    getViewObjects();
    if (sceneNr_ == kScene93) {
        scene_.load("S93", 0);  // CSzene_Load(DAT_0047521C, 0)
        getViewObjects();
        seaLO_ = obj("S93_MeerLO");
        seaLU_ = obj("S93_MeerLU");
        seaRO_ = obj("S93_MeerRO");
        takeoff_ = obj("S93_DragonTakeoff");
        breath_ = obj("S93_DragonBreath");
        landing_ = obj("S93_DragonLanding");
        hut_ = scene_.getArea("S93_Huette");
        sea_ = scene_.getArea("S93_Meer");
        forest_ = scene_.getArea("S93_Wald");
        to98_ = scene_.getArea("S93_To98");
        dragon_ = scene_.getArea("S93_Dragon");
        wings_ = loadSfxVol("Sfx_DragonWings", 0x46);
        if (takeoff_) {
            AnimCue c;  // CAnimLayer_AddCueEx
            c.type = 2;
            c.buffer = wings_.get();
            c.action = 1;
            c.frame = 0x42;
            c.vol = 0x5A;
            c.x = f32(0x3E19999A);
            c.z = f32(0x3E4CCCCD);
            takeoff_->addCue(c);
            c.frame = 0x57;
            c.vol = 100;
            c.x = 0.0f;
            c.z = f32(0x3DCCCCCD);
            takeoff_->addCue(c);
        }
        breathTimer_ = std::make_unique<Event>();
    } else if (sceneNr_ == kScene98) {
        scene_.load("S98", 0);  // CSzene_Load(DAT_0047525C, 0)
        getViewObjects();
        pickUp_ = obj("Gen_RodPickUp");
        flag98_ = obj("S98_Fahne");
        seaL_ = obj("S98_MeerL");
        seaR_ = obj("S98_MeerR");
        to93_ = scene_.getArea("S98_To93");
        pit_ = scene_.getArea("S98_Grube");
    }
    ambientTimer_ = std::make_unique<Event>();
    seagull_ = loadSfx("Sfx_Seagull");
    wind_ = loadSfx("Sfx_Wind");
    wind2_ = loadSfx("Sfx_Wind2");
    ocean_ = loadSfxVol("Sfx_Ocean", 0x46);
    music_ = playMusic("Loc26_IslandOfMegophias");
    if (ocean_) ocean_->playLoop();  // vtable +0x10
}

void Loc26::destroyLocal() {
    ambientTimer_.reset();
    breathTimer_.reset();
    for (auto* b : {&seagull_, &wind_, &wind2_, &ocean_, &wings_}) b->reset();
}

bool Loc26::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo
    return wait();
}

bool Loc26::walk2(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo2
    return wait();
}

bool Loc26::playRange(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

void Loc26::commentHere(const char* key) {
    // CGame_WalkAndComment an der aktuellen Fußposition (+0x15D0/+0x15D4) und Richtung (+0x15CC)
    if (!hero_) return;
    walkAndComment(hero_->footX, hero_->footY, hero_->dir, key);
}

void Loc26::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    if (sceneNr_ == kScene93) {
        setEventProc(&Loc26::onEvent93);
        if (hut_) hut_->setEvent(1);
        if (sea_) sea_->setEvent(2);
        if (flag(kTo98Open) == 0 && forest_) forest_->setEvent(3);
        if (flag(kTo98Open) != 0 && to98_) to98_->setEvent(4);
        if (dragon_) dragon_->setEvent(5);
        for (AnimLayer* a : {seaLO_, seaLU_, seaRO_})
            if (a) a->start(0x0D);
        const auto showDragon = [this] {
            if (breath_) {
                breath_->showFrame(10);
                breath_->enable();
            }
            if (hero_) hero_->show(scene_.clicksEnabled);
        };
        if (restored_ == 0) {
            if (prevScene_ == kScene58) {
                // Landung nach dem Flug
                playRange(landing_, 10, 1, -1);
                showDragon();
                if (hero_) hero_->setPosDir(0x229, 0x15E, 3);
                walk2(0x200, 0x163, 2);
            } else if (prevScene_ == kScene95) {
                showDragon();
                if (hero_) hero_->setPosDir(0x4F, 0xE7, 0);
                walk2(0x5D, 0x104, 7);
            } else if (prevScene_ == kScene98) {
                showDragon();
                if (hero_) hero_->setPosDir(0x182, 0xA0, 0);
                walk2(0x184, 0xAA, 0);
            }
            if (flag(kFirstFlight) != 0) {
                flag(kFirstFlight) = 0;
                scene_.disableInput();
                heroSay("093_ROD_01");  // CGame_SayActor("Loc26", …, Held)
                {
                    const int cb = beginWait();
                    speak("Loc26", "093_DRA_01", breath_, 0x0C, 10, 10, cb);
                    wait();
                }
                scene_.enableInput();
            }
        } else {
            showDragon();
        }
        setTimerHandler(*breathTimer_, static_cast<std::uint32_t>(random(10, 20) * 1000), 0, 6);
    } else if (sceneNr_ == kScene98) {
        setEventProc(&Loc26::onEvent98);
        if (seaL_) seaL_->start(0x0D);
        if (seaR_) seaR_->start(0x0D);
        if (flag(kFlagThere) != 0 && flag(k3F18) != 0 && flag98_) {
            flag98_->playEndEvent(0x0D, 0);
            if (flag98_->area) {
                flag98_->area->setEvent(0x0C);
                flag98_->area->active = true;  // +0xB8
            }
        }
        if (to93_) to93_->setEvent(10);
        if (pit_) pit_->setEvent(0x0B);
        if (InventoryBar* bar = scene_.inventory()) {
            bar->setItemEvent(9, 0x0D);
            bar->setItemEvent(0x0B, 0x0D);
        }
        if (restored_ == 0) {
            if (hero_) hero_->setPosDir(0x181, 0x9C, 0);
            walk2(0x184, 0xAA, 0);
        } else if (hero_) {
            hero_->show(scene_.clicksEnabled);
        }
    }
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(5, 10) * 1000), 0, kAmbient);
    scene_.enableInput();
}

void Loc26::onEvent93(Event& e, int) {
    switch (e.param) {
    case 1:  // Hütte → 27/95
        if (walk(0x4F, 0xE7, 5)) changeScene(kScene95, 0x1B);
        return;
    case 2: commentHere("093_ROD_03"); return;
    case 3: commentHere("093_ROD_04"); return;
    case 4:  // → 98
        if (flag(kTo98Open) == 0) return;
        if (walk(0x182, 0xA0, 4)) changeScene(kScene98, 0x1A);
        return;
    case 5: {  // Drache
        const bool atDragon =
            hero_ && std::abs(hero_->footX - 0x22A) < 10 && std::abs(hero_->footY - 0x15D) < 10;
        if (!walk(0x229, 0x15E, 3)) return;
        if (atDragon) {
            // aufsteigen und zurück nach 58
            breathTimer_->enabled = false;
            breathTimer_->pending = false;
            if (breath_) breath_->disable();
            if (hero_) hero_->hide(scene_.clicksEnabled);
            playRange(takeoff_, 11, 1, -1);
            changeScene(kScene58, 0x10);
            return;
        }
        {
            const int cb = beginWait();
            speak("Loc26", "093_ROD_02", hero_, 0x0C, 0, 8, cb);
            wait();
        }
        {
            const int cb = beginWait();
            speak("Loc26", "093_DRA_02", breath_, 0x0C, 10, 10, cb);
            wait();
        }
        return;
    }
    case 6:  // Drache atmet
        if (breath_) breath_->playRangeNoEvent(11, 1, 10);
        setTimerHandler(*breathTimer_, static_cast<std::uint32_t>(random(10, 20) * 1000), 0, 6);
        return;
    case kAmbient: {
        const int r = random(1, 3);
        AudioBuffer* b = nullptr;
        if (r == 1) {
            const int v = random(0x32, 100);
            if (seagull_) seagull_->setVolume(v);  // CAudioBuffer_SetVolume
            b = seagull_.get();
        } else if (r == 2) {
            b = wind_.get();
        } else if (r == 3) {
            b = wind2_.get();
        }
        if (b) playSfxRandom3D(*b);
        setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(2, 10) * 1000), 0, kAmbient);
        return;
    }
    default:
        return;
    }
}

void Loc26::onEvent98(Event& e, int) {
    switch (e.param) {
    case 10:  // → 93
        if (walk(0x181, 0x9C, 4)) changeScene(kScene93, 0x1A);
        return;
    case 0x0B:  // Grube
        walkAndComment(0xA8, 0x12E, 5, "098_ROD_01b");
        flag(kPitSeen) = 1;
        return;
    case 0x0C:  // Fahne (Gegenstand 2)
        if (flag(kFlagThere) == 0) return;
        if (flag(kFlagStage) == 1) {
            flag(kFlagStage) = 2;
            walkAndComment(0x56, 0x100, 4, "098_ROD_01");
            return;
        }
        if (flag(kFlagStage) != 2) return;
        if (!walk(0x56, 0x100, 4)) return;
        scene_.disableInput();
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playRange(pickUp_, 11, 0, 7);
        if (flag98_) flag98_->disable();
        if (InventoryBar* bar = scene_.inventory()) bar->add(2);
        flag(kFlagThere) = 0;
        playRange(pickUp_, 10, 8, -1);
        if (hero_) hero_->show(scene_.clicksEnabled);
        scene_.enableInput();
        return;
    case 0x0D:  // Gegenstand 9 oder 11
        commentHere("098_ROD_02");
        return;
    case kAmbient: {
        const int r = random(1, 3);
        AudioBuffer* b = r == 1 ? seagull_.get() : r == 2 ? wind_.get() : r == 3 ? wind2_.get() : nullptr;
        if (b) playSfxRandom3D(*b);
        setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(2, 10) * 1000), 0, kAmbient);
        return;
    }
    default:
        return;
    }
}

}  // namespace rah
