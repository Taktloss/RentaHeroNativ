// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc24.hpp"

#include <bit>

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
float f32(std::uint32_t bits) { return std::bit_cast<float>(bits); }
constexpr std::size_t kLeftShip = 0x4088;  // Rodrigo hat das Schiff mit Helm verlassen (liest Loc08)
constexpr std::size_t kShifted = 0x408C;   // Kampf ist nach rechts gerückt
constexpr int kHelm = 5;                   // Gegenstand Helm
constexpr int kBottle = 0x18;              // Gegenstand Flasche
constexpr int kAmbient = 100;
constexpr int kBlockL = 0x72, kBlockT = 0xFD, kBlockR = 300, kBlockB = 0x136;  // Kampf versperrt den Weg
}  // namespace

Loc24::Loc24(GameContext& ctx, int scene, int prevScene) : Game(ctx, 24, scene, prevScene) {
    getViewObjects();
    scene_.load("S103", 0xFFFF);  // DAT_00474F20, CSzene_LoadAll
    getViewObjects();
    table_ = scene_.getObject("S103_Tisch");
    capComes_ = obj("S103_CapComes");
    capAttack1_ = obj("S103_CapAttack1");
    capAttack2_ = obj("S103_CapAttack2");
    capAttack3_ = obj("S103_CapAttack3");
    capCameTalk_ = obj("S103_CapCameTalk");
    capCovers_ = obj("S103_CapDecktUnten");
    rodTakesHelm_ = obj("S103_RodTakesHelm");
    helm_ = scene_.getObject("S103_Helm");
    sabAttack2_ = obj("S103_SabAttack2");
    sabCovers1_ = obj("S103_SabDeckt1");
    sabCovers2_ = obj("S103_SabDeckt2");
    sabDrawsSword_ = obj("S103_SabDrawsSword");
    bottle_ = scene_.getObject("S103_Flasche");
    rodPickUp_ = obj("S103_RodPickUp");
    sabWalk_ = obj("SabWalk");
    steam_ = obj("Gen_SmallSteam");
    oven_ = scene_.getArea("S103_Ofen");
    lever_ = scene_.getArea("S103_Hebel");
    instruments_ = scene_.getArea("S103_Instrumente");
    to102_ = scene_.getArea("S103_To102");
    swords_ = scene_.getArea("S103_Swords");
    sword1_ = loadSfx("Sfx_Sword1");
    sword2_ = loadSfx("Sfx_Sword2");
    swordPull_ = loadSfx("Sfx_SwordPull");
    workFix_ = loadSfx("Sfx_Work_Fix");
    stepL_ = loadSfx("Gen_StepLeft");
    stepR_ = loadSfx("Gen_StepRight");
    stepCyber_ = loadSfxVol("Sfx_Step_Cyber", 0x3C);
    ocean_ = loadSfxVol("Sfx_Ocean", 0x32);
    seagull_ = loadSfxVol("Sfx_Seagull", 0x32);
    woosh_ = loadSfxVol("Sfx_Woosh", 0x46);
    thunder_ = loadSfxVol("Sfx_Thunder", 0x46);
    thunder2_ = loadSfxVol("Sfx_Thunder2", 0x46);
    danger_ = loadSfx("Sfx_DangerStrings");
    piano_ = loadSfx("Sfx_PianoStroke");
    const auto cue = [](AnimLayer* a, int frame, const std::unique_ptr<AudioBuffer>& b) {
        if (!a) return;
        AnimCue c;
        c.type = 1;
        c.frame = frame;
        c.buffer = b.get();
        a->addCue(c);  // CAnimLayer_AddCue 0x00442BB0
    };
    cue(capAttack1_, 0x18, stepCyber_);
    cue(capAttack1_, 0x14, sword1_);
    cue(capAttack2_, 7, stepCyber_);
    cue(capAttack2_, 10, sword2_);
    cue(capAttack3_, 2, stepCyber_);
    cue(capAttack3_, 2, woosh_);
    cue(capAttack3_, 7, sword1_);
    cue(capAttack3_, 10, stepR_);
    cue(capAttack3_, 0x0F, piano_);
    cue(sabAttack2_, 5, sword2_);
    cue(sabDrawsSword_, 5, swordPull_);
    cue(sabWalk_, 0x54, stepL_);
    cue(sabWalk_, 0x5A, stepR_);
    cue(sabWalk_, 0x0C, stepL_);
    cue(sabWalk_, 0x12, stepR_);
    cue(capComes_, 1, workFix_);
    cue(capComes_, 0, danger_);
    cue(capComes_, 3, stepCyber_);
    cue(capComes_, 8, stepR_);
    cue(capComes_, 0x0E, stepCyber_);
    cue(capComes_, 0x14, stepR_);
    cue(capCovers_, 7, stepCyber_);
    ambientTimer_ = std::make_unique<Event>();
    music_ = playMusic("Loc23_PirateShip");
    if (music_) {
        music_->setReverb(4, f32(0x3E54FDF4), f32(0x3EF4BC6A), 0.0f);
        music_->setReverbMix(f32(0x3DCCCCCD));  // 0x004375B0
    }
}

