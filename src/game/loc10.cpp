// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc10.hpp"

#include <cmath>
#include <cstdio>

#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
constexpr std::size_t kNight = 0x3E80;         // Wald bei Nacht (Ankunft aus Szene 1001)
constexpr std::size_t kGliderHere = 0x3F24;    // Gleiter steht in Szene 37
constexpr std::size_t kJasmin = 0x3F28;        // Jasmin begleitet Rodrigo
constexpr std::size_t kMetJasmin = 0x3F2C;     // Kommentar zum Wald (037_ROD_01/02)
constexpr std::size_t kGirlHasTeddyWish = 0x3F30;  // Mädchen springt Seil in Szene 38
constexpr std::size_t kGirlTalk = 0x3F34;      // Gesprächsstufe mit dem Mädchen
constexpr std::size_t kBush38 = 0x3F38;        // Busch in Szene 38 (Weg nach 57) weg
constexpr std::size_t kBush38b = 0x3F3C;       // Busch in Szene 38 (Weg nach 68) weg
constexpr std::size_t kBush39 = 0x3F40;        // Busch in Szene 39 (Weg nach 99) weg
constexpr std::size_t kMushroom39 = 0x3F44;    // Pilz in Szene 39 liegt
constexpr std::size_t kJasTree = 0x3F48;       // Jasmin sitzt auf dem Baum (Szene 40)
constexpr std::size_t kJasCalled = 0x3F4C;     // Baum schon angesprochen
constexpr std::size_t kJasTalk = 0x3F50;       // Gesprächsstufe mit Jasmin auf dem Baum
constexpr std::size_t kStraw = 0x3F54;         // Stroh/Feuer-Abfolge in Szene 40 (0…4)
constexpr std::size_t kNecklace = 0x3F58;      // Halskette liegt
constexpr std::size_t kTeddy = 0x3F5C;         // Teddy in Szene 41 (1 → 2 → 3)
constexpr std::size_t kBush42 = 0x3F60;        // Busch in Szene 42 (Weg nach 91) weg
constexpr std::size_t kSlug = 0x3F64;          // Schnecke (3 = explodiert)
constexpr std::size_t kLeaf = 0x3F68;          // Blatt in Szene 44 sichtbar
constexpr std::size_t kLeafReachable = 0x3F6C; // Blatt anklickbar
constexpr int kAmbient = 200;
constexpr int kGliderBob = 10;
constexpr int kGirlJump = 0x1B;
constexpr int kGirlStep = 0x1D;
constexpr int kJasCall = 0x4B;
constexpr double kTwoPi = 6.283185307;            // 0x0046B468
constexpr double kBobStep = 0.20943951023333332;  // 0x0046B470 (2π/30)
constexpr double kBobAmp = -2.0;                  // 0x0046B478
}  // namespace

