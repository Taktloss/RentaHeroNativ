// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc01.hpp"

#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
constexpr std::size_t kKanalHere = 0x3E94;  // Kanalwesen ist da (Start 1)
constexpr std::size_t kKanalTalk = 0x3E98;  // Gesprächsstufe (Start 1)
constexpr std::size_t kSackDropped = 0x3E9C;
constexpr std::size_t kTalkStage4 = 0x3EC4;
constexpr int kBobHandler = 0x00407DC0;     // Funktionszeiger als Handler-Wert
constexpr int kAmbientEvent = 200;
constexpr int kItemEvent = 0x65;            // Gegenstand 13 benutzt
}  // namespace

Loc01::Loc01(GameContext& ctx, int scene, int prevScene) : Game(ctx, 1, scene, prevScene) {
    // +0x24 = 0 … = 1 (GAME.md): ohne Leser während des Ctors
    g_.fnHandlers[kBobHandler] = {this, [this](Event&, int) { bobGlider(); }};
    getViewObjects();
    if (sceneNr_ == 46) {
        if (prevScene_ == 47) flag(kKanalHere) = 0;
        scene_.load("46", flag(kKanalHere) != 0 ? 1u : 0u);
        getViewObjects();
        if (flag(kKanalHere) == 0) {
            scene_.replaceBackground("back46_ok");  // 0x0043FD90(name, 1)
            kanalOut_ = loadAnim("KanalRausCam");
            kanalIn_ = loadAnim("KanalReinCam");
        } else {
            kanalTalk_ = dynamic_cast<AnimLayer*>(scene_.getObject("KanalTalk"));
        }
        clothes_ = loadSfxVol("Sfx_Clothes_Scratched", 70);
    } else if (sceneNr_ == 8) {
        scene_.load("8", 0xFFFF);
        getViewObjects();
        if (hero_) hero_->hide(scene_.clicksEnabled);
        flyAll_ = loadAnim("FlyGanzWegCam");
        gliderPass_ = loadSfxVol("Sfx_Glider_PassingBy", 70);
    } else if (sceneNr_ == 9) {
        scene_.load("9", 0xFFFF);
        getViewObjects();
        pressLeverTop_ = dynamic_cast<AnimLayer*>(scene_.getObject("PressLeverObenLayer"));
        lever_ = scene_.getObject("HebelOben");
        clickClack_ = loadSfxVol("Sfx_ClickelDiClack", 70);
    } else if (sceneNr_ == 1007) {
        scene_.load("7b", 0xFFFF);
        getViewObjects();
        loadLiftAnims();
        pressLeverMid_ = dynamic_cast<AnimLayer*>(scene_.getObject("PressLeverMitteLayer"));
        lever_ = scene_.getObject("Hebel");
        lowerFloor_ = scene_.getArea("007b_UntererStock");
        liftDown_ = scene_.getArea("007b_LiftRunter");
        plant2_ = scene_.getArea("007b_Pflanze");
        stairs2_ = scene_.getArea("007b_Treppe");
        lift2_ = scene_.getArea("007b_Lift");
        sign2_ = scene_.getArea("007b_Schild");
        clickClack_ = loadSfxVol("Sfx_ClickelDiClack", 70);
        lift_ = loadSfxVol("Sfx_Lift", 70);
    } else {
        sceneNr_ = 7;
        unsigned mask = flag(kKanalHere) != 0 ? 1u : 0u;
        auto* bar = scene_.inventory();
        if (bar && bar->has(7)) mask |= 2;
        if (flag(kKanalHere) != 0 && bar && bar->has(13)) mask |= 4;
        scene_.load("7a", mask);
        getViewObjects();
        loadCamAnims();
        glider_ = scene_.getObject("GliderLayer");
        sack_ = scene_.getObject("SackLayer");
        if (mask & 1) kanalLowTalk_ = dynamic_cast<AnimLayer*>(scene_.getObject("KanalLowTalk"));
        if (mask & 2) sackAnim_ = dynamic_cast<AnimLayer*>(scene_.getObject("SackAnimLayer"));
        if (mask & 4) {
            troot_ = dynamic_cast<AnimLayer*>(scene_.getObject("TrootLayer"));
            kanalLowRun_ = dynamic_cast<AnimLayer*>(scene_.getObject("KanalLowRun"));
        }
        kanalGone_ = scene_.getObject("KanalWegLayer");
        gliderArea_ = scene_.getArea("GliderLayer");
        door_ = scene_.getArea("007a_Tuer");
        stairs_ = scene_.getArea("007a_Treppe");
        plant_ = scene_.getArea("007a_Pflanze");
        sign_ = scene_.getArea("007a_Schild");
        bubble_ = loadSfx("Sfx_Bubble");
        horn_ = loadSfx("Sfx_SignalHorn_Modern");
        moneyBag_ = loadSfxVol("Sfx_MoneyBag", 70);
        woosh_ = loadSfxVol("Sfx_Woosh", 70);
        tock_ = loadSfxVol("Sfx_Tock", 50);
        gliderPass_ = loadSfxVol("Sfx_Glider_PassingBy", 70);
        stepLeft_ = loadSfxVol("Gen_StepLeft", 75);
        stepRight_ = loadSfxVol("Gen_StepRight", 75);
    }
    bird_ = loadSfx("Sfx_Bird");
    bird3_ = loadSfx("Sfx_Bird3");
    gliderPass3_ = loadSfx("Sfx_Glider_PassingBy3");
    wind_ = loadSfx("Sfx_Wind2");
    hack_ = loadSfxVol("Sfx_Work_Hack2", 70);
    ambient_ = std::make_unique<Event>();
    music_ = playMusic("Loc01_Smashville");
    if (music_) {
        music_->setReverb(0x10, 0.111f, 2.767f, 0.224f);  // wie Loc37
        music_->setReverbMix(0.1f);
    }
}

