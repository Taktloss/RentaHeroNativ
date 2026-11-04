// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc22.hpp"

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
constexpr std::size_t kTharainGone = 0x3FEC;  // Tharain ist fort (Loc20 ohne Tharain)
constexpr std::size_t kFirstVisit = 0x4060;   // erster Besuch: Kommentar 081_ROD_01 (Start 1)
constexpr std::size_t kSlots = 0x4064;        // 6 Plätze (12, 2, 4, 6, 8, 10 Uhr): Kristall-Nummer oder 0
constexpr int kBag = 0x10;                    // Beutel mit Kristallen
constexpr int kTo1081 = 1, kTo79 = 2, kTo81 = 3, kBigStone = 4, kSlot0 = 10, kOpenBag = 0x1E;
const char* const kClock[6] = {"12", "02", "04", "06", "08", "10"};
}  // namespace

Loc22::Loc22(GameContext& ctx, int scene, int prevScene) : Game(ctx, 22, scene, prevScene) {
    getViewObjects();
    if (sceneNr_ == 0x51) {
        scene_.load("S81", 0xFFFF);  // DAT_00474AD0, CSzene_LoadAll
        getViewObjects();
        enter_ = loadAnim("S81_Enter");
        exit_ = loadAnim("S81_Exit");
        to1081_ = loadAnim("S81_To1081");
        for (int i = 0; i < 6; ++i)
            stones81_[i] = scene_.getObject(std::string("S81_Stone") + kClock[i] + "Uhr");
        to1081_Area_ = scene_.getArea("S81_To1081");
        to79_ = scene_.getArea("S81_To79");
    } else if (sceneNr_ == 0x439) {
        scene_.load("S1081", 0xFFFF);
        getViewObjects();
        bigStone_ = obj("S1081_BigStone");
        for (int i = 0; i < 6; ++i) rod_[i] = obj(std::string("S1081_Rod") + kClock[i] + "Uhr");
        for (int i = 0; i < 6; ++i)
            stones_[i] = scene_.getObject(std::string("S1081_Stone") + kClock[i] + "Uhr");
        rodStroke_ = obj("S1081_RodStreich");
        to81_ = scene_.getArea("S1081_To81");
        danger_ = loadSfx("Sfx_DangerStringsHigh");
        door_ = loadSfx("Sfx_Door_Opened");
        mobileDown_ = loadSfx("Sfx_MobileDown");
        bigStoneSfx_ = loadSfx("Sfx_BigStone");
        const auto cue = [](AnimLayer* a, int frame, AudioBuffer* b) {
            if (!a) return;
            AnimCue c;
            c.type = 1;
            c.frame = frame;
            c.buffer = b;
            a->addCue(c);  // CAnimLayer_AddCue 0x00442BB0
        };
        for (AnimLayer* r : rod_) cue(r, 0x1B, door_.get());
        for (int f : {1, 0x0B, 0x15, 0x1F, 0x29, 0x33}) cue(bigStone_, f, bigStoneSfx_.get());
    }
    music_ = playMusic("Loc22_CrystalCave");
    if (music_) {
        music_->setReverb(10, 0.5f, 10.0f, 0.3f);  // 0x3F000000, 0x41200000, 0x3E99999A
        music_->setReverbMix(0.1f);                // 0x004375B0
    }
}

void Loc22::destroyLocal() {
    enter_.reset();
    exit_.reset();
    to1081_.reset();
    danger_.reset();
    door_.reset();
    mobileDown_.reset();
    bigStoneSfx_.reset();
}

bool Loc22::playEnd(AnimLayer* a, unsigned mode) {
    const int cb = beginWait();
    if (a) a->playEndEvent(mode, cb);
    return wait();
}

bool Loc22::playRange(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

bool Loc22::anyPlaced() {
    bool any = false;
    for (int i = 0; i < 6; ++i)
        if (flag(kSlots + 4 * static_cast<std::size_t>(i)) != 0) any = true;
    return any;
}

void Loc22::setCrystalEvents() {
    if (auto* bar = scene_.inventory())
        for (int id = 0x11; id <= 0x16; ++id) bar->setItemEvent(id, id);  // Inventory_SetEvent
}

void Loc22::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    setEventProc(&Loc22::onEvent);
    scene_.disableAll();
    scene_.disableInput();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    if (sceneNr_ == 0x51) {
        if (to1081_Area_) to1081_Area_->setEvent(kTo1081);
        if (to79_) to79_->setEvent(kTo79);
        for (int i = 0; i < 6; ++i)
            if (flag(kSlots + 4 * static_cast<std::size_t>(i)) != 0 && stones81_[i]) stones81_[i]->enable();
        flag(kTharainGone) = 1;
        if (restored_ == 0 && hero_) {
            if (prevScene_ == 0x439) {
                hero_->setPosDir(0x199, 0x13E, 5);
                hero_->walkTo(0x186, 0x14A, 0, 1);  // CActor_WalkTo2
                scene_.enableInput();
                return;
            }
            if (!anyPlaced()) {
                // Rodrigo klettert herein (Vollbild-Animation)
                scene_.setModal(enter_.get());
                playEnd(enter_.get(), 10);
            }
            hero_->setPosDir(0xD1, 0x155, 7);
            if (flag(kFirstVisit) == 0) {
                hero_->walkTo(0xDC, 0x15E, 0, 5);
                scene_.enableInput();
                return;
            }
            const int cb = beginWait();
            hero_->walkTo(0xDC, 0x15E, cb, 5);
            wait();
            delay(1000);
            heroSay("081_ROD_01");
            flag(kFirstVisit) = 0;
            scene_.enableInput();
            return;
        }
        if (hero_) hero_->show(scene_.clicksEnabled);
    } else if (sceneNr_ == 0x439) {
        if (bigStone_) {
            bigStone_->enable();
            if (bigStone_->area) bigStone_->area->setEvent(kBigStone);
        }
        if (rodStroke_) rodStroke_->enable();
        for (int i = 0; i < 6; ++i) {
            if (flag(kSlots + 4 * static_cast<std::size_t>(i)) != 0 && stones_[i]) {
                stones_[i]->enable();
                if (stones_[i]->area) stones_[i]->area->setEvent(kSlot0 + i);
            }
        }
        setCrystalEvents();
        if (auto* bar = scene_.inventory()) bar->setItemEvent(kBag, kOpenBag);
        if (to81_) to81_->setEvent(kTo81);
    }
    scene_.enableInput();
}