Loc10::Loc10(GameContext& ctx, int scene, int prevScene) : Game(ctx, 10, scene, prevScene) {
    getViewObjects();
    if (prevScene_ == 113) flag(kGliderHere) = 1;
    switch (sceneNr_) {
    case 37: {
        unsigned mask = prevScene_ == 66 ? 1u : 0u;
        if (prevScene_ == 113) mask = 2;
        if (flag(kGliderHere) != 0) mask |= 4;
        if (prevScene_ == 1001) {
            flag(kNight) = 1;
            prevScene_ = 38;
        }
        scene_.load("S37", mask);
        getViewObjects();
        rodFallDown_ = obj("S37_RodFallDown");
        jasFallDown_ = obj("S37_JasFallDown");
        jasPointN_ = obj("S37_JasPointN");
        gliderArrives_ = obj("S37_GliderArrives");
        glider_ = scene_.getObject("S37_Glider");
        gliderLeaves_ = obj("S37_GliderLeaves");
        gliderTimer_ = std::make_unique<Event>();
        passing_ = loadSfxVol("Sfx_Glider_PassingBy", 70);
        knockWet_ = loadSfx("Sfx_Knock_Wet");
        break;
    }
    case 38: {
        const unsigned b0 = flag(kJasmin) != 0 ? 1u : 0u;
        unsigned mask = b0;
        if (flag(kGirlHasTeddyWish) != 0) {
            mask = b0 | 2;
            if (auto* bar = scene_.inventory(); bar && bar->has(12)) mask = b0 | 6;
        }
        scene_.load("S38", mask);
        getViewObjects();
        rodOffersTeddy_ = obj("S38_RodOffersTeddy");
        girl_ = obj("S38_Girl");
        girlJumps_ = obj("S38_GirlJumps");
        rodStealsRope_ = obj("S38_RodStealsRope");
        jasPointsNW_ = obj("S38_JasPointsNW");
        bush_ = scene_.getObject("Gen_Busch");
        bushClone_ = scene_.getObject("Gen_BuschClone");
        girlTimer_ = std::make_unique<Event>();
        knockWet_ = loadSfx("Sfx_Knock_Wet");
        stepSoft_ = loadSfxVol("Sfx_Step_Soft", 70);
        break;
    }
    case 39:
        scene_.load("S39", 0xFFFF);  // CSzene_LoadAll
        getViewObjects();
        bush_ = scene_.getObject("Gen_Busch");
        mushroom39_ = scene_.getObject("S39_Pilz");
        rodPickUp_ = obj("Gen_RodPickUp");
        break;
    case 40:
        scene_.load("S40", flag(kJasTree) != 0 ? 1u : 0u);
        getViewObjects();
        climbsTree_ = obj("S40_RodClimbsTree");
        dropsStraw_ = obj("S40_RodDropsStraw");
        killsFire_ = obj("S40_RodKillsFire");
        looksRoots_ = obj("S40_RodLooksRoots");
        setsFire_ = obj("S40_RodSetsFire");
        takesStraw_ = obj("S40_RodTakesStraw");
        watchesNecklace_ = obj("S40_RodWatchesNecklace");
        takesNecklace_ = obj("S40_RodTakesNecklace");
        necklaceDropped_ = obj("S40_NecklaceIsDropped");
        necklace_ = scene_.getObject("S40_Necklace");
        straw_ = scene_.getObject("S40_Straw");
        fireStarts_ = obj("S40_FireStarts");
        fireLoops_ = obj("S40_FireLoops");
        fireStops_ = obj("S40_FireStops");
        branch_ = obj("S40_Branch");
        jasminTimer_ = std::make_unique<Event>();
        clothes_ = loadSfxVol("Sfx_Clothes_Scratched", 70);
        stepSoft_ = loadSfxVol("Sfx_Step_Soft", 70);
        match_ = loadSfxVol("Sfx_Match_BeingLit", 50);
        fire_ = loadSfxVol("Sfx_Fire", 70);
        break;
    case 41:
        scene_.load("S41", 0xFFFF);
        getViewObjects();
        rodTakesTeddy_ = obj("S41_RodTakesTeddy");
        break;
    case 42:
        scene_.load("S42", 0xFFFF);
        getViewObjects();
        bush_ = scene_.getObject("Gen_Busch");
        bushClone_ = scene_.getObject("Gen_BuschClone");
        break;
    case 43:
        scene_.load("S43", 0xFFFF);
        getViewObjects();
        break;
    case 44:
        scene_.load("S44", flag(kLeaf) != 0 ? 1u : 0u);
        getViewObjects();
        rodTakesLeaf_ = obj("S44_RodTakesLeaf");
        slugExplodes_ = obj("S44_SlugExplodes");
        leaf_ = scene_.getObject("S44_Leaf");
        explosion_ = loadSfx("Sfx_Explosion_Misc2");
        eat_ = loadSfx("Sfx_Eat");
        break;
    case 66:
        scene_.load("S66", 0xFFFF);
        getViewObjects();
        woodChase_ = loadAnim("S66_WoodChase");
        explosion_ = loadSfx("Sfx_Explosion_Misc2");
        gliderFlying_ = loadSfxVol("Sfx_Glider_Flying", 90);
        break;
    default: break;
    }
    if (flag(kNight) == 0) {
        bird_ = loadSfx("Sfx_Bird");
        bird2_ = loadSfx("Sfx_Bird2");
        bird3_ = loadSfx("Sfx_Bird3");
        chirp_ = loadSfx("Sfx_Chirp");
        fly2_ = loadSfx("Sfx_Fly2");
    } else {
        owl_ = loadSfx("Sfx_Owl");
        thunder_ = loadSfx("Sfx_Thunder");
        thunder2_ = loadSfx("Sfx_Thunder2");
    }
    ambientTimer_ = std::make_unique<Event>();
    if (sceneNr_ == 66) {
        music_ = playMusic("Loc35_RodrigoOnGlider");
    } else if (flag(kNight) != 0) {
        char name[20];
        std::snprintf(name, sizeof name, "S%02d_Back_nd", sceneNr_);  // wsprintfA
        scene_.replaceBackground(name);  // 0x0043FD90(name, 1)
        music_ = playMusic("Loc10_ForestInDarkness");
        if (hero_) hero_->setColor(-122, -84, -24);  // CLayer_SetColor 0x004473E0
    } else {
        music_ = playMusic("Loc10_ForestBeforeDarkness");
    }
    if (music_) {
        music_->setReverb(4, 0.208f, 0.478f, 0.0f);  // 0x3E54FDF4, 0x3EF4BC6A, 0
        music_->setReverbMix(0.1f);                  // 0x3DCCCCCD
    }
}

void Loc10::destroyLocal() {
    // Ereignisse: 0x0043F160 (austragen) + Löschen
    gliderTimer_.reset();
    girlTimer_.reset();
    jasminTimer_.reset();
    woodChase_.reset();
    ambientTimer_.reset();
    for (auto* b : {&bird_, &bird2_, &bird3_, &chirp_, &owl_, &thunder_, &thunder2_, &passing_, &knockWet_,
                    &stepSoft_, &clothes_, &match_, &fire_, &explosion_, &eat_, &gliderFlying_, &fly2_})
        b->reset();
}

bool Loc10::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo
    return wait();
}

