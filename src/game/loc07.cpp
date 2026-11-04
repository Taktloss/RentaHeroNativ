// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc07.hpp"

#include <cstdio>
#include <string>

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
constexpr std::size_t kVisits = 0x3EF4;     // Ankünfte von der Leiter (Kommentar 047_ROD_01/02)
constexpr std::size_t kSack = 0x3EF8;       // Sack liegt in Szene 52 (Start 1)
constexpr std::size_t kRope = 0x3EFC;       // Seil hängt an der Kurbel (Start 1: altes Seil)
constexpr std::size_t kRopeFixed = 0x3F00;  // neues Seil (Gegenstand 10) befestigt
constexpr std::size_t kGrid = 0x3F04;       // Gitter in 52/53 unten (Start 1)
constexpr std::size_t kPirateSeen = 0x3F08; // Pirat in Szene 51 gesehen
constexpr std::size_t kRopeForGirl = 0x3F30;// altes Seil gerissen → Mädchen springt Seil (Loc10)
constexpr std::size_t kVulturesGone = 0x3FC8;  // Aasfresser haben den Piraten (Loc17); ≠ 0 → kein Pirat in 51
constexpr int kBubble = 1;
}  // namespace

Loc07::Loc07(GameContext& ctx, int scene, int prevScene) : Game(ctx, 7, scene, prevScene) {
    getViewObjects();
    if (sceneNr_ == 0x34) {
        unsigned mask = flag(kSack) != 0 ? 1u : 0u;
        if (flag(kGrid) != 0) mask |= 2;
        scene_.load("52", mask);  // DAT_00472324
        getViewObjects();
        crankL_ = scene_.getObject("52_Kurbel");
        grid_ = scene_.getObject("52_Gitter");
        rope_ = scene_.getObject("52_Seil");
        ropeTears_ = obj("52_SeilReisst");
        sack_ = scene_.getObject("52_Sack");
    } else {
        scene_.load(std::to_string(sceneNr_), 0xFFFF);  // wsprintf("%d") + CSzene_LoadAll
        getViewObjects();
    }
    bubbleTimer_ = std::make_unique<Event>();
    bubble_ = loadSfxVol("Sfx_Bubble", 0x32);
    winding_ = loadSfx("Sfx_Winding");
    gridDrops_ = loadSfx("Sfx_Grid_Drops");
    rubberband_ = loadSfx("Sfx_Rubberband");
    fixBelt_ = loadSfx("Sfx_Work_FixBelt");
    danger_ = loadSfx("Sfx_DangerStringsLow");
    piano_ = loadSfx("Sfx_PianoStroke");
    music_ = playMusic("Loc07_Sewage");
    if (music_) {
        music_->setReverb(0x15, 0.652f, 2.886f, 0.25f);  // 0x3F26E979, 0x4038B439, 0x3E800000
        music_->setReverbMix(0.1f);                      // 0x3DCCCCCD
    }
}

void Loc07::destroyLocal() {
    // Reihenfolge wie 0x0040F9F0
    bubbleTimer_.reset();
    for (auto* b : {&bubble_, &winding_, &gridDrops_, &rubberband_, &fixBelt_, &danger_, &piano_}) b->reset();
}

bool Loc07::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo
    return wait();
}

bool Loc07::playEnd(AnimLayer* a, unsigned mode) {
    const int cb = beginWait();
    if (a) a->playEndEvent(mode, cb);
    return wait();
}