Loc01::~Loc01() {
    shutdown();
    const auto it = g_.fnHandlers.find(kBobHandler);
    if (it != g_.fnHandlers.end() && it->second.owner == this) g_.fnHandlers.erase(it);
}

void Loc01::destroyLocal() {
    // Reihenfolge wie 0x00405B90
    ambient_.reset();
    flyAway_.reset();
    returnHome_.reset();
    stairHomeDown_.reset();
    stairHomeUp_.reset();
    stairLiftUp_.reset();
    stairLiftDown_.reset();
    liftMidUp_.reset();
    liftMidDown_.reset();
    liftUpMid_.reset();
    liftDownMid_.reset();
    flyAll_.reset();
    kanalOut_.reset();
    kanalIn_.reset();
    bubble_.reset();
    horn_.reset();
    moneyBag_.reset();
    woosh_.reset();
    tock_.reset();
    clickClack_.reset();
    gliderPass_.reset();
    lift_.reset();
    stepLeft_.reset();
    stepRight_.reset();
    bird_.reset();
    bird3_.reset();
    gliderPass3_.reset();
    wind_.reset();
    hack_.reset();
    // Original-Fehler: +0x12C wird nie gelöscht (Puffer bleibt beim Tonsystem angemeldet)
    (void)clothes_.release();
}

void Loc01::loadCamAnims() {
    const bool mk = flag(kKanalHere) != 0;
    flyAway_ = loadAnim(mk ? "FlyWegCam_mk" : "FlyWegCam_ok");
    returnHome_ = loadAnim(mk ? "ReturnHomeCam_mk" : "ReturnHomeCam_ok");
    stairHomeDown_ = loadAnim(mk ? "StairHomeDownCam_mk" : "StairHomeDownCam_ok");
    stairHomeUp_ = loadAnim(mk ? "StairHomeUpCam_mk" : "StairHomeUpCam_ok");
    stairLiftUp_ = loadAnim(mk ? "StairLiftRaufCam_mk" : "StairLiftRaufCam_ok");
    stairLiftDown_ = loadAnim(mk ? "StairLiftRunterCam_mk" : "StairLiftRunterCam_ok");
}

void Loc01::loadLiftAnims() {
    liftMidUp_ = loadAnim("LiftMitteRauf");
    liftUpMid_ = loadAnim("LiftRaufMitte");
    const bool mk = flag(kKanalHere) != 0;
    liftMidDown_ = loadAnim(mk ? "LiftMitteRunter_mk" : "LiftMitteRunter_ok");
    liftDownMid_ = loadAnim(mk ? "LiftRunterMitte_mk" : "LiftRunterMitte_ok");
}

