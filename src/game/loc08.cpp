// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc08.hpp"

#include <cstdio>

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
constexpr std::size_t kSabTrap = 0x3F0C;   // Fass: 1 Sabrina springt heraus, 2 danach
constexpr std::size_t kSabTalk = 0x3F10;   // Gesprächsstufe mit Sabrina (1…5)
constexpr std::size_t kFlagGiven = 0x3F14; // Fahne (Gegenstand 2) am Boot
constexpr std::size_t kSab = 0x3F18;       // Sabrina: 0 fort, 1 da, 2 Klappe offen, 3 im Gang, 4 Boot
constexpr std::size_t kForestOnce = 0x3F1C;// Weg in den Wald schon nach 58 geführt
constexpr std::size_t kBoatBack = 0x3F20;  // Boot zurück (Szene 100)
constexpr std::size_t kLeftShip = 0x4088;  // Rodrigo hat das Schiff mit Helm verlassen (Loc24 0x00428A53); ≠ 0 → Wald einmal nach 58, Boot verweigert
constexpr int kAmbient = 0x48;
constexpr int kWindTimer = 0x46;
constexpr int kBushTimer = 0x47;
}  // namespace

Loc08::Loc08(GameContext& ctx, int scene, int prevScene) : Game(ctx, 8, scene, prevScene) {
    getViewObjects();
    unsigned mask = flag(kSab) < 2 ? 1u : 0u;
    if (flag(kSab) < 4) mask |= 2;
    if (sceneNr_ == 99) {
        scene_.load("099", mask);  // DAT_004727B8
        getViewObjects();
        if (flag(kSab) == 0) rodInCam_ = loadAnim("RodReinCam");
        bush_ = obj("Busch");
        hatch_ = obj("Klappe");
        rodClimbIn_ = obj("RodClimbIn");
        rodClimbOut_ = obj("RodClimbOut");
        sabJump_ = obj("SabJump");
        rodScared_ = obj("RodSchreck");
        sabPoint_ = obj("SabPoint");
        sabLever_ = obj("SabLever");
        sabClimbIn_ = obj("SabClimbIn");
        sabClimbOut_ = obj("SabClimbOut");
        sabWalk_ = obj("SabWalk");
        sabStand_ = obj("SabStand");
        landing_ = scene_.getArea("099_AnlegePlatz");
        town_ = scene_.getArea("099_Stadt");
        barrel_ = scene_.getArea("099_Fass");
        forest_ = scene_.getArea("099_Wald");
        passage_ = scene_.getArea("099_Gang");
        bushTimer_ = std::make_unique<Event>();
        windTimer_ = std::make_unique<Event>();
        wind_ = loadSfx("Sfx_Wind");
    } else if (sceneNr_ == 100) {
        if (flag(kBoatBack) == 0) mask |= 4;
        scene_.load("100", mask);  // DAT_00472824
        getViewObjects();
        surf_ = obj("Brandung");
        boatAway_ = obj("BoatAway");
        boatReturns_ = obj("BoatReturns");
        boatBack_ = scene_.getObject("BoatBack");
        boat_ = obj("Boot");  // DAT_004727EC
        boatFlag_ = obj("BootFahne");
        rodFlag_ = obj("RodFahne");
        sabWalk_ = obj("SabWalk");
        sabStand_ = obj("SabStand");
        wayBack_ = scene_.getArea("100_WayBack");
    }
    seagull_ = loadSfx("Sfx_Seagull");
    knockWet_ = loadSfxVol("Sfx_Knock_Wet", 0x46);
    woosh_ = loadSfxVol("Sfx_Woosh", 0x46);
    fixBelt_ = loadSfxVol("Sfx_Work_FixBelt", 0x46);
    creak_ = loadSfxVol("Sfx_Door_CreakShort", 0x46);
    paddling_ = loadSfxVol("Sfx_Paddling", 0x32);
    thunder_ = loadSfx("Sfx_Thunder");
    thunder2_ = loadSfx("Sfx_Thunder2");
    ocean_ = loadSfxVol("Sfx_Ocean", 0x1E);
    ambientTimer_ = std::make_unique<Event>();
    music_ = playMusic("Loc08_Harbour");
    if (music_) {
        music_->setReverb(0x10, 0.111f, 2.767f, 0.224f);  // 0x3DE353F8, 0x40311687, 0x3E656042
        music_->setReverbMix(0.1f);                       // 0x3DCCCCCD
    }
}

