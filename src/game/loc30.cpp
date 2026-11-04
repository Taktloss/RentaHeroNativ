// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc30.hpp"

#include <bit>

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
float f32(std::uint32_t bits) { return std::bit_cast<float>(bits); }
constexpr std::size_t kWindowSeen = 0x40FC;  // durchs Fenster geschaut (Szene 35) → zweite Reihe
constexpr std::size_t kGridHint = 0x4100;    // Jasper hat vom Gitter erzählt
constexpr std::size_t kTalk1 = 0x4104;       // erste Gesprächsreihe (1…3)
constexpr std::size_t kTalk2 = 0x4108;       // zweite Gesprächsreihe (1…4)
constexpr int kFromWindow = 0x23;
}  // namespace

Loc30::Loc30(GameContext& ctx, int scene, int prevScene) : Game(ctx, 30, scene, prevScene) {
    scene_.load("S34", 0xFFFF);  // CSzene_LoadAll(DAT_00475F04); Original ohne GetViewObjects davor
    getViewObjects();
    jasGetsUp_ = obj("S34_JasAufsteh");
    jasTalk_ = obj("S34_JasStehTalk");
    rodKnock_ = obj("S34_RodKnock");
    straw_ = scene_.getObject("S34_Straw");
    gridOpen_ = scene_.getObject("S34_GridOpen");
    winding_ = loadSfx("Sfx_Winding");
    tock_ = loadSfx("Sfx_Tock");
    boxPushed_ = loadSfx("Sfx_Box_Pushed_Short");
    clothes_ = loadSfx("Sfx_Clothes_Scratched");
    const auto cue = [](AnimLayer* a, int frame, AudioBuffer* b) {
        if (!a) return;
        AnimCue c;
        c.type = 1;
        c.frame = frame;
        c.buffer = b;
        a->addCue(c);  // CAnimLayer_AddCue 0x00442BB0
    };
    cue(rodKnock_, 0x0B, tock_.get());
    cue(rodKnock_, 0x13, tock_.get());
    if (prevScene_ != kFromWindow) {
        rodComesIn_ = loadAnim("S34_RodReinCam");
        knock_ = loadSfx("Sfx_Knock_Wet");
        cue(rodComesIn_.get(), 0x0E, tock_.get());
        cue(rodComesIn_.get(), 0x18, knock_.get());
    }
    music_ = playMusic("Loc30_PrisionInEndavin");
    if (music_) {
        music_->setReverb(2, f32(0x3ED58106), f32(0x3ECCCCCD), f32(0x3F2A7EFA));
        music_->setReverbMix(f32(0x3DCCCCCD));  // 0x004375B0
    }
}

void Loc30::destroyLocal() {
    rodComesIn_.reset();
    for (auto* b : {&winding_, &tock_, &boxPushed_, &knock_, &clothes_}) b->reset();
}

bool Loc30::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo
    return wait();
}

void Loc30::jasSay(const char* key) {
    sayActor("Loc30", key, jasTalk_, 0);  // CGame_SayActor ohne Warten
}

void Loc30::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // CSzene+0x5C0
    setEventProc(&Loc30::onEvent);
    scene_.clearFlag510();  // CSzene_ClearFlag510 0x0044B190
    Layer* sun = scene_.getObject("S34_Sun");
    if (prevScene_ == kFromWindow) {
        if (hero_) {
            hero_->show(scene_.clicksEnabled);
            hero_->setPosDir(0xB6, 0x122, 0);
        }
        if (jasTalk_) jasTalk_->enable();
        if (sun) sun->enable();
        if (straw_) straw_->enable();
    } else if (restored_ == 0) {
        // Rodrigo wird hereingeworfen, Jasper steht auf
        scene_.setModal(rodComesIn_.get());
        {
            const int cb = beginWait();
            if (rodComesIn_) rodComesIn_->playEndEvent(10, cb);
            wait();
        }
        if (jasGetsUp_) jasGetsUp_->enable();
        if (sun) sun->enable();
        if (straw_) straw_->enable();
        if (InventoryBar* bar = scene_.inventory()) bar->clear();  // Inventory_Clear 0x004418D0
        if (hero_) {
            hero_->show(scene_.clicksEnabled);
            hero_->setPosDir(0x118, 0x122, 1);
        }
        if (jasGetsUp_) jasGetsUp_->playEndEvent(11, 8);
        walk(0x112, 0x132, 0);
        heroSay("034_ROD_01");
        // Original: Sprung zu den Bereichen (0x0042FB8B), kein weiteres Enable
    } else {
        if (sun) sun->enable();
        if (straw_) straw_->enable();
        if (hero_) hero_->show(scene_.clicksEnabled);
        if (jasTalk_) jasTalk_->enable();
    }
    static constexpr struct { const char* name; int ev; } kAreas[] = {
        {"S34_Door", 1}, {"S34_Ring1", 2}, {"S34_Ring2", 3}, {"S34_Plate", 4}, {"S34_Window", 5}};
    for (const auto& a : kAreas)
        if (Area* ar = scene_.getArea(a.name)) ar->setEvent(a.ev);
    if (straw_ && straw_->area) straw_->area->setEvent(7);
    if (jasTalk_ && jasTalk_->area) jasTalk_->area->setEvent(6);
    scene_.enableInput();
}

