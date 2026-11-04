// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc19.hpp"

#include "engine/actor.hpp"
#include "engine/area.hpp"

namespace rah {

namespace {
constexpr std::size_t kJasmin = 0x3F28;      // Jasmin begleitet Rodrigo (Loc10)
constexpr std::size_t kJasminOnTree = 0x3F48;  // Ende der Ramil-Abfolge → Jasmin sitzt in Szene 40 auf dem Baum (Loc10), Loc09 setzt 0; hier: Weg links → 21/71
constexpr std::size_t kDoorOpen = 0x3FF0;    // Tür zu Szene 70 offen
constexpr std::size_t kDwarfOut = 0x3FF4;    // Zwerg 1 steht vor dem Tor
constexpr std::size_t kJasTalkOn = 0x3FFC;   // Jasmin an der Kette ansprechbar
constexpr std::size_t kRamilStep = 0x4004;   // Ramil-Auftritt: 1 bereit, 2 erledigt
constexpr std::size_t kJasTalk = 0x4008;     // Gespräch mit Jasmin an der Kette (1…4)
constexpr std::size_t kRamTalk = 0x400C;     // Gespräch mit Ramil (1…4)
constexpr std::size_t kDwarfTalk = 0x4010;   // Gespräch mit Zwerg 1 (1, 2)
constexpr std::size_t kJasHere = 0x4014;     // Jasmin ist in Szene 69 dabei
constexpr std::size_t kJasLeft = 0x4018;     // Jasmin ist gegangen
constexpr std::size_t kLayers = 0x401C;      // Ebenen: 1 Kette+Ramil, 2 Jasmin/Ramil hinten, −1 keine
constexpr std::size_t kTo71 = 0x4028;        // Weg nach 71 offen
constexpr int kAmbient = 1000;
}  // namespace

Loc19::Loc19(GameContext& ctx, int scene, int prevScene) : Game(ctx, 19, scene, prevScene) {
    getViewObjects();
    const char* musicName = nullptr;
    const auto birds = [this] {
        bird3_ = loadSfx("Sfx_Bird3");
        bird2_ = loadSfx("Sfx_Bird2");
        bird_ = loadSfx("Sfx_Bird");
        chirp_ = loadSfx("Sfx_Chirp");
        thunder_ = loadSfx("Sfx_Thunder");
        thunder2_ = loadSfx("Sfx_Thunder2");
    };
    if (sceneNr_ == 0x44) {
        scene_.load("S68", 0);  // DAT_004740B0
        getViewObjects();
        birds();
        toWest_ = scene_.getArea("S68_To67");
        toEast_ = scene_.getArea("S68_To69");
        musicName = "Loc19_Esragoth";
    } else if (sceneNr_ == 0x45) {
        scene_.load("S69", 0);  // DAT_00474088
        getViewObjects();
        guardAmazed_ = loadAnim("S69_WacheStaunt");
        jasRun_ = obj("S69_JasLauf");
        jasChain_ = obj("S69_JasWalkKette");
        ramAngry_ = obj("S69_RamBoese");
        ramWalk_ = obj("S69_RamWalk");
        ramRun_ = obj("S69_RamRun");
        rodStare_ = obj("S69_RodStarrt");
        rodPoint_ = obj("S69_RodZeig");
        thaTalk_ = obj("S69_ThaTalk");
        thaWalk_ = obj("S69_ThaWalk");
        door_ = obj("S69_Tuer");
        hug_ = obj("S69_Umarm");
        guard_ = obj("S69_Wache");
        dwarf1_ = obj("S69_Zwerg1");
        ramBack_ = scene_.getObject("S69_RamBack");
        ramCloseup_ = obj("S69_RamTalkCloseup");
        jasBack_ = scene_.getObject("S69_JasBack");
        jasCloseup_ = obj("S69_JasTalkCloseup");
        jasTalk_ = obj("S69_JasTalk");
        ramBackTalk_ = obj("S69_RamBackTalk");
        ramTalkAnim_ = obj("S69_RamTalkAnim");
        guardAmazedBack_ = scene_.getObject("S69_WacheStauntBack");
        guardTalk_ = obj("S69_WacheTalk");
        to71_ = scene_.getArea("S69_To71");
        to70_ = scene_.getArea("S69_To70");
        to68_ = scene_.getArea("S69_To68");
        stepLeft_ = loadSfxVol("Gen_StepLeft", 0x5A);
        stepRight_ = loadSfxVol("Gen_StepRight", 0x5A);
        doorOpen_ = loadSfx("Sfx_Door_Creaking_Medium");
        doorClose_ = loadSfx("Sfx_Door_Creaking_Medium1");
        birds();
        const auto cue = [](AnimLayer* L, int frame, AudioBuffer* b) {
            if (!L) return;
            AnimCue c;
            c.type = 1;
            c.frame = frame;
            c.buffer = b;
            L->addCue(c);  // CAnimLayer_AddCue 0x00442BB0
        };
        AudioBuffer* l = stepLeft_.get();
        AudioBuffer* r = stepRight_.get();
        cue(ramRun_, 1, l);
        cue(ramRun_, 7, r);
        cue(hug_, 7, r);
        cue(dwarf1_, 3, r);
        cue(dwarf1_, 8, l);
        cue(jasRun_, 4, r);
        cue(jasRun_, 10, l);
        cue(jasChain_, 0x59, r);
        cue(jasChain_, 0x5F, l);
        cue(jasChain_, 0x4D, r);
        cue(jasChain_, 0x53, l);
        cue(ramWalk_, 0x58, r);
        cue(ramWalk_, 0x5D, l);
        cue(ramWalk_, 0x4D, r);
        cue(ramWalk_, 0x52, l);
        cue(ramWalk_, 0x1B, r);
        cue(ramWalk_, 0x21, l);
        cue(thaWalk_, 1, r);
        cue(thaWalk_, 4, l);
        cue(thaWalk_, 0x0C, r);
        cue(thaWalk_, 0x13, l);
        cue(thaWalk_, 0x1A, r);
        cue(thaWalk_, 0x0B, l);
        musicName = "Loc19_Esragoth";
    } else if (sceneNr_ == 0x65) {
        scene_.load("S68", 0);
        getViewObjects();
        scene_.replaceBackground("S68_Back_nd");  // 0x0043FD90(…, 1)
        birds();
        toWest_ = scene_.getArea("S68_To67");
        toEast_ = scene_.getArea("S101_To69");
        musicName = "Loc10_ForestInDarkness";
    }
    if (musicName) music_ = playMusic(musicName);
    ambientTimer_ = std::make_unique<Event>();
    if (music_) {
        music_->setReverb(0x10, 0.111f, 2.767f, 0.224f);  // wie Loc01/Loc08
        music_->setReverbMix(0.1f);
    }
}

void Loc19::destroyLocal() {
    // Reihenfolge wie 0x0041F830
    ambientTimer_.reset();
    guardAmazed_.reset();
    for (auto* b : {&doorOpen_, &doorClose_, &stepLeft_, &stepRight_, &chirp_, &bird3_, &bird2_, &bird_, &thunder_,
                    &thunder2_})
        b->reset();
}

bool Loc19::walk(int x, int y, int dir) { return walkActor(hero_, x, y, dir); }

bool Loc19::walkActor(Actor* a, int x, int y, int dir) {
    const int cb = beginWait();
    if (a) a->walkTo(x, y, cb, dir);  // CActor_WalkTo / WalkTo2
    return wait();
}

bool Loc19::playEnd(AnimLayer* a, unsigned mode) {
    const int cb = beginWait();
    if (a) a->playEndEvent(mode, cb);
    return wait();
}

bool Loc19::playRange(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

bool Loc19::moveWait(Layer* l, double x, double y, double z, double zoom, int steps) {
    const int cb = beginWait();
    if (l) l->moveTo(x, y, z, zoom, steps, cb);  // CLayer_MoveTo 0x004475B0
    return wait();
}

void Loc19::say(AnimLayer* who, const char* key, int from, int to) {
    const int cb = beginWait();
    speak("Loc19", key, who, 0x0C, from, to, cb);
    wait();
}

void Loc19::sayNoActor(const char* key) {
    const int cb = beginWait();
    speak("Loc19", key, cb);  // CGame_SayNoActor 0x004403E0
    wait();
}

void Loc19::enableAt(Layer* l, double x, double y, double z, double zoom) {
    if (!l) return;
    l->enable();
    l->setPosZ(x, y, z, zoom);  // CLayer_SetPosZ 0x004474C0
}

void Loc19::setActive(Layer* l, bool on) {
    if (l && l->area) l->area->active = on;  // Bereich +0xB8
}

void Loc19::ambient(int lo, int hi, std::initializer_list<AudioBuffer*> sounds) {
    const int n = static_cast<int>(sounds.size());
    const int k = random(1, n);
    if (k >= 1 && k <= n) {
        AudioBuffer* b = sounds.begin()[k - 1];
        const int vol = random(0x32, 100);  // Zufallsfolge wie im Original, auch ohne Ton
        if (b) {
            b->setVolume(vol);
            playSfxRandom3D(*b);
        }
    }
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(lo, hi) * 1000), 0, kAmbient);
}

