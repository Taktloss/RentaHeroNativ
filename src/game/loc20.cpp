// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc20.hpp"

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
constexpr std::size_t kTharainGone = 0x3FEC;  // Tharain nicht mehr da (gesetzt in Loc22)
constexpr std::size_t kFirstVisit = 0x4020;   // erster Besuch: Begrüßung (Start 1)
constexpr std::size_t kTalkDone4024 = 0x4024;  // nach dem letzten Gespräch 1; wird nirgends gelesen (einziger Schreiber 0x00422C02)
constexpr std::size_t kTo71 = 0x4028;         // Weg nach 71 offen (liest Loc19)
constexpr std::size_t kTalkDone = 0x402C;     // Gespräch beendet – Szene darf verlassen werden
constexpr std::size_t kTalk = 0x4030;         // Gesprächsstufe mit Tharain (1…8)
constexpr int kTo69 = 1;
constexpr int kFloor = 2;
constexpr int kTharain = 10;
}  // namespace

Loc20::Loc20(GameContext& ctx, int scene, int prevScene) : Game(ctx, 20, scene, prevScene) {
    getViewObjects();
    sceneNr_ = 0x46;
    scene_.load("S70", flag(kTharainGone) == 0 ? 1u : 0u);  // DAT_00474404
    getViewObjects();
    desk_ = scene_.getObject("S70_Pult");
    book_ = scene_.getObject("S70_Book");
    rodCatch_ = obj("S70_RodCatchStone");
    tharain_ = obj("S70_Tharain");
    stoneFlies_ = obj("S70_StoneFlies");
    thaTakes_ = obj("S70_ThaTakesStone");
    stone_ = scene_.getObject("S70_Stone");
    const char* colorNames[] = {"S70_Blue", "S70_Cyan", "S70_Green", "S70_Red", "S70_Violet", "S70_Yellow"};
    for (int i = 0; i < 6; ++i) colors_[i] = scene_.getObject(colorNames[i]);
    to69_ = scene_.getArea("S70_To69");
    floor_ = scene_.getArea("S70_Boden");
    if (flag(kTharainGone) == 0) {
        mobileDown_ = loadSfx("Sfx_MobileDown");
        if (stoneFlies_) {
            AnimCue c;
            c.type = 1;
            c.frame = 0;
            c.buffer = mobileDown_.get();
            stoneFlies_->addCue(c);  // CAnimLayer_AddCue 0x00442BB0
        }
    }
    music_ = playMusic("Loc20_Library");
    if (music_) {
        music_->setReverb(10, 0.5f, 10.0f, 0.3f);  // 0x3F000000, 0x41200000, 0x3E99999A
        music_->setReverbMix(0.1f);                // 0x3DCCCCCD
    }
}

void Loc20::destroyLocal() { mobileDown_.reset(); }

bool Loc20::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo
    return wait();
}

bool Loc20::playEnd(AnimLayer* a, unsigned mode) {
    const int cb = beginWait();
    if (a) a->playEndEvent(mode, cb);
    return wait();
}

bool Loc20::playRange(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

void Loc20::turn(int dir) {
    const int cb = beginWait();
    if (hero_) hero_->turn(dir, cb);  // CActor_Turn 0x00435BF0
    wait();
}

void Loc20::tharainSay(const char* key) {
    const int cb = beginWait();
    speak("Loc20", key, tharain_, 0x0C, 0x0F, 0x2D, cb);
    wait();
}

void Loc20::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    setEventProc(&Loc20::onEvent);
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    if (to69_) to69_->setEvent(kTo69);
    if (hero_) {
        hero_->show(scene_.clicksEnabled);
        if (restored_ == 0) hero_->setPosDir(0x1C, 0x138, 6);
    }
    if (desk_) desk_->enable();
    if (book_) book_->enable();
    for (Layer* c : colors_)
        if (c) c->enable();
    if (flag(kFirstVisit) == 0) {
        // eine zufällige Farbe fehlt
        const int k = random(1, 6);
        if (k >= 1 && k <= 6 && colors_[k - 1]) colors_[k - 1]->disable();
    }
    if (flag(kTharainGone) != 0) {
        if (floor_) floor_->setEvent(kFloor);
        if (restored_ == 0 && hero_) hero_->walkTo(0xFD, 0x159, 0, 5);  // CActor_WalkTo2
        scene_.enableInput();
        return;
    }
    if (tharain_) {
        tharain_->enable();
        if (tharain_->area) tharain_->area->setEvent(kTharain);
    }
    if (book_ && book_->area) book_->area->setEvent(kTharain);
    if (desk_ && desk_->area) desk_->area->setEvent(kTharain);
    if (flag(kTalk) < 6 && stone_) {
        stone_->enable();
        if (stone_->area) stone_->area->setEvent(kTharain);
    }
    if (flag(kFirstVisit) != 0) scene_.disableInput();
    if (restored_ == 0) walk(0xFD, 0x159, 5);
    if (flag(kFirstVisit) != 0) {
        tharainSay("070_THA_01");
        heroSay("070_ROD_01");
        tharainSay("070_THA_02");
        flag(kFirstVisit) = 0;
    }
    scene_.enableInput();
}

