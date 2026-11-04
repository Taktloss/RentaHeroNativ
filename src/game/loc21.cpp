// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc21.hpp"

#include <cstdio>

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
constexpr std::size_t kDrillAt = 0x4034;     // Bohrer steht in 73 (1), 76 (2) oder 79 (3)
constexpr std::size_t kGloomUsed = 0x4038;   // Leuchtstein (Gegenstand 23) im Kessel in 73
constexpr std::size_t kSwitch = 0x403C;      // Weiche in 74 umgestellt (Bohrer fährt nach 79)
constexpr std::size_t kSwitchTalk = 0x4040;  // Kommentar zur Weiche (Start 1, danach 2)
constexpr std::size_t kSchutt76 = 0x4044;    // Schutt in 76 (Start 1 … 3 weggebohrt)
constexpr std::size_t kTinder = 0x4048;      // Zunder liegt in 76
constexpr std::size_t kSchutt79 = 0x404C;    // Schutt in 79 (Start 1 … 3 weggebohrt → 81)
constexpr std::size_t kStone80 = 0x4054;     // Stein in 80: 1 in der Wand, 2 am Boden, 3 genommen
constexpr std::size_t kTouch80 = 0x4058;     // Stein am Boden angefasst (Start 1, danach 2)
constexpr std::size_t kWallHint = 0x405C;    // Hinweis „Wand abgetastet“ gegeben
constexpr int kDrip = 200;

constexpr int kRodBlockL = 0x19E, kRodBlockT = 200, kRodBlockR = 0x27F, kRodBlockB = 0x167;
}  // namespace

void Loc21::addDrillCues(AnimLayer* drill) {
    if (!drill) return;
    // orig: CAnimLayer_AddCueEx 0x00442C70 (bild, Sfx_Winding, Aktion 1, Vol., 0, 0, 0, x, 0, z):
    // der Bohrer entfernt sich und wird leiser
    struct C { int frame, vol; float x, z; };
    static constexpr C cues[] = {{0x01, 90, 0.1f, 0.1f},   {0x0D, 100, 0.0f, 0.1f},  {0x19, 80, -0.1f, 0.15f},
                                 {0x25, 70, -0.1f, 0.2f},  {0x31, 60, -0.1f, 0.25f}, {0x3D, 50, -0.1f, 0.3f},
                                 {0x49, 40, -0.1f, 0.35f}, {0x55, 30, -0.1f, 0.4f}};
    for (const C& k : cues) {
        AnimCue c;
        c.type = 2;
        c.frame = k.frame;
        c.buffer = winding_.get();
        c.action = 1;
        c.vol = k.vol;
        c.x = k.x;
        c.z = k.z;
        drill->addCue(c);
    }
}

static void addSimpleCue(AnimLayer* a, int frame, AudioBuffer* b) {
    if (!a) return;
    AnimCue c;
    c.type = 1;
    c.frame = frame;
    c.buffer = b;
    a->addCue(c);  // CAnimLayer_AddCue 0x00442BB0
}

