// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc27.hpp"

#include <bit>
#include <cstdio>

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
float f32(std::uint32_t bits) { return std::bit_cast<float>(bits); }
constexpr std::size_t kHelmHint = 0x3E90;   // Megophias hat vom Helm erzählt (liest Loc16)
constexpr std::size_t kPitSeen = 0x4094;    // Grube in 98 gesehen (Loc26) → zweite Gesprächsreihe
constexpr std::size_t kTo98Open = 0x4098;   // Weg nach 98 frei (Loc26)
constexpr std::size_t kSecretKnown = 0x40A8;  // Boden untersucht: Stuhl führt in den Geheimraum
constexpr std::size_t kChairStage = 0x40AC;   // 1: Boden noch nicht anklickbar
constexpr std::size_t kFloorStage = 0x40B0;   // 1: erster Kommentar am Boden
constexpr std::size_t kFirst96 = 0x40B4;      // erster Besuch im Geheimraum
constexpr std::size_t kSocketOn = 0x40B8;     // Steckkontakt anklickbar
constexpr std::size_t kHelmDropped = 0x40BC;  // Helm liegt im Geheimraum
constexpr std::size_t kTalk = 0x40C0;         // Gesprächsstufe mit Megophias
constexpr int kHelm = 5;
constexpr int kScene93 = 0x5D, kScene95 = 0x5F, kScene96 = 0x60, kFromLoad = 0x3E9;
constexpr int kUseHandler = 0x0042C590;
}  // namespace

Loc27::Loc27(GameContext& ctx, int scene, int prevScene) : Game(ctx, 27, scene, prevScene) {
    getViewObjects();
    stepL_ = loadSfx("Gen_StepLeft");
    stepR_ = loadSfx("Gen_StepRight");
    ambientTimer_ = std::make_unique<Event>();
    const auto cue = [](AnimLayer* a, int frame, AudioBuffer* b) {
        if (!a) return;
        AnimCue c;
        c.type = 1;
        c.frame = frame;
        c.buffer = b;
        a->addCue(c);  // CAnimLayer_AddCue 0x00442BB0
    };
    InventoryBar* bar = scene_.inventory();
    const bool helmHere = (bar && bar->has(kHelm)) || flag(kHelmDropped) != 0;
    if (sceneNr_ == kScene96) {
        if (prevScene_ == kFromLoad) flag(kPitSeen) = 1;  // 0x0042B038: CMP [+0x30], 0x3E9 → 0x4094 = 1
        if (flag(kPitSeen) != 0 && flag(kTalk) > 3) flag(kTalk) = 1;
        scene_.load("S96", helmHere ? 2 : 0);  // CSzene_Load(DAT_004753CC, maske)
        getViewObjects();
        rodEnters_ = loadAnim("S96_RodEntersRoom");
        rodAndEye_ = obj("S96_RodAndEye");
        rodDropsHelm_ = obj("S96_RodDropsHelm");
        helm_ = scene_.getObject("S96_Helm");
        rodWaves_ = obj("S96_RodWinkt");
        eye_ = scene_.getObject("S96_Eye");
        eyeDown_ = scene_.getObject("S96_EyeDown");
        steam_ = obj("S96_Steam");
        bubble_ = obj("S96_Bubble");
        microphone_ = obj("S96_Microphone");
        pump_ = obj("S96_Pump");
        exit95_ = scene_.getArea("S96_ExitTo95");
        if (!helmHere) megophias_ = scene_.getArea("S96_Megophias");
        socket_ = scene_.getArea("S96_Steckkontakt");
        machines_ = scene_.getArea("S96_Anlagen");
        wings_ = loadSfx("Sfx_DragonWings");
        wings2_ = loadSfx("Sfx_DragonWings2");
        bigStone_ = loadSfx("Sfx_BigStone");
        bubbleSfx_ = loadSfx("Sfx_Bubble");
        servo_ = loadSfx("Sfx_ServoMotor");
        endTime_ = loadSfxVol("Sfx_EndTimeFeeling", 0x19);
        if (endTime_) endTime_->setFreq(0x19);  // CAudioBuffer_SetFreq
        cue(bubble_, 0x15, bubbleSfx_.get());
        cue(pump_, 3, wings2_.get());
        cue(pump_, 0x16, wings2_.get());
        cue(steam_, 1, wings_.get());
        cue(rodAndEye_, 1, servo_.get());
        cue(rodAndEye_, 1, servo_.get());  // Original: zweimal
        cue(rodAndEye_, 10, servo_.get());
        cue(rodAndEye_, 0x0F, servo_.get());
        cue(rodAndEye_, 0x14, servo_.get());
        cue(rodWaves_, 6, servo_.get());
        if (helmHere) {
            helmOn_ = loadAnim("S96_HelmAuf");
            doorOpened_ = loadSfx("Sfx_Door_Opened");
            cue(helmOn_.get(), 0x0E, wings2_.get());
            cue(helmOn_.get(), 0x21, wings2_.get());
            cue(rodDropsHelm_, 0x25, doorOpened_.get());
        }
    } else {
        sceneNr_ = kScene95;
        scene_.load("S95", 0);  // CSzene_Load(DAT_00475440, 0)
        getViewObjects();
        enterHut_ = loadAnim("S95_EnterHut");
        enterSecret_ = loadAnim("S95_EnterSecretRoom");
        windowW_ = scene_.getArea("S95_WindowW");
        windowN_ = scene_.getArea("S95_WindowN");
        windowE_ = scene_.getArea("S95_WindowE");
        chair_ = scene_.getArea("S95_Chair");
        floor_ = scene_.getArea("S95_Floor");
        exit93_ = scene_.getArea("S95_ExitTo93");
        mobileUp_ = loadSfx("Sfx_MobileUp");
        ocean_ = loadSfxVol("Sfx_Ocean", 0x19);
        seagull_ = loadSfx("Sfx_Seagull");
        cue(enterSecret_.get(), 4, mobileUp_.get());
    }
    // this+0x24 = 1: setzt schon CGame_ctor (0x0043F522); kein Leser in CGame gefunden
    music_ = playMusic("Loc26_IslandOfMegophias");
    if (music_) {
        music_->setReverb(2, f32(0x3ED58106), f32(0x3ECCCCCD), f32(0x3F2A7EFA));
        music_->setReverbMix(f32(0x3DCCCCCD));  // 0x004375B0
    }
    g_.fnHandlers[kUseHandler] = {this, [this](Event&, int t) { useItem(t); }};
}