void Loc19::start() {
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60 (geprüft)
    if (sceneNr_ == 0x44) {
        if (music_) music_->fadeIn(80, 2000);
        setEventProc(&Loc19::onEvent68);
        if (restored_ == 0) {
            int dir = -1;
            if (prevScene_ == 0x26) {
                if (hero_) {
                    hero_->show(scene_.clicksEnabled);
                    hero_->setPosDir(4, 0x14B, 6);
                }
                if (flag(kJasmin) != 0 && second_) {
                    second_->show(scene_.clicksEnabled);
                    second_->setPosDir(4, 0x138, 6);
                    second_->walkTo(0x153, 0x13E, 0, 5);
                }
                dir = 6;
            } else if (prevScene_ == 0x45) {
                if (hero_) {
                    hero_->show(scene_.clicksEnabled);
                    hero_->setPosDir(0x22A, 0x111, 2);
                }
                dir = 2;
            }
            if (dir >= 0 && hero_) hero_->walkTo(0x10A, 0x145, 0, dir);
        } else {
            if (hero_) hero_->show(scene_.clicksEnabled);
            if (prevScene_ != 0 && flag(kJasmin) != 0 && second_) second_->show(scene_.clicksEnabled);
        }
        scene_.enableInput();
        if (toWest_) toWest_->setEvent(10);
        if (toEast_) toEast_->setEvent(11);
        scene_.enableInput();
    } else if (sceneNr_ == 0x45) {
        start69();
    } else if (sceneNr_ == 0x65) {
        if (music_) music_->fadeIn(80, 2000);
        setEventProc(&Loc19::onEvent101);
        if (hero_) {
            hero_->show(scene_.clicksEnabled);
            hero_->blockRect(0x168, 200, 0x276, 0x159);
        }
        scene_.disableInput();
        if (restored_ == 0) {
            if (hero_) hero_->setPosDir(7, 0x124, 7);
            walk(0x101, 0x138, 6);
            say(hero_, "101_ROD_01", 0, 8);
        }
        scene_.enableInput();
        if (toWest_) toWest_->setEvent(10);
        if (toEast_) toEast_->setEvent(11);
    }
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(2, 10) * 1000), 0, kAmbient);
}