Loc21::Loc21(GameContext& ctx, int scene, int prevScene) : Game(ctx, 21, scene, prevScene) {
    char name[12];
    std::snprintf(name, sizeof name, "S%d", sceneNr_);  // DAT_004746EC
    scene_.load(name, 0xFFFF);                           // CSzene_LoadAll
    getViewObjects();
    winding_ = loadSfx("Sfx_Winding");
    drill_ = loadSfx("Sfx_Drill");
    door_ = loadSfx("Sfx_Door_Opened");
    wings_ = loadSfx("Sfx_DragonWings");
    explosion_ = loadSfx("Sfx_Explosion_Bass2");
    drip_ = loadSfx("Sfx_Drip");  // so im Original; Datei fehlt in den Daten → stumm (Original-Eigenheit)
    dripTimer_ = std::make_unique<Event>();
    switch (sceneNr_) {
    case 0x47:
        rodTakesShovel_ = obj("S71_RodTakesShovel");
        shovel_ = scene_.getObject("S71_Shovel");
        break;
    case 0x48:
        rodFeelsWall_ = obj("S72_RodFeelsWall");
        break;
    case 0x49:
        rodUsesGloom_ = obj("S73_RodUsesGloom");
        drill73_ = obj("S73_Drill");
        lever73_ = scene_.getObject("S73_Hebel");
        rodGrab_ = obj("Gen_RodGreif");
        smallSteam_ = obj("Gen_SmallSteam");
        splash_ = loadSfxVol("Sfx_WaterSplash", 0x3C);
        addSimpleCue(rodUsesGloom_, 0x29, splash_.get());
        addDrillCues(drill73_);
        addSimpleCue(smallSteam_, 2, wings_.get());
        addSimpleCue(rodGrab_, 0x0B, door_.get());
        break;
    case 0x4A:
        lever74_ = obj("S74_Hebel");
        rodUsesSwitch_ = obj("S74_RodUsesWeiche");
        rodLooksSwitch_ = obj("S74_RodLooksWeiche");
        break;
    case 0x4C:
    case 0x4F:
        drillLeaves_ = obj("Gen_DrillLeaves");
        drillDrills_ = obj("Gen_DrillDrills");
        schutt_ = obj("Gen_Schutt");
        schutt2_ = obj("Gen_Schutt2");
        rodGrab_ = obj("Gen_RodGreif");
        smallSteam_ = obj("Gen_SmallSteam");
        if (sceneNr_ == 0x4C) {
            rodPickUp_ = obj("Gen_RodPickUp");
            tinder_ = scene_.getObject("S76_Zunder");
        }
        addDrillCues(drillLeaves_);
        addSimpleCue(drillDrills_, 1, drill_.get());
        addSimpleCue(schutt_, 0x0A, explosion_.get());
        addSimpleCue(schutt_, 0x14, explosion_.get());
        addSimpleCue(schutt_, 0x1E, explosion_.get());
        addSimpleCue(schutt_, 0x1E, explosion_.get());  // Original: zweimal Bild 30
        addSimpleCue(schutt2_, 0x14, explosion_.get());
        addSimpleCue(schutt2_, 0x21, explosion_.get());
        addSimpleCue(rodGrab_, 8, door_.get());
        addSimpleCue(smallSteam_, 2, wings_.get());
        break;
    case 0x50:
        rodHacks_ = obj("S80_RodHacks");
        gloomOnFloor_ = scene_.getObject("S80_GloomOnFloor");
        rodTakesPickAxe_ = obj("S80_RodTakesPickAxe");
        rodTakesStone_ = obj("S80_RodTakesStone");
        rodTriesTake_ = obj("S80_RodTriesTake");
        pickAxe_ = scene_.getObject("S80_PickAxe");
        hack_ = loadSfx("Sfx_Work_Hack2");
        addSimpleCue(rodHacks_, 0x24, hack_.get());
        sizzle_ = loadSfx("Sfx_Sizzle");
        addSimpleCue(rodTriesTake_, 9, sizzle_.get());
        break;
    default:
        break;
    }
    setTimerHandler(*dripTimer_, static_cast<std::uint32_t>(random(5, 15) * 1000), 0, kDrip);
    music_ = playMusic("Loc21_DwarvesMine");
    if (music_) music_->setReverb(0x15, 0.652f, 2.886f, 0.25f);  // 0x3F26E979, 0x4038B439, 0x3E800000
}

void Loc21::destroyLocal() {
    dripTimer_.reset();
    splash_.reset();
    winding_.reset();
    wings_.reset();
    door_.reset();
    drill_.reset();
    explosion_.reset();
    hack_.reset();
    sizzle_.reset();
    drip_.reset();
}

bool Loc21::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo
    return wait();
}

bool Loc21::playEnd(AnimLayer* a, unsigned mode) {
    const int cb = beginWait();
    if (a) a->playEndEvent(mode, cb);
    return wait();
}