Loc27::~Loc27() {
    shutdown();
    const auto it = g_.fnHandlers.find(kUseHandler);
    if (it != g_.fnHandlers.end() && it->second.owner == this) g_.fnHandlers.erase(it);
}

void Loc27::destroyLocal() {
    ambientTimer_.reset();
    helmOn_.reset();
    enterHut_.reset();
    enterSecret_.reset();
    rodEnters_.reset();
    for (auto* b : {&stepL_, &stepR_, &mobileUp_, &ocean_, &seagull_, &wings_, &wings2_, &bigStone_, &bubbleSfx_,
                    &doorOpened_, &servo_, &endTime_})
        b->reset();
}

bool Loc27::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo / WalkTo2
    return wait();
}

void Loc27::megSay(const char* key) {
    const int cb = beginWait();
    sayActor("Loc27", key, microphone_, cb);  // CGame_SayActor
    wait();
}

void Loc27::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    setEventProc(&Loc27::onEvent);
    if (InventoryBar* bar = scene_.inventory()) bar->setUseHandler(kUseHandler);  // 0x0043FA60
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    if (sceneNr_ == kScene96) {
        if (exit95_) exit95_->setEvent(10);
        if (megophias_) megophias_->setEvent(0x0B);
        if (socket_) socket_->setEvent(0x0C);
        if (machines_) machines_->setEvent(0x0D);
        if (flag(kSocketOn) == 0 && socket_) socket_->active = false;  // +0xB8
        if (flag(kHelmDropped) != 0 && helm_) {
            helm_->enable();
            if (helm_->area) helm_->area->setEvent(0x15);
        }
        if (endTime_) endTime_->playLoop();  // vtable +0x10
        const auto showEye = [this] {
            if (eye_) {
                eye_->enable();
                if (eye_->area) eye_->area->setEvent(0x14);
            }
        };
        if (restored_ == 0) {
            const auto cue = [](AnimLayer* a, int frame, AudioBuffer* b) {
                if (!a) return;
                AnimCue c;
                c.type = 1;
                c.frame = frame;
                c.buffer = b;
                a->addCue(c);
            };
            cue(rodEnters_.get(), 5, stepL_.get());
            cue(rodEnters_.get(), 0x0C, bigStone_.get());
            cue(rodEnters_.get(), 0x0F, stepR_.get());
            cue(rodEnters_.get(), 0x19, bigStone_.get());
            cue(rodEnters_.get(), 0x0D, wings2_.get());
            cue(rodEnters_.get(), 0x22, wings2_.get());
            playAnimWait(rodEnters_.get());
            if (pump_) pump_->enable();
            if (microphone_) microphone_->enable();
            // Rodrigo als freie Ebene in den Raum laufen lassen
            scene_.setHero(nullptr);    // 0x0044A6E0
            scene_.insertLayer(hero_);  // 0x0044AA00
            if (hero_) hero_->setPosZ(-96.0, 78.0, 1.0, 115.0);
            if (eyeDown_) eyeDown_->enable();
            if (hero_) {
                cue(hero_, 0x57, stepL_.get());
                cue(hero_, 0x5D, stepR_.get());
                hero_->playRangeNoEvent(13, 0x54, 0x5F);
            }
            {
                const int cb = beginWait();
                if (hero_) hero_->moveTo(164.0, 100.0, 0.0, 78.0, 15, cb);  // 0x004475B0
                wait();
            }
            scene_.removeLayer(hero_);  // 0x0044AA70
            scene_.setHero(hero_);
            if (hero_) hero_->hide(scene_.clicksEnabled);
            if (eyeDown_) eyeDown_->disable();
            if (flag(kFirst96) != 0) speak("Loc27", "096_ROD_01", 0);  // CGame_SayNoActor
            {
                const int cb = beginWait();
                if (rodAndEye_) rodAndEye_->playEndEvent(10, cb);
                wait();
            }
            if (hero_) {
                hero_->setPosDir(0xEB, 0x165, 5);
                hero_->show(scene_.clicksEnabled);
            }
            {
                // Original: BeginWait ohne Wait – der Platz bleibt belegt, bis der Weg endet
                const int cb = beginWait();
                if (hero_) hero_->walkTo(0xFA, 0x15E, cb, 5);
            }
            showEye();
            if (flag(kFirst96) != 0) {
                flag(kFirst96) = 0;
                megSay("096_MEG_01");
            }
        } else {
            if (hero_) hero_->show(scene_.clicksEnabled);
            if (pump_) pump_->enable();
            if (microphone_) microphone_->enable();
            showEye();
        }
    } else {
        if (windowW_) windowW_->setEvent(1);
        if (windowN_) windowN_->setEvent(2);
        if (windowE_) windowE_->setEvent(3);
        if (chair_) chair_->setEvent(4);
        if (floor_) floor_->setEvent(5);
        if (exit93_) exit93_->setEvent(6);
        if (ocean_) ocean_->playLoop();
        if (flag(kChairStage) == 1 && floor_) floor_->active = false;
        if (prevScene_ == kScene96) {
            if (hero_) {
                hero_->show(scene_.clicksEnabled);
                if (restored_ == 0) hero_->setPosDir(0x138, 0x125, 1);
            }
        } else if (restored_ == 0) {
            if (enterHut_) {
                AnimCue c;
                c.type = 1;
                c.frame = 5;
                c.buffer = stepL_.get();
                enterHut_->addCue(c);
                c.frame = 0x0F;
                c.buffer = stepR_.get();
                enterHut_->addCue(c);
            }
            playAnimWait(enterHut_.get());
            scene_.setHero(nullptr);
            scene_.insertLayer(hero_);
            if (hero_) {
                hero_->setPosZ(103.0, 64.0, 1.0, 125.0);
                hero_->setColor(-90, -70, -50);  // CLayer_SetColor 0x004473E0
                AnimCue c;
                c.type = 1;
                c.frame = 0x4B;
                c.buffer = stepL_.get();
                hero_->addCue(c);
                c.frame = 0x51;
                c.buffer = stepR_.get();
                hero_->addCue(c);
                hero_->playRangeNoEvent(13, 0x48, 0x53);
            }
            {
                const int cb = beginWait();
                // 0x004475F0: Farbe auf (1,1,1) blenden
                if (hero_) hero_->tweenTo(242.0, 90.0, 0.0, 84.0, 1, 1, 1, 15, cb);
                wait();
            }
            if (hero_) hero_->setColor(0, 0, 0);
            scene_.removeLayer(hero_);
            scene_.setHero(hero_);
            if (hero_) {
                hero_->show(scene_.clicksEnabled);
                hero_->setPosDir(0x14C, 0x165, 5);
            }
            scene_.enableInput();
            if (hero_) hero_->walkToClick(0x17B, 0x141);
        } else if (hero_) {
            hero_->show(scene_.clicksEnabled);
        }
    }
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(5, 10) * 1000), 0, 0x1E);
    scene_.enableInput();
}