bool Loc10::playWait(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

void Loc10::setLayerEvent(Layer* l, int ev) {
    if (l && l->area) l->area->setEvent(ev);  // UNVERIFIED: Original ohne Prüfung
}

void Loc10::setAreaEvent(const char* name, int ev) {
    if (Area* a = scene_.getArea(name)) a->setEvent(ev);  // UNVERIFIED: Original ohne Prüfung
}

void Loc10::start() {
    if (sceneNr_ == 66) {
        if (music_) music_->play();  // vtable +8 statt Einblenden
    } else if (music_) {
        music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    }
    scene_.disableAll();
    if (sceneNr_ != 66)
        if (auto* bg = scene_.background()) bg->enable();  // CSzene+0x5C0
    setEventProc(&Loc10::onEvent);
    const auto place = [this](int x, int y, int d, int wx, int wy, int wd) {
        if (!hero_) return;
        hero_->setPosDir(x, y, d);
        hero_->walkTo(wx, wy, 0, wd);  // CActor_WalkTo2
    };
    bool showAtEnd = false;  // Sprungziel vor dem Umgebungs-Timer: CActor_Show
    switch (sceneNr_) {
    case 37:
        for (const auto& [n, ev] : {std::pair{"S37_To38", 1}, {"S37_To39", 2}, {"S37_To40", 3}, {"S37_To41", 4},
                                    {"S37_To42", 5}, {"S37_To43", 6}, {"S37_Pilz", 7}, {"S37_Wald", 8}})
            setAreaEvent(n, ev);
        if (restored_ == 0) {
            switch (prevScene_) {
            case 38: place(325, 112, 0, 325, 130, 0); break;
            case 39: place(634, 135, 1, 575, 150, 1); break;
            case 40: place(630, 203, 2, 591, 198, 2); break;
            case 41: place(309, 353, 4, 320, 325, 4); break;
            case 42: place(10, 257, 6, 89, 255, 6); break;
            case 43: place(10, 195, 6, 79, 199, 6); break;
            case 66: {
                // Absturz im Wald: Rodrigo und Jasmin fallen herunter
                scene_.disableInput();
                flag(kMetJasmin) = 1;
                flag(kJasmin) = 1;
                flag(kBush38b) = 1;
                flag(0x3F04) = 1;
                flag(0x3EFC) = 0;
                flag(0x3F00) = 0;
                if (jasFallDown_) jasFallDown_->start(11);
                playWait(rodFallDown_, 11, 0, 7);
                play(knockWet_.get());
                playWait(rodFallDown_, 10, 8, -1);
                place(163, 204, 6, 220, 204, 6);
                {
                    const int cb = beginWait();
                    speak("Loc10", "037_JAS_01", cb);  // CGame_SayNoActor
                    wait();
                }
                disable(jasFallDown_);
                if (second_) {
                    second_->setPosDir(254, 187, 2);
                    int cb = beginWait();
                    second_->walkTo(329, 226, cb, 2);
                    wait();
                    cb = beginWait();
                    second_->turn(4, cb);
                    wait();
                    second_->hide(scene_.clicksEnabled);
                }
                enable(jasPointN_);
                break;
            }
            case 113:
                // Landung mit dem Gleiter
                if (passing_) {
                    passing_->play();
                    passing_->fadeFreq(25, 2000);
                }
                {
                    const int cb = beginWait();
                    if (gliderArrives_) gliderArrives_->playEndEvent(10, cb);
                    wait();
                }
                place(250, 197, 4, 250, 200, 0);
                break;
            default: break;
            }
        } else {
            if (prevScene_ == 66) enable(jasPointN_);
            showHero();
        }
        if (flag(kGliderHere) != 0) {
            if (hero_) hero_->blockRect(193, 189, 323, 199);
            enable(glider_);
            setLayerEvent(glider_, 9);
            if (glider_) {
                bobX_ = static_cast<int>(glider_->x);  // __ftol
                bobY_ = static_cast<int>(glider_->y);
            }
            bobUnused_ = 0;
            bobPhase_ = 0;
            setTimerHandler(*gliderTimer_, 40, 1, kGliderBob);  // periodisch
        }
        break;
    case 38:
        if (flag(kGirlHasTeddyWish) != 0) {
            enable(girlJumps_);
            setLayerEvent(girlJumps_, 0x1A);
            setTimerHandler(*girlTimer_, static_cast<std::uint32_t>(random(10, 20) * 100), 0, kGirlJump);
            if (auto* bar = scene_.inventory()) bar->setItemEvent(12, 0x1C);
        }
        setAreaEvent("S38_To37", 0x14);
        setAreaEvent("S38_To39", 0x15);
        setAreaEvent("S38_Pilz", 0x18);
        setAreaEvent("S38_Wald", 0x19);
        if (flag(kBush38) == 0) {
            enable(bushClone_);
            if (hero_) hero_->blockRect(230, 40, 420, 160);
        } else {
            setAreaEvent("S38_To57", 0x16);
        }
        if (flag(kBush38b) == 0) {
            enable(bush_);
            if (hero_) hero_->blockRect(0, 0, 170, 190);
        } else {
            setAreaEvent("S38_To68", 0x17);
        }
        if (restored_ == 0) {
            switch (prevScene_) {
            case 37:
                if (flag(kJasmin) == 0) {
                    place(337, 355, 4, 343, 328, 4);
                } else {
                    scene_.disableInput();
                    if (second_) {
                        second_->setPosDir(329, 345, 4);
                        second_->walkTo(197, 198, 0, 2);
                    }
                    if (hero_) hero_->setPosDir(337, 330, 4);
                    const int cb = beginWait();
                    if (hero_) hero_->walkTo(371, 210, cb, 2);
                    wait();
                    scene_.enableInput();
                }
                break;
            case 39: place(635, 195, 2, 576, 196, 2); break;
            case 57: place(318, 108, 0, 318, 124, 0); break;
            case 68:
            case 101: place(10, 136, 7, 103, 165, 7); break;
            default: break;
            }
        } else {
            showHero();
            if (flag(kJasmin) != 0 && second_) {
                second_->setPosDir(197, 198, 2);
                second_->show(scene_.clicksEnabled);
            }
        }
        break;
    case 39:
        setAreaEvent("S39_To37", 0x28);
        setAreaEvent("S39_To38", 0x29);
        setAreaEvent("S39_To40", 0x2A);
        setAreaEvent("S39_Pilz1", 0x2C);
        setAreaEvent("S39_Pilz2", 0x2D);
        setAreaEvent("S39_Pilz3", 0x2E);
        setAreaEvent("S39_Wald", 0x2F);
        if (flag(kBush39) == 0) {
            enable(bush_);
            if (hero_) hero_->blockRect(460, 110, 639, 260);
        } else {
            setAreaEvent("S39_To99", 0x2B);
        }
        if (flag(kMushroom39) != 0) {
            enable(mushroom39_);
            setLayerEvent(mushroom39_, 0x30);
        }
        if (restored_ == 0) {
            switch (prevScene_) {
            case 37: place(7, 292, 5, 55, 291, 5); break;
            case 38: place(5, 211, 6, 53, 216, 6); break;
            case 40: place(365, 355, 4, 374, 335, 4); break;
            case 99: place(635, 190, 2, 572, 194, 2); break;
            default: break;
            }
        } else {
            showAtEnd = true;
        }
        break;
    case 40:
        setAreaEvent("S40_To37", 0x3C);
        setAreaEvent("S40_To39", 0x3D);
        setAreaEvent("S40_Pilze", 0x3E);
        setAreaEvent("S40_Pilz", 0x3F);
        setAreaEvent("S40_Wald", 0x40);
        setAreaEvent("S40_Baum", 0x42);
        enable(branch_);
        if (flag(kJasTree) != 0) {
            if (flag(kJasTalk) < 2) setTimerHandler(*jasminTimer_, 20000, 0, kJasCall);
            if (auto* bar = scene_.inventory()) {
                bar->setItemEvent(15, 0x4C);
                bar->setItemEvent(26, 0x4D);
            }
            switch (flag(kStraw)) {
            case 1: setAreaEvent("S40_Root", 0x41); break;
            case 2:
                enable(straw_);
                setLayerEvent(straw_, 0x44);
                break;
            case 3:
                if (fireLoops_) fireLoops_->start(13);
                if (fire_) fire_->playLoop();  // vtable +0x10
                setLayerEvent(fireLoops_, 0x45);
                break;
            case 4: enable(straw_); break;
            default: break;
            }
            if (flag(kNecklace) != 0) {
                enable(necklace_);
                setLayerEvent(necklace_, 0x43);
            }
        }
        if (restored_ == 0) {
            if (prevScene_ == 37) place(5, 205, 6, 55, 211, 7);
            else if (prevScene_ == 39) place(314, 76, 0, 318, 128, 0);
        } else {
            showAtEnd = true;
        }
        break;
    case 41:
        setAreaEvent("S41_To37", 0x50);
        setAreaEvent("S41_To42", 0x51);
        setAreaEvent("S41_Pilz", 0x52);
        setAreaEvent("S41_Wald", 0x53);
        if (restored_ == 0) {
            if (prevScene_ == 37) place(346, 96, 0, 337, 136, 0);
            else if (prevScene_ == 42) place(5, 200, 6, 54, 203, 7);
        } else {
            showAtEnd = true;
        }
        break;
    case 42:
        if (flag(kBush42) == 0) {
            enable(bush_);
            enable(bushClone_);
            if (hero_) hero_->blockRect(220, 260, 380, 359);
        } else {
            setAreaEvent("S42_To91", 0x5C);
        }
        setAreaEvent("S42_To37", 0x5A);
        setAreaEvent("S42_To41", 0x5B);
        setAreaEvent("S42_Wald", 0x5D);
        if (restored_ == 0) {
            if (prevScene_ == 37) place(628, 119, 1, 572, 143, 1);
            else if (prevScene_ == 41) place(628, 203, 2, 594, 203, 2);
            else if (prevScene_ == 91) place(309, 355, 4, 323, 333, 4);
        } else {
            showAtEnd = true;
        }
        break;
    case 43:
        setAreaEvent("S43_To37", 100);
        setAreaEvent("S43_To44", 0x65);
        setAreaEvent("S43_Pilz", 0x67);
        setAreaEvent("S43_Wald", 0x66);
        if (restored_ == 0) {
            if (prevScene_ == 37) place(635, 192, 2, 588, 196, 2);
            else if (prevScene_ == 44) place(303, 120, 0, 312, 138, 0);
        } else {
            showAtEnd = true;
        }
        break;
    case 44:
        setAreaEvent("S44_To43", 0x6E);
        setAreaEvent("S44_Pilz", 0x71);
        setAreaEvent("S44_Wald", 0x6F);
        setAreaEvent("S44_Busch", 0x70);
        if (auto* bar = scene_.inventory()) bar->setItemEvent(8, 0x73);
        if (flag(kLeaf) != 0) {
            enable(leaf_);
            if (flag(kLeafReachable) != 0) setLayerEvent(leaf_, 0x72);
        }
        if (restored_ == 0) place(374, 355, 4, 374, 318, 4);
        else showAtEnd = true;
        break;
    case 66: {
        // Flucht mit dem Gleiter über den Wald, Absturz
        AnimLayer* a = woodChase_.get();
        scene_.setModal(a);
        if (gliderFlying_) gliderFlying_->playLoop();
        playWait(a, 11, 0, 5);
        speak("Loc10", "066_JAS_01", 0);
        playWait(a, 11, 6, 29);
        speak("Loc10", "066_JAS_02", 0);
        playWait(a, 11, 30, 40);
        if (explosion_) {
            explosion_->setVolume(80);
            explosion_->play();
        }
        playWait(a, 11, 41, 47);
        speak("Loc10", "066_JAS_03", 0);
        playWait(a, 11, 48, 61);
        if (explosion_) {
            explosion_->setVolume(100);
            explosion_->play();
        }
        playWait(a, 10, 62, -1);
        flag(kGliderHere) = 0;
        changeScene(37, 10);
        break;
    }
    default: break;
    }
    if (showAtEnd) showHero();
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(5, 15) * 1000), 0, kAmbient);
    scene_.enableInput();
}