bool Loc21::playRange(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

void Loc21::area(const char* name, int ev) {
    if (Area* a = scene_.getArea(name)) a->setEvent(ev);
}

void Loc21::areaActive(const char* name, bool on) {
    if (Area* a = scene_.getArea(name)) a->active = on;  // Bereich +0xB8
}

void Loc21::arrive(int x, int y, int dir, int tx, int ty, int tdir) {
    if (!hero_) return;
    hero_->setPosDir(x, y, dir);
    hero_->walkTo(tx, ty, 0, tdir);  // CActor_WalkTo2 ohne Ereignis
}

void Loc21::go(int x, int y, int dir, int scene) {
    if (walk(x, y, dir)) changeScene(scene, 0x15);
}

void Loc21::showSchutt(int state) {
    // Schutthaufen je Zustand: 1 ganz, 2 halb (Bilder 20/21), 3 weg (Bilder 40/43)
    if (!schutt_ || !schutt2_) return;
    if (state < 1 || state > 3) return;
    schutt_->enable();
    schutt2_->enable();
    if (state == 2) {
        schutt_->showFrame(0x14);
        schutt2_->showFrame(0x15);
    } else if (state == 3) {
        schutt_->showFrame(0x28);
        schutt2_->showFrame(0x2B);
    }
}

void Loc21::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // CSzene+0x5C0
    setEventProc(&Loc21::onEvent);
    const bool fresh = restored_ == 0;
    // Ankunft: jeder Zweig endet mit EnableInput; CActor_Show nur nach dem Laden eines Spielstands
    switch (sceneNr_) {
    case 0x47:  // 71: Eingang mit Schaufel
        area("S71_To74", 0x0B);
        area("S71_Wall", 0x0C);
        area("S71_To69", 0x0A);
        if (auto* bar = scene_.inventory(); bar && !bar->has(0x0B) && shovel_) {
            shovel_->enable();
            if (shovel_->area) shovel_->area->setEvent(0x0D);
        }
        if (fresh) {
            if (prevScene_ == 0x4A)
                arrive(0x55, 0xE4, 0, 0x61, 0x113, 0);
            else
                arrive(0x276, 0x136, 2, 0x21D, 0x136, 2);
        }
        break;
    case 0x48:  // 72
        area("S72_To74", 0x15);
        area("S72_Wall", 0x16);
        area("S72_To73", 0x14);
        if (fresh) {
            if (prevScene_ == 0x49) arrive(0x1D9, 0xE0, 0, 0x1B4, 0x110, 1);
            else if (prevScene_ == 0x4A) arrive(0x82, 0xDC, 0, 0x91, 0x10B, 0);
            else { scene_.enableInput(); return; }
        }
        break;
    case 0x49:  // 73: Kessel und Bohrer
        area("S73_To72", 0x1E);
        area("S73_To75", 0x1F);
        area("S73_Wall", 0x20);
        if (auto* bar = scene_.inventory()) bar->setItemEvent(0x17, 0x23);  // Inventory_SetEvent
        if (flag(kDrillAt) == 1) {
            if (drill73_) {
                drill73_->enable();
                if (drill73_->area) drill73_->area->setEvent(0x21);
            }
            if (lever73_) {
                lever73_->enable();
                if (lever73_->area) lever73_->area->setEvent(0x22);
            }
        }
        if (fresh) {
            if (prevScene_ == 0x48) arrive(0xBD, 0xDC, 0, 0xBA, 0x103, 0);
            else if (prevScene_ == 0x4B) arrive(0x215, 0xE4, 0, 0x1DE, 0x120, 1);
            else { scene_.enableInput(); return; }
        }
        break;
    case 0x4A:  // 74: Weiche
        area("S74_To71", 0x28);
        area("S74_To72", 0x29);
        area("S74_To75", 0x2A);
        area("S74_To77", 0x2B);
        area("S74_Wall", 0x2C);
        if (lever74_) {
            lever74_->enable();
            if (lever74_->area) lever74_->area->setEvent(0x2D);
            if (flag(kSwitch) != 0) lever74_->showFrame(10);
        }
        if (auto* bar = scene_.inventory()) bar->setItemEvent(0x0B, 0x2E);
        if (fresh) {
            switch (prevScene_) {
            case 0x47: arrive(0x26D, 0xD8, 0, 0x250, 0xED, 1); break;
            case 0x48: arrive(0xD3, 0xB4, 0, 0xEA, 0xCD, 0); break;
            case 0x4B: arrive(0x1D2, 0xB0, 0, 0x1C1, 0xD0, 1); break;
            case 0x4D: arrive(10, 0xE4, 6, 0x46, 0xF7, 7); break;
            default: scene_.enableInput(); return;
            }
        }
        break;
    case 0x4B:  // 75
        area("S75_To74", 0x33);
        area("S75_To73", 0x32);
        area("S75_To76", 0x34);
        area("S75_Wall", 0x35);
        if (fresh) {
            if (prevScene_ == 0x49) arrive(0x15C, 200, 0, 0x14F, 0x105, 0);
            else if (prevScene_ == 0x4A) arrive(0x49, 0xCC, 0, 0x59, 0x10B, 0);
            else if (prevScene_ == 0x4C) arrive(0x27B, 0x107, 2, 0x224, 0x10F, 2);
            else { scene_.enableInput(); return; }
        }
        break;
    case 0x4C:  // 76: Schutt, Bohrer
        area("S76_To75", 0x3C);
        area("S76_Wall", 0x3D);
        showSchutt(flag(kSchutt76));
        if (flag(kDrillAt) == 2) {
            if (drillLeaves_) drillLeaves_->enable();
            area("S76_DriveLever", 0x3E);
            area("S76_DrillLever", 0x3F);
        }
        if (flag(kTinder) != 0 && tinder_) {
            if (tinder_->area) tinder_->area->setEvent(0x40);
            tinder_->enable();
        }
        if (fresh) arrive(0xD5, 0xD0, 0, 200, 0x104, 0);
        break;
    case 0x4D:  // 77
        area("S77_To74", 0x46);
        area("S77_To78", 0x47);
        area("S77_To80", 0x48);
        area("S77_Wall", 0x49);
        if (fresh) {
            if (prevScene_ == 0x4A) arrive(10, 0xE7, 6, 0x73, 0x118, 7);
            else if (prevScene_ == 0x4E) arrive(0x13E, 0xD0, 0, 0x145, 0xFB, 0);
            else if (prevScene_ == 0x50) arrive(0x276, 0xD8, 0, 0x243, 0x10F, 1);
            else { scene_.enableInput(); return; }
        }
        break;
    case 0x4E:  // 78
        area("S78_Wall", 0x52);
        area("S78_To77", 0x50);
        area("S78_To79", 0x51);
        if (fresh) {
            if (prevScene_ == 0x4D) arrive(0x2F, 0xDD, 6, 0x6E, 0x107, 7);
            else if (prevScene_ == 0x4F) arrive(0x270, 0x101, 2, 0x211, 0x115, 2);
            else { scene_.enableInput(); return; }
        }
        break;
    case 0x4F:  // 79: Schutt vor dem Ausgang nach 81
        area("S79_To78", 0x5A);
        area("S79_Wall", 0x5C);
        showSchutt(flag(kSchutt79));
        if (flag(kSchutt79) == 1 || flag(kSchutt79) == 2) {
            if (hero_) hero_->blockRect(kRodBlockL, kRodBlockT, kRodBlockR, kRodBlockB);
        } else if (flag(kSchutt79) == 3) {
            area("S79_To81", 0x5B);
        }
        if (flag(kDrillAt) == 3) {
            if (drillLeaves_) drillLeaves_->enable();
            area("S79_DriveLever", 0x5D);
            area("S79_DrillLever", 0x5E);
        }
        if (fresh) {
            if (prevScene_ == 0x4E) arrive(0xCF, 0xD0, 0, 0xC4, 0x10B, 0);
            else if (prevScene_ == 0x51) arrive(0x231, 0xD8, 0, 0x1F7, 0x119, 1);
            else { scene_.enableInput(); return; }
        }
        break;
    case 0x50:  // 80: Spitzhacke und Stein
        area("S80_To77", 100);
        area("S80_Wall", 0x65);
        if (auto* bar = scene_.inventory()) {
            if (!bar->has(9) && pickAxe_) {
                pickAxe_->enable();
                if (pickAxe_->area) pickAxe_->area->setEvent(0x66);
            }
            bar->setItemEvent(9, 0x68);
            bar->setItemEvent(0x0B, 0x69);
        }
        if (flag(kStone80) == 2 && gloomOnFloor_) {
            gloomOnFloor_->enable();
            if (gloomOnFloor_->area) gloomOnFloor_->area->setEvent(0x67);
        }
        if (fresh) arrive(0x99, 0xDC, 0, 0xA6, 0x110, 0);
        break;
    default:
        scene_.enableInput();
        return;
    }
    if (!fresh && hero_) hero_->show(scene_.clicksEnabled);
    scene_.enableInput();
}

