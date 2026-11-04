// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc23.hpp"

#include <bit>

#include "engine/actor.hpp"
#include "engine/area.hpp"

namespace rah {

namespace {
float f32(std::uint32_t bits) { return std::bit_cast<float>(bits); }
constexpr std::size_t kBox = 0x407C;     // Kiste: 1 links (vor der Luke), 2 rechts (vor der Tür)
constexpr std::size_t kSabOut = 0x4080;  // Sabrina ist aus der Kajüte
constexpr std::size_t kNeverSet4084 = 0x4084;  // nur CGameState_Reset schreibt (0), einziger Leser 0x00427919: Zweig „Sabrina stumm“ ist im Original tot
// Sperrrechtecke
constexpr int kDoorL = 0x184, kDoorT = 0xD1, kDoorR = 0x1EA, kDoorB = 0xF8;  // vor der Kajütentür
constexpr int kBoxL = 0x16D, kBoxT = 0xE8, kBoxR = 0x235, kBoxB = 0x10A;     // Kiste links
constexpr int kSabL = 0x118, kSabT = 0xF8, kSabR = 0x14F, kSabB = 0x10B;     // Sabrina steht
}  // namespace

Loc23::Loc23(GameContext& ctx, int scene, int prevScene) : Game(ctx, 23, scene, prevScene) {
    getViewObjects();
    scene_.load("S102", 0);  // DAT_00474D60
    getViewObjects();
    escape_ = loadAnim("S102_Escape");
    enter_ = loadAnim("S102_EnterShip");
    boxLeft_ = scene_.getObject("S102_KisteLinks");
    boxToLeft_ = obj("S102_KisteNachLinks");
    boxToRight_ = obj("S102_KisteNachRechts");
    boxRight_ = scene_.getObject("S102_KisteRechts");
    doorClosed_ = scene_.getObject("S102_TuerZu");
    doorOpen_ = scene_.getObject("S102_TuerOffen");
    rodShake_ = obj("S102_RodRuettel");
    sabDoor_ = obj("S102_SabTuer");
    sabWalk_ = obj("SabWalk");
    explMisc_ = loadSfxVol("Sfx_Explosion_Misc2", 0x50);
    explBass_ = loadSfxVol("Sfx_Explosion_Bass", 0x50);
    boxPushed_ = loadSfxVol("Sfx_Box_Pushed", 0x50);
    boxShort_ = loadSfxVol("Sfx_Box_Pushed_Short", 0x50);
    stepL_ = loadSfxVol("Gen_StepLeft", 0x5A);
    stepR_ = loadSfxVol("Gen_StepRight", 0x5A);
    doorSlam_ = loadSfxVol("Sfx_Door_Slammed", 0x5A);
    creak_ = loadSfx("Sfx_Wood_Creaking");
    splash_ = loadSfxVol("Sfx_WaterSplash", 0x3C);
    metalHit_ = loadSfx("Sfx_Metal_DoubleHit");
    ocean_ = loadSfxVol("Sfx_Ocean", 0x3C);
    const auto cue = [](AnimLayer* a, int frame, AudioBuffer* b) {
        if (!a) return;
        AnimCue c;
        c.type = 1;
        c.frame = frame;
        c.buffer = b;
        a->addCue(c);  // CAnimLayer_AddCue 0x00442BB0
    };
    const auto cueFreq = [](AnimLayer* a, int frame, AudioBuffer* b) {
        if (!a) return;
        AnimCue c;  // CAnimLayer_AddCueEx(bild, puffer, 1, 0, 0, 80, 0, 0, 0, 0): Frequenz 80 %
        c.type = 2;
        c.frame = frame;
        c.buffer = b;
        c.action = 1;
        c.freq = 0x50;
        a->addCue(c);
    };
    cue(sabDoor_, 0x0D, doorSlam_.get());
    for (int f : {3, 0x0E, 0x19, 0x26, 0x31, 0x3C, 0x49, 0x55}) cue(sabWalk_, f, stepL_.get());
    for (int f : {8, 0x13, 0x1E, 0x2B, 0x36, 0x42, 0x4F, 0x5B}) cue(sabWalk_, f, stepR_.get());
    cueFreq(boxToLeft_, 0x0C, boxPushed_.get());
    cueFreq(boxToLeft_, 0x29, boxShort_.get());
    cueFreq(boxToRight_, 0x0C, boxPushed_.get());
    AnimLayer* esc = escape_.get();
    cue(esc, 0x15, splash_.get());
    cue(esc, 0x34, creak_.get());
    cue(esc, 0x45, creak_.get());
    cue(esc, 0x48, splash_.get());
    for (int f : {0x51, 0x5D, 0x66, 0x71}) {
        cue(esc, f, creak_.get());
        cue(esc, f, splash_.get());
    }
    cue(esc, 0x85, explMisc_.get());
    cue(esc, 0x87, explBass_.get());
    cue(rodShake_, 0x1B, creak_.get());
    cue(rodShake_, 7, creak_.get());
    cue(rodShake_, 0x30, metalHit_.get());
    to100_ = scene_.getArea("S102_To100");
    to103_ = scene_.getArea("S102_To103");
    music_ = playMusic("Loc23_PirateShip");
    if (music_) {
        music_->setReverb(0x10, f32(0x3DE353F8), f32(0x40311687), f32(0x3E656042));
        music_->setReverbMix(f32(0x3DCCCCCD));  // 0x004375B0
    }
}

void Loc23::destroyLocal() {
    escape_.reset();
    enter_.reset();
    doorSlam_.reset();
    stepL_.reset();
    stepR_.reset();
    boxPushed_.reset();
    explBass_.reset();
    explMisc_.reset();
    creak_.reset();
    boxShort_.reset();
    splash_.reset();
    metalHit_.reset();
    ocean_.reset();
}

bool Loc23::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo
    return wait();
}