void Loc08::destroyLocal() {
    // Reihenfolge wie 0x00411AD0
    rodInCam_.reset();
    if (sceneNr_ == 99) {
        bushTimer_.reset();
        windTimer_.reset();
        wind_.reset();
    }
    ambientTimer_.reset();
    for (auto* b : {&seagull_, &knockWet_, &woosh_, &fixBelt_, &creak_, &paddling_, &thunder_, &thunder2_, &ocean_})
        b->reset();
}

bool Loc08::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo
    return wait();
}

bool Loc08::walk2(int x, int y, int dir) {
    // CActor_WalkTo2 0x00434D90 = WalkTo (ACTOR.md)
    return walk(x, y, dir);
}

bool Loc08::playEnd(AnimLayer* a, unsigned mode) {
    const int cb = beginWait();
    if (a) a->playEndEvent(mode, cb);
    return wait();
}

bool Loc08::playRange(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

bool Loc08::moveWait(Layer* l, double x, double y, double z, double zoom, int steps) {
    const int cb = beginWait();
    if (l) l->moveTo(x, y, z, zoom, steps, cb);  // CLayer_MoveTo 0x004475B0
    return wait();
}

void Loc08::sabSpeak(std::string_view key, int mode, bool waitDone) {
    // CGame_Speak("Loc08", key, SabStand, mode, 4, 7, ereignis)
    if (!waitDone) {
        speak("Loc08", key, sabStand_, mode, 4, 7, 0);
        return;
    }
    const int cb = beginWait();
    speak("Loc08", key, sabStand_, mode, 4, 7, cb);
    wait();
}

void Loc08::heroSayGer(std::string_view key) {
    // CGame_SayActor("Loc08", key, Held, warten)
    const int cb = beginWait();
    sayActor("Loc08", key, hero_, cb);
    wait();
}

void Loc08::sabStand(double x, double y, double z, double zoom) {
    if (!sabStand_) return;
    sabStand_->showFrame(4);
    sabStand_->setPosZ(x, y, z, zoom);
    sabStand_->enable();
    if (sabStand_->area) sabStand_->area->setEvent(0x0D);
}

void Loc08::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // CSzene+0x5C0
    setEventProc(&Loc08::onEvent);
    if (ocean_) {
        ocean_->playLoop();  // vtable +0x10
        ocean_->fadeVolume(0x1E, 2000);
    }
    if (sceneNr_ == 99) start99();
    else if (sceneNr_ == 100) start100();
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(10, 30) * 100), 0, kAmbient);
    scene_.enableInput();
}

