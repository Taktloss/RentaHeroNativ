// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc17.hpp"

#include <cstdio>

#include "engine/actor.hpp"
#include "engine/area.hpp"

namespace rah {

namespace {
constexpr std::size_t kMushroom39 = 0x3F44;  // Pilz in Szene 39 liegt (Loc10); erster Besuch hier setzt 1
constexpr std::size_t kPirate = 0x3FA8;       // Loc16: Pirat folgt nach 58
constexpr std::size_t kVulturesGone = 0x3FC8; // Aasfresser sind satt (Pirat gefressen)
constexpr std::size_t kBush = 0x3FCC;         // Busch-Stufe (1…4; 4 = Busch weg, Ohr frei)
constexpr std::size_t kEarSaid = 0x3FD0;      // Kommentar am Ohr gesagt
constexpr std::size_t kFirstVisit = 0x3FD4;  // Start 1, nur hier gelesen und auf 0 gesetzt (0x0041D4EE/0x0041D4F6)
constexpr std::size_t k3FD8 = 0x3FD8;         // Busch zweimal angesehen (2)
constexpr std::size_t kFromSkull = 0x3FE4;  // Loc18 setzt 1 beim Öffnen des Kiefers (0x0041EDFE): Rodrigo kommt aus dem Schädel
constexpr int kScene59 = 0x3B, kScene60 = 0x3C, kScene61 = 0x3D, kScene58 = 0x3A;
constexpr int kAmbient = 100;
}  // namespace

Loc17::Loc17(GameContext& ctx, int scene, int prevScene) : Game(ctx, 17, scene, prevScene) {
    getViewObjects();
    char name[20];
    if (sceneNr_ == kScene59) {
        scene_.load("S59", 0xFFFF);  // CSzene_LoadAll(DAT_00473BF0)
        getViewObjects();
        for (int i = 0; i < 7; ++i) {
            std::snprintf(name, sizeof name, "Aasfresser%d", i + 1);  // DAT_00473C90
            vultures_[i] = dynamic_cast<AnimLayer*>(scene_.getObject(name));
            if (vultures_[i]) {  // Port-Schutz, Original ohne Prüfung; alle Namen in den Daten vorhanden (tools/name_check.py)
                homeX_[i] = static_cast<int>(vultures_[i]->x);  // __ftol
                homeY_[i] = static_cast<int>(vultures_[i]->y);
            }
        }
        to58_ = scene_.getArea("S59_To58");
        skeleton_ = scene_.getArea("S59_Skeleton");
        wall_ = scene_.getArea("S59_Wall");
        head_ = scene_.getArea("S59_Head");
        smack_ = loadSfx("Sfx_Schmatzen");
    } else if (sceneNr_ == kScene60) {
        scene_.load("S60", 0xFFFF);  // CSzene_LoadAll(DAT_00473CE4)
        getViewObjects();
        rodOut_ = loadAnim("S60_RodRaus");
        tryBush_ = loadAnim("S60_TryOpenSkullMitBusch");
        tryNoBush_ = loadAnim("S60_TryOpenSkullOhneBusch");
        for (int i = 0; i < 7; ++i) {
            std::snprintf(name, sizeof name, "Aasfresser%d", i + 1);
            vultures_[i] = dynamic_cast<AnimLayer*>(scene_.getObject(name));
        }
        bushAway_ = dynamic_cast<AnimLayer*>(scene_.getObject("S60_BuschWeg"));
        bush_ = scene_.getObject("S60_Busch");
        leafLU_ = scene_.getObject("S60_BlattLU");
        leafRO_ = scene_.getObject("S60_BlattRO");
        light_ = scene_.getObject("S60_Licht");
        splash_ = dynamic_cast<AnimLayer*>(scene_.getObject("S60_Spritz"));
        to59_ = scene_.getArea("S60_To59");
        skull_ = scene_.getArea("S60_Skull");
        mouth_ = scene_.getArea("S60_Mouth");
        ear_ = scene_.getArea("S60_Ear");
        if (prevScene_ == kScene61 && flag(kVulturesGone) == 0 && flag(kFromSkull) != 0) {
            smack_ = loadSfx("Sfx_Schmatzen");
            burst_ = loadSfx("Sfx_Platzen");
            ooh_ = loadSfx("Sfx_OOh");
            tableHit_ = loadSfx("Sfx_TableHit");
            knock_ = loadSfxVol("Sfx_Knock_Wet", 0x46);
            const auto cue = [](AnimLayer* a, int frame, AudioBuffer* b) {
                if (!a) return;
                AnimCue c;
                c.type = 1;
                c.frame = frame;
                c.buffer = b;
                a->addCue(c);  // CAnimLayer_AddCue 0x00442BB0
            };
            cue(rodOut_.get(), 0x2C, knock_.get());
            cue(splash_, 5, tableHit_.get());
            cue(splash_, 9, ooh_.get());
            cue(splash_, 0x0C, burst_.get());
        }
    }
    bird_ = loadSfx("Sfx_Bird");
    bird2_ = loadSfx("Sfx_Bird2");
    bird3_ = loadSfx("Sfx_Bird3");
    chirp_ = loadSfx("Sfx_Chirp");
    ambientTimer_ = std::make_unique<Event>();
    music_ = playMusic("Loc17_UglyAndDisgusting");
}

void Loc17::destroyLocal() {
    ambientTimer_.reset();
    rodOut_.reset();
    tryBush_.reset();
    tryNoBush_.reset();
    for (auto* b : {&smack_, &burst_, &ooh_, &tableHit_, &knock_, &bird_, &bird2_, &bird3_, &chirp_}) b->reset();
}

bool Loc17::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo
    return wait();
}