void Loc20::onEvent(Event& e, int) {
    switch (e.param) {
    case kTo69:
        if (flag(kTalkDone) == 0) {
            // Tharain ruft Rodrigo zurück
            scene_.disableInput();
            if (hero_) hero_->walkTo(0x7C, 0x150, 0, 3);  // CActor_WalkTo2 ohne Warten
            tharainSay("070_THA_13");
            walk(0xFD, 0x159, 5);
            turn(0);
            heroSay("070_ROD_13");
            turn(4);
            scene_.enableInput();
            return;
        }
        // Original: zweites CGame_Wait ohne neuen Warteplatz
        if (walk(0x1C, 0x138, 2) && wait()) changeScene(0x45, 0x13);
        return;
    case kFloor:
        walkAndComment(0x1B3, 0x132, 1, "070_ROD_14");
        return;
    case kTharain:
        if (!walk(0xFD, 0x159, 5)) return;
        switch (flag(kTalk)) {
        case 1:
            heroSay("070_ROD_03");
            tharainSay("070_THA_03");
            flag(kTalk) = 2;
            return;
        case 2:
            heroSay("070_ROD_04");
            tharainSay("070_THA_04");
            flag(kTalk) = 3;
            return;
        case 3:
            heroSay("070_ROD_05");
            tharainSay("070_THA_05");
            heroSay("070_ROD_06");
            flag(kTalk) = 4;
            return;
        case 4:
            heroSay("070_ROD_07");
            tharainSay("070_THA_07");
            flag(kTalk) = 5;
            return;
        case 5:
            // Tharain nimmt den Stein
            heroSay("070_ROD_08");
            tharainSay("070_THA_08");
            scene_.disableInput();
            if (stone_) stone_->disable();
            if (thaTakes_) thaTakes_->start(10);
            playRange(tharain_, 11, 0, 0x0E);
            if (stoneFlies_) stoneFlies_->enable();
            scene_.enableInput();
            flag(kTalk) = 6;
            return;
        case 6:
            // Tharain wirft Rodrigo den Stein zu (Gegenstand 21)
            heroSay("070_ROD_09");
            tharainSay("070_THA_09");
            scene_.disableInput();
            speak("Loc20", "070_THA_10", tharain_, 0x0C, 0x0F, 0x2D, 0);  // ohne Warten
            if (hero_) hero_->walkTo(0x126, 0x138, 0, 5);
            playRange(stoneFlies_, 11, 0, 0x0C);
            if (hero_) hero_->hide(scene_.clicksEnabled);
            if (stoneFlies_) stoneFlies_->playRangeNoEvent(10, 0x0D, -1);
            playEnd(rodCatch_, 10);
            if (hero_) hero_->show(scene_.clicksEnabled);
            walk(0xFD, 0x159, 5);
            if (auto* bar = scene_.inventory()) bar->add(0x15);  // Inventory_Add
            scene_.enableInput();
            flag(kTalk) = 7;
            return;
        case 7:
            heroSay("070_ROD_11");
            tharainSay("070_THA_11");
            flag(kTalk) = 8;
            return;
        case 8:
            heroSay("070_ROD_12");
            tharainSay("070_THA_12");
            flag(kTalkDone4024) = 1;
            flag(kTo71) = 1;
            flag(kTalkDone) = 1;
            return;
        default:
            return;
        }
    default:
        return;
    }
}

}  // namespace rah