void Loc22::takeCrystal(int slot) {
    const std::size_t off = kSlots + 4 * static_cast<std::size_t>(slot);
    scene_.disableInput();
    if (rodStroke_) rodStroke_->disable();
    if (stones_[slot]) stones_[slot]->disable();
    if (firstTake_ != 0) {
        speak("Loc22", "1081_ROD_02", 0);  // CGame_SayNoActor ohne Warten
        firstTake_ = 0;
    }
    playRange(rod_[slot], 10, 0x1F, 1);  // rückwärts: Hand vom Platz zurück
    if (rodStroke_) rodStroke_->enable();
    if (auto* bar = scene_.inventory()) bar->add(flag(off));
    setCrystalEvents();
    flag(off) = 0;
    scene_.enableInput();
}

void Loc22::placeCrystal(int item) {
    scene_.disableInput();
    for (int i = 0; i < 6; ++i) {
        const std::size_t off = kSlots + 4 * static_cast<std::size_t>(i);
        if (flag(off) != 0) continue;
        firstTake_ = 1;
        if (rodStroke_) rodStroke_->disable();
        if (auto* bar = scene_.inventory()) bar->remove(item);
        playRange(rod_[i], 10, 1, -1);
        if (rodStroke_) rodStroke_->enable();
        if (stones_[i]) {
            stones_[i]->enable();
            if (stones_[i]->area) stones_[i]->area->setEvent(kSlot0 + i);
        }
        // Steinscheibe dreht sich je nach Kristall um ein Sechstel (Tabelle +0xF8/+0x110)
        // Gegenstände außerhalb 17…22 würden im Original fremde Felder lesen; nicht erreichbar: nur die
        // Ereignisse 0x11…0x16 führen hierher, registriert für genau diese Gegenstände (setCrystalEvents)
        const int k = item - 0x11;
        if (k >= 0 && k < 6) playRange(bigStone_, 11, turnFrom_[k], turnTo_[k]);
        flag(off) = item;
        break;
    }
    if (flag(kSlots) == 0x15 && flag(kSlots + 4) == 0x16 && flag(kSlots + 8) == 0x11 &&
        flag(kSlots + 12) == 0x12 && flag(kSlots + 16) == 0x14 && flag(kSlots + 20) == 0x13) {
        // gelöst: die Steinscheibe öffnet sich → Loc09, Szene 89
        if (danger_) danger_->play();  // vtable +0x08
        playEnd(bigStone_, 11);
        changeScene(0x59, 9);
    }
    scene_.enableInput();
}

void Loc22::onEvent(Event& e, int type) {
    switch (e.param) {
    case kTo1081: {
        const int cb = beginWait();
        if (hero_) hero_->walkTo(0x199, 0x13E, cb, 5);
        if (!wait()) return;
        if (!anyPlaced()) {
            scene_.setModal(to1081_.get());
            playEnd(to1081_.get(), 11);
        }
        changeScene(0x439, 0x16);
        return;
    }
    case kTo79: {
        const int cb = beginWait();
        if (hero_) hero_->walkTo(0xD1, 0x14C, cb, 4);
        if (!wait()) return;
        if (!anyPlaced()) {
            scene_.setModal(exit_.get());
            playEnd(exit_.get(), 11);
        }
        changeScene(0x4F, 0x15);
        return;
    }
    case kTo81:
        changeScene(0x51, 0x16);
        return;
    case kBigStone: {
        const int r = random(1, 2);
        if (r == 1) {
            scene_.disableInput();
            speak("Loc22", "1081_ROD_01", 0);
            playEnd(rodStroke_, 12);
            scene_.enableInput();
        } else if (r == 2) {
            speak("Loc22", "1081_ROD_03", 0);
        }
        return;
    }
    case 10: case 11: case 12: case 13: case 14: case 15:
        takeCrystal(e.param - kSlot0);
        return;
    case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16:
        placeCrystal(type);  // Original: Gegenstand aus dem zweiten Parameter
        return;
    case kOpenBag:
        // Beutel öffnen: Kristalle 17–20 und 22 (21 ist Tharains Stein aus Loc20)
        scene_.disableInput();
        speak("Loc22", "1081_ROD_04", 0);
        if (mobileDown_) mobileDown_->play();
        if (auto* bar = scene_.inventory()) {
            bar->remove(kBag);
            bar->add(0x11);
            delay(100);
            bar->add(0x12);
            delay(100);
            bar->add(0x13);
            delay(100);
            bar->add(0x14);
            delay(100);
            bar->add(0x16);
        }
        setCrystalEvents();
        scene_.enableInput();
        return;
    default:
        return;
    }
}

}  // namespace rah