void Loc08::start99() {
    if (landing_) landing_->setEvent(3);
    if (town_) town_->setEvent(5);
    if (barrel_) barrel_->setEvent(7);
    if (forest_) forest_->setEvent(9);
    if (flag(kSab) < 4) {
        if (Layer* ship = scene_.getObject("099_PirateShip")) {
            ship->enable();
            if (ship->area) ship->area->setEvent(1);
        }
    }
    bool walkIn = true;
    if (prevScene_ == 100) {
        // aus dem Gang zurück
        if (bush_) bush_->enable();
        if (hatch_) {
            hatch_->showFrame(10);
            hatch_->enable();
        }
        if (passage_) passage_->setEvent(0x0B);
        if (restored_ == 0) {
            playEnd(rodClimbOut_, 10);
            if (hero_) hero_->setPosDir(0x167, 0xDD, 4);
            if (flag(kSab) > 3) {
                if (hero_) hero_->walkTo(0x14A, 0xDD, 0, 0);  // CActor_WalkTo2
            } else {
                if (hero_) hero_->walkTo(0x12F, 0xE8, 0, 6);
                playEnd(sabClimbOut_, 10);
                if (sabWalk_) {
                    sabWalk_->setPosZ(347.0, 172.0, 205.0, 33.0);
                    sabWalk_->playRangeNoEvent(0x0D, 0x24, 0x2F);
                }
                moveWait(sabWalk_, 306.0, 173.0, 205.0, 30.0, 0x14);
                if (sabWalk_) sabWalk_->disable();
                sabStand(306.0, 173.0, 205.0, 30.0);
                flag(kSab) = 3;
            }
        } else {
            if (hero_) hero_->show(scene_.clicksEnabled);
            if (flag(kSab) <= 3) {
                sabStand(306.0, 173.0, 205.0, 30.0);
                flag(kSab) = 3;
            }
        }
        walkIn = false;
    }
    if (walkIn) {
        switch (flag(kSab)) {
        case 0:
            if (restored_ == 0 && rodInCam_) {
                scene_.setModal(rodInCam_.get());
                playEnd(rodInCam_.get(), 10);
            }
            break;
        case 1:
            sabStand(433.0, 176.0, 92.0, 49.0);
            break;
        case 2:
        case 3:
            if (flag(kSab) == 2) sabStand(368.0, 171.0, 143.0, 40.0);
            else sabStand(306.0, 173.0, 205.0, 30.0);
            [[fallthrough]];
        case 4:
            if (hatch_) hatch_->showFrame(10);
            if (passage_) passage_->setEvent(0x0B);
            break;
        default:
            break;
        }
        if (bush_) bush_->start(0x0C);
        if (wind_) wind_->play();
        if (hatch_) hatch_->enable();
        if (restored_ == 0) {
            if (hero_) {
                hero_->setPosDir(0x274, 0xFC, 1);
                hero_->walkTo(0x21A, 0x126, 0, 2);  // CActor_WalkTo2
            }
        } else if (hero_) {
            hero_->show(scene_.clicksEnabled);
        }
    }
    setTimerHandler(*windTimer_, 15000, 1, kWindTimer);  // periodisch
}

void Loc08::start100() {
    if (ocean_) ocean_->setVolume(0x46);
    if (surf_) surf_->start(0x0D);
    if (flag(kSab) < 4) {
        if (Layer* ship = scene_.getObject("100_PirateShip")) {
            ship->enable();
            if (ship->area) ship->area->setEvent(1);
        }
    }
    if (auto* bar = scene_.inventory()) bar->setItemEvent(2, 0x66);  // Inventory_SetEvent
    if (prevScene_ == 99) {
        if (flag(kSab) == 4) {
            if (restored_ == 0) {
                if (hero_) {
                    hero_->setPosDir(0x1D3, 0xD0, 0);
                    hero_->walkTo(0x149, 0xE6, 0, 1);
                }
            } else if (hero_) {
                hero_->show(scene_.clicksEnabled);
            }
            if (boatBack_) {
                boatBack_->enable();
                if (boatBack_->area) boatBack_->area->setEvent(100);
            }
        } else {
            if (boat_) {
                boat_->start(0x0D);
                if (boat_->area) boat_->area->setEvent(100);
            }
            if (restored_ == 0) {
                if (hero_) {
                    hero_->setPosDir(0x1D3, 0xD0, 0);
                    hero_->walkTo(0x149, 0xE6, 0, 1);
                }
                if (sabWalk_) {
                    sabWalk_->setPosZ(600.0, 151.0, 221.0, 44.0);
                    sabWalk_->playRangeNoEvent(0x0D, 0x24, 0x2F);
                }
                moveWait(sabWalk_, 341.0, 149.0, 167.0, 50.0, 0x3C);
                if (sabWalk_) sabWalk_->disable();
            } else if (hero_) {
                hero_->show(scene_.clicksEnabled);
            }
            sabStand(346.0, 149.0, 167.0, 48.0);
        }
    } else {
        // Rückkehr mit dem Boot
        flag(kSab) = 4;
        if (restored_ == 0) {
            playRange(boatReturns_, 11, 0, 6);
            if (paddling_) paddling_->play();
            playRange(boatReturns_, 11, 7, 0x13);
            if (paddling_) paddling_->play();
            playRange(boatReturns_, 10, 0x14, -1);
            flag(kBoatBack) = 1;
            if (boatBack_) {
                boatBack_->enable();
                if (boatBack_->area) boatBack_->area->setEvent(100);
            }
            if (hero_) {
                hero_->setPosDir(0x12E, 0xE2, 0);
                hero_->walkTo(0x12E, 0xE6, 0, 0);
            }
            if (wayBack_) wayBack_->setEvent(0x65);
            return;
        }
        if (hero_) hero_->show(scene_.clicksEnabled);
        if (boatBack_) {
            boatBack_->enable();
            if (boatBack_->area) boatBack_->area->setEvent(100);
        }
    }
    if (wayBack_) wayBack_->setEvent(0x65);
}