void Loc19::start69() {
    if (music_) music_->fadeIn(80, 2000);
    setEventProc(&Loc19::onEvent69);
    if (door_) door_->enable();
    if (guard_) guard_->enable();
    if (flag(kLayers) == 1) {
        enableAt(jasChain_, 179.0, 163.0, 43.0, 106.0);
        if (jasChain_) jasChain_->seekFrame(0x0C);
        enableAt(ramTalkAnim_, 225.0, 205.0, 65.0, 90.0);
        setActive(jasChain_, true);
        if (hero_) hero_->blockRect(0xB7, 0x12E, 0x114, 0x141);
    } else if (flag(kLayers) == 2) {
        if (flag(kRamTalk) < 3) enableAt(jasTalk_, 410.0, 188.0, 47.0, 82.0);
        if (ramBackTalk_) ramBackTalk_->enable();
        setActive(ramBackTalk_, true);
        if (ramBackTalk_) ramBackTalk_->setPosZ(425.0, 214.0, 42.0, 82.0);
    }
    if (restored_ == 0) {
        if (prevScene_ == 0x44) {
            if (hero_) {
                hero_->show(scene_.clicksEnabled);
                hero_->setPosDir(0x277, 0xF4, 2);
            }
            if (flag(kJasmin) == 0) {
                walk(0x22E, 0x104, 2);
            } else {
                // Ankunft mit Jasmin: die Wache spricht sie an
                flag(kJasmin) = 0;
                flag(kJasHere) = 1;
                if (second_) {
                    second_->show(scene_.clicksEnabled);
                    second_->setPosDir(0x277, 0xF4, 2);
                    second_->walkToClick(0x218, 0x108);
                }
                walk(0x22E, 0x104, 2);
                speak("Loc19", "069_ZW2_01", 0);  // CGame_SayNoActor ohne Warten
                playRange(guard_, 11, 0, 0x12);
                sayNoActor("069_JAS_01");
                say(guard_, "069_ZW2_02", 0x18, 0x1E);
                sayNoActor("069_JAS_02");
                say(hero_, "069_ROD_02", 0, 8);
                if (guard_) guard_->playRange(11, 0, 0x12, 0);
                if (dwarf1_) {
                    dwarf1_->setPosZ(-13.0, 210.0, 67.0, 100.0);
                    dwarf1_->playRangeNoEvent(0x0D, 0, 0x0B);
                }
                moveWait(dwarf1_, 36.0, 210.0, 67.0, 100.0, 0x0C);
                if (dwarf1_) dwarf1_->showFrame(0x0C);
                say(dwarf1_, "069_ZW1_02", 0x0C, 0x10);
                setActive(second_, true);
                flag(kDwarfOut) = 1;
            }
            if (hero_) hero_->blockRect(0x124, 0xE2, 0x14C, 0x125);
        } else if (prevScene_ == 0x46) {
            // aus dem Tor (Szene 70)
            if (hero_) hero_->show(scene_.clicksEnabled);
            scene_.disableInput();
            if (hero_) hero_->setPosDir(0x13C, 0x115, 0);
            if (doorOpen_) doorOpen_->play();
            playEnd(door_, 11);
            walk(0x14C, 0x138, 0);
            if (doorClose_) doorClose_->play();
            playRange(door_, 11, 10, 0);
            if (guard_) guard_->enable();
            scene_.enableInput();
        } else if (prevScene_ == 0x47) {
            if (hero_) {
                hero_->show(scene_.clicksEnabled);
                hero_->setPosDir(0x27, 0x12A, 7);
            }
            walk(0x4E, 0x137, 7);
            if (hero_) hero_->blockRect(0x124, 0xE2, 0x14C, 0x125);
        }
    } else {
        if (hero_) {
            hero_->show(scene_.clicksEnabled);
            if (prevScene_ != 0x46) hero_->blockRect(0x124, 0xE2, 0x14C, 0x125);
        }
        if (flag(kJasHere) != 0 && flag(kLayers) == -1 && second_) second_->show(scene_.clicksEnabled);
        if (flag(kDwarfOut) != 0) {
            enableAt(dwarf1_, 36.0, 210.0, 67.0, 100.0);
            if (dwarf1_) dwarf1_->showFrame(0x0C);
        }
    }
    if (to71_) to71_->setEvent(1);
    if (to70_) to70_->setEvent(2);
    if (to68_) to68_->setEvent(3);
    if (second_ && second_->area) {
        second_->area->setEvent(7);
        second_->area->active = true;
    }
    if (jasChain_ && jasChain_->area) jasChain_->area->setEvent(7);
    if (ramBackTalk_ && ramBackTalk_->area) ramBackTalk_->area->setEvent(8);
    if (dwarf1_ && dwarf1_->area) dwarf1_->area->setEvent(4);
    if (guard_ && guard_->area) guard_->area->setEvent(5);
    setActive(guard_, flag(kRamTalk) != 1);
    if (to70_) to70_->active = flag(kDoorOpen) != 0;
    if (to71_) to71_->active = flag(kTo71) != 0;
    scene_.enableInput();
    if (hero_) hero_->blockRect(0x1EC, 0x104, 0x1FD, 0x10B);
}