bool Loc07::playRange(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

bool Loc07::moveWait(Layer* l, double x, double y, int steps) {
    const int cb = beginWait();
    if (l) l->moveTo(x, y, 0.0, 0.0, steps, cb);  // CLayer_MoveTo 0x004475B0
    return wait();
}

void Loc07::startBack(const char* name) {
    // UNVERIFIED: das Original ruft CAnimLayer_Start ohne Nullprüfung (Szene 49 nutzt „48_BackAnim“)
    if (AnimLayer* a = obj(name)) a->start(0x0D);
}

void Loc07::area(const char* name, int ev) {
    if (Area* a = scene_.getArea(name)) a->setEvent(ev);
}

void Loc07::arrive(int x, int y, int dir, int tx, int ty, int tdir) {
    if (!hero_) return;
    hero_->setPosDir(x, y, dir);
    hero_->walkTo(tx, ty, 0, tdir);  // CActor_WalkTo2 ohne Ereignis
}

void Loc07::go(int x, int y, int dir, int scene) {
    if (walk(x, y, dir)) changeScene(scene, 7);
}

void Loc07::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // CSzene+0x5C0
    setEventProc(&Loc07::onEvent);
    const bool fresh = restored_ == 0;
    bool known = true;  // Szene bekannt → bei Spielstand Held zeigen
    switch (sceneNr_) {
    case 0x2F:  // 47: unter der Leiter
        startBack("47_BackAnim");
        area("047_DoorTo49", 0x20);
        area("047_DoorTo50", 0x1E);
        area("047_DoorTo51", 0x1F);
        area("047_DeadEnd", 0x21);
        area("047_Ladder", 0x22);
        if (fresh) {
            switch (prevScene_) {
            case 0x2E: {
                // Rodrigo rutscht die Leiter herunter
                AnimLayer* down = obj("47_RodRunter");
                if (down) {
                    down->setPos(0x98, -0x46);  // vtable +0x14
                    down->start(0x0D);
                }
                moveWait(down, 0.0, 80.0, 0x1E);
                if (down) down->disable();
                playEnd(obj("47_RodRunterStop"), 10);
                if (hero_) hero_->setPosDir(0xAF, 0xDB, 4);
                walk(0xAF, 0xE1, 0);
                if (flag(kVisits) == 0) {
                    heroSay("047_ROD_01");
                    ++flag(kVisits);
                } else if (flag(kVisits) == 1) {
                    heroSay("047_ROD_02");
                }
                break;
            }
            case 0x31: arrive(0x232, 0xBB, 0, 0x21E, 0xC3, 1); break;
            case 0x32: arrive(4, 0xEA, 6, 0x38, 0xF2, 7); break;
            case 0x33: arrive(0x1DE, 0xB7, 0, 0x1DE, 0xB9, 0); break;
            default: break;
            }
        }
        break;
    case 0x30:  // 48
        startBack("48_BackAnim");
        area("048_DoorTo50", 0x28);
        area("048_DeadEnd", 0x29);
        if (fresh) arrive(0x40, 0xCE, 0, 0x4E, 0xF8, 0);
        break;
    case 0x31:  // 49 – Original startet „48_BackAnim“
        startBack("48_BackAnim");
        area("049_DoorTo52", 0x2D);
        area("049_DoorTo47", 0x2E);
        if (fresh) {
            if (prevScene_ == 0x2F) arrive(0x202, 0xBD, 0, 0x1F8, 0xCB, 1);
            else if (prevScene_ == 0x34) arrive(0x3E, 0xCC, 0, 0x50, 0xFA, 0);
        }
        break;
    case 0x32:  // 50
        startBack("50_BackAnim");
        area("050_DoorTo54", 0x32);
        area("050_DoorTo48", 0x33);
        area("050_DoorTo47", 0x35);
        area("050_DeadEnd", 0x34);
        if (fresh) {
            if (prevScene_ == 0x2F) arrive(0x230, 0xDD, 1, 0x216, 0xF1, 1);
            else if (prevScene_ == 0x30) arrive(0x10F, 0xE4, 0, 0x10F, 0xEA, 0);
            else if (prevScene_ == 0x36) arrive(0x20, 0xDE, 0, 0x35, 0xEF, 7);
        }
        break;
    case 0x33:  // 51
        startBack("51_BackAnim");
        if (Layer* l = scene_.getObject("51_Light")) l->enable();
        pirate_ = obj("51_PiratGuck");
        area("051_DoorTo54", 0x3C);
        area("051_DoorTo47", 0x3D);
        area("051_DoorTo52", 0x3E);
        area("051_DoorTo55", 0x3F);
        if (fresh) {
            switch (prevScene_) {
            case 0x2F: arrive(0xF4, 0xD1, 0, 0xF4, 0xEA, 0); break;
            case 0x34: arrive(0x1B2, 0xD3, 0, 0x1B2, 0xE9, 0); break;
            case 0x36:
                if (hero_) hero_->setPosDir(0x1F, 0xD4, 0);
                if (flag(kPirateSeen) == 0 && flag(kVulturesGone) == 0) {
                    // ein Pirat schaut kurz herein
                    scene_.disableInput();
                    walk(0x5D, 0xFD, 7);
                    if (pirate_) pirate_->setPos(-12, 0xA5);
                    if (danger_) danger_->play();
                    if (pirate_) pirate_->enable();
                    moveWait(pirate_, 5.0, 0.0, 10);  // 0x40140000
                    playEnd(pirate_, 11);
                    moveWait(pirate_, -12.0, 0.0, 10);  // 0xC0280000
                    if (pirate_) pirate_->disable();
                    flag(kPirateSeen) = 1;
                    scene_.enableInput();
                } else if (hero_) {
                    hero_->walkTo(0x3A, 0xE4, 0, 7);
                }
                break;
            case 0x37: arrive(0x1FC, 0xF7, 2, 0x1E0, 0xF7, 2); break;
            default: break;
            }
        }
        break;
    case 0x34:  // 52: Kurbel, Gitter, Sack
        startBack("52_BackAnim");
        if (Layer* l = scene_.getObject("52_Behind")) l->enable();
        if (crankL_) crankL_->enable();
        if (flag(kGrid) != 0 && crankL_ && crankL_->area) crankL_->area->setEvent(0x4A);
        if (flag(kRope) == 0) {
            if (auto* bar = scene_.inventory()) bar->setItemEvent(10, 0x4C);  // Inventory_SetEvent
        } else if (rope_) {
            rope_->enable();
        }
        if (flag(kSack) != 0 && sack_) {
            sack_->enable();
            if (sack_->area) sack_->area->setEvent(0x4B);
        }
        if (grid_) grid_->enable();
        if (flag(kGrid) == 0) {
            if (grid_) grid_->setPos(0x1FB, -40);  // hochgezogen
        } else if (hero_) {
            hero_->blockRect(0x208, 200, 0x27F, 0x10E);
        }
        area("052_DoorTo56", 0x46);
        area("052_DoorTo51", 0x47);
        area("052_DoorTo49", 0x48);
        area("052_DoorTo53", 0x49);
        if (fresh) {
            switch (prevScene_) {
            case 0x31: arrive(0x1A5, 0xCF, 0, 0x1A5, 0xE8, 0); break;
            case 0x33: arrive(0xE9, 0xCB, 0, 0xE9, 0xE8, 0); break;
            case 0x35: arrive(0x24E, 0xE9, 0, 0x1F9, 0xFF, 1); break;
            case 0x38: arrive(0x0B, 0xED, 0, 0x28, 0xED, 7); break;
            default: break;
            }
        }
        break;
    case 0x35: {  // 53: Gitter zur Schlucht
        startBack("53_BackAnim");
        area("053_DoorTo52", 0x50);
        area("053_DoorTo57", 0x51);
        Layer* grid = scene_.getObject("53_Gitter");
        if (grid) grid->enable();
        if (flag(kGrid) == 0) {
            if (grid) grid->setPos(0, -100);
        } else if (hero_) {
            hero_->blockRect(0x46, 0xB4, 0x7D, 0xEB);
        }
        if (fresh) {
            if (prevScene_ == 0x34) arrive(100, 0xBC, 0, 0x6E, 0xD5, 7);
            else if (prevScene_ == 0x39) arrive(0x219, 0xB8, 1, 0x1E6, 0xD2, 1);
        }
        break;
    }
    case 0x36:  // 54
        startBack("54_BackAnim");
        if (Layer* l = scene_.getObject("54_Light")) l->enable();
        area("054_DoorTo50", 0x5A);
        area("054_DoorTo51", 0x5B);
        if (fresh) {
            if (prevScene_ == 0x32) arrive(0x6C, 0xB8, 0, 0x72, 0xDD, 7);
            else if (prevScene_ == 0x33) arrive(0x210, 0xB4, 1, 0x1F8, 0xCB, 1);
        }
        break;
    case 0x37:  // 55
        startBack("55_BackAnim");
        area("055_DoorTo51", 100);
        if (fresh) arrive(0x90, 0xBC, 0, 0xA7, 0xCB, 0);
        break;
    case 0x38:  // 56
        startBack("56_BackAnim");
        area("056_DoorTo52", 0x6E);
        if (fresh) arrive(0x40, 0xCE, 0, 0x4E, 0xF8, 7);
        break;
    default:
        known = false;
        break;
    }
    if (known && !fresh && hero_) hero_->show(scene_.clicksEnabled);
    setTimerHandler(*bubbleTimer_, static_cast<std::uint32_t>(random(5, 7) * 1000), 0, kBubble);
    scene_.enableInput();
}