bool Loc17::playRange(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

void Loc17::showForeground60() {
    for (Layer* l : {leafLU_, leafRO_, light_, bush_})
        if (l) l->enable();
}

void Loc17::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    setEventProc(&Loc17::onEvent);
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(5, 15) * 1000), 0, kAmbient);
    if (flag(kFirstVisit) != 0) {
        flag(kFirstVisit) = 0;
        flag(kMushroom39) = 1;
    }
    if (sceneNr_ == kScene59)
        start59();
    else if (sceneNr_ == kScene60)
        start60();
    else
        scene_.enableInput();
}

void Loc17::start59() {
    if (to58_) to58_->setEvent(1);
    if (skeleton_) skeleton_->setEvent(2);
    if (wall_) wall_->setEvent(3);
    if (head_) head_->setEvent(4);
    if (restored_ == 0) {
        if (prevScene_ == kScene58) {
            if (hero_) {
                hero_->setPosDir(0x12D, 0x163, 4);
                hero_->show(scene_.clicksEnabled);
            }
            if (flag(kVulturesGone) == 0) {
                // die Aasfresser fressen, dann fliegen sie nacheinander auf
                scene_.disableInput();
                for (AnimLayer* v : vultures_)
                    if (v) v->enable();
                if (smack_) smack_->play();  // vtable +0x08
                int ev = 0x1E;
                for (AnimLayer* v : vultures_) {
                    if (v) {
                        v->setStepCount(0);  // 0x00443300
                        v->playRangeNoEvent(13, 0, 2);
                    }
                    const int n = random(10, 20);
                    if (v) v->setCountEvent(ev, n);  // 0x00443530
                    delay(static_cast<std::uint32_t>(random(100, 400)));
                    ev += 2;
                }
                delay(1000);
                scene_.enableInput();
            }
            scene_.enableInput();
            return;
        }
        if (prevScene_ != kScene60) {
            scene_.enableInput();  // Original: ohne Show
            return;
        }
        if (hero_) hero_->setPosDir(0x177, 0x119, 0);
    }
    if (hero_) hero_->show(scene_.clicksEnabled);
    scene_.enableInput();
}