bool Loc23::playEnd(AnimLayer* a, unsigned mode) {
    const int cb = beginWait();
    if (a) a->playEndEvent(mode, cb);
    return wait();
}

bool Loc23::moveWait(Layer* l, double x, double y, double z, double zoom, int steps) {
    const int cb = beginWait();
    if (l) l->moveTo(x, y, z, zoom, steps, cb);  // 0x004475B0
    return wait();
}

void Loc23::sabSay(const char* key) {
    const int cb = beginWait();
    speak("Loc23", key, cb);  // CGame_SayNoActor
    wait();
}

void Loc23::setActive(Layer* l, bool on) {
    if (l && l->area) l->area->active = on;  // Bereich +0xB8
}

void Loc23::sabAtDoor() {
    // Sabrina steht an der Kajütentür (Bild 52) und ist anklickbar
    if (!sabWalk_) return;
    sabWalk_->enable();
    sabWalk_->setPosZ(358.0, 151.0, 1.0, 238.0);
    sabWalk_->showFrame(0x34);
    if (sabWalk_->area) sabWalk_->area->setEvent(4);
    setActive(sabWalk_, true);
}

void Loc23::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    setEventProc(&Loc23::onEvent);
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    if (ocean_) ocean_->playLoop();                    // vtable +0x10
    if (prevScene_ == 100) {
        if (boxLeft_ && boxLeft_->area) boxLeft_->area->setEvent(3);
        if (boxRight_ && boxRight_->area) boxRight_->area->setEvent(3);
        if (to100_) to100_->setEvent(1);
        if (to103_) to103_->setEvent(2);
        if (flag(kSabOut) == 0) {
            if (to103_) to103_->active = false;
            if (doorClosed_) {
                doorClosed_->enable();
                if (doorClosed_->area) doorClosed_->area->setEvent(5);
            }
        } else if (doorOpen_) {
            doorOpen_->enable();
        }
        if (flag(kBox) == 1) {
            if (boxLeft_) boxLeft_->enable();
            setActive(boxLeft_, true);
            setActive(doorClosed_, false);
        } else {
            if (boxRight_) boxRight_->enable();
            setActive(boxRight_, true);
            if (flag(kSabOut) == 0) setActive(doorClosed_, true);
        }
        if (restored_ == 0) {
            // Rodrigo klettert an Bord (Vollbild), dann als freie Ebene bis an Deck
            scene_.setModal(enter_.get());
            {
                const int cb = beginWait();
                if (enter_) enter_->playRange(10, cb, 0, -1);
                wait();
            }
            sabAtDoor();
            if (hero_) {
                hero_->setPosDir(0x83, 0x153, 4);
                scene_.disableInput();
                scene_.setHero(nullptr);    // 0x0044A6E0
                scene_.insertLayer(hero_);  // 0x0044AA00
                // Original: die Cues kommen bei jeder Ankunft erneut hinzu
                AnimCue c;
                c.type = 1;
                c.frame = 0x40;
                c.buffer = stepL_.get();
                hero_->addCue(c);
                c.frame = 0x46;
                c.buffer = stepR_.get();
                hero_->addCue(c);
                hero_->setPosZ(-89.0, 167.0, 1.0, 184.0);
                hero_->playRangeNoEvent(13, 0x3C, 0x47);
                moveWait(hero_, 46.0, 86.0, 29.0, 80.0, 30);
                scene_.removeLayer(hero_);  // 0x0044AA70
                scene_.setHero(hero_);
                hero_->show(scene_.clicksEnabled);
                hero_->setPosDir(0x83, 0x153, 4);
                const int cb = beginWait();
                hero_->walkTo(0x83, 0x14A, cb, 4);  // CActor_WalkTo2
                wait();
            }
        } else {
            if (hero_) hero_->show(scene_.clicksEnabled);
            if (flag(kSabOut) == 0) {
                sabAtDoor();
            } else if (sabWalk_) {
                sabWalk_->enable();
                sabWalk_->setPosZ(251.0, 96.0, 109.0, 111.0);
                sabWalk_->showFrame(0x0C);
                setActive(sabWalk_, true);
                if (hero_) hero_->blockRect(kSabL, kSabT, kSabR, kSabB);
            }
        }
        if (flag(kSabOut) == 0 && hero_) hero_->blockRect(kDoorL, kDoorT, kDoorR, kDoorB);
        if (flag(kBox) == 1 && hero_) hero_->blockRect(kBoxL, kBoxT, kBoxR, kBoxB);
    } else if (prevScene_ == 0x67) {
        // Flucht vom Schiff (Vollbild) → Hafen, Szene 100
        scene_.setModal(escape_.get());
        {
            const int cb = beginWait();
            if (escape_) escape_->playRange(11, cb, 0, -1);
            wait();
        }
        changeScene(100, 8);
    }
    scene_.enableInput();
}