bool Loc01::playWait(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

void Loc01::stopBackground() {
    // UNVERIFIED: vtable +0x24 einer statischen Ebene; in Szene 8 ist der Hintergrund ein Video
    if (auto* a = dynamic_cast<AnimLayer*>(scene_.background())) a->stop(0);
}

void Loc01::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    setEventProc(&Loc01::onEvent);
    auto* bar = scene_.inventory();
    if (((sceneNr_ == 7 && flag(kKanalHere) != 0) || sceneNr_ == 46) && bar) bar->setItemEvent(13, kItemEvent);
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();
    if (hero_) hero_->show(scene_.clicksEnabled);
    const auto areaEvent = [this](const char* name, int ev) {
        // UNVERIFIED: das Original prüft nicht auf fehlende Bereiche
        if (Area* a = scene_.getArea(name)) a->setEvent(ev);
    };
    int walkX = 0, walkY = 0;  // LAB_0040656A: CActor_WalkToClick
    bool walk = false;
    if (sceneNr_ == 8) {
        if (hero_) {
            hero_->hide(scene_.clicksEnabled);
            scene_.enableInput();
        }
        areaEvent("008_Office", 30);
        areaEvent("008_Kloster", 31);
        areaEvent("008_Kneipe", 32);
        areaEvent("008_Raus", 33);
        auto* bg = dynamic_cast<AnimLayer*>(scene_.background());
        if (prevScene_ == 7) {
            if (bg) bg->start(0xD);  // CAnimLayer_Loop 0x00443230
        } else {
            if (bg) bg->playRangeNoEvent(11, -1, 0);
            changeScene(7, 1);
        }
    } else if (sceneNr_ == 7) {
        if (door_) door_->setEvent(1);
        if (stairs_) stairs_->setEvent(2);
        if (plant_) plant_->setEvent(4);
        if (sign_) sign_->setEvent(5);
        if (gliderArea_) gliderArea_->setEvent(6);
        if (glider_) glider_->enable();
        bobGlider();
        if (flag(kKanalHere) == 0 && kanalGone_) kanalGone_->enable();
        if (restored_ == 0) {
            if (prevScene_ == 6) {
                if (hero_) hero_->setPosDir(103, 139, 2);
                scene_.setModal(stairHomeDown_.get());
                if (!playWait(stairHomeDown_.get(), 11, 0, 4)) return;
                playSfx(stepRight_.get());
                if (!playWait(stairHomeDown_.get(), 11, 5, 10)) return;
                playSfx(stepLeft_.get());
                if (!playWait(stairHomeDown_.get(), 11, 11, 16)) return;
                playSfx(stepRight_.get());
                if (!playWait(stairHomeDown_.get(), 11, 17, 22)) return;
                playSfx(stepLeft_.get());
                if (!playWait(stairHomeDown_.get(), 10, 23, -1)) return;
                if (bar && bar->has(7)) {
                    dropSack();
                } else {
                    walk = true;
                    walkX = 123;
                    walkY = 156;
                }
            } else if (prevScene_ == 9 || prevScene_ == 46 || prevScene_ == 1007) {
                stairsFromLift();
            } else {
                arriveByGlider();
            }
        }
    } else if (sceneNr_ == 9) {
        areaEvent("009_Lift", 20);
        areaEvent("009_Pflanze", 21);
        areaEvent("009_SchildLinks", 22);
        areaEvent("009_SchildRechts", 23);
        areaEvent("009_Tuer", 24);
        areaEvent("009_Klingel", 25);
        areaEvent("009_Besen", 26);
        if (lever_) lever_->enable();
        if (restored_ == 0) {
            walk = true;
            if (prevScene_ == 10) {
                if (hero_) hero_->setPosDir(534, 273, 2);
                walkX = 469;
                walkY = 277;
            } else {
                if (hero_) hero_->setPosDir(152, 241, 0);
                walkX = 161;
                walkY = 271;
            }
        }
    } else if (sceneNr_ == 46) {
        areaEvent("046_Gang", 40);
        areaEvent("046_Kanal", 41);
        areaEvent("046_Lift", 42);
        areaEvent("046_Schild", 43);
        areaEvent("046_Schild2", 44);
        if (flag(kKanalHere) != 0) {
            if (kanalTalk_) kanalTalk_->enable();
            areaEvent("KanalTalk", 45);
            if (Area* a = scene_.getArea("046_Gang")) a->active = false;  // +0xB8 = 0
        }
        if (restored_ == 0) {
            if (prevScene_ != 47) {
                if (hero_) {
                    hero_->setPosDir(418, 137, 1);
                    hero_->walkToClick(396, 180);
                }
                if (flag(kKanalHere) != 0 && kanalTalk_) kanalTalk_->playRangeNoEvent(11, 0, 5);
            } else {
                if (hero_) hero_->setPosDir(209, 260, 7);
                scene_.setModal(kanalOut_.get());
                if (!playWait(kanalOut_.get(), 11, 0, 25)) return;
                playSfx(clothes_.get());
                if (!playWait(kanalOut_.get(), 10, 26, -1)) return;
                walk = true;
                walkX = 264;
                walkY = 284;
            }
        }
    } else if (sceneNr_ == 1007) {
        if (lowerFloor_) lowerFloor_->setEvent(10);
        if (liftDown_) liftDown_->setEvent(10);
        if (plant2_) plant2_->setEvent(11);
        if (stairs2_) stairs2_->setEvent(12);
        if (lift2_) lift2_->setEvent(13);
        if (sign2_) sign2_->setEvent(14);
        if (lever_) lever_->enable();
        if (restored_ == 0) {
            walk = true;
            walkX = 308;
            walkY = 198;
            if (prevScene_ == 9) {
                if (hero_) hero_->setPosDir(353, 165, 1);
                playSfx(lift_.get());
                playAnimWait(liftUpMid_.get());
            } else if (prevScene_ != 46) {
                if (hero_) hero_->setPosDir(292, 204, 5);
            } else {
                if (hero_) hero_->setPosDir(353, 165, 1);
                playSfx(clickClack_.get());
                playSfx(lift_.get());
                playAnimWait(liftDownMid_.get());
            }
        }
    }
    if (walk && hero_) hero_->walkToClick(walkX, walkY);
    // LAB_00406572
    scene_.enableInput();
    setTimerHandler(*ambient_, static_cast<std::uint32_t>(random(5, 15) * 1000), 0, kAmbientEvent);
}