void Loc07::crank() {
    if (!walk(0x1C4, 0xF5, 6)) return;
    const auto turnStart = [this] {
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playEnd(obj("52_RodKurbelStart"), 10);
    };
    const auto back = [this](const char* say) {
        if (hero_) hero_->show(scene_.clicksEnabled);
        if (!walk(0x1C4, 0xF6, 0)) return false;
        if (say) heroSay(say);
        return true;
    };
    const bool rope = flag(kRope) != 0;
    if (flag(kRopeFixed) == 0 && flag(kGrid) != 0) {
        if (!rope) {
            // ohne Seil: Kurbel dreht leer
            turnStart();
            if (winding_) winding_->play();
            playEnd(obj("52_RodKurbel"), 10);
            playEnd(obj("52_RodKurbelStop"), 10);
            back("052_ROD_02");
            return;
        }
        // altes Seil reißt, das Gitter fällt wieder
        turnStart();
        if (grid_) grid_->moveTo(0.0, -40.0, 0.0, 0.0, 0x46, 0);  // 0xC0440000
        if (winding_) winding_->play();
        playEnd(obj("52_RodKurbel"), 10);
        if (rope_) rope_->disable();
        if (grid_) grid_->moveTo(0.0, 75.0, 0.0, 0.0, 5, 0);  // 0x4052C000
        flag(kRope) = 0;
        flag(kRopeForGirl) = 1;
        if (rubberband_) rubberband_->play();
        if (AnimLayer* t = obj("52_SeilReisst")) t->start(10);
        playRange(obj("52_RodKurbelStop"), 10, 0, 4);
        if (gridDrops_) gridDrops_->play();
        playRange(obj("52_RodKurbelStop"), 10, 5, -1);
        if (!back(nullptr)) return;
        if (piano_) piano_->play();
        heroSay("052_ROD_01");
        return;
    }
    if (!rope) return;
    if (flag(kRopeFixed) == 0 || flag(kGrid) == 0) return;
    // neues Seil: Gitter hochziehen
    turnStart();
    if (grid_) grid_->moveTo(0.0, -40.0, 0.0, 0.0, 0x46, 0);
    flag(kGrid) = 0;
    if (crankL_ && crankL_->area) crankL_->area->active = false;
    if (hero_) hero_->unblockRect(0x208, 200, 0x27F, 0x10E);
    for (int i = 0; i < 3; ++i) {
        if (winding_) winding_->play();
        playEnd(obj("52_RodKurbel"), 10);
    }
    playEnd(obj("52_RodKurbelStop"), 10);
    back("052_ROD_06");
}