void Loc30::talkJasper() {
    if (!walk(0x165, 300, 6)) return;
    if (flag(kWindowSeen) == 0) {
        switch (flag(kTalk1)) {
        case 1:
            flag(kTalk1) = 2;
            heroSay("034_ROD_02");
            jasSay("034_JAS_02");
            return;
        case 2:
            flag(kTalk1) = 3;
            heroSay("034_ROD_03");
            jasSay("034_JAS_03");
            return;
        case 3:
            switch (random(1, 3)) {
            case 1:
                heroSay("034_ROD_04");
                jasSay("034_JAS_04");
                return;
            case 2:
                heroSay("034_ROD_05");
                jasSay("034_JAS_05");
                return;
            case 3:
                heroSay("034_ROD_06");
                jasSay(random(1, 2) == 1 ? "034_JAS_01" : "034_JAS_06");
                return;
            default:
                return;
            }
        default:
            return;
        }
    }
    switch (flag(kTalk2)) {
    case 1:
        flag(kTalk2) = 2;
        heroSay("034_ROD_07");
        jasSay("034_JAS_07");
        return;
    case 2:
        flag(kTalk2) = 3;
        heroSay("034_ROD_08");
        jasSay("034_JAS_08");
        return;
    case 3: {
        scene_.disableInput();
        flag(kTalk2) = 4;
        heroSay("034_ROD_09");
        {
            const int cb = beginWait();
            speak("Loc30", "034_JAS_09", jasTalk_, 0x0D, 0, 4, cb);
            wait();
        }
        jasSay("034_JAS_10");
        scene_.enableInput();
        return;
    }
    case 4:
        flag(kGridHint) = 1;
        heroSay("034_ROD_11");
        jasSay("034_JAS_11");
        return;
    default:
        return;
    }
}

void Loc30::onEvent(Event& e, int) {
    switch (e.param) {
    case 1:  // Tür: klopfen
        if (!walk(0x128, 0x119, 5)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        speak("Loc30", "034_JAS_12", jasTalk_, 0x0D, 0, 4, 0);
        {
            const int cb = beginWait();
            if (rodKnock_) rodKnock_->playEndEvent(10, cb);
            wait();
        }
        if (hero_) {
            hero_->show(scene_.clicksEnabled);
            hero_->walkTo(0x128, 0x122, 0, 0);  // CActor_WalkTo2 ohne Ereignis
        }
        if (jasTalk_) jasTalk_->showFrame(0);
        return;
    case 2:
        if (walk(0x1BA, 0x148, 4)) jasSay("034_JAS_16");
        return;
    case 3:
        if (walk(0x1C8, 0x14B, 7)) jasSay("034_JAS_17");
        return;
    case 4:
        if (walk(0x1C6, 0x153, 7)) jasSay("034_JAS_15");
        return;
    case 5:  // Fenster → 28/35
        if (!walk(0xB6, 0x122, 4)) return;
        flag(kWindowSeen) = 1;
        changeScene(0x23, 0x1C);
        return;
    case 6:
        talkJasper();
        return;
    case 7: {  // Stroh: Gitter
        if (!walk(0xE5, 0x13B, 7)) return;
        if (flag(kGridHint) == 0) {
            speak("Loc30", "034_JAS_13", jasTalk_, 0x0D, 0, 4, 0);
            if (hero_) hero_->hide(scene_.clicksEnabled);
            {
                const int cb = beginWait();
                if (auto* a = obj("S34_RodSchautStroh")) a->playEndEvent(10, cb);
                wait();
            }
            if (hero_) hero_->show(scene_.clicksEnabled);
            if (jasTalk_) jasTalk_->showFrame(0);
            return;
        }
        scene_.disableInput();
        speak("Loc30", "034_JAS_14", jasTalk_, 0x0D, 0, 4, 0);
        if (hero_) hero_->hide(scene_.clicksEnabled);
        if (straw_) straw_->disable();
        AnimLayer* grid = obj("S34_RodOpensGrid");
        if (grid) {
            AnimCue c;
            c.type = 1;
            c.frame = 0x18;
            c.buffer = boxPushed_.get();
            grid->addCue(c);
            c.frame = 0x42;
            c.buffer = winding_.get();
            grid->addCue(c);
        }
        {
            const int cb = beginWait();
            if (grid) grid->playRange(11, cb, 0, 0x29);
            wait();
        }
        if (InventoryBar* bar = scene_.inventory()) bar->add(0x0F);
        {
            const int cb = beginWait();
            if (grid) grid->playRange(10, cb, 0x2A, -1);
            wait();
        }
        if (jasTalk_) jasTalk_->showFrame(0);
        speak("Loc30", "034_ROD_12", 0);  // CGame_SayNoActor
        AnimLayer* shaft = obj("S34_RodInSchacht");
        if (shaft) {
            AnimCue c;
            c.type = 1;
            c.frame = 0x16;
            c.buffer = clothes_.get();
            shaft->addCue(c);
        }
        {
            const int cb = beginWait();
            if (shaft) shaft->playEndEvent(10, cb);
            wait();
        }
        scene_.enableInput();
        changeScene(0x24, 0x1C);  // → 28/36 (= Szene 31, Flucht)
        return;
    }
    case 8:  // Jasper ist aufgestanden
        if (jasGetsUp_) jasGetsUp_->disable();
        jasSay("034_JAS_01");
        if (jasTalk_) {
            jasTalk_->enable();
            if (jasTalk_->area) jasTalk_->area->active = true;  // +0xB8
        }
        return;
    default:
        return;
    }
}

}  // namespace rah