void Loc01::onEvent(Event& e, int) {
    const int id = e.param;
    // Zufällige Bemerkung (007_ROD_02…04) nach einem Gang
    const auto remark = [this] {
        const int r = random(1, 3);
        sayActor("Loc01", r == 2 ? "007_ROD_03" : r == 3 ? "007_ROD_04" : "007_ROD_02", hero_, 0);
    };
    const auto walkTo = [this](int x, int y, int dir) {
        const int cb = beginWait();
        if (hero_) hero_->walkTo(x, y, cb, dir);
        return wait();
    };
    switch (id) {
    case 1: stairsHomeUp(); return;
    case 2:
    case 3: stairsToLift(); return;
    case 4:
        if (walkTo(315, 140, 3)) {
            const int cb = beginWait();
            speak("Loc01", "007_ROD_05", cb);  // CGame_SayNoActor
            wait();
        }
        return;
    case 5:
        if (walkTo(445, 191, 7)) remark();
        return;
    case 6: flyAway(); return;
    case 10:
    case 13:
        if (!walkTo(340, 182, 5)) return;
        scene_.disableInput();
        if (lever_) lever_->disable();
        if (hero_) hero_->hide(scene_.clicksEnabled);
        if (!playWait(pressLeverMid_, 11, 0, 19)) return;
        playSfx(clickClack_.get());
        if (!playWait(pressLeverMid_, 10, 20, -1)) return;
        scene_.enableInput();
        playSfx(lift_.get());
        if (id == 10) {
            playAnimWait(liftMidDown_.get());
            changeScene(46, 1);
        } else {
            playAnimWait(liftMidUp_.get());
            changeScene(9, 1);
        }
        return;
    case 11:
        if (walkTo(386, 227, 6)) sayActor("Loc01", "007_ROD_05", hero_, 0);
        return;
    case 12:
        if (walkTo(298, 199, 1)) changeScene(7, 1);  // = Fall 30
        return;
    case 14:
        if (walkTo(287, 195, 3)) remark();
        return;
    case 20:
        if (!walkTo(152, 241, 0)) return;
        scene_.disableInput();
        if (lever_) lever_->disable();
        if (hero_) hero_->hide(scene_.clicksEnabled);
        if (!playWait(pressLeverTop_, 11, 0, 6)) return;
        playSfx(clickClack_.get());
        if (!playWait(pressLeverTop_, 11, 7, -1)) return;
        scene_.enableInput();
        changeScene(1007, 1);
        return;
    case 21:
        if (walkTo(276, 263, 5)) sayActor("Loc01", "009_ROD_03", hero_, 0);
        return;
    case 22:
        if (walkTo(391, 267, 5)) sayActor("Loc01", "009_ROD_01", hero_, 0);
        return;
    case 23:
        if (walkTo(518, 283, 6)) sayActor("Loc01", "009_ROD_02", hero_, 0);
        return;
    case 24:
        if (walkTo(534, 273, 4)) changeScene(10, 5);
        return;
    case 25: walkAndComment(518, 283, 4, "009_ROD_05"); return;
    case 26: walkAndComment(564, 273, 6, "009_ROD_04"); return;
    case 30: changeScene(7, 1); return;
    case 31:
    case 32:
    case 33:
        if (id == 31 && scene_.inventory() && scene_.inventory()->has(14)) {
            sayActor("Loc01", "008_ROD_01", nullptr, 0);
            return;
        }
        stopBackground();
        if (gliderPass_) {
            gliderPass_->setFreq(25);
            gliderPass_->play();
            gliderPass_->fadeFreq(100, 1000);
        }
        playAnimWait(flyAll_.get());
        if (id == 31) changeScene(26, 6);
        else if (id == 32) changeScene(12, 3);
        else changeScene(113, 31);
        return;
    case 40:
        if (walkTo(184, 160, 0)) sayActor("Loc01", "046_ROD_08", hero_, 0);
        return;
    case 41:
        if (!walkTo(212, 256, 3)) return;
        if (flag(kKanalHere) != 0) {
            const int r = random(1, 3);
            speak("Loc01", r == 2 ? "046_KAN_07" : r == 3 ? "046_KAN_08" : "046_KAN_06", kanalTalk_, 0x0C, 5, 9, 0);
            return;
        }
        scene_.disableInput();
        scene_.setModal(kanalIn_.get());
        if (!playWait(kanalIn_.get(), 11, 0, 8)) return;
        playSfx(clothes_.get());
        if (!playWait(kanalIn_.get(), 11, 9, -1)) return;
        changeScene(47, 7);
        return;
    case 42: {
        if (!walkTo(418, 137, 1)) return;
        scene_.disableInput();
        if (flag(kKanalHere) != 0) {
            const int cb = beginWait();
            if (kanalTalk_) kanalTalk_->playRange(11, cb, 5, 0);
        }
        // ohne Kanalwesen wartet CGame_Wait auf den schon freien Platz des Gangs → sofort true
        if (!wait()) return;
        scene_.enableInput();
        changeScene(1007, 1);
        return;
    }
    case 43:
    case 44:
        if (id == 43 ? walkTo(416, 279, 6) : walkTo(394, 339, 6)) remark();
        return;
    case 45:
        if (flag(kKanalHere) != 0) talkKanal();
        return;
    case kItemEvent:
        if (sceneNr_ == 7 && flag(kKanalHere) != 0) {
            useTroot();
            return;
        }
        if (sceneNr_ == 46) {
            sayActor("Loc01", "046_ROD_09", hero_, 0);
            return;
        }
        describeItem(13);
        return;
    case kAmbientEvent: {
        AudioBuffer* b = nullptr;
        switch (random(1, 9)) {
        case 1:
            b = bird_.get();
            if (b) b->setVolume(random(50, 100));
            break;
        case 2:
            b = bird3_.get();
            if (b) b->setVolume(random(50, 100));
            break;
        case 3:
        case 4:
            b = gliderPass3_.get();
            if (b) b->setVolume(random(50, 100));
            break;
        case 5:
        case 6:
        case 7: b = wind_.get(); break;
        case 8:
        case 9:
            b = hack_.get();
            if (b) b->setVolume(random(50, 100));
            break;
        default: break;
        }
        if (b) playSfxRandom3D(*b);
        setTimerHandler(*ambient_, static_cast<std::uint32_t>(random(3, 6) * 1000), 0, kAmbientEvent);
        return;
    }
    default: return;
    }
}

