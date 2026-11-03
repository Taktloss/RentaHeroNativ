// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc14.hpp"

#include <bit>

#include "engine/actor.hpp"
#include "engine/area.hpp"

namespace rah {

namespace {
float f32(std::uint32_t bits) { return std::bit_cast<float>(bits); }
constexpr std::size_t kFireOk = 0x3F7C;    // Kamin darf angesprochen werden (nach Gesprächsstufe 4)
constexpr std::size_t kFlowers = 0x3F80;   // Blumen angesehen
constexpr std::size_t kFireTalk = 0x3F84;  // über den Kamin gesprochen
constexpr std::size_t kTalk = 0x3F88;      // Gesprächsstufe mit Cynthia (Start 1)
constexpr std::size_t kBed = 0x3F8C;       // Doppelbett: 1, 2 angesprochen, 3 Klappbett vorgeführt
constexpr int kAttackScene = 0x59;         // Herkunft: Zwischensequenz Loc09
}  // namespace

Loc14::Loc14(GameContext& ctx, int scene, int prevScene) : Game(ctx, 14, scene, prevScene) {
    sceneNr_ = 0x5A;
    scene_.load("090", prevScene == kAttackScene ? 1u : 0u);  // DAT_004735C4
    getViewObjects();
    if (prevScene_ == kAttackScene) {
        wakeUp_ = loadAnim("S90_RodWachtAuf");
        rodStandUp_ = obj("S90_Rodaufsteh");
        cynSit_ = obj("S90_Cynsitz");
        rodSitUp_ = obj("S90_Rodaufsetz");
        rodTalk_ = obj("S90_RodTalk");
    }
    fireplace_ = obj("S90_Kamin");
    cynTalk_ = obj("S90_CynTalk");
    sickBed_ = scene_.getArea("S90_Krankenbett");
    doubleBed_ = scene_.getArea("S90_Doppelbett");
    flowers_ = scene_.getArea("S90_Blumen");
    exit_ = scene_.getArea("S90_Exit");
    trough_ = scene_.getArea("S90_Trog");
    shelves_ = scene_.getArea("S90_Regale");
    fireArea_ = scene_.getArea("S90_KaminArea");
    fire_ = loadSfxVol("Sfx_Fire", 0);
    if (flag(kBed) < 3) {
        bed_ = loadAnim("S90_Bett");
        woosh_ = loadSfx("Sfx_Woosh");
        hack_ = loadSfx("Sfx_Work_Hack");
        clang_ = loadSfx("Sfx_ServoMotor_ShortClang_wav");
        if (bed_) {  // Port: Original prüft nicht
            AnimCue c;
            c.type = 1;
            c.frame = 0x1A;
            c.buffer = clang_.get();
            bed_->addCue(c);
            c.frame = 0x2B;
            c.buffer = woosh_.get();
            bed_->addCue(c);
            AnimCue x;  // CAnimLayer_AddCueEx(56, Hack, 1, 0, 0, 0, 0, 0.1, 0, 0.2)
            x.type = 2;
            x.frame = 0x38;
            x.buffer = hack_.get();
            x.action = 1;
            x.x = f32(0x3DCCCCCD);
            x.z = f32(0x3E4CCCCD);
            bed_->addCue(x);
        }
    }
    music_ = playMusic("Loc14_Cynthia");
    setMusicCynthia();
}

void Loc14::setMusicCynthia() {
    if (!music_) return;
    music_->setReverb(4, f32(0x3E54FDF4), f32(0x3EF4BC6A), 0.0f);
    music_->setReverbMix(f32(0x3DCCCCCD));  // 0x004375B0
}

void Loc14::destroyLocal() {
    wakeUp_.reset();
    bed_.reset();
    fire_.reset();
    woosh_.reset();
    hack_.reset();
    clang_.reset();
}

bool Loc14::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo
    return wait();
}

void Loc14::turn(int dir) {
    const int cb = beginWait();
    if (hero_) hero_->turn(dir, cb);  // CActor_Turn 0x00435BF0
    wait();
}

void Loc14::cynSay(const char* key) {
    const int cb = beginWait();
    sayActor("Loc14", key, cynTalk_, cb);
    wait();
}

void Loc14::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    setEventProc(&Loc14::onEvent);
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    if (fire_) {
        fire_->playLoop();          // vtable +0x10
        fire_->fadeVolume(25, 2000);  // CAudioBuffer_FadeVolume
    }
    if (sickBed_) sickBed_->setEvent(1);
    if (doubleBed_) doubleBed_->setEvent(2);
    if (flowers_) flowers_->setEvent(3);
    if (exit_) exit_->setEvent(4);
    if (trough_) trough_->setEvent(5);
    if (shelves_) shelves_->setEvent(6);
    if (fireArea_) {
        fireArea_->setEvent(10);
        if (flag(kFireOk) == 0) fireArea_->active = false;  // +0xB8
    }
    if (fireplace_) fireplace_->start(0x0D);  // CAnimLayer_Loop 0x00443230
    if (cynTalk_ && cynTalk_->area) cynTalk_->area->setEvent(11);
    if (restored_ != 0) {
        if (cynTalk_) cynTalk_->enable();
        if (hero_) hero_->show(scene_.clicksEnabled);
        scene_.enableInput();
        return;
    }
    if (prevScene_ == kAttackScene) {
        // Rodrigo wacht im Krankenbett auf
        if (cynSit_) cynSit_->enable();
        if (rodSitUp_) rodSitUp_->enable();
        playAnimWait(wakeUp_.get());
        {
            const int cb = beginWait();
            if (rodSitUp_) rodSitUp_->playEndEvent(11, cb);
            wait();
        }
        if (rodTalk_) rodTalk_->enable();
        {
            const int cb = beginWait();
            sayActor("Loc14", "090_ROD_01", rodTalk_, cb);
            wait();
        }
        delay(500);
        if (rodTalk_) rodTalk_->disable();
        if (rodSitUp_) rodSitUp_->disable();
        if (cynSit_) cynSit_->start(11);
        {
            const int cb = beginWait();
            if (rodStandUp_) rodStandUp_->playRange(11, cb, 3, -1);
            wait();
        }
        if (rodStandUp_) rodStandUp_->disable();
        if (hero_) hero_->setPosDir(0x1CB, 0x127, 1);
        walk(0x197, 0x145, 5);
        if (cynSit_) cynSit_->disable();
        if (cynTalk_) cynTalk_->enable();
        cynSay("090_CYN_01");
        scene_.enableInput();
        return;
    }
    if (cynTalk_) cynTalk_->enable();
    if (hero_) {
        hero_->setPosDir(10, 0x15E, 6);
        hero_->walkToClick(0x8F, 0x150);  // CActor_WalkToClick
    }
    scene_.enableInput();
}