void Loc19::closeup(const char* key, int who, std::uint32_t before, std::uint32_t after, bool invalidate) {
    const auto areaOn = [](Layer* l) { return l && l->area && l->area->active; };
    const bool guardArea = areaOn(guard_), dwarfArea = areaOn(dwarf1_), chainArea = areaOn(jasChain_);
    setActive(guard_, false);
    setActive(dwarf1_, false);
    setActive(jasChain_, false);
    // Sichtbarkeit (+0x1A4) merken und ausblenden – Reihenfolge wie im Original
    Layer* layers[] = {jasRun_, jasChain_, ramAngry_, ramWalk_, rodStare_, rodPoint_, thaTalk_, thaWalk_, door_,
                       hug_, guard_, dwarf1_, ramBack_, ramCloseup_, jasBack_, jasCloseup_};
    bool was[std::size(layers)];
    for (std::size_t i = 0; i < std::size(layers); ++i) {
        was[i] = layers[i] && layers[i]->active;
        if (layers[i]) layers[i]->disable();
    }
    const bool heroOn = hero_ && hero_->active;
    if (hero_) hero_->hide(scene_.clicksEnabled);
    const bool secondOn = second_ && second_->active;
    if (second_) second_->hide(scene_.clicksEnabled);
    Layer* bg = scene_.background();
    const bool bgOn = bg && bg->active;
    if (bg) bg->disable();
    Layer* late[] = {jasTalk_, ramBackTalk_, ramTalkAnim_};
    bool wasLate[3];
    for (std::size_t i = 0; i < 3; ++i) {
        wasLate[i] = late[i] && late[i]->active;
        if (late[i]) late[i]->disable();
    }
    Area* areas[] = {to71_, to70_, to68_};
    bool areaWas[3];
    for (std::size_t i = 0; i < 3; ++i) {
        areaWas[i] = areas[i] && areas[i]->active;
        if (areas[i]) areas[i]->active = false;
    }
    scene_.disableInput();
    Layer* back = nullptr;
    AnimLayer* talker = nullptr;
    if (who == 0) {
        back = ramBack_;
        talker = ramCloseup_;
    } else if (who == 1) {
        back = jasBack_;
        talker = jasCloseup_;
    } else if (who == 2) {
        scene_.setModal(guardAmazed_.get());
        playEnd(guardAmazed_.get(), 10);
        back = guardAmazedBack_;
        talker = guardTalk_;
    }
    if (talker) {
        if (back) back->enable();
        if (who != 2) talker->enable();
        if (before) delay(before);
        say(talker, key, 0, 4);
        if (after) delay(after);
        if (back) back->disable();
        talker->disable();
    }
    scene_.enableInput();
    setActive(dwarf1_, dwarfArea);
    setActive(guard_, guardArea);
    setActive(jasChain_, chainArea);
    if (hero_) heroOn ? hero_->show(scene_.clicksEnabled) : hero_->hide(scene_.clicksEnabled);
    if (second_) secondOn ? second_->show(scene_.clicksEnabled) : second_->hide(scene_.clicksEnabled);
    if (bg) bgOn ? bg->enable() : bg->disable();
    for (std::size_t i = 0; i < std::size(layers); ++i)
        if (layers[i]) was[i] ? layers[i]->enable() : layers[i]->disable();
    for (std::size_t i = 0; i < 3; ++i)
        if (late[i]) wasLate[i] ? late[i]->enable() : late[i]->disable();
    for (std::size_t i = 0; i < 3; ++i)
        if (areas[i]) areas[i]->active = areaWas[i];
    if (invalidate) scene_.invalidateAll();  // CSzene_InvalidateAll 0x0044C1E0
}