void Loc21::driveLever(bool in79) {
    // orig: 0x00424600 Ereignisse 0x3E (76) und 0x5D (79)
    if (!walk(0x17F, 0x144, 4)) return;
    scene_.disableInput();
    if (hero_) hero_->hide(scene_.clicksEnabled);
    if (rodGrab_) rodGrab_->setPosZ(336.0, 156.0, 18.0, 106.0);  // 0x004474C0
    playEnd(rodGrab_, 10);
    if (hero_) hero_->show(scene_.clicksEnabled);
    if (drillDrills_) drillDrills_->disable();
    if (!in79) {
        areaActive("S76_DriveLever", false);
        areaActive("S76_DrillLever", false);
        if (drillLeaves_) drillLeaves_->start(10);
        // Weiche gestellt → nach 79, sonst zurück nach 73
        flag(kDrillAt) = flag(kSwitch) == 0 ? 1 : 3;
        scene_.enableInput();
        if (hero_) hero_->walkTo(0x17F, 0x15E, 0, 4);  // CActor_WalkTo2
        return;
    }
    if (flag(kSwitch) == 0) {
        // Weiche falsch: der Bohrer fährt ein Stück und kommt zurück
        if (hero_) hero_->walkTo(0x17F, 0x15E, 0, 4);
        playRange(drillLeaves_, 10, 0, 100);
        delay(3000);
        playRange(drillLeaves_, 11, 100, 0);
        scene_.enableInput();
        faceAndSay("079_ROD_06");
        return;
    }
    areaActive("S79_DriveLever", false);
    areaActive("S79_DrillLever", false);
    if (drillLeaves_) drillLeaves_->start(10);
    flag(kDrillAt) = 2;
    scene_.enableInput();
    if (hero_) hero_->walkTo(0x17F, 0x15E, 0, 4);
}