void Loc01::arriveByGlider() {
    scene_.disableInput();
    if (hero_) hero_->setPosDir(188, 170, 0);
    if (gliderPass_) {
        gliderPass_->setFreq(100);
        gliderPass_->play();
        gliderPass_->fadeFreq(25, 2000);
    }
    scene_.setModal(returnHome_.get());
    const int cb = beginWait();
    if (returnHome_) {
        if (prevScene_ == 5 || prevScene_ == 8) returnHome_->playEndEvent(10, cb);
        else returnHome_->playRange(10, cb, 15, -1);
    }
    wait();
    if (prevScene_ == 4 || prevScene_ == 5) {
        faceAndSay("005_ROD_01");
        stairsHomeUp();
        return;
    }
    if (hero_) hero_->walkToClick(315, 157);
}

void Loc01::stairsFromLift() {
    if (hero_) hero_->setPosDir(472, 183, 1);
    scene_.disableInput();
    scene_.setModal(stairLiftDown_.get());
    AnimLayer* a = stairLiftDown_.get();
    if (!playWait(a, 11, 0, 5)) return;
    playSfx(stepRight_.get());
    if (!playWait(a, 11, 6, 9)) return;
    playSfx(stepLeft_.get());
    if (!playWait(a, 11, 10, 15)) return;
    playSfx(stepRight_.get());
    if (!playWait(a, 11, 16, 21)) return;
    playSfx(stepRight_.get());
    if (!playWait(a, 10, 22, -1)) return;
    playSfx(stepRight_.get());
    if (hero_) hero_->walkToClick(392, 174);
}