void Loc19::onEvent68(Event& e, int) {
    switch (e.param) {
    case 10:
        if (flag(kJasmin) != 0) {
            // Jasmin will nicht zurück
            walk(4, 0x14B, 2);
            speak("Loc19", "068_JAS_01", 0);
            walk(0x10A, 0x145, 6);
            return;
        }
        if (walk(4, 0x14B, 2)) changeScene(0x26, 10);
        return;
    case 11:
        if (!walk(0x1B3, 299, 6)) return;
        if (flag(kJasmin) != 0 && second_) second_->walkToClick(0x22A, 0x10C);
        scene_.disableInput();
        walk(0x22A, 0x111, 6);
        changeScene(0x45, 0x13);
        return;
    case kAmbient:
        ambient(2, 10, {bird_.get(), bird2_.get(), bird3_.get(), chirp_.get()});
        return;
    default:
        return;
    }
}

void Loc19::onEvent101(Event& e, int) {
    switch (e.param) {
    case 10:
        if (walk(7, 0x124, 2)) changeScene(0x26, 10);
        return;
    case 11:
        walkAndComment(0x101, 0x138, 6, "101_ROD_02");
        return;
    case kAmbient:
        ambient(5, 15, {thunder_.get(), thunder2_.get()});
        return;
    default:
        return;
    }
}

