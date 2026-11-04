// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc35.hpp"

#include "engine/inventory.hpp"

namespace rah {

namespace {
constexpr int kTimerEvent = 1;  // EventValue(1)
constexpr int kAnimEvent = 2;   // EventValue(2): Animationsende
constexpr int kItemEvent = 3;   // Gegenstand 27 benutzt
constexpr int kItem = 27;
}  // namespace

AnimLayer* Loc35::obj(std::string_view name) {
    // Original prüft weder auf fehlende noch auf statische Ebenen; alle Namen vorhanden und
    // animiert (A16/AVI, tools/name_check.py) – die Prüfungen sind reiner Port-Schutz
    return dynamic_cast<AnimLayer*>(scene_.getObject(name));
}

Loc35::Loc35(GameContext& ctx, int scene, int prevScene) : Game(ctx, 1, scene, prevScene) {
    getViewObjects();
    scene_.load("S1", 0);
    getViewObjects();
    timer_ = std::make_unique<Event>();
    back_ = obj("S1_Back");
    back2_ = obj("S1_Back2");
    toCloseup_ = obj("S1_BackToCloseup");
    fromCloseup_ = obj("S1_CloseupToBack");
    checksItems_ = obj("S1_RodChecksItems");
    sign_ = obj("S1_Schild");
    drawsBlaster_ = obj("S1_RodDrawsBlaster");
    onGlider_ = obj("S1_RodOnGlider");
    closeup_ = obj("S1_RodToCloseup");
    glider_ = loadSfxVol("Sfx_Glider_Flying", 75);
    if (drawsBlaster_) {
        // orig: Rent-A-Hero.exe 0x00442C70 CAnimLayer_AddCueEx(61, puffer, 0, 30, 1000, 30, 1000, 0, 0, 0)
        AnimCue c;
        c.type = 2;
        c.frame = 61;
        c.buffer = glider_.get();
        c.action = 0;
        c.vol = 30;
        c.volMs = 1000;
        c.freq = 30;
        c.freqMs = 1000;
        drawsBlaster_->addCue(c);
    }
    signSfx_ = loadSfxVol("Sfx_SignFlyingBy", 80);
    door_ = loadSfx("Sfx_Door_Opened");
    music_ = playMusic("Loc35_RodrigoOnGlider");
}

void Loc35::destroyLocal() {
    timer_.reset();
    glider_.reset();
    signSfx_.reset();
    door_.reset();
}

void Loc35::start() {
    if (music_) music_->play();  // vtable +0x10
    setEventProc(&Loc35::onEvent);
    scene_.disableAll();
    if (auto* bar = scene_.inventory()) {
        bar->add(kItem);
        bar->setItemEvent(kItem, kItemEvent);
    }
    if (back_) back_->playRangeNoEvent(13, 1, -1);
    if (onGlider_) onGlider_->playRange(10, kAnimEvent, 1, -1);
    setTimerHandler(*timer_, 2000, 0, kTimerEvent);
    if (glider_) glider_->playLoop();  // vtable +0x10
}

void Loc35::onEvent(Event& e, int) {
    if (e.param == kTimerEvent) {
        switch (stage_) {
        case 1:
            speak("Loc35", "001_ROD_01", 0);
            setTimerHandler(*timer_, 5000, 0, kTimerEvent);
            stage_ = 2;
            return;
        case 2:
            if (action_ != 10) action_ = 2;
            setTimerHandler(*timer_, 20000, 0, kTimerEvent);
            stage_ = 3;
            return;
        case 3:
        case 4:
        case 5:
            if (action_ != 10) action_ = 5;
            ++stage_;
            return;
        case 6:
            if (action_ != 10) action_ = 9;
            return;
        default: return;
        }
    }
    if (e.param == kItemEvent) {
        if (auto* bar = scene_.inventory()) bar->remove(kItem);
        if (door_) door_->play();
        action_ = 10;
        return;
    }
    if (e.param != kAnimEvent) return;
    lastAction_ = action_;
    switch (action_) {
    case 1:
        if (onGlider_) onGlider_->playRange(10, kAnimEvent, 1, -1);
        return;
    case 2:
        if (onGlider_) onGlider_->disable();
        if (checksItems_) checksItems_->playRange(10, kAnimEvent, 1, -1);
        speak("Loc35", "001_ROD_02", 0);
        if (action_ != 10) action_ = 3;
        return;
    case 3:
        if (checksItems_) checksItems_->playRange(10, kAnimEvent, 1, -1);
        speak("Loc35", "001_ROD_03", 0);
        if (action_ != 10) action_ = 4;
        return;
    case 4:
        scene_.enableInput();
        if (onGlider_) onGlider_->playRange(10, kAnimEvent, 1, -1);
        if (checksItems_) checksItems_->disable();
        return;
    case 5: {
        scene_.disableInput();
        if (onGlider_) onGlider_->disable();
        if (closeup_) closeup_->playRange(10, 0, 1, -1);
        if (back_) back_->disable();
        int cb = beginWait();
        if (toCloseup_) toCloseup_->playRange(10, cb, 1, -1);
        wait();
        if (toCloseup_) toCloseup_->disable();
        if (back2_) back2_->playRange(13, 0, 1, -1);
        const char* key = stage_ == 4 ? "011_ROD_01" : stage_ == 5 ? "011_ROD_02" : stage_ == 6 ? "011_ROD_03" : nullptr;
        if (key) {
            cb = beginWait();
            speak("Loc35", key, closeup_, 0x0C, 0x29, 0x2C, cb);
            wait();
        }
        if (closeup_) closeup_->playRange(11, 0, 0x28, 1);  // rückwärts
        scene_.enableInput();
        if (back2_) back2_->disable();
        cb = beginWait();
        if (fromCloseup_) fromCloseup_->playRange(10, cb, 1, -1);
        wait();
        if (closeup_) closeup_->disable();
        if (fromCloseup_) fromCloseup_->disable();
        if (back_) back_->playRange(13, 0, 1, -1);
        setTimerHandler(*timer_, 6000, 0, kTimerEvent);
        if (onGlider_) onGlider_->playRange(10, kAnimEvent, 1, -1);
        if (action_ != 10) action_ = 4;
        return;
    }
    case 9:
        scene_.disableInput();
        if (onGlider_) onGlider_->disable();
        speak("Loc35", "011_ROD_04", 0);
        if (auto* bar = scene_.inventory()) bar->remove(kItem);
        break;
    case 10:
        scene_.disableInput();
        if (onGlider_) onGlider_->disable();
        break;
    default: return;
    }
    // 9 und 10: Blaster ziehen bis Bild 60, Schild, Rest ab Bild 61 → Intro Teil 2
    int cb = beginWait();
    if (drawsBlaster_) drawsBlaster_->playRange(11, cb, 1, 60);
    wait();
    if (signSfx_) signSfx_->play();
    if (sign_) sign_->start(10);
    cb = beginWait();
    if (drawsBlaster_) drawsBlaster_->playRange(11, cb, 61, -1);
    wait();
    changeScene(115, 40);
}

}  // namespace rah