void Loc27::useItem(int item) {
    if (item != kHelm) {
        describeItem(item);  // CGame_DescribeItem
        return;
    }
    if (flag(kHelmDropped) != 0) return;
    // Helm in die Anlage legen
    scene_.disableInput();
    walk(0x97, 0x137, 6);  // CActor_WalkTo2
    if (InventoryBar* bar = scene_.inventory()) bar->remove(kHelm);
    if (hero_) hero_->hide(scene_.clicksEnabled);
    {
        const int cb = beginWait();
        if (rodDropsHelm_) rodDropsHelm_->playEndEvent(10, cb);
        wait();
    }
    if (hero_) {
        hero_->setPosDir(0xA0, 0x133, 5);
        hero_->show(scene_.clicksEnabled);
    }
    if (helm_) {
        helm_->enable();
        if (helm_->area) helm_->area->setEvent(0x15);
    }
    flag(kHelmDropped) = 1;
    scene_.enableInput();
    megSay("096_MEG_28");
    heroSay("096_ROD_28");
    megSay("096_MEG_29");
}

void Loc27::talkMegophias() {
    if (!walk(0xFA, 0x155, 5)) return;  // CActor_WalkTo2
    if (flag(kPitSeen) == 0) {
        const int t = flag(kTalk);
        if (t >= 1 && t <= 0x0E && t != 9) {
            char rod[16], meg[16];
            std::snprintf(rod, sizeof rod, "096_ROD_%02d", t + 1);
            std::snprintf(meg, sizeof meg, "096_MEG_%02d", t + 1);
            // Stufen 10–14 überspringen ROD_11/MEG_11 (gehört zu Stufe 9)
            if (t >= 10) {
                std::snprintf(rod, sizeof rod, "096_ROD_%02d", t + 2);
                std::snprintf(meg, sizeof meg, "096_MEG_%02d", t + 2);
            }
            heroSay(rod);
            megSay(meg);
            flag(kTalk) = t + 1;
            return;
        }
        switch (t) {
        case 9:
            heroSay("096_ROD_10");
            megSay("096_MEG_10");
            heroSay("096_ROD_11");
            megSay("096_MEG_11");
            flag(kTalk) = 10;
            return;
        case 0x0F:
            heroSay("096_ROD_17");
            megSay("096_MEG_17");
            if (hero_) hero_->hide(scene_.clicksEnabled);
            if (eye_) eye_->disable();
            {
                const int cb = beginWait();
                if (rodAndEye_) rodAndEye_->playEndEvent(10, cb);
                wait();
            }
            if (hero_) {
                hero_->setPosDir(0xEE, 0x15E, 5);
                hero_->show(scene_.clicksEnabled);
            }
            if (eye_) {
                eye_->enable();
                if (eye_->area) eye_->area->active = true;
            }
            flag(kSocketOn) = 1;
            if (socket_) socket_->active = true;
            if (hero_) hero_->turnTo(3);  // 0x00435BA0
            flag(kTalk) = 0x10;
            return;
        case 0x10:
            heroSay("096_ROD_18");
            megSay("096_MEG_18");
            heroSay("096_ROD_19");
            megSay("096_MEG_19");
            flag(kTo98Open) = 1;
            return;
        default:
            return;
        }
    }
    switch (flag(kTalk)) {
    case 1:
        flag(kTalk) = 2;
        heroSay("096_ROD_20");
        megSay("096_MEG_20");
        return;
    case 2:
        flag(kTalk) = 3;
        heroSay("096_ROD_21");
        megSay("096_MEG_21");
        return;
    case 3:
        heroSay("096_ROD_22");
        megSay("096_MEG_22");
        speak("Loc27", "096_ROD_23", 0);  // CGame_SayNoActor
        flag(kHelmHint) = 1;
        return;
    default:
        return;
    }
}