void Loc24::destroyLocal() {
    ambientTimer_.reset();
    for (auto* b : {&sword1_, &sword2_, &swordPull_, &workFix_, &stepL_, &stepR_, &stepCyber_, &ocean_,
                    &seagull_, &woosh_, &thunder_, &thunder2_, &danger_, &piano_})
        b->reset();
}

bool Loc24::walk2(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo2
    return wait();
}

bool Loc24::moveWait(Layer* l, double x, double y, double z, double zoom, int steps) {
    const int cb = beginWait();
    if (l) l->moveTo(x, y, z, zoom, steps, cb);  // 0x004475B0
    return wait();
}

bool Loc24::playRange(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

void Loc24::hideSabrina() {
    for (Layer* l : {static_cast<Layer*>(sabAttack2_), static_cast<Layer*>(sabCovers1_),
                     static_cast<Layer*>(sabCovers2_), static_cast<Layer*>(sabDrawsSword_),
                     static_cast<Layer*>(sabWalk_)})
        if (l) l->disable();
}

void Loc24::hideCaptain() {
    for (AnimLayer* a : {capComes_, capAttack1_, capAttack2_, capAttack3_, capCameTalk_, capCovers_})
        if (a) a->disable();
}

void Loc24::playFight(AnimLayer* a, int ev, int from, int to) {
    // Kampfbild abspielen (Ereignis am Ende) und anklickbar machen
    if (!a) return;
    a->playRange(11, ev, from, to);
    if (a->area) a->area->active = true;  // +0xB8
}

void Loc24::shiftFighters() {
    // 0x00446EC0: Kampf um 56 bzw. 36 Punkte nach rechts
    for (AnimLayer* a : {sabAttack2_, sabCovers1_})
        if (a) a->moveBy(0x38, 0);
    for (AnimLayer* a : {capAttack1_, capAttack2_, capCovers_})
        if (a) a->moveBy(0x24, 0);
}

void Loc24::fightRound() {
    switch (round_) {
    case 0:  // Kapitän greift an, Sabrina pariert
        hideSabrina();
        hideCaptain();
        playFight(capAttack1_, 10, 0x0E, 0x1B);
        playFight(sabCovers1_, 0x0C, 0, 5);
        return;
    case 1:
        hideCaptain();
        playFight(capAttack2_, 0x0D, 0, 10);
        if (capAttack2_) {
            capAttack2_->setStepCount(0);       // 0x00443300
            capAttack2_->setCountEvent(0x0B, 8);  // 0x00443530: Sabrina pariert bei Schritt 8
        }
        return;
    case 2:
        hideCaptain();
        playFight(capCovers_, 0x11, 0, 10);
        if (capCovers_) {
            capCovers_->setStepCount(0);
            capCovers_->setCountEvent(0x0F, 8);  // Sabrina greift an
        }
        return;
    case 3:  // Rodrigo trägt den Helm: der Kapitän stolpert
        hideCaptain();
        hideSabrina();
        speak("Loc24", "103_CAP_06", 0x1E);  // CGame_SayNoActor, Ende → Ereignis 0x1E
        playFight(capAttack3_, 0x12, 0, 0x0F);
        if (sabCovers2_) {
            sabCovers2_->playRangeNoEvent(11, 0, 0x0B);
            if (sabCovers2_->area) sabCovers2_->area->active = true;
        }
        return;
    default:
        return;
    }
}

void Loc24::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    setEventProc(&Loc24::onEvent);
    scene_.disableAll();
    scene_.disableInput();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    if (table_) table_->enable();
    if (steam_) steam_->start(0x0D);
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(10, 30) * 100), 0, kAmbient);
    if (ocean_) ocean_->playLoop();  // vtable +0x10
    InventoryBar* bar = scene_.inventory();
    if (bar && !bar->has(kBottle) && bottle_) {
        bottle_->enable();
        if (bottle_->area) bottle_->area->setEvent(7);
    }
    if (bar && !bar->has(kHelm) && helm_) {
        helm_->enable();
        if (helm_->area) helm_->area->setEvent(0x16);
    }
    if (restored_ == 0) {
        // Rodrigo kommt herunter, Sabrina folgt und zieht ihr Schwert, der Kapitän erscheint
        if (hero_) {
            hero_->setPosDir(0x8E, 0x108, 6);
            hero_->walkTo(0x11F, 0x154, 0, 7);  // CActor_WalkTo2 ohne Ereignis
        }
        delay(0x5DC);
        if (sabWalk_) {
            sabWalk_->enable();
            sabWalk_->setPosZ(108.0, 139.0, 198.0, 90.0);
            sabWalk_->playRangeNoEvent(13, 0x54, 0x5F);
        }
        moveWait(sabWalk_, 178.0, 0.0, 0.0, 0.0, 11);
        if (sabWalk_) sabWalk_->playRangeNoEvent(13, 0x0C, 0x17);
        moveWait(sabWalk_, 174.0, 133.0, 125.0, 118.0, 10);
        if (sabWalk_) sabWalk_->playRangeNoEvent(13, 0x54, 0x5F);
        moveWait(sabWalk_, 284.0, 133.0, 152.0, 118.0, 10);
        for (int f : {4, 0x10, 0x1C}) {
            if (sabWalk_) sabWalk_->showFrame(f);
            delay(100);
        }
        if (sabWalk_) sabWalk_->disable();
        if (sabDrawsSword_) sabDrawsSword_->enable();
        playRange(capComes_, 11, 0, 9);
        if (capComes_) capComes_->setZ(0x73);  // 0x00446FD0
        playRange(capComes_, 10, 10, -1);
        if (hero_) hero_->turnTo(4);  // 0x00435BA0
        {
            const int cb = beginWait();
            speak("Loc24", "103_CAP_01", capCameTalk_, 0x0B, 0, 0x2A, cb);
            wait();
        }
        if (sabDrawsSword_) sabDrawsSword_->start(11);
        {
            const int cb = beginWait();
            speak("Loc24", "103_SAB_01", cb);  // CGame_SayNoActor
            wait();
        }
        if (capCameTalk_) capCameTalk_->disable();
        playRange(capAttack1_, 11, 0, 0x0D);
    } else if (hero_) {
        hero_->show(scene_.clicksEnabled);
    }
    if (flag(kShifted) != 0) shiftFighters();
    fightRound();
    if (oven_) oven_->setEvent(1);
    if (lever_) lever_->setEvent(2);
    if (instruments_) instruments_->setEvent(3);
    if (to102_) to102_->setEvent(4);
    if (swords_) swords_->setEvent(5);
    if (table_ && table_->area) table_->area->setEvent(6);
    for (AnimLayer* a : {capAttack1_, capAttack2_, capAttack3_, capCovers_})
        if (a && a->area) a->area->setEvent(0x14);
    for (AnimLayer* a : {sabAttack2_, sabCovers1_, sabCovers2_})
        if (a && a->area) a->area->setEvent(0x15);
    if (bar && !bar->has(kHelm) && hero_) hero_->blockRect(kBlockL, kBlockT, kBlockR, kBlockB);
    scene_.enableInput();
}