void Loc10::leave37(int id) {
    if (flag(kJasmin) == 0) {
        struct Exit { int x, y, dir, scene; };
        static constexpr Exit kExits[] = {{325, 112, 4, 38}, {634, 135, 5, 39}, {630, 203, 6, 40},
                                          {309, 353, 0, 41}, {10, 257, 1, 42},  {10, 195, 2, 43}};
        const Exit& e = kExits[id - 1];
        if (walk(e.x, e.y, e.dir)) changeScene(e.scene, 10);
        return;
    }
    if (id == 1) {
        if (!walk(334, 147, 4)) return;
        scene_.disableInput();
        disable(jasPointN_);
        if (second_) second_->walkTo(325, 112, 0, 4);
        walk(325, 112, 4);
        changeScene(38, 10);
        return;
    }
    // Jasmin zeigt nach Norden: „Da lang“
    struct Refuse { int x, y, dir, bx, by, bdir; };
    static constexpr Refuse kRefuse[] = {{550, 156, 5, 388, 198, 1}, {566, 206, 6, 421, 211, 2},
                                         {313, 298, 0, 271, 233, 5}, {194, 235, 1, 240, 213, 6},
                                         {187, 202, 2, 228, 205, 6}};
    const Refuse& r = kRefuse[id - 2];
    if (!walk(r.x, r.y, r.dir)) return;
    scene_.disableInput();
    speak("Loc10", "037_JAS_02", 0);
    const int cb = beginWait();
    if (jasPointN_) jasPointN_->playEndEvent(11, cb);
    wait();
    scene_.enableInput();
    if (hero_) hero_->walkTo(r.bx, r.by, 0, r.bdir);
}