void Loc23::onEvent(Event& e, int) {
    switch (e.param) {
    case 1:  // zurück zum Hafen: Sabrina hält ihn auf
        sabSay("102_SAB_05");
        return;
    case 2:  // Luke → Szene 103
        if (flag(kBox) == 2 && walk(0x19E, 0xF6, 5)) changeScene(0x67, 0x18);
        return;
    case 3:  // Kiste schieben
        if (flag(kBox) == 1) {
            if (!walk(0x165, 0xF4, 7)) return;
            scene_.disableInput();
            if (hero_) hero_->hide(scene_.clicksEnabled);
            if (boxLeft_) boxLeft_->disable();
            playEnd(boxToRight_, 10);
            if (boxRight_) boxRight_->enable();
            setActive(boxRight_, true);
            if (hero_) {
                hero_->show(scene_.clicksEnabled);
                hero_->setPosDir(0x1A7, 0x10A, 7);
            }
            flag(kBox) = 2;
            faceAndSay("102_ROD_02");
            if (hero_) hero_->unblockRect(kBoxL, kBoxT, kBoxR, kBoxB);
            if (flag(kSabOut) == 0)
                setActive(doorClosed_, true);
            else if (to103_)
                to103_->active = true;
            scene_.enableInput();
            return;
        }
        if (!walk(0x19E, 0xF1, 7)) return;
        scene_.disableInput();
        if (hero_) hero_->hide(scene_.clicksEnabled);
        if (boxRight_) boxRight_->disable();
        playEnd(boxToLeft_, 10);
        if (boxLeft_) boxLeft_->enable();
        setActive(boxLeft_, true);
        if (hero_) {
            hero_->show(scene_.clicksEnabled);
            hero_->setPosDir(0x16B, 0xF7, 7);
            const int cb = beginWait();
            hero_->walkTo(0x157, 0x10B, cb, 7);  // CActor_WalkTo2
            wait();
        }
        flag(kBox) = 1;
        faceAndSay("102_ROD_02");
        if (hero_) hero_->blockRect(kBoxL, kBoxT, kBoxR, kBoxB);
        if (to103_) to103_->active = false;
        setActive(doorClosed_, false);
        scene_.enableInput();
        return;
    case 4:  // Sabrina
        if (flag(kNeverSet4084) != 0) return;
        if (flag(kSabOut) == 0 && flag(kBox) == 2) {
            // Sabrina geht zur Tür, schlägt sie zu und kommt heraus
            if (!walk(0x13B, 0x163, 7)) return;
            scene_.disableInput();
            sabSay("102_SAB_03");
            if (sabWalk_) sabWalk_->playRange(13, 0, 0x30, 0x3B);
            moveWait(sabWalk_, 290.0, 140.0, 86.0, 151.0, 0x14);
            if (sabWalk_) {
                sabWalk_->showFrame(0x3C);
                sabWalk_->playRange(13, 0, 0x3C, 0x47);
            }
            moveWait(sabWalk_, 294.0, 109.0, 86.0, 121.0, 0x0F);
            if (sabWalk_) sabWalk_->playRange(13, 0, 0x48, 0x53);
            moveWait(sabWalk_, 339.0, 80.0, 142.0, 111.0, 10);
            if (sabWalk_) sabWalk_->disable();
            if (doorClosed_) doorClosed_->disable();
            playEnd(sabDoor_, 10);
            if (doorOpen_) doorOpen_->enable();
            if (sabWalk_) {
                sabWalk_->enable();
                sabWalk_->playRange(13, 0, 0x18, 0x23);
            }
            moveWait(sabWalk_, 311.0, 96.0, 86.0, 111.0, 10);
            if (sabWalk_) sabWalk_->playRange(13, 0, 0x24, 0x2F);
            moveWait(sabWalk_, 251.0, 96.0, 109.0, 111.0, 10);
            if (sabWalk_) {
                sabWalk_->stop(0);  // vtable +0x24
                sabWalk_->showFrame(0x0C);
            }
            setActive(sabWalk_, true);
            if (hero_) hero_->blockRect(kSabL, kSabT, kSabR, kSabB);
            scene_.enableInput();
            if (to103_) to103_->active = true;
            flag(kSabOut) = 1;
            return;
        }
        sabSay("102_SAB_04");
        return;
    case 5:  // Kajütentür rütteln
        if (flag(kBox) != 2 || flag(kSabOut) != 0) return;
        if (!walk(0x19E, 0xF6, 5)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playEnd(rodShake_, 10);
        if (hero_) hero_->show(scene_.clicksEnabled);
        faceAndSay("102_ROD_01");
        return;
    default:
        return;
    }
}

}  // namespace rah