void Loc17::start60() {
    if (to59_) to59_->setEvent(0x32);
    if (skull_) skull_->setEvent(0x33);
    if (mouth_) mouth_->setEvent(0x34);
    if (flag(kFromSkull) == 0 && flag(kBush) <= 3) {
        if (bush_ && bush_->area) bush_->area->setEvent(0x36);
    } else {
        if (flag(kFromSkull) == 0 && bush_) bush_->setPos(0x26C, 0x66);  // vtable +0x14: Busch beiseite
        if (ear_) ear_->setEvent(0x35);
    }
    if (flag(kVulturesGone) != 0 && splash_) {
        splash_->showFrame(0x1D);
        splash_->enable();
    }
    if (restored_ == 0) {
        if (prevScene_ == kScene59) {
            showForeground60();
            if (hero_) {
                hero_->setPosDir(8, 0x128, 6);
                hero_->walkTo(0x5F, 0x139, 0, 7);
            }
            scene_.enableInput();
            return;
        }
        if (flag(kFromSkull) != 0) {
            // Rodrigo kommt aus dem Schädel; die Aasfresser stürzen sich auf den Piraten
            scene_.disableInput();
            scene_.setModal(rodOut_.get());
            {
                const int cb = beginWait();
                if (rodOut_) rodOut_->playEndEvent(10, cb);
                wait();
            }
            if (splash_) splash_->enable();
            showForeground60();
            if (hero_) hero_->setPosDir(0x140, 0x14E, 7);
            walk(0x140, 0x154, 4);
            if (smack_) smack_->play();  // Original ohne Prüfung; nur geladen bei 0x3FC8 == 0. Danach führt
            // kein Weg mehr in den Schädel (Fall 53: nur 060_ROD_09), also auch keine Ankunft aus 61
            for (int i = 0; i < 7; ++i) {
                AnimLayer* v = vultures_[i];
                const int y = random(200, 0xE6);
                if (v) {
                    v->setPos(-0x5C, y);  // vtable +0x14
                    v->playRangeNoEvent(13, 6, 0x11);
                }
                const int steps = random(0x14, 0x1E);
                if (v) v->moveTo(350.0, 256.0, 0.0, 48.0, steps, 0x3C + i);  // 0x004475B0
                delay(static_cast<std::uint32_t>(random(100, 400)));
            }
            delay(3000);
            {
                const int cb = beginWait();
                if (splash_) splash_->playEndEvent(11, cb);
                wait();
            }
            flag(kVulturesGone) = 1;
            flag(kPirate) = 1;
            walk(0x140, 0x15E, 0);
            heroSay("060_ROD_01");
            scene_.enableInput();
            scene_.enableInput();
            return;
        }
        showForeground60();
        if (hero_) hero_->setPosDir(0x270, 0xF4, 0);
    } else {
        showForeground60();
    }
    if (hero_) hero_->show(scene_.clicksEnabled);
    scene_.enableInput();
}