void Loc21::drillLever(bool in79) {
    // orig: 0x00424600 Ereignisse 0x3F (76) und 0x5E (79)
    const std::size_t st = in79 ? kSchutt79 : kSchutt76;
    scene_.disableInput();
    if (flag(st) == 1) {
        const int cb = beginWait();
        sayActor("Loc21", in79 ? "079_ROD_02" : "076_ROD_02", hero_, cb);
    }
    wait();  // Original: auch ohne BeginWait
    walk(0x1A1, 0x15A, 4);
    if (hero_) hero_->hide(scene_.clicksEnabled);
    if (rodGrab_) rodGrab_->setPosZ(370.0, 158.0, 9.0, 119.0);
    playEnd(rodGrab_, 10);
    if (hero_) hero_->show(scene_.clicksEnabled);
    if (drillLeaves_) drillLeaves_->disable();
    if (drillDrills_) drillDrills_->start(0x0D);
    {
        const int cb = beginWait();
        if (hero_) hero_->walkTo(0x104, 0x163, cb, 6);
        if (!in79) wait();  // 79: Original wartet nicht auf das Ende des Wegs
    }
    delay(3000);
    const auto stopDrill = [this] {
        if (drillDrills_) drillDrills_->stop(0x0B);  // vtable +0x24
    };
    switch (flag(st)) {
    case 1:
        if (schutt_) schutt_->playRangeNoEvent(11, 0, 0x13);
        playRange(schutt2_, 11, 0, 0x14);
        delay(1000);
        stopDrill();
        if (in79) {
            faceAndSay("079_ROD_03");
            flag(st) = 2;
        } else {
            flag(st) = 2;
            faceAndSay("076_ROD_04");
        }
        break;
    case 2:
        if (in79) {
            if (schutt_) schutt_->playRangeNoEvent(11, 0x14, 0x28);
            playRange(schutt2_, 11, 0x15, 0x2B);
            delay(1000);
            stopDrill();
            faceAndSay("079_ROD_05");
            flag(st) = 3;
            area("S79_To81", 0x5B);
            if (hero_) hero_->unblockRect(kRodBlockL, kRodBlockT, kRodBlockR, kRodBlockB);
        } else {
            if (schutt_) schutt_->playRangeNoEvent(11, 0x14, -1);
            playRange(schutt2_, 11, 0x15, 0x26);
            if (tinder_) {
                if (tinder_->area) tinder_->area->setEvent(0x40);
                tinder_->enable();
            }
            playRange(schutt2_, 11, 0x27, -1);
            flag(kTinder) = 1;
            delay(1000);
            stopDrill();
            flag(st) = 3;
            faceAndSay("076_ROD_03");
        }
        break;
    case 3:
        stopDrill();
        faceAndSay(in79 ? "079_ROD_06" : "076_ROD_04");
        break;
    default:
        break;
    }
    scene_.enableInput();
}