void Loc01::stairsHomeUp() {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(104, 142, cb, 6);
    if (!wait()) return;
    scene_.disableInput();
    scene_.setModal(stairHomeUp_.get());
    AnimLayer* a = stairHomeUp_.get();
    if (!playWait(a, 11, 0, 6)) return;
    playSfx(stepLeft_.get());
    if (!playWait(a, 11, 7, 11)) return;
    playSfx(stepRight_.get());
    if (!playWait(a, 11, 12, 18)) return;
    playSfx(stepLeft_.get());
    if (!playWait(a, 11, 19, 24)) return;
    playSfx(stepRight_.get());
    if (!playWait(a, 11, 25, -1)) return;
    playSfx(stepLeft_.get());
    changeScene(6, 4);
}

void Loc01::stairsToLift() {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(471, 183, cb, 5);
    if (!wait()) return;
    scene_.disableInput();
    scene_.setModal(stairLiftUp_.get());
    AnimLayer* a = stairLiftUp_.get();
    if (!playWait(a, 11, 0, 5)) return;
    playSfx(stepLeft_.get());
    if (!playWait(a, 11, 6, 12)) return;
    playSfx(stepRight_.get());
    if (!playWait(a, 11, 13, 17)) return;
    playSfx(stepLeft_.get());
    if (!playWait(a, 11, 18, 24)) return;
    playSfx(stepRight_.get());
    if (!playWait(a, 11, 25, -1)) return;
    playSfx(stepLeft_.get());
    changeScene(1007, 1);
}

void Loc01::flyAway() {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(188, 170, cb, 0);
    if (!wait()) return;
    scene_.disableInput();
    scene_.setModal(flyAway_.get());
    if (!playWait(flyAway_.get(), 11, 0, 34)) return;
    if (gliderPass_) {
        gliderPass_->setFreq(25);
        gliderPass_->play();
        gliderPass_->fadeFreq(100, 1000);
    }
    if (!playWait(flyAway_.get(), 11, 35, -1)) return;
    changeScene(8, 1);
}