void Loc27::onEvent(Event& e, int) {
    switch (e.param) {
    case 1: walkAndComment(0xB0, 0x153, 3, "095_ROD_01"); return;
    case 2:  // Fenster Nord (bzw. Stuhl/Geheimgang)
        if (flag(kChairStage) == 1) {
            if (floor_) floor_->active = true;
            walkAndComment(0x138, 0x125, 5, "095_ROD_01");
            flag(kChairStage) = 2;
            return;
        }
        if (flag(kSecretKnown) == 0) {
            walkAndComment(0x138, 0x125, 5, "095_ROD_02");
            return;
        }
        if (!walk(0x138, 0x125, 5)) return;  // CActor_WalkTo2
        scene_.disableInput();
        if (flag(kFirst96) != 0) heroSay("095_ROD_02");
        playAnimWait(enterSecret_.get());
        changeScene(kScene96, 0x1B);
        return;
    case 3: walkAndComment(0x1C7, 0x159, 7, "095_ROD_01"); return;
    case 4: walkAndComment(0xF2, 0x12A, 3, "095_ROD_03"); return;
    case 5:  // Boden
        if (flag(kFloorStage) == 1) {
            walkAndComment(0xED, 299, 7, "095_ROD_04");
            flag(kFloorStage) = 2;
            return;
        }
        walkAndComment(0xED, 299, 7, "095_ROD_05");
        flag(kSecretKnown) = 1;
        return;
    case 6:  // → 93
        if (walk(0x136, 0x162, 0)) changeScene(kScene93, 0x1A);
        return;
    case 10:  // Ausgang → 95
        if (flag(kHelmDropped) == 0) {
            if (flag(kTo98Open) == 0) {
                // Megophias lässt Rodrigo noch nicht gehen
                if (hero_) hero_->walkToClick(3, 0x157);
                delay(1000);
                if (hero_) {
                    hero_->stop(0);    // vtable +0x24
                    hero_->turnTo(4);  // 0x00435BA0
                }
                megSay("096_MEG_26");
                walk(0xFA, 0x155, 5);  // CActor_WalkTo2
                return;
            }
            if (!walk(3, 0x157, 2)) return;
            if (hero_) hero_->hide(scene_.clicksEnabled);
            changeScene(kScene95, 0x1B);
            return;
        }
        if (!walk(0x2F, 0x14D, 2)) return;
        {
            const int cb = beginWait();
            if (hero_) hero_->turn(0, cb);  // CActor_Turn
            wait();
        }
        heroSay("096_ROD_26");
        return;
    case 0x0B:
        talkMegophias();
        return;
    case 0x0C:  // Steckkontakt
        if (!walk(0xDA, 0x121, 3)) return;
        heroSay("096_ROD_25");
        megSay("096_MEG_25");
        return;
    case 0x0D:  // Anlagen
        if (walk(0xE2, 0x118, 5)) megSay("096_MEG_23");
        return;
    case 0x14:  // Auge
        if (!walk(0xEB, 0x165, 5)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        if (eye_) eye_->disable();
        speak("Loc27", "096_ROD_24", 0);  // CGame_SayNoActor
        {
            const int cb = beginWait();
            if (rodWaves_) rodWaves_->playEndEvent(10, cb);
            wait();
        }
        if (hero_) hero_->show(scene_.clicksEnabled);
        {
            // Original: BeginWait ohne Wait (megSay belegt danach einen weiteren Platz)
            const int cb = beginWait();
            if (hero_) hero_->walkTo(0xFA, 0x15E, cb, 5);
        }
        if (eye_) {
            eye_->enable();
            if (eye_->area) eye_->area->active = true;
        }
        megSay("096_MEG_24");
        return;
    case 0x15:  // Helm aufsetzen → 25/104
        if (!walk(0xA0, 0x133, 5)) return;
        playAnimWait(helmOn_.get());
        changeScene(0x68, 0x19);
        return;
    case 0x1E:
        if (sceneNr_ == kScene95) {
            if (seagull_) {
                seagull_->setVolume(random(0x19, 0x32));  // CAudioBuffer_SetVolume
                playSfxRandom3D(*seagull_);
            }
        } else if (sceneNr_ == kScene96) {
            const int r = random(1, 3);
            if (r == 1 && bubble_) bubble_->playRangeNoEvent(10, 0, -1);
            else if (r == 2 && pump_) pump_->playRangeNoEvent(11, 0, -1);
            else if (r == 3 && steam_) steam_->playRangeNoEvent(10, 0, -1);
        }
        setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(5, 10) * 1000), 0, 0x1E);
        return;
    default:
        return;
    }
}

}  // namespace rah