void Loc08::onEvent(Event& e, int) {
    if (e.param != kAmbient) {
        if (sceneNr_ == 99) onEvent99(e.param);
        else if (sceneNr_ == 100) onEvent100(e.param);
        return;
    }
    AudioBuffer* b = nullptr;
    switch (random(1, 0x10)) {
    case 1: case 2: case 3: case 4: case 5: case 6:
        if (ocean_) ocean_->fadeFreq(random(0x50, 100), 3000);
        break;
    case 7: case 8: case 9: b = seagull_.get(); break;
    case 10: b = thunder_.get(); break;
    case 11: b = thunder2_.get(); break;
    default: break;
    }
    if (b) playSfxRandom3D(*b);
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(10, 30) * 100), 0, kAmbient);
}

void Loc08::onEvent99(int id) {
    switch (id) {
    case 1:
        walkAndComment(0x72, 0xF6, 3, "099_ROD_10");
        return;
    case 3:
        walkAndComment(0x1AA, 0x102, 1, "099_ROD_11");
        return;
    case 5:
        walkAndComment(0x122, 0xDE, 4, "099_ROD_09");
        return;
    case 7:
        if (flag(kSabTrap) == 1) {
            // Fass: Sabrina springt heraus, Rodrigo erschrickt
            if (!walk2(0x1E6, 0xFE, 4)) return;
            scene_.disableInput();
            flag(kSabTrap) = 2;
            if (sabJump_) {
                sabJump_->setPos(0x1AC, 0xB4);                       // vtable +0x14
                sabJump_->moveTo(428.0, 167.0, 92.0, 0.0, 0x0F, 0);  // ohne Warten
            }
            playRange(sabJump_, 11, 0, 0x12);
            if (knockWet_) knockWet_->play();
            if (hero_) hero_->hide(scene_.clicksEnabled);
            if (rodScared_) rodScared_->playRangeNoEvent(11, 0, 7);
            playRange(sabJump_, 11, 0x13, 0x15);
            if (woosh_) woosh_->play();
            playRange(sabJump_, 11, 0x16, 0x29);
            if (rodScared_) rodScared_->playRangeNoEvent(11, 8, -1);
            playRange(sabJump_, 10, 0x2A, -1);
            flag(kSab) = 1;
            if (sabStand_) {
                sabStand_->showFrame(4);
                sabStand_->setPosZ(433.0, 176.0, 92.0, 49.0);
                sabStand_->enable();
            }
            if (rodScared_) rodScared_->disable();
            if (hero_) {
                hero_->setDirection(3);  // 0x00435B80
                hero_->show(scene_.clicksEnabled);
            }
            sabSpeak("099_SAB_01", 0x0C, false);
            scene_.enableInput();
            if (sabStand_ && sabStand_->area) sabStand_->area->setEvent(0x0D);
        } else if (flag(kSabTrap) == 2) {
            walkAndComment(0x1E6, 0xFE, 4, "099_ROD_08");
        }
        return;
    case 9:
        if (flag(kSab) == 1 && flag(kSabTalk) < 4) sabSpeak("099_SAB_08", 0x0C, false);
        if (!walk(0x26A, 0x102, 5)) return;
        if (flag(kLeftShip) == 0 || flag(kForestOnce) != 0) {
            changeScene(0x27, 10);
            return;
        }
        flag(kForestOnce) = 1;
        changeScene(0x3A, 0x10);
        return;
    case 0x0B: {
        // durch die Klappe in den Gang
        if (!walk(0x167, 0xDB, 0)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playEnd(rodClimbIn_, 10);
        const int s = flag(kSab);
        if (s == 2 || s == 3) {
            if (sabStand_) sabStand_->disable();
            if (s == 2) {
                if (sabWalk_) {
                    sabWalk_->setPosZ(368.0, 171.0, 143.0, 40.0);
                    sabWalk_->playRangeNoEvent(0x0D, 0x24, 0x2F);
                }
                moveWait(sabWalk_, 330.0, 171.0, 143.0, 40.0, 0x14);
            } else if (sabWalk_) {
                sabWalk_->setPosZ(306.0, 173.0, 205.0, 30.0);
            }
            if (sabWalk_) sabWalk_->playRangeNoEvent(0x0D, 0x54, 0x5F);
            moveWait(sabWalk_, 347.0, 172.0, 205.0, 33.0, 0x14);
            if (sabWalk_) sabWalk_->disable();
            playEnd(sabClimbIn_, 10);
        }
        changeScene(100, 8);
        return;
    }
    case 0x0D:
        switch (flag(kSab)) {
        case 1: {
            if (!walk2(0x1E6, 0xFE, 3)) return;
            switch (flag(kSabTalk)) {
            case 1:
                flag(kSabTalk) = 2;
                heroSay("099_ROD_01");
                sabSpeak("099_SAB_02", 0x0C, false);
                return;
            case 2:
                flag(kSabTalk) = 3;
                heroSay("099_ROD_02");
                sabSpeak("099_SAB_03", 0x0C, false);
                return;
            case 3:
                scene_.disableInput();
                flag(kSabTalk) = 4;
                heroSay("099_ROD_03");
                wait();  // CGame_Wait ohne neuen Platz (Original)
                walk(0x1E6, 0x102, 0);
                heroSay("099_ROD_04");
                sabSpeak("099_SAB_04", 0x0C, false);
                scene_.enableInput();
                return;
            case 4:
                scene_.disableInput();
                flag(kSabTalk) = 5;
                heroSay("099_ROD_05");
                sabSpeak("099_SAB_05", 0x0C, false);
                if (sabStand_) sabStand_->disable();
                if (sabPoint_) sabPoint_->setPos(0x1B0, 0xB0);
                playEnd(sabPoint_, 10);
                if (sabStand_) {
                    sabStand_->enable();
                    if (sabStand_->area) sabStand_->area->active = true;
                }
                scene_.enableInput();
                return;
            case 5:
                // Sabrina öffnet die Klappe
                heroSay("099_ROD_06");
                scene_.disableInput();
                sabSpeak("099_SAB_06", 0x0C, true);
                if (sabStand_) sabStand_->disable();
                if (sabWalk_) {
                    sabWalk_->setPosZ(433.0, 176.0, 92.0, 49.0);
                    sabWalk_->playRangeNoEvent(0x0D, 0x24, 0x2F);
                }
                moveWait(sabWalk_, 368.0, 0.0, 0.0, 0.0, 0x14);
                if (sabWalk_) sabWalk_->playRangeNoEvent(0x0D, 0x3C, 0x47);
                moveWait(sabWalk_, 368.0, 171.0, 143.0, 40.0, 0x14);
                if (sabWalk_) sabWalk_->disable();
                flag(kSab) = 2;
                playRange(sabLever_, 11, 0, 5);
                if (fixBelt_) fixBelt_->play();
                playRange(sabLever_, 11, 6, 0x0E);
                if (creak_) creak_->play();
                if (hatch_) hatch_->start(11);
                playRange(sabLever_, 10, 0x0F, -1);
                if (sabStand_) {
                    sabStand_->setPosZ(368.0, 171.0, 143.0, 40.0);
                    sabStand_->enable();
                    if (sabStand_->area) sabStand_->area->setEvent(0x0D);
                }
                if (passage_) passage_->setEvent(0x0B);
                scene_.enableInput();
                return;
            default:
                return;
            }
        }
        case 2:
            if (!walk2(0x15F, 0xE9, 6)) return;
            heroSay("099_ROD_07");
            sabSpeak("099_SAB_07", 0x0C, false);
            return;
        case 3:
            if (!walk2(0x12F, 0xE8, 5)) return;
            heroSay("099_ROD_07");
            sabSpeak("099_SAB_07", 0x0C, false);
            return;
        default:
            return;
        }
    case kWindTimer:
        if (wind_) wind_->play();
        setTimerHandler(*bushTimer_, 500, 0, kBushTimer);
        return;
    case kBushTimer:
        if (bush_) bush_->start(0x0C);
        return;
    default:
        return;
    }
}

void Loc08::onEvent100(int id) {
    switch (id) {
    case 1:
        walkAndComment(0x136, 0xEB, 3, "100_ROD_02");
        return;
    case 0x0D:
        if (!walk(0x153, 0xE8, 5)) return;
        heroSayGer("100_ROD_03");
        sabSpeak("100_SAB_02", 0x0D, true);
        return;
    case 100:
        if (flag(kLeftShip) != 0) {
            faceAndSay("100_ROD_01");
            return;
        }
        if (flag(kFlagGiven) == 0) {
            // Original: Warteplatz belegt, aber ohne CGame_Wait
            speak("Loc08", "100_SAB_01", sabStand_, 0x0D, 4, 7, beginWait());
            return;
        }
        // mit Sabrina im Boot hinausfahren
        if (!walk(0x174, 0xE6, 7)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        if (boat_) boat_->disable();
        if (boatFlag_) boatFlag_->disable();
        playRange(boatAway_, 11, 0, 0x18);
        if (sabStand_) sabStand_->disable();
        if (sabWalk_) {
            sabWalk_->playRangeNoEvent(0x0D, 0x0C, 0x17);
            sabWalk_->moveTo(344.0, 146.0, 119.0, 57.0, 10, 0);
        }
        playRange(boatAway_, 11, 0x19, 0x23);
        if (sabWalk_) sabWalk_->disable();
        playRange(boatAway_, 11, 0x24, 0x50);
        if (paddling_) paddling_->play();
        playRange(boatAway_, 11, 0x51, 0x68);
        if (paddling_) paddling_->play();
        playRange(boatAway_, 11, 0x69, 0x80);
        if (paddling_) paddling_->play();
        playRange(boatAway_, 11, 0x81, -1);
        moveWait(boatAway_, -100.0, 0.0, 0.0, 0.0, 10);
        if (boatAway_) boatAway_->disable();
        flag(kSab) = 4;
        changeScene(0x67, 0x17);
        return;
    case 0x65:
        if (!walk(0x1D3, 0xD0, 6)) return;
        changeScene(99, 8);
        return;
    case 0x66:
        // Fahne (Gegenstand 2) am Boot anbringen
        if (!walk(0x174, 0xE6, 7)) return;
        scene_.disableInput();
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playRange(rodFlag_, 11, 0, 0x19);
        if (boat_) boat_->disable();
        if (boatFlag_) boatFlag_->start(0x0D);
        playRange(rodFlag_, 10, 0x1A, -1);
        flag(kFlagGiven) = 1;
        if (auto* bar = scene_.inventory()) bar->remove(2);  // Inventory_Remove
        if (boatFlag_ && boatFlag_->area) boatFlag_->area->setEvent(100);
        if (hero_) hero_->show(scene_.clicksEnabled);
        heroSayGer("100_ROD_04");
        sabSpeak("100_SAB_03", 0x0D, false);
        scene_.enableInput();
        return;
    default:
        // Abweichung: Original zeigt MessageBoxA("Unknown_Event____", "Message") (modal)
        std::fprintf(stderr, "Loc08: Unknown_Event____ (%d)\n", id);
        return;
    }
}

}  // namespace rah