void Loc19::onEvent69(Event& e, int) {
    const int id = e.param;
    if (id > kAmbient) return;
    if (id == kAmbient) {
        ambient(2, 10, {bird_.get(), bird2_.get(), bird3_.get(), chirp_.get()});
        return;
    }
    switch (id) {
    case 1:
        if (flag(kJasminOnTree) != 0) {
            if (walk(0x11, 0x114, 2)) changeScene(0x47, 0x15);
            return;
        }
        if (flag(kRamilStep) == 1) {
            // Ramil bringt Jasmin an der Kette
            scene_.disableInput();
            if (hero_) hero_->walkTo(0x112, 0x14D, 0, 3);
            if (jasChain_) jasChain_->enable();
            if (ramWalk_) {
                ramWalk_->enable();
                ramWalk_->setPosZ(-78.0, 250.0, 17.0, 118.0);
            }
            if (jasChain_) jasChain_->setPosZ(-90.0, 174.0, 17.0, 130.0);
            if (ramWalk_) ramWalk_->playRangeNoEvent(0x0D, 0, 7);
            if (jasChain_) {
                jasChain_->playRangeNoEvent(0x0D, 0x54, 0x5F);
                jasChain_->moveTo(179.0, 163.0, 43.0, 106.0, 0x28, 0);
            }
            moveWait(ramWalk_, 225.0, 206.0, 65.0, 90.0, 0x28);
            if (jasChain_) jasChain_->stop(0);  // vtable +0x24
            if (ramWalk_) ramWalk_->stop(0);
            if (ramWalk_) ramWalk_->seekFrame(0);
            if (jasChain_) jasChain_->seekFrame(0x0C);
            if (hero_) hero_->hide(scene_.clicksEnabled);
            if (ramWalk_) ramWalk_->disable();
            closeup("069_RAM_23", 0, 0, 0, false);
            closeup("069_JAS_23", 1, 1000, 500, true);
            enableAt(ramTalkAnim_, 225.0, 205.0, 65.0, 90.0);
            setActive(jasChain_, true);
            if (rodStare_) rodStare_->setPosZ(243.0, 172.0, 24.0, 102.0);
            playRange(rodStare_, 11, 0, 6);
            delay(500);
            playRange(rodStare_, 11, 7, -1);
            if (rodStare_) rodStare_->disable();
            closeup("069_JAS_20", 1, 1000, 500, true);
            flag(kLayers) = 1;
            if (hero_) {
                hero_->show(scene_.clicksEnabled);
                hero_->blockRect(0xB7, 0x12E, 0x114, 0x141);
            }
            scene_.enableInput();
            flag(kJasTalkOn) = 1;
            flag(kRamilStep) = 2;
            return;
        }
        if (!walk(0x75, 0x13C, 2)) return;
        say(hero_, "069_ROD_24", 0, 4);
        return;
    case 2:
        // Tor zu Szene 70
        if (flag(kDoorOpen) == 0) return;
        if (!walk(0x16F, 0x137, 3)) return;
        scene_.disableInput();
        if (doorOpen_) doorOpen_->play();
        playEnd(door_, 11);
        walk(0x13C, 0x115, 4);
        changeScene(0x46, 0x14);
        return;
    case 3:
        if (flag(kJasLeft) == 0) {
            if (!walk(0x23F, 0x106, 6)) return;
            const int cb = beginWait();
            if (flag(kLayers) == 2) speak("Loc19", "069_JAS_03", jasTalk_, 0x0C, 0, 8, cb);
            else speak("Loc19", "069_JAS_03", cb);
            wait();
            walk(0x212, 0x11D, 2);
            return;
        }
        if (flag(kJasHere) != 0) return;
        if (flag(kDoorOpen) == 0) {
            // Thalion öffnet das Tor
            scene_.disableInput();
            say(hero_, "069_ROD_04", 0, 8);
            sayNoActor("069_THA_04");
            if (hero_) hero_->walkTo(0x16F, 0x137, 0, 3);
            enableAt(thaWalk_, 288.0, 196.0, 93.0, 65.0);
            if (doorOpen_) doorOpen_->play();
            playEnd(door_, 11);
            closeup("069_ZW2_04", 2, 0, 0, true);
            say(hero_, "069_ROD_05", 0, 8);
            if (thaWalk_) thaWalk_->disable();
            say(thaTalk_, "069_THA_05", 0, 8);
            if (thaTalk_) thaTalk_->disable();
            playRange(thaWalk_, 11, 0x0B, 0x19);
            flag(kDoorOpen) = 1;
            if (to70_) to70_->active = true;
            flag(kDwarfOut) = 0;
            flag(kLayers) = -1;
            if (hero_) hero_->unblockRect(0x1A2, 0x138, 0x1DB, 0x150);
            scene_.enableInput();
            changeScene(0x46, 0x14);
            return;
        }
        if (walk(0x277, 0xF4, 6)) changeScene(0x44, 0x13);
        return;
    case 4:
        // Zwerg 1
        if (flag(kDwarfOut) == 0) return;
        if (flag(kDwarfTalk) == 1) {
            if (!walk(0x92, 0x135, 2)) return;
            scene_.disableInput();
            say(hero_, "069_ROD_06", 0, 8);
            say(dwarf1_, "069_ZW1_06", 0x0C, 0x10);
            if (second_) second_->walkTo(0x1A8, 0x137, 0, 1);
            if (ramRun_) {
                ramRun_->setPosZ(-97.0, 264.0, 1.0, 118.0);
                ramRun_->start(0x0D);
            }
            moveWait(ramRun_, 359.0, 223.0, 1.0, 97.0, 0x1E);
            if (ramRun_) ramRun_->disable();
            if (second_) second_->hide(scene_.clicksEnabled);
            if (hug_) hug_->setPosZ(396.0, 160.0, 47.0, 82.0);
            playEnd(hug_, 10);
            if (hug_) hug_->disable();
            if (hero_) hero_->walkTo(0x15A, 0x13D, 0, 6);
            enableAt(jasTalk_, 410.0, 188.0, 47.0, 82.0);
            if (ramBackTalk_) ramBackTalk_->enable();
            setActive(ramBackTalk_, true);
            if (ramBackTalk_) ramBackTalk_->setPosZ(425.0, 214.0, 42.0, 82.0);
            sayNoActor("069_RAM_06");
            delay(1000);
            playRange(ramBackTalk_, 11, 0, 10);
            say(ramBackTalk_, "069_RAM_07", 10, 0x0C);
            playRange(ramBackTalk_, 11, 10, 0);
            flag(kLayers) = 2;
            if (hero_) hero_->blockRect(0x177, 0x12E, 0x1DB, 0x150);
            scene_.enableInput();
            flag(kDwarfTalk) = 2;
            return;
        }
        if (!walk(0x92, 0x135, 2)) return;
        say(hero_, "069_ROD_08", 0, 8);
        say(dwarf1_, "069_ZW1_08", 0x0C, 0x10);
        return;
    case 5:
        if (!walk(0x1D3, 0x124, 5)) return;
        say(hero_, "069_ROD_14", 0, 8);
        say(guard_, "069_ZW2_14", 0, 2);
        return;
    case 7:
        if (flag(kJasTalkOn) != 0) {
            if (!walk(0x112, 0x14D, 3)) return;
            switch (flag(kJasTalk)) {
            case 1:
                scene_.disableInput();
                if (hero_) hero_->hide(scene_.clicksEnabled);
                if (rodPoint_) rodPoint_->setPosZ(230.0, 171.0, 1.0, 103.0);
                playEnd(rodPoint_, 11);
                sayNoActor("069_ROD_17");
                playRange(rodPoint_, 10, 5, 0);
                if (hero_) hero_->show(scene_.clicksEnabled);
                playRange(ramTalkAnim_, 11, 6, 0x0D);
                closeup("069_JAS_17", 1, 0, 500, true);
                if (ramTalkAnim_) ramTalkAnim_->showFrame(0);
                setActive(jasChain_, true);
                scene_.enableInput();
                flag(kJasTalk) = 2;
                return;
            case 2:
            case 3: {
                const bool two = flag(kJasTalk) == 2;
                scene_.disableInput();
                sayNoActor(two ? "069_ROD_18" : "069_ROD_19");
                closeup(two ? "069_JAS_18" : "069_JAS_19", 1, 0, 500, true);
                setActive(jasChain_, true);
                scene_.enableInput();
                flag(kJasTalk) = two ? 3 : 4;
                return;
            }
            case 4:
                // Ramil nimmt Jasmin mit, Thalion holt Rodrigo
                scene_.disableInput();
                sayNoActor("069_ROD_21");
                playRange(ramTalkAnim_, 11, 6, 0x0D);
                say(ramTalkAnim_, "069_RAM_21", 0x0E, 0x12);
                playRange(ramTalkAnim_, 11, 0x0D, 6);
                enableAt(thaWalk_, 288.0, 196.0, 93.0, 65.0);
                if (doorOpen_) doorOpen_->play();
                if (door_) door_->playEndEvent(11, 0);
                sayNoActor("069_THA_21");
                sayNoActor("069_JAS_21");
                playRange(ramTalkAnim_, 11, 6, 0x0D);
                say(ramTalkAnim_, "069_RAM_22", 0x0E, 0x12);
                playRange(ramTalkAnim_, 11, 0x0D, 6);
                if (jasChain_) jasChain_->disable();
                if (jasRun_) {
                    jasRun_->setPosZ(178.0, 178.0, 36.0, 102.0);
                    jasRun_->fps = 0x14;    // +0x54
                    jasRun_->dirty = true;  // +0x1A0
                    jasRun_->start(0x0D);
                }
                moveWait(jasRun_, 593.0, 174.0, 157.0, 70.0, 0x23);
                if (jasRun_) jasRun_->disable();
                delay(500);
                if (ramTalkAnim_) ramTalkAnim_->disable();
                if (ramWalk_) {
                    ramWalk_->enable();
                    ramWalk_->seekFrame(0x1C);
                    ramWalk_->setPosZ(211.0, 209.0, 48.0, 96.0);
                    ramWalk_->playRangeNoEvent(0x0D, 0x18, 0x23);
                }
                moveWait(ramWalk_, -69.0, 245.0, 12.0, 118.0, 0x28);
                if (ramWalk_) ramWalk_->disable();
                playRange(thaWalk_, 11, 0x0B, 0x19);
                if (doorClose_) doorClose_->play();
                playRange(door_, 11, 10, 0);
                if (thaWalk_) thaWalk_->disable();
                faceAndSay("069_ROD_22");
                flag(kJasminOnTree) = 1;
                flag(kJasHere) = 0;
                flag(kLayers) = -1;
                if (hero_) hero_->unblockRect(0xB7, 0x12E, 0x114, 0x141);
                scene_.enableInput();
                return;
            default:
                return;
            }
        }
        if (flag(kJasLeft) != 0) return;
        say(hero_, "069_ROD_16", 0, 8);
        say(second_, "069_JAS_16", 0, 8);
        return;
    case 8:
        // Ramil (von hinten)
        switch (flag(kRamTalk)) {
        case 1:
            if (!walk(0x15A, 0x13D, 7)) return;
            scene_.disableInput();
            say(hero_, "069_ROD_09", 0, 8);
            playRange(ramBackTalk_, 11, 0, 10);
            say(ramBackTalk_, "069_RAM_09", 10, 0x0C);
            say(hero_, "069_ROD_10", 0, 8);
            playRange(ramBackTalk_, 11, 10, 0);
            setActive(ramBackTalk_, true);
            flag(kRamTalk) = 2;
            setActive(guard_, true);
            scene_.enableInput();
            return;
        case 2:
            if (!walk(0x15A, 0x13D, 7)) return;
            scene_.disableInput();
            say(hero_, "069_ROD_11", 0, 8);
            playRange(ramBackTalk_, 11, 0, 10);
            say(ramBackTalk_, "069_RAM_11", 10, 0x0C);
            playRange(ramBackTalk_, 11, 10, 0);
            scene_.disableInput();
            say(jasTalk_, "069_JAS_11", 0, 8);
            if (jasTalk_) jasTalk_->disable();
            if (second_) {
                second_->show(scene_.clicksEnabled);
                second_->setPosDir(0x1A8, 0x137, 1);
            }
            if (hero_) {
                hero_->unblockRect(0x177, 0x12E, 0x1DB, 0x150);
                hero_->blockRect(0x1A2, 0x138, 0x1DB, 0x150);
            }
            walkActor(second_, 4, 0x15C, 1);
            setActive(second_, false);
            if (second_) second_->hide(scene_.clicksEnabled);
            flag(kJasLeft) = 1;
            flag(kJasHere) = 0;
            setActive(ramBackTalk_, true);
            flag(kRamTalk) = 3;
            scene_.enableInput();
            return;
        case 3:
        case 4: {
            const bool three = flag(kRamTalk) == 3;
            if (!walk(0x172, 0x13D, 7)) return;
            scene_.disableInput();
            say(hero_, three ? "069_ROD_12" : "069_ROD_13", 0, 8);
            closeup(three ? "069_RAM_12" : "069_RAM_13", 0, 0, 0, true);
            setActive(ramBackTalk_, true);
            flag(kRamTalk) = 4;
            scene_.enableInput();
            return;
        }
        default:
            return;
        }
    default:
        return;
    }
}

}  // namespace rah