void Loc17::onEvent(Event& e, int) {
    const int p = e.param;
    // Aasfresser fliegen auf: 0x1E/0x20/… Aufflattern, 0x1F/0x21/… Abflug nach rechts
    if (p >= 0x1E && p <= 0x2B) {
        AnimLayer* v = vultures_[(p - 0x1E) / 2];
        if (!v) return;
        if ((p & 1) == 0) {
            v->playRange(11, p + 1, 2, 5);
        } else {
            v->playRangeNoEvent(13, 6, 0x11);
            const int steps = random(0x14, 0x1E);
            v->moveTo(485.0, 0.0, 0.0, 0.0, steps, 0);
            v->fps = 0x14;  // +0x54
        }
        return;
    }
    if (p >= 0x3C && p <= 0x42) {  // Aasfresser am Ziel
        if (vultures_[p - 0x3C]) vultures_[p - 0x3C]->disable();
        return;
    }
    switch (p) {
    case 1:  // → 58
        if (!walk(0x12D, 0x163, 0)) return;
        if (flag(kVulturesGone) == 0) {
            // die Aasfresser kehren zurück
            scene_.disableInput();
            if (smack_) smack_->play();
            for (int i = 0; i < 7; ++i) {
                AnimLayer* v = vultures_[i];
                if (!v) continue;
                v->setPos(0x1EA, 0xFF);
                v->enable();
                v->playRangeNoEvent(13, 0x12, 0x1D);
                v->moveTo(homeX_[i], homeY_[i], 0.0, 0.0, 0x1E, 0);
            }
            delay(0x5DC);
            scene_.enableInput();
        }
        changeScene(kScene58, 0x10);
        return;
    case 2: {  // Skelett
        if (!walk(0x68, 0x14C, 4)) return;
        char key[16];
        std::snprintf(key, sizeof key, "059_ROD_%02d", random(1, 6));
        comment(key);
        return;
    }
    case 3:  // Wand
        if (!walk(0x17F, 0x150, 6)) return;
        heroSay(flag(kVulturesGone) == 0 ? "059_ROD_07" : "059_ROD_08");
        return;
    case 4:  // Kopf → 60
        if (walk(0x177, 0x118, 4)) changeScene(kScene60, 0x11);
        return;
    case 0x32:  // → 59
        if (walk(8, 0x128, 2)) changeScene(kScene59, 0x11);
        return;
    case 0x33:
        if (walk(399, 0x15E, 4)) comment("060_ROD_02");
        return;
    case 0x34: {  // Mund
        if (!walk(0x150, 0x155, 4)) return;
        if (flag(kVulturesGone) != 0) {
            comment("060_ROD_09");
            return;
        }
        scene_.disableInput();
        AnimLayer* a = flag(kBush) < 4 ? tryBush_.get() : tryNoBush_.get();
        scene_.setModal(a);
        {
            const int cb = beginWait();
            if (a) a->playEndEvent(10, cb);
            wait();
        }
        if (!walk(0x154, 0x155, 0)) return;  // Original: Rückkehr ohne EnableInput
        heroSay("060_ROD_03");
        scene_.enableInput();
        return;
    }
    case 0x35:  // Ohr → 61
        if (!walk(0x270, 0xF4, 3)) return;
        if (flag(kVulturesGone) != 0) {
            comment("060_ROD_09");
            return;
        }
        if (flag(kEarSaid) == 0) {
            heroSay("060_ROD_08");
            flag(kEarSaid) = 1;
        }
        changeScene(kScene61, 0x12);
        return;
    case 0x36:  // Busch
        if (!walk(0x270, 0xF4, 3)) return;
        switch (flag(kBush)) {
        case 1:
            comment("060_ROD_04");
            flag(kBush) = 2;
            return;
        case 2:
            comment("060_ROD_05");
            if (flag(k3FD8) == 2) flag(kBush) = 3;
            flag(k3FD8) = 2;
            return;
        case 3:
            scene_.disableInput();
            heroSay("060_ROD_06");
            if (hero_) hero_->hide(scene_.clicksEnabled);
            playRange(bushAway_, 11, 0, 9);
            if (bush_) bush_->moveTo(630.0, 0.0, 0.0, 0.0, 10, 0);
            playRange(bushAway_, 10, 10, -1);
            if (hero_) hero_->show(scene_.clicksEnabled);
            comment("060_ROD_07");
            flag(kBush) = 4;
            if (bush_ && bush_->area) bush_->area->active = false;  // +0xB8
            if (ear_) ear_->setEvent(0x35);
            scene_.enableInput();
            return;
        default:
            return;
        }
    case kAmbient: {
        const int r = random(1, 4);
        AudioBuffer* birds[] = {bird_.get(), bird2_.get(), bird3_.get(), chirp_.get()};
        if (r >= 1 && r <= 4) {
            AudioBuffer* b = birds[r - 1];
            const int v = random(0x32, 100);
            if (b) {
                b->setVolume(v);  // CAudioBuffer_SetVolume
                playSfxRandom3D(*b);
            }
        }
        setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(5, 10) * 1000), 0, kAmbient);
        return;
    }
    default:
        return;
    }
}

}  // namespace rah