void Loc01::dropSack() {
    scene_.disableInput();
    int cb = beginWait();
    if (hero_) hero_->walkTo(123, 156, cb, 7);
    if (!wait()) return;
    if (hero_) hero_->hide(scene_.clicksEnabled);
    if (!playWait(sackAnim_, 11, 0, 33)) return;
    playSfx(moneyBag_.get());
    if (!playWait(sackAnim_, 11, 34, 50)) return;
    playSfx(moneyBag_.get());
    if (!playWait(sackAnim_, 11, 51, 70)) return;
    if (sack_) sack_->enable();
    flag(kKanalTalk) = 1;
    if (auto* bar = scene_.inventory()) bar->remove(7);
    if (sack_) sack_->moveTo(251.0, 360.0, 0.0, 0.0, 12, 0);
    if (!playWait(sackAnim_, 10, 71, -1)) return;
    if (sack_) sack_->disable();
    playSfx(bubble_.get());
    if (hero_) hero_->show(scene_.clicksEnabled);
    scene_.enableInput();
    flag(kSackDropped) = 1;
    sayActor("Loc01", "007_ROD_01", hero_, 0);
}

void Loc01::useTroot() {
    scene_.disableInput();
    int cb = beginWait();
    if (hero_) hero_->walkTo(123, 156, cb, 7);
    if (!wait()) return;
    if (hero_) hero_->hide(scene_.clicksEnabled);
    if (!playWait(troot_, 11, 0, 54)) return;
    playSfx(horn_.get());
    if (!playWait(troot_, 11, 55, 68)) return;
    if (kanalLowTalk_) kanalLowTalk_->playRangeNoEvent(11, 0, 5);
    if (troot_) troot_->playRange(11, 0, 69, 80);
    cb = beginWait();
    speak("Loc01", "007_KAN_01", kanalLowTalk_, 0x0B, 5, 9, cb);
    if (!wait()) return;
    if (troot_) troot_->disable();
    if (hero_) hero_->show(scene_.clicksEnabled);
    if (kanalLowTalk_) kanalLowTalk_->disable();
    if (!playWait(kanalLowRun_, 11, 0, 15)) return;
    playSfx(woosh_.get());
    if (!playWait(kanalLowRun_, 10, 16, -1)) return;
    playSfx(tock_.get());
    if (kanalGone_) kanalGone_->enable();
    flag(kKanalHere) = 0;
    if (auto* bar = scene_.inventory()) bar->setItemEvent(13, 0);
    loadCamAnims();
    scene_.enableInput();
}

void Loc01::talkKanal() {
    int cb = beginWait();
    if (hero_) hero_->walkTo(176, 256, cb, 3);
    if (!wait()) return;
    const int stage = flag(kKanalTalk);
    const auto answer = [this](const char* key, int next) {
        speak("Loc01", key, kanalTalk_, 0x0C, 5, 9, 0);
        flag(kKanalTalk) = next;
    };
    const auto ask = [this](const char* key) {
        const int c = beginWait();
        sayActor("Loc01", key, hero_, c);
        return wait();
    };
    if (flag(kSackDropped) == 0) {
        if (stage == 1) {
            if (ask("046_ROD_01")) answer("046_KAN_01", 2);
            return;
        }
        walkAndComment(176, 256, 3, "046_ROD_02");
        return;
    }
    switch (stage) {
    case 1:
        if (ask("046_ROD_03")) answer("046_KAN_02", 2);
        return;
    case 2:
        if (ask("046_ROD_04")) answer("046_KAN_03", 3);
        return;
    case 3:
        if (ask("046_ROD_05")) {
            answer("046_KAN_04", 4);
            flag(kTalkStage4) = 1;
        }
        return;
    case 4:
        if (ask("046_ROD_06")) answer("046_KAN_05", 5);
        return;
    default: walkAndComment(176, 256, 3, "046_ROD_07"); return;
    }
}

void Loc01::bobGlider() {
    if (!glider_) return;
    if (bobUp_ == 0) {
        bobUp_ = 1;
        glider_->moveTo(0.0, 63.0, 0.0, 0.0, 3, kBobHandler);
        return;
    }
    bobUp_ = 0;
    glider_->moveTo(0.0, 65.0, 0.0, 0.0, 4, kBobHandler);
}

}  // namespace rah