void Loc10::leave38(int id) {
    if (flag(kJasmin) == 0) {
        switch (id) {
        case 0x14:
            if (walk(337, 355, 0)) changeScene(37, 10);
            return;
        case 0x15:
            if (walk(635, 195, 6)) changeScene(39, 10);
            return;
        case 0x16:
            if (walk(318, 108, 4)) changeScene(57, 15);
            return;
        case 0x17:
            if (!walk(10, 136, 3)) return;
            if (flag(kNight) != 0) changeScene(101, 19);
            else changeScene(68, 19);
            return;
        default: return;
        }
    }
    if (id == 0x17) {
        if (!walk(110, 168, 3)) return;
        scene_.disableInput();
        disable(jasPointsNW_);
        if (second_) second_->walkTo(10, 136, 0, 3);
        walk(10, 136, 3);
        scene_.enableInput();
        if (second_) second_->hide(scene_.clicksEnabled);
        changeScene(68, 19);
        return;
    }
    // Jasmin zeigt nach Nordwesten
    struct Refuse { int x, y, dir, bx, by, bdir; };
    static constexpr Refuse kRefuse[] = {{330, 299, 0, 333, 248, 3}, {531, 199, 6, 390, 195, 2},
                                         {322, 153, 4, 315, 174, 1}};
    const Refuse& r = kRefuse[id - 0x14];
    if (!walk(r.x, r.y, r.dir)) return;
    scene_.disableInput();
    speak("Loc10", "038_JAS_01", 0);
    if (second_) second_->hide(scene_.clicksEnabled);
    const int cb = beginWait();
    if (jasPointsNW_) jasPointsNW_->playEndEvent(11, cb);
    wait();
    scene_.enableInput();
    if (hero_) hero_->walkTo(r.bx, r.by, 0, r.bdir);
}

void Loc10::girlTalk(const char* rod, const char* girl, int next) {
    scene_.disableInput();
    stopEvent(girlTimer_.get());
    if (girlJumps_ && girlJumps_->frame != 0) {
        const int cb = beginWait();
        girlJumps_->playEndEvent(12, cb);
    }
    wait();  // Original wartet auch ohne BeginWait (letzter Platz, schon frei)
    heroSay(rod);
    disable(girlJumps_);
    {
        const int cb = beginWait();
        speak("Loc10", girl, girl_, 13, 0, 8, cb);
        wait();
    }
    disable(girl_);
    enable(girlJumps_);
    if (girlJumps_ && girlJumps_->area) girlJumps_->area->active = true;
    setTimerHandler(*girlTimer_, static_cast<std::uint32_t>(random(5, 10) * 1000), 0, kGirlJump);
    flag(kGirlTalk) = next;
    scene_.enableInput();
}

void Loc10::jasminTree() {
    switch (flag(kStraw)) {
    case 0:
    case 1:
    case 2: {
        const bool first = flag(kStraw) != 2;
        // Rodrigo klettert auf den Baum
        scene_.disableInput();
        heroSay("040_ROD_05");
        speak("Loc10", "040_JAS_05", 0);
        hideHero();
        playWait(climbsTree_, 11, 0, 33);
        play(clothes_.get());
        playWait(climbsTree_, 10, 34, -1);
        showHero();
        if (first) {
            flag(kStraw) = 1;
            setAreaEvent("S40_Root", 0x41);
        }
        scene_.enableInput();
        faceAndSay("040_ROD_18");
        return;
    }
    case 3:
        heroSay("040_ROD_07");
        speak("Loc10", "040_JAS_07", 0);
        return;
    case 4:
        heroSay("040_ROD_06");
        speak("Loc10", "040_JAS_06", 0);
        return;
    default: return;
    }
}