void Loc14::onEvent(Event& e, int) {
    switch (e.param) {
    case 1:
        walkAndComment(0x1D8, 0x142, 5, "090_ROD_11");
        return;
    case 2:  // Doppelbett (Klappbett)
        if (!walk(0x16A, 0x115, 2)) return;
        if (flag(kBed) == 1) {
            heroSay("090_ROD_12");
            if (hero_) hero_->turnTo(6);  // 0x00435BA0
            cynSay("090_CYN_12");
            flag(kBed) = 2;
            return;
        }
        if (flag(kBed) != 2) {
            turn(0);
            heroSay("090_ROD_16");
            return;
        }
        if (flag(kFireTalk) == 0 || flag(kTalk) != 2) {
            heroSay("090_ROD_13");
            if (hero_) hero_->turnTo(6);
            cynSay("090_CYN_13");
            return;
        }
        // Cynthia führt das Klappbett vor
        scene_.disableInput();
        turn(6);
        heroSay("090_ROD_14");
        cynSay("090_CYN_14");
        turn(2);
        music_ = playMusic("Loc14_LoveIsBreakingTheNight");
        if (music_) {
            music_->fadeIn(100, 1000);  // 0x00437FD0
            music_->setReverb(6, f32(0x3ECE5604), f32(0x4088ED91), 0.5f);
            music_->setReverbMix(f32(0x3DCCCCCD));
        }
        playAnimWait(bed_.get());
        music_ = playMusic("Loc14_Cynthia");
        if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn
        setMusicCynthia();
        turn(6);
        heroSay("090_ROD_15");
        cynSay("090_CYN_15");
        flag(kBed) = 3;
        scene_.enableInput();
        return;
    case 3:  // Blumen
        if (!walk(0x180, 0x10F, 5)) return;
        heroSay("090_ROD_17");
        if (hero_) hero_->turnTo(7);
        cynSay("090_CYN_17");
        flag(kFlowers) = 1;
        return;
    case 4:  // Ausgang → Loc12, Szene 91
        if (!walk(0x15B, 299, 5)) return;
        heroSay("090_ROD_21");
        sayActor("Loc14", "090_CYN_21", cynTalk_, 0);  // ohne Warten
        if (walk(0, 0x15E, 2)) changeScene(0x5B, 0x0C);
        return;
    case 5:
        walkAndComment(0x1FC, 0x155, 7, "090_ROD_22");
        return;
    case 6:
        walkAndComment(0x96, 0x143, 3, "090_ROD_18");
        return;
    case 10:  // Kamin
        if (!walk(0x16A, 0x127, 4)) return;
        turn(5);
        if (flag(kFlowers) == 0) {
            heroSay("090_ROD_19");
            cynSay("090_CYN_19");
            return;
        }
        heroSay("090_ROD_20");
        cynSay("090_CYN_20");
        flag(kFireTalk) = 1;
        if (flag(kTalk) > 2) flag(kTalk) = 1;
        return;
    case 11:  // Cynthia
        if (!walk(0x17E, 0x116, 6)) return;
        if (flag(kFireTalk) != 0) {
            if (flag(kTalk) == 1) {
                heroSay("090_ROD_09");
                cynSay("090_CYN_09");
                flag(kTalk) = 2;
                return;
            }
            heroSay("090_ROD_10");
            cynSay("090_CYN_10");
            return;
        }
        switch (flag(kTalk)) {
        case 1:
            flag(kTalk) = 2;
            heroSay("090_ROD_02");
            cynSay("090_CYN_02");
            return;
        case 2:
            flag(kTalk) = 3;
            heroSay("090_ROD_03");
            cynSay("090_CYN_03");
            return;
        case 3:
            scene_.disableInput();
            flag(kTalk) = 4;
            heroSay("090_ROD_04");
            cynSay("090_CYN_04");
            turn(0);
            delay(700);
            turn(6);
            scene_.enableInput();
            return;
        case 4:
            scene_.disableInput();
            flag(kTalk) = 5;
            heroSay("090_ROD_05");
            cynSay("090_CYN_05");
            heroSay("090_ROD_06");
            walk(0x17E, 0x163, 0);
            heroSay("090_ROD_07");
            turn(5);
            flag(kFireOk) = 1;
            if (fireArea_) fireArea_->active = true;
            scene_.enableInput();
            return;
        case 5:
            heroSay("090_ROD_08");
            cynSay("090_CYN_08");
            return;
        default:
            return;
        }
    default:
        return;
    }
}

}  // namespace rah