void Loc24::onEvent(Event& e, int) {
    switch (e.param) {
    case 1: walkAndComment(0xE6, 0x15E, 2, "103_ROD_08"); return;
    case 2: walkAndComment(0xD9, 0x147, 3, "103_ROD_10"); return;
    case 3: walkAndComment(0xD9, 0x147, 3, "103_ROD_09"); return;
    case 4: {  // Treppe → 102
        auto* bar = scene_.inventory();
        if (!bar || !bar->has(kHelm)) {
            walkAndComment(0xE4, 0x14B, 4, "103_ROD_05");
            return;
        }
        if (walk2(0x8E, 0x108, 2)) {
            flag(kLeftShip) = 1;
            changeScene(0x66, 0x17);
        }
        return;
    }
    case 5: walkAndComment(0x1A8, 0x165, 4, "103_ROD_11"); return;
    case 6: walkAndComment(0x1A8, 0x165, 4, "103_ROD_04"); return;
    case 7:  // Flasche (Gegenstand 24)
        if (!walk2(0x1D5, 0x162, 4)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playRange(rodPickUp_, 10, 0, 9);
        if (bottle_) bottle_->disable();
        if (auto* bar = scene_.inventory()) bar->add(kBottle);
        playRange(rodPickUp_, 10, 10, -1);
        if (hero_) hero_->show(scene_.clicksEnabled);
        return;
    case 10:  // Runde vorbei
        if (round_ != 3) round_ = random(0, 2);
        fightRound();
        return;
    case 0x0B:
        hideSabrina();
        playFight(sabCovers1_, 0x0C, 0, 5);
        return;
    case 0x0C:
        if (sabCovers1_) {
            sabCovers1_->playRangeNoEvent(11, 4, 0);
            if (sabCovers1_->area) sabCovers1_->area->active = true;
        }
        return;
    case 0x0D:
        playFight(capAttack2_, 10, 9, 0);
        return;
    case 0x0F:
        hideSabrina();
        playFight(sabAttack2_, 0x10, 0, 5);
        return;
    case 0x10:
        if (sabAttack2_) {
            sabAttack2_->playRangeNoEvent(11, 4, 0);
            if (sabAttack2_->area) sabAttack2_->area->active = true;
        }
        return;
    case 0x11:
        playFight(capCovers_, 10, 9, 0);
        return;
    case 0x12:  // Kapitän ist gestolpert: Kampf rückt nach rechts
        round_ = 0;
        fightRound();
        shiftFighters();
        flag(kShifted) = 1;
        return;
    case 0x14:
        speak("Loc24", "103_SAB_02", 0);
        return;
    case 0x15:
        walkAndComment(0x178, 0x162, 4, "103_ROD_03");
        return;
    case 0x16:  // Helm (Gegenstand 5)
        if (!walk2(0x20E, 0x144, 2)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        if (helm_) helm_->disable();
        {
            const int cb = beginWait();
            if (rodTakesHelm_) rodTakesHelm_->playEndEvent(10, cb);
            wait();
        }
        if (auto* bar = scene_.inventory()) bar->add(kHelm);
        round_ = 3;
        if (hero_) {
            hero_->show(scene_.clicksEnabled);
            hero_->walkTo(0x1D7, 0x15D, 0, 1);
        }
        return;
    case 0x1E:  // nach 103_CAP_06: Weg frei
        speak("Loc24", "103_SAB_07", 0);
        if (hero_) hero_->unblockRect(kBlockL, kBlockT, kBlockR, kBlockB);
        return;
    case kAmbient: {
        const int r = random(1, 16);
        AudioBuffer* b = nullptr;
        if (r >= 1 && r <= 6) {
            if (ocean_) ocean_->fadeFreq(random(0x50, 100), 3000);  // CAudioBuffer_FadeFreq
        } else if (r <= 9) {
            b = seagull_.get();
        } else if (r == 10) {
            b = thunder_.get();
        } else if (r == 11) {
            b = thunder2_.get();
        }
        if (b) playSfxRandom3D(*b);
        setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(10, 30) * 100), 0, kAmbient);
        return;
    }
    default:
        return;
    }
}

}  // namespace rah