void Loc10::onEvent(Event& e, int) {
    const int id = e.param;
    switch (id) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6: leave37(id); return;
    case 7: walkAndComment(221, 312, 1, "037_ROD_03"); return;
    case 8: walkAndComment(170, 199, 4, flag(kMetJasmin) == 0 ? "037_ROD_01" : "037_ROD_02"); return;
    case 9:
        // Abflug mit dem Gleiter
        if (!walk(250, 197, 4)) return;
        stopEvent(gliderTimer_.get());
        disable(glider_);
        hideHero();
        playWait(gliderLeaves_, 11, 0, 42);
        if (passing_) {
            passing_->setFreq(25);
            passing_->play();
            passing_->fadeFreq(100, 2000);
        }
        playWait(gliderLeaves_, 10, 43, -1);
        scene_.invalidateAll();
        flag(kGliderHere) = 0;
        changeScene(8, 1);
        return;
    case kGliderBob: {
        // Gleiter schaukelt: y = Ruhelage − (int)(sin(phase) · −2)
        const int dy = static_cast<int>(std::sin(bobPhase_) * kBobAmp);  // __ftol
        if (glider_) glider_->setPosZ(bobX_, bobY_ - dy, 0.0, 0.0);      // 0x004474C0
        bobPhase_ += kBobStep;
        scene_.invalidateAll();
        if (kTwoPi <= bobUnused_) bobUnused_ -= kTwoPi;
        if (kTwoPi <= bobPhase_) bobPhase_ -= kTwoPi;
        return;
    }
    case 0x14:
    case 0x15:
    case 0x16:
    case 0x17: leave38(id); return;
    case 0x18: walkAndComment(451, 193, 5, "038_ROD_08"); return;
    case 0x19: walkAndComment(539, 191, 4, "038_ROD_01"); return;
    case 0x1A:
        stopEvent(girlTimer_.get());
        setTimerHandler(*girlTimer_, 15000, 0, kGirlJump);
        if (!walk(338, 218, 2)) return;
        switch (flag(kGirlTalk)) {
        case 1: girlTalk("038_ROD_02", "038_M\xC4" "D_02", 2); return;
        case 2: girlTalk("038_ROD_03", "038_M\xC4" "D_03", 3); return;
        case 3:
            heroSay("038_ROD_04");
            flag(kGirlTalk) = 4;
            return;
        case 4: girlTalk("038_ROD_05", "038_M\xC4" "D_05", 5); return;
        case 5: girlTalk("038_ROD_06", "038_M\xC4" "D_06", 6); return;
        case 6: {
            // Rodrigo klaut das Springseil
            scene_.disableInput();
            stopEvent(girlTimer_.get());
            if (girlJumps_ && girlJumps_->frame != 0) {
                const int cb = beginWait();
                girlJumps_->playEndEvent(12, cb);
            }
            wait();
            disable(girlJumps_);
            hideHero();
            playWait(rodStealsRope_, 10, 0, 43);
            play(knockWet_.get());
            speak("Loc10", "038_ROD_07", 0);
            playWait(rodStealsRope_, 10, 44, -1);
            enable(girlJumps_);
            showHero();
            if (girlJumps_ && girlJumps_->area) girlJumps_->area->active = true;
            girlTimer_->enabled = true;
            fireEvent(g_, *girlTimer_, girlTimer_->type);  // CEvent_Fire(ev, ev+0x10)
            const int cb = beginWait();
            if (hero_) hero_->turn(0, cb);
            wait();
            heroSay("038_ROD_11");
            scene_.enableInput();
            return;
        }
        default: return;
        }
    case kGirlJump:
        if (girlJumps_) {
            girlJumps_->start(12);
            girlJumps_->setFrameEvent(kGirlStep, 21);
        }
        setTimerHandler(*girlTimer_, static_cast<std::uint32_t>(random(5, 10) * 1000), 0, kGirlJump);
        return;
    case 0x1C: {
        // Teddy (Gegenstand 12) auf das Mädchen
        if (flag(kGirlHasTeddyWish) == 0) {
            describeItem(12);
            return;
        }
        stopEvent(girlTimer_.get());
        setTimerHandler(*girlTimer_, 15000, 0, kGirlJump);
        if (!walk(338, 218, 2)) return;
        scene_.disableInput();
        stopEvent(girlTimer_.get());
        if (girlJumps_ && girlJumps_->frame != 0) {
            const int cb = beginWait();
            girlJumps_->playEndEvent(12, cb);
        }
        wait();
        heroSay("038_ROD_09");
        hideHero();
        disable(girlJumps_);
        AnimLayer* a = rodOffersTeddy_;
        playWait(a, 11, 0, 17);
        if (a) a->playRange(11, 0, 18, 23);
        {
            const int cb = beginWait();
            speak("Loc10", "038_M\xC4" "D_09", cb);
            wait();
        }
        if (a) a->playRange(11, 0, 24, 35);
        {
            const int cb = beginWait();
            speak("Loc10", "038_ROD_10", cb);
            wait();
        }
        speak("Loc10", "038_M\xC4" "D_10", 0);
        playWait(a, 11, 36, 55);
        play(stepSoft_.get());
        for (const auto& [f, t] : {std::pair{56, 61}, {62, 67}, {68, 73}, {74, 79}, {80, 85}}) {
            playWait(a, 11, f, t);
            play(stepSoft_.get());
        }
        playWait(a, 10, 86, -1);
        showHero();
        flag(kGirlHasTeddyWish) = 0;
        if (auto* bar = scene_.inventory()) {
            bar->add(10);
            bar->remove(12);
        }
        scene_.enableInput();
        faceAndSay("038_ROD_12");
        return;
    }
    case kGirlStep:
        play(stepSoft_.get());
        if (girlJumps_) girlJumps_->setFrameEvent(kGirlStep, 31);
        return;
    case 0x28:
        if (walk(7, 292, 1)) changeScene(37, 10);
        return;
    case 0x29:
        if (walk(5, 211, 2)) changeScene(38, 10);
        return;
    case 0x2A:
        if (walk(365, 355, 0)) changeScene(40, 10);
        return;
    case 0x2B:
        if (walk(635, 190, 6)) changeScene(99, 8);
        return;
    case 0x2C: walkAndComment(455, 228, 7, "039_ROD_01"); return;
    case 0x2D: walkAndComment(455, 228, 7, "039_ROD_02"); return;
    case 0x2E: walkAndComment(455, 228, 7, "039_ROD_03"); return;
    case 0x2F: walkAndComment(357, 166, 4, "039_ROD_FX1"); return;
    case 0x30:
        // Pilz aufheben (Gegenstand 8)
        if (!walk(465, 183, 2)) return;
        scene_.disableInput();
        hideHero();
        playWait(rodPickUp_, 11, 0, 7);
        disable(mushroom39_);
        if (auto* bar = scene_.inventory()) bar->add(8);
        flag(kMushroom39) = 0;
        playWait(rodPickUp_, 10, 8, -1);
        showHero();
        scene_.enableInput();
        faceAndSay("037_ROD_03");
        return;
    case 0x3C:
        if (walk(5, 205, 2)) changeScene(37, 10);
        return;
    case 0x3D:
        if (walk(314, 76, 4)) changeScene(39, 10);
        return;
    case 0x3E: walkAndComment(176, 231, 1, "040_ROD_11"); return;
    case 0x3F: walkAndComment(381, 247, 0, "040_ROD_12"); return;
    case 0x40: walkAndComment(218, 176, 3, "040_ROD_01"); return;
    case 0x41: {
        if (!walk(432, 220, 6)) return;
        hideHero();
        const int cb = beginWait();
        if (looksRoots_) looksRoots_->playEndEvent(10, cb);
        wait();
        showHero();
        faceAndSay("040_ROD_18");
        return;
    }
    case 0x42:
        if (flag(kJasTree) != 0) {
            if (flag(kJasCalled) != 0) {
                if (!walk(432, 220, 6)) return;
                switch (flag(kJasTalk)) {
                case 1:
                    flag(kJasTalk) = 2;
                    stopEvent(jasminTimer_.get());
                    heroSay("040_ROD_02");
                    speak("Loc10", "040_JAS_02", 0);
                    return;
                case 2:
                    flag(kJasTalk) = 3;
                    heroSay("040_ROD_03");
                    speak("Loc10", "040_JAS_03", 0);
                    return;
                case 3:
                    flag(kJasTalk) = 4;
                    heroSay("040_ROD_04");
                    speak("Loc10", "040_JAS_04", 0);
                    return;
                case 4: jasminTree(); return;
                default: return;
                }
            }
            flag(kJasCalled) = 1;
        }
        walkAndComment(420, 218, 6, "040_ROD_01");
        return;
    case 0x43:
        // Halskette aufheben (Gegenstand 16)
        if (!walk(327, 251, 0)) return;
        hideHero();
        playWait(takesNecklace_, 10, 0, 12);
        disable(necklace_);
        playWait(takesNecklace_, 10, 13, -1);
        showHero();
        if (auto* bar = scene_.inventory()) bar->add(16);
        flag(kNecklace) = 0;
        heroSay("040_ROD_10");
        return;
    case 0x44:
        // Stroh nehmen (Gegenstand 15)
        if (!walk(432, 220, 6)) return;
        hideHero();
        playWait(takesStraw_, 11, 0, 8);
        disable(straw_);
        playWait(takesStraw_, 10, 9, -1);
        flag(kStraw) = 1;
        setAreaEvent("S40_Root", 0x41);
        if (auto* bar = scene_.inventory()) {
            bar->add(15);
            bar->setItemEvent(15, 0x4C);
        }
        showHero();
        faceAndSay("040_ROD_09");
        return;
    case 0x45:
        // Feuer austreten
        if (!walk(432, 220, 6)) return;
        scene_.disableInput();
        hideHero();
        playWait(killsFire_, 11, 0, 12);
        play(stepSoft_.get());
        playWait(killsFire_, 11, 13, 19);
        play(stepSoft_.get());
        playWait(killsFire_, 10, 20, -1);
        showHero();
        disable(fireLoops_);
        playWait(fireStops_, 11, 0, 12);
        if (fire_) fire_->stop();  // vtable +0x14
        playWait(fireStops_, 10, 13, -1);
        enable(straw_);
        if (hero_) hero_->walkTo(380, 220, 0, 6);
        flag(kStraw) = 4;
        scene_.enableInput();
        return;
    case kJasCall:
        if (flag(kJasCalled) != 0) {
            if (branch_) branch_->start(12);
            speak("Loc10", "040_JAS_01", 0);
        }
        setTimerHandler(*jasminTimer_, static_cast<std::uint32_t>(random(10, 20) * 1000), 0, kJasCall);
        return;
    case 0x4C:
        // Stroh (15) unter den Baum legen
        if (flag(kJasTree) == 0 || flag(kStraw) != 1) {
            describeItem(15);
            return;
        }
        if (!walk(432, 220, 6)) return;
        hideHero();
        if (auto* bar = scene_.inventory()) bar->remove(15);
        playWait(dropsStraw_, 11, 0, 16);
        enable(straw_);
        playWait(dropsStraw_, 10, 17, -1);
        flag(kStraw) = 2;
        setLayerEvent(straw_, 0x44);
        if (Area* a = scene_.getArea("S40_Root")) a->active = false;
        showHero();
        heroSay("040_ROD_14");
        speak("Loc10", "040_JAS_14", 0);
        return;
    case 0x4D:
        // Gegenstand 26 (Streichhölzer): Stroh anzünden, Jasmin lässt die Kette fallen
        if (flag(kJasTree) == 0 || flag(kStraw) != 2) {
            describeItem(26);
            return;
        }
        if (!walk(432, 220, 6)) return;
        scene_.disableInput();
        heroSay("040_ROD_16");
        speak("Loc10", "040_JAS_16", 0);
        hideHero();
        playWait(setsFire_, 11, 0, 12);
        play(match_.get());
        playWait(setsFire_, 10, 13, -1);
        showHero();
        if (hero_) hero_->walkTo(380, 220, 0, 6);
        disable(straw_);
        playWait(fireStarts_, 11, 0, 4);
        if (fire_) fire_->playLoop();
        playWait(fireStarts_, 10, 5, -1);
        if (fireLoops_) fireLoops_->start(13);
        flag(kStraw) = 3;
        setLayerEvent(fireLoops_, 0x45);
        speak("Loc10", "040_JAS_17", 0);
        hideHero();
        if (watchesNecklace_) watchesNecklace_->start(11);
        playWait(necklaceDropped_, 11, 0, 5);
        play(stepSoft_.get());
        playWait(necklaceDropped_, 10, 6, -1);
        disable(watchesNecklace_);
        enable(necklace_);
        setLayerEvent(necklace_, 0x43);
        flag(kNecklace) = 1;
        showHero();
        {
            const int cb = beginWait();
            if (hero_) hero_->turn(1, cb);
            wait();
        }
        scene_.enableInput();
        return;
    case 0x50:
        if (walk(346, 96, 4)) changeScene(37, 10);
        return;
    case 0x51:
        if (walk(5, 200, 2)) changeScene(42, 10);
        return;
    case 0x52: walkAndComment(60, 203, 4, "040_ROD_11"); return;
    case 0x53:
        switch (flag(kTeddy)) {
        case 1:
            if (walk(441, 210, 6)) {
                faceAndSay("041_ROD_01");
                flag(kTeddy) = 2;
            }
            return;
        case 2: {
            // Teddy aus dem Wald holen (Gegenstand 12)
            if (!walk(441, 210, 6)) return;
            scene_.disableInput();
            heroSay("041_ROD_02");
            hideHero();
            const int cb = beginWait();
            if (rodTakesTeddy_) rodTakesTeddy_->playEndEvent(10, cb);
            wait();
            showHero();
            if (auto* bar = scene_.inventory()) bar->add(12);
            flag(kTeddy) = 3;
            scene_.enableInput();
            return;
        }
        case 3: walkAndComment(441, 210, 6, "041_ROD_01"); return;
        default: return;
        }
    case 0x5A:
        if (walk(628, 119, 5)) changeScene(37, 10);
        return;
    case 0x5B:
        if (walk(628, 203, 6)) changeScene(41, 10);
        return;
    case 0x5C:
        if (walk(309, 355, 0)) changeScene(91, 12);
        return;
    case 0x5D: walkAndComment(247, 173, 4, "042_ROD_01"); return;
    case 100:
        if (walk(635, 192, 6)) changeScene(37, 10);
        return;
    case 0x65:
        if (walk(303, 120, 4)) changeScene(44, 10);
        return;
    case 0x66: walkAndComment(207, 180, 3, "043_ROD_01"); return;
    case 0x67: walkAndComment(280, 247, 0, "043_ROD_02"); return;
    case 0x6E:
        if (walk(328, 355, 0)) changeScene(43, 10);
        return;
    case 0x6F: walkAndComment(168, 184, 4, "044_ROD_01"); return;
    case 0x70: walkAndComment(293, 184, 4, "044_ROD_02"); return;
    case 0x71: walkAndComment(433, 185, 6, "044_ROD_04"); return;
    case 0x72:
        // Blatt nehmen (Gegenstand 6)
        if (!walk(323, 184, 4)) return;
        if (rodTakesLeaf_) {
            // vtable +0x14 SetPos: jedes Mal 30 Pixel weiter rechts
            rodTakesLeaf_->setPos(static_cast<int>(rodTakesLeaf_->x) + 30, static_cast<int>(rodTakesLeaf_->y));
        }
        hideHero();
        playWait(rodTakesLeaf_, 11, 19, 26);
        disable(leaf_);
        flag(kLeaf) = 0;
        flag(kLeafReachable) = 0;
        playWait(rodTakesLeaf_, 10, 27, -1);
        if (auto* bar = scene_.inventory()) bar->add(6);
        showHero();
        faceAndSay("044_ROD_05");
        return;
    case 0x73:
        // Pilz (Gegenstand 8) an die Schnecke verfüttern
        if (!walk(293, 184, 4)) return;
        scene_.disableInput();
        if (auto* bar = scene_.inventory()) bar->remove(8);
        flag(kSlug) = 3;
        playWait(slugExplodes_, 11, 0, 46);
        play(eat_.get());
        playWait(slugExplodes_, 11, 47, 68);
        hideHero();
        if (explosion_) {
            explosion_->setVolume(100);
            explosion_->play();
        }
        if (rodTakesLeaf_) rodTakesLeaf_->playRangeNoEvent(11, 0, 18);
        playWait(slugExplodes_, 10, 69, -1);
        setLayerEvent(leaf_, 0x72);
        flag(kLeafReachable) = 1;
        disable(rodTakesLeaf_);
        showHero();
        faceAndSay("044_ROD_03");
        scene_.enableInput();
        return;
    case kAmbient: {
        if (flag(kNight) != 0) {
            AudioBuffer* b = nullptr;
            switch (random(1, 4)) {
            case 1:
            case 2: b = owl_.get(); break;
            case 3: b = thunder_.get(); break;
            case 4: b = thunder2_.get(); break;
            default: break;
            }
            if (b) playSfxRandom3D(*b);
        } else {
            AudioBuffer* b = nullptr;
            switch (random(1, 5)) {
            case 1: b = bird_.get(); break;
            case 2: b = bird2_.get(); break;
            case 3: b = bird3_.get(); break;
            case 4: b = chirp_.get(); break;
            case 5: b = fly2_.get(); break;
            default: break;
            }
            if (b) {
                b->setVolume(random(50, 100));
                playSfxRandom3D(*b);
            }
        }
        setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(5, 10) * 1000), 0, kAmbient);
        return;
    }
    default:
        // Abweichung: Original zeigt MessageBoxA("Unknown_Event____", "Message") (modal)
        std::fprintf(stderr, "Loc10: Unknown_Event____ (%d)\n", id);
        return;
    }
}

}  // namespace rah