void Loc21::onEvent(Event& e, int) {
    switch (e.param) {
    case 0x0A: if (walk(0x276, 0x136, 6)) changeScene(0x45, 0x13); return;  // → 19/69
    case 0x0B: go(0x55, 0xE4, 4, 0x4A); return;
    case 0x0C: walkAndComment(0x14A, 0x12A, 4, "071_ROD_02"); return;
    case 0x0D:  // Schaufel (Gegenstand 11)
        if (!walk(0xFB, 0x117, 5)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        if (shovel_) shovel_->disable();
        if (auto* bar = scene_.inventory()) bar->add(0x0B);
        playEnd(rodTakesShovel_, 10);
        if (hero_) hero_->show(scene_.clicksEnabled);
        faceAndSay("071_ROD_01");
        return;
    case 0x14: go(0x1D9, 0xE0, 4, 0x49); return;
    case 0x15: go(0x82, 0xDC, 4, 0x4A); return;
    case 0x16:  // Wand abtasten
        if (!walk(0x11E, 0x123, 5)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playEnd(rodFeelsWall_, 10);
        if (hero_) hero_->show(scene_.clicksEnabled);
        faceAndSay("072_ROD_01");
        return;
    case 0x1E: go(0xBD, 0xDC, 4, 0x48); return;
    case 0x1F: go(0x215, 0xE4, 4, 0x4B); return;
    case 0x20: walkAndComment(0x143, 0x124, 4, "073_ROD_01"); return;
    case 0x21: walkAndComment(0x202, 0x147, 3, flag(kGloomUsed) == 0 ? "073_ROD_03" : "073_ROD_02"); return;
    case 0x22:  // Hebel am Bohrer in 73
        if (!walk(0x1CF, 0x158, 4)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playEnd(rodGrab_, 10);
        if (hero_) hero_->show(scene_.clicksEnabled);
        if (flag(kGloomUsed) == 0) {
            faceAndSay("073_ROD_05");
            return;
        }
        scene_.disableInput();
        if (lever73_) lever73_->disable();
        if (drill73_ && drill73_->area) drill73_->area->active = false;
        if (flag(kSwitch) == 0) {
            // Bohrer fährt nach 76
            flag(kDrillAt) = 2;
            if (drill73_) drill73_->start(10);
            delay(3000);
            scene_.enableInput();
            return;
        }
        // Weiche falsch gestellt: Bohrer fährt los und kommt zurück
        playEnd(drill73_, 11);
        playRange(drill73_, 11, 99, 0);
        if (lever73_) {
            lever73_->enable();
            if (lever73_->area) lever73_->area->active = true;
        }
        if (drill73_ && drill73_->area) drill73_->area->active = true;
        scene_.enableInput();
        faceAndSay("076_ROD_04");
        return;
    case 0x23:  // Leuchtstein (Gegenstand 23) in den Kessel
        if (!walk(0x1B9, 0x155, 3)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        if (auto* bar = scene_.inventory()) bar->remove(0x17);
        playEnd(rodUsesGloom_, 10);
        if (smallSteam_) smallSteam_->start(10);
        flag(kGloomUsed) = 1;
        if (hero_) hero_->show(scene_.clicksEnabled);
        faceAndSay("073_ROD_02");
        return;
    case 0x28: go(0x26D, 0xD8, 5, 0x47); return;
    case 0x29: go(0xD3, 0xB4, 4, 0x48); return;
    case 0x2A: go(0x1D2, 0xB0, 5, 0x4B); return;
    case 0x2B: go(10, 0xE4, 2, 0x4D); return;
    case 0x2C: walkAndComment(0x159, 0xDC, 4, "074_ROD_02"); return;
    case 0x2D:  // Weiche ansehen
        if (!walk(0xF0, 0xE7, 2)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playEnd(rodLooksSwitch_, 10);
        if (hero_) hero_->show(scene_.clicksEnabled);
        faceAndSay("074_ROD_01");
        return;
    case 0x2E:  // Schaufel an der Weiche: umstellen
        if (!walk(0xC2, 0xEC, 5)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        if (flag(kSwitch) == 0) {
            playRange(rodUsesSwitch_, 11, 0, 0x26);
            if (lever74_) lever74_->start(11);
            if (winding_) winding_->play();  // vtable +0x08
            playRange(rodUsesSwitch_, 10, 0x27, -1);
            flag(kSwitch) = 1;
        } else {
            playRange(rodUsesSwitch_, 11, 0x4B, 0x32);
            if (lever74_) lever74_->playRangeNoEvent(11, 10, 0);
            if (winding_) winding_->play();
            playRange(rodUsesSwitch_, 10, 0x31, 0);
            flag(kSwitch) = 0;
        }
        if (hero_) hero_->show(scene_.clicksEnabled);
        if (flag(kSwitchTalk) == 1) {
            faceAndSay("074_ROD_03");
            flag(kSwitchTalk) = 2;
            return;
        }
        faceAndSay("074_ROD_04");
        return;
    case 0x32: go(0x15C, 200, 4, 0x49); return;
    case 0x33: go(0x49, 0xCC, 4, 0x4A); return;
    case 0x34: go(0x27B, 0x107, 6, 0x4C); return;
    case 0x35: walkAndComment(0xCF, 0x10C, 4, "075_ROD_01"); return;
    case 0x3C: go(0xD5, 0xD0, 4, 0x4B); return;
    case 0x3D: walkAndComment(0x157, 0x134, 4, "076_ROD_01"); return;
    case 0x3E: driveLever(false); return;
    case 0x3F: drillLever(false); return;
    case 0x40:  // Zunder aufheben (Gegenstand 26)
        if (!walk(0x139, 0x134, 4)) return;
        scene_.disableInput();
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playRange(rodPickUp_, 11, 0, 7);
        if (tinder_) tinder_->disable();
        if (auto* bar = scene_.inventory()) bar->add(0x1A);
        flag(kTinder) = 0;
        playRange(rodPickUp_, 10, 8, -1);
        if (hero_) hero_->show(scene_.clicksEnabled);
        scene_.enableInput();
        faceAndSay("069_ROD_26");
        return;
    case 0x46: go(10, 0xE7, 2, 0x4A); return;
    case 0x47: go(0x13E, 0xD0, 4, 0x4E); return;
    case 0x48: go(0x276, 0xD8, 4, 0x50); return;
    case 0x49: walkAndComment(0x1C5, 0x10C, 4, "077_ROD_01"); return;
    case 0x50: go(0x2F, 0xDD, 2, 0x4D); return;
    case 0x51: go(0x270, 0x101, 6, 0x4F); return;
    case 0x52: walkAndComment(0x142, 0x114, 4, "078_ROD_01"); return;
    case 0x5A: go(0xCF, 0xD0, 4, 0x4E); return;
    case 0x5B:  // → Loc22/81
        scene_.disableInput();
        walk(0x231, 0xD8, 4);
        changeScene(0x51, 0x16);
        return;
    case 0x5C: walkAndComment(0x14B, 0x124, 4, "079_ROD_01"); return;
    case 0x5D: driveLever(true); return;
    case 0x5E: drillLever(true); return;
    case 100: go(0x99, 0xDC, 4, 0x4D); return;
    case 0x65:  // Wand in 80
        if (flag(kStone80) == 1 && feltWall_ != 0) {
            walkAndComment(0x16F, 0x133, 6, "080_ROD_02");
            flag(kWallHint) = 1;
            return;
        }
        if (!walk(0x118, 300, 6)) return;
        scene_.disableInput();
        delay(500);
        if (hero_) hero_->turnTo(2);  // 0x00435BA0
        delay(500);
        feltWall_ = 1;
        scene_.enableInput();
        faceAndSay("080_ROD_01");
        return;
    case 0x66:  // Spitzhacke (Gegenstand 9)
        if (!walk(0x15A, 300, 5)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        if (pickAxe_) pickAxe_->disable();
        if (auto* bar = scene_.inventory()) bar->add(9);
        playEnd(rodTakesPickAxe_, 10);
        if (auto* bar = scene_.inventory()) bar->setItemEvent(9, 0x68);
        if (hero_) hero_->show(scene_.clicksEnabled);
        faceAndSay("080_ROD_07");
        return;
    case 0x67:  // Stein am Boden: heiß
        if (flag(kTouch80) == 1) {
            if (!walk(0x16F, 0x133, 6)) return;
            if (hero_) hero_->hide(scene_.clicksEnabled);
            playEnd(rodTriesTake_, 10);
            flag(kTouch80) = 2;
            if (hero_) hero_->show(scene_.clicksEnabled);
            faceAndSay("080_ROD_03");
        } else if (flag(kTouch80) == 2) {
            walkAndComment(0x16F, 0x133, 6, "080_ROD_04");
        }
        return;
    case 0x68:  // Spitzhacke an der Wand
        if (flag(kStone80) != 1 || flag(kWallHint) == 0) {
            describeItem(9);
            return;
        }
        if (!walk(0x194, 0x12E, 6)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playEnd(rodHacks_, 10);
        if (gloomOnFloor_) {
            gloomOnFloor_->enable();
            if (gloomOnFloor_->area) gloomOnFloor_->area->setEvent(0x67);
        }
        flag(kStone80) = 2;
        if (hero_) hero_->show(scene_.clicksEnabled);
        faceAndSay("080_ROD_06");
        return;
    case 0x69:  // Schaufel: Stein aufnehmen (Gegenstand 23)
        if (flag(kStone80) != 2) {
            describeItem(0x0B);
            return;
        }
        if (!walk(0x16F, 0x133, 6)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        scene_.disableInput();
        playRange(rodTakesStone_, 11, 0, 0x2C);
        if (gloomOnFloor_) gloomOnFloor_->disable();
        flag(kStone80) = 3;
        playRange(rodTakesStone_, 10, 0x2D, -1);
        if (auto* bar = scene_.inventory()) bar->add(0x17);
        if (hero_) hero_->show(scene_.clicksEnabled);
        faceAndSay("080_ROD_05");
        scene_.enableInput();
        return;
    case kDrip:
        if (drip_) playSfxRandom3D(*drip_);
        setTimerHandler(*dripTimer_, static_cast<std::uint32_t>(random(5, 15) * 1000), 0, kDrip);
        return;
    default:
        return;
    }
}

}  // namespace rah