void Loc07::onEvent(Event& e, int) {
    switch (e.param) {
    case kBubble:
        if (bubble_) {
            bubble_->setFreq(random(0x32, 100));
            bubble_->play();
        } else {
            random(0x32, 100);
        }
        setTimerHandler(*bubbleTimer_, static_cast<std::uint32_t>(random(5, 10) * 1000), 0, kBubble);
        return;
    // Szene 47
    case 0x1E: go(4, 0xEA, 2, 0x32); return;
    case 0x1F: go(0x1DE, 0xB7, 4, 0x33); return;
    case 0x20: go(0x232, 0xBB, 4, 0x31); return;
    case 0x21: if (walk(0x13F, 0xD8, 4)) heroSay("000_ROD_01"); return;
    case 0x22: {
        // Leiter hinauf nach 1/46
        if (!walk(0xAF, 0xDB, 4)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playEnd(obj("47_RodRaufStart"), 10);
        AnimLayer* up = obj("47_RodRauf");
        if (up) up->start(0x0D);
        moveWait(up, 0.0, -80.0, 0x32);  // 0xC0540000
        changeScene(0x2E, 1);
        return;
    }
    // Szene 48
    case 0x28: go(0x40, 0xCE, 4, 0x32); return;
    case 0x29: if (walk(499, 0xCF, 5)) heroSay("000_ROD_01"); return;
    // Szene 49
    case 0x2D: go(0x3E, 0xCC, 4, 0x34); return;
    case 0x2E: go(0x202, 0xBD, 4, 0x2F); return;
    // Szene 50
    case 0x32: go(0x20, 0xDE, 4, 0x36); return;
    case 0x33: go(0x110, 0xEA, 4, 0x30); return;
    case 0x34: if (walk(0x1D2, 0xE0, 4)) heroSay("000_ROD_01"); return;
    case 0x35: go(0x230, 0xDD, 4, 0x2F); return;
    // Szene 51
    case 0x3C: go(0x1F, 0xD4, 4, 0x36); return;
    case 0x3D: go(0xF4, 0xD1, 4, 0x2F); return;
    case 0x3E: go(0x1B2, 0xD3, 4, 0x34); return;
    case 0x3F: go(0x1FC, 0xF7, 4, 0x37); return;
    // Szene 52
    case 0x46: go(0x0B, 0xD8, 4, 0x38); return;
    case 0x47: go(0xE9, 0xCB, 4, 0x33); return;
    case 0x48: go(0x1A5, 0xCF, 4, 0x31); return;
    case 0x49:
        if (flag(kGrid) == 0) {
            go(0x24E, 0xE9, 6, 0x35);
            return;
        }
        // am Gitter rütteln
        if (!walk(0x1F7, 0xFE, 6)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playEnd(obj("52_RodRuettel"), 10);
        if (hero_) hero_->show(scene_.clicksEnabled);
        if (walk(0x1F7, 0x103, 0)) heroSay("052_ROD_05");
        return;
    case 0x4A:
        crank();
        return;
    case 0x4B:
        // Sack ins Wasser stoßen
        if (flag(kSack) == 0) return;
        if (!walk(0xF4, 0xF9, 6)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playRange(obj("52_RodRunterbeug"), 11, 0, 0x10);
        if (sack_) sack_->moveTo(640.0, 0.0, 0.0, 0.0, 0x28, 0);  // 0x40840000
        flag(kSack) = 0;
        playRange(obj("52_RodRunterbeug"), 10, 0x11, -1);
        if (hero_) hero_->show(scene_.clicksEnabled);
        if (walk(0x13D, 0xF1, 6)) heroSay("052_ROD_03");
        return;
    case 0x4C:
        // neues Seil (Gegenstand 10) an der Kurbel befestigen
        if (!walk(0x1C4, 0xF5, 6)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playRange(obj("52_RodFixRope"), 11, 0, 0x1E);
        if (rope_) rope_->enable();
        if (auto* bar = scene_.inventory()) bar->remove(10);  // Inventory_Remove
        if (fixBelt_) fixBelt_->play();
        playRange(obj("52_RodFixRope"), 10, 0x1F, -1);
        flag(kRopeFixed) = 1;
        flag(kRope) = 1;
        flag(kVisits) = 2;
        if (hero_) hero_->show(scene_.clicksEnabled);
        if (walk(0x1C4, 0xF6, 0)) heroSay("052_ROD_04");
        return;
    // Szene 53
    case 0x50:
        if (flag(kGrid) != 0) {
            walkAndComment(0x8E, 0xD7, 4, "053_ROD_01");
            return;
        }
        go(100, 0xBC, 4, 0x34);
        return;
    case 0x51:
        if (walk(0x219, 0xB8, 4)) changeScene(0x39, 0x0F);
        return;
    // Szene 54
    case 0x5A: go(0x6C, 0xB8, 4, 0x32); return;
    case 0x5B: go(0x210, 0xB4, 4, 0x33); return;
    // Szene 55
    case 100: go(0x90, 0xBC, 4, 0x33); return;
    // Szene 56
    case 0x6E: go(0x40, 0xCE, 4, 0x34); return;
    default:
        // Abweichung: Original zeigt MessageBoxA("Unknown_Event____", "Message") (modal)
        std::fprintf(stderr, "Loc07: Unknown_Event____ (%d)\n", e.param);
        return;
    }
}

}  // namespace rah
