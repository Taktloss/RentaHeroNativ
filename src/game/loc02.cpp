// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc02.hpp"

#include <cstdio>

#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
constexpr std::size_t kStrangerHere = 0x3EA0;  // der Fremde sitzt in der Kneipe (Ladebit 1)
constexpr std::size_t kGuests = 0x3EA4;        // Gespräch mit den Gästen (1 → 2 → 3)
constexpr std::size_t kSancho = 0x3EA8;        // Gesprächsstufe mit Sancho
constexpr std::size_t kSanchoLate = 0x3EAC;    // Sancho, wenn der Fremde da ist
constexpr std::size_t kEar = 0x3EB0;           // Sanchos Ohr (1…3)
constexpr std::size_t kStranger = 0x3EB4;      // Gesprächsstufe mit dem Fremden
constexpr std::size_t kBeer = 0x3EB8;          // Bier des Fremden (1 → 2 → 3)
constexpr std::size_t kChest = 0x3EBC;         // Brust des Fremden (Klicks bis 3)
constexpr std::size_t kSanGreet = 0x3EC0;      // Sancho begrüßt beim Betreten der Theke
constexpr std::size_t kKanStage4 = 0x3EC4;  // Loc01 Szene 46, Gesprächsstufe 4 erreicht (0x00407B80); schaltet Sancho-Stufe 7 frei (0x00409420)
constexpr std::size_t kPirates = 0x3EC8;       // Piratenangriff gesehen (Loc03)
constexpr std::size_t kSabMet = 0x3ECC;        // Loc03: Wachen weg
}  // namespace

Loc02::Loc02(GameContext& ctx, int scene, int prevScene) : Game(ctx, 2, scene, prevScene) {
    getViewObjects();
    const unsigned strangerMask = flag(kStrangerHere) != 0 ? 1u : 0u;
    switch (sceneNr_) {
    case 15:
        scene_.load("S0015", 0);
        getViewObjects();
        freNeck_ = loadAnim("S0015_FreNeck");
        freBum_ = loadAnim("S0015_FreBum");
        freKnife_ = loadAnim("S0015_FreMesser");
        frePack_ = loadAnim("S0015_FrePack");
        pirates_ = loadAnim("S0015_Piraten");
        rodWonder_ = loadAnim("S0015_RodWunder");
        freDrink_ = obj("S0015_FreDrink");
        freTalk_ = obj("S0015_FreTalk");
        rodTalk_ = obj("S0015_RodTalk");
        drinkTimer_ = std::make_unique<Event>();
        tock_ = loadSfxVol("Sfx_Tock", 50);
        gluck_ = loadSfxVol("Sfx_Gluck", 50);
        danger_ = loadSfx("Sfx_DangerStrings");
        eat_ = loadSfx("Sfx_Eat");
        woosh_ = loadSfx("Sfx_Woosh");
        boing2_ = loadSfx("Sfx_Boing2");
        dangerHigh_ = loadSfx("Sfx_DangerStringsHigh");
        dangerLow_ = loadSfx("Sfx_DangerStringsLow");
        tableHit_ = loadSfxVol("Sfx_TableHit", 80);
        break;
    case 13:
        scene_.load("S0013", strangerMask);
        getViewObjects();
        if (strangerMask && prevScene_ == 2013) from2013_ = loadAnim("S0013_From2013");
        if (strangerMask) stranger13_ = scene_.getObject("S0013_Fre");
        stepLeft_ = loadSfxVol("Gen_StepLeft", 75);
        stepRight_ = loadSfxVol("Gen_StepRight", 75);
        break;
    case 14:
        scene_.load("S0014", 0xFFFF);  // CSzene_LoadAll
        getViewObjects();
        sanAnim_ = obj("S0014_SanAnim");
        boing_ = loadSfx("Sfx_Boing");
        boioing_ = loadSfx("Sfx_Boioing");
        knockWet_ = loadSfx("Sfx_Knock_Wet");
        noseBroken_ = loadSfx("Sfx_Nose_BeingBroken");
        tock_ = loadSfx("Sfx_Tock");
        break;
    case 2013:
        scene_.load("S2013", strangerMask);
        getViewObjects();
        if (strangerMask && prevScene_ == 13) from0013_ = loadAnim("S2013_From0013");
        if (strangerMask) scene_.replaceBackground("S2013_Back_mf");  // 0x0043FD90(…, 1)
        stepLeft_ = loadSfxVol("Gen_StepLeft", 75);
        stepRight_ = loadSfxVol("Gen_StepRight", 75);
        break;
    case 1013:
        scene_.load("S1013", 0xFFFF);  // CSzene_LoadAll
        getViewObjects();
        guest1_ = obj("S1013_Gast1");
        guest2_ = obj("S1013_Gast2");
        sancho1013_ = obj("S1013_San");
        sanchoTimer_ = std::make_unique<Event>();
        guestTimer_ = std::make_unique<Event>();
        knockWet_ = loadSfx("Sfx_Knock_Wet");
        gluck_ = loadSfxVol("Sfx_Gluck", 50);
        tock_ = loadSfxVol("Sfx_Tock", 50);
        break;
    default: break;
    }
    music_ = playMusic("Loc02_SanchosPub");
    if (music_) {
        music_->setReverb(4, 0.208f, 0.478f, 0.0f);  // 0x3E54FDF4, 0x3EF4BC6A, 0
        music_->setReverbMix(0.1f);                  // 0x3DCCCCCD
    }
}

Loc02::~Loc02() { shutdown(); }

void Loc02::destroyLocal() {
    if (sceneNr_ == 1013) {
        sanchoTimer_.reset();
        guestTimer_.reset();
        // Original-Fehler: in Szene 1013 springt der Abbau an den Klängen vorbei
        (void)knockWet_.release();
        (void)gluck_.release();
        (void)tock_.release();
        return;
    }
    drinkTimer_.reset();
    from2013_.reset();
    from0013_.reset();
    freNeck_.reset();
    freBum_.reset();
    freKnife_.reset();
    frePack_.reset();
    pirates_.reset();
    rodWonder_.reset();
    for (auto* b : {&boing_, &boioing_, &knockWet_, &noseBroken_, &tock_, &gluck_, &stepLeft_, &stepRight_,
                    &danger_, &eat_, &woosh_, &boing2_, &dangerHigh_, &dangerLow_, &tableHit_})
        b->reset();
}

bool Loc02::playWait(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

void Loc02::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    scene_.disableAll();
    if (Layer* bg = scene_.background()) bg->enable();  // CSzene+0x5C0
    setEventProc(&Loc02::onEvent);
    const auto area = [this](const char* name, int ev) {
        if (Area* a = scene_.getArea(name)) a->setEvent(ev);  // Port-Schutz, Original ohne Prüfung; alle Namen in den Daten vorhanden (tools/name_check.py)
    };
    switch (sceneNr_) {
    case 15:
        area("S0015_To2013A", 40);
        area("S0015_To2013B", 40);
        area("S0015_Fremder", 41);
        if (flag(kChest) < 3) area("S0015_FremderBrust", 42);
        if (flag(kBeer) < 3) area("S0015_Bier", 43);
        drink(6, 0);
        scene_.enableInput();
        return;
    case 13: {
        bool show = true;
        if (restored_ == 0) {
            if (prevScene_ == 1013) {
                if (hero_) {
                    hero_->setPosDir(393, 350, 4);
                    hero_->walkTo(393, 345, 0, 4);  // CActor_WalkTo2
                }
                show = false;
            } else if (prevScene_ == 2013) {
                if (flag(kStrangerHere) != 0) {
                    // Rodrigo kommt die Treppe herunter (Schritte im Wechsel)
                    scene_.setModal(from2013_.get());
                    playWait(from2013_.get(), 11, 0, 6);
                    play(stepLeft_.get());
                    playWait(from2013_.get(), 11, 7, 11);
                    play(stepRight_.get());
                    playWait(from2013_.get(), 11, 12, 16);
                    play(stepLeft_.get());
                    playWait(from2013_.get(), 11, 17, 21);
                    play(stepRight_.get());
                    playWait(from2013_.get(), 11, 22, 27);
                    play(stepLeft_.get());
                    playWait(from2013_.get(), 10, 28, -1);
                }
                if (hero_) {
                    hero_->setPosDir(485, 250, 0);
                    hero_->walkTo(485, 255, 0, 0);
                }
                show = false;
            } else if (hero_) {
                hero_->setPosDir(43, 279, 7);
            }
        }
        if (show && hero_) hero_->show(scene_.clicksEnabled);
        if (flag(kStrangerHere) != 0 && stranger13_) stranger13_->enable();
        area("S0013_Exit", 1);
        area("S0013_To2013", 2);
        area("S0013_To1013", 3);
        scene_.enableInput();
        return;
    }
    case 14:
        area("S0014_To1013", 30);
        area("S0014_Sancho", 31);
        area("S0014_SanchosOhr", 32);
        if (auto* bar = scene_.inventory()) bar->setItemEvent(13, 33);
        if (flag(kSanGreet) != 0) {
            counterSan("014_SAN_01");
            flag(kSanGreet) = 0;
        }
        scene_.enableInput();
        return;
    case 2013:
        if (restored_ == 0) {
            if (prevScene_ == 13) {
                if (flag(kStrangerHere) != 0) {
                    scene_.setModal(from0013_.get());
                    playWait(from0013_.get(), 11, 0, 3);
                    play(stepLeft_.get());
                    playWait(from0013_.get(), 11, 4, 10);
                    play(stepRight_.get());
                    playWait(from0013_.get(), 11, 11, 17);
                    play(stepLeft_.get());
                    playWait(from0013_.get(), 11, 18, 24);
                    play(stepRight_.get());
                    playWait(from0013_.get(), 10, 25, -1);
                    play(stepRight_.get());
                }
                if (hero_) {
                    hero_->setPosDir(394, 270, 3);
                    hero_->walkTo(380, 280, 0, 0);
                }
            } else if (prevScene_ == 15 && hero_) {
                hero_->setPosDir(260, 355, 4);
                hero_->walkTo(295, 301, 0, 5);
            }
        } else if (hero_) {
            hero_->show(scene_.clicksEnabled);
        }
        area("S2013_To0013", 21);
        if (flag(kStrangerHere) != 0) area("S2013_Fremder", 20);
        scene_.enableInput();
        return;
    case 1013:
        if (restored_ == 0) {
            if (prevScene_ == 13) {
                if (hero_) {
                    hero_->setPosDir(375, 356, 4);
                    hero_->show(scene_.clicksEnabled);
                }
            } else if (prevScene_ == 14 && hero_) {
                hero_->setPosDir(427, 232, 5);
                hero_->walkTo(351, 244, 0, 1);
            }
        } else if (hero_) {
            hero_->show(scene_.clicksEnabled);
        }
        for (const auto& [L, ev] : {std::pair<AnimLayer*, int>{sancho1013_, 10}, {guest1_, 11}, {guest2_, 11}}) {
            if (!L) continue;
            L->enable();
            if (L->area) L->area->setEvent(ev);
        }
        area("S1013_To0013", 12);
        setTimerHandler(*sanchoTimer_, static_cast<std::uint32_t>(random(10, 20) * 100), 0, 13);
        if (flag(kPirates) == 0 || flag(kGuests) == 3)
            setTimerHandler(*guestTimer_, static_cast<std::uint32_t>(random(10, 20) * 100), 0, 14);
        scene_.enableInput();
        return;
    default:
        scene_.enableInput();
        return;
    }
}

void Loc02::onEvent(Event& e, int) {
    switch (sceneNr_) {
    case 15: onStranger(e.param); return;
    case 13: onEntrance(e.param); return;
    case 14: onCounter(e.param); return;
    case 1013: onTaproom(e.param); return;
    case 2013: onBackRoom(e.param); return;
    default: return;
    }
}

void Loc02::onEntrance(int id) {
    const auto go = [this](int x, int y, int dir) {
        const int cb = beginWait();
        if (hero_) hero_->walkTo(x, y, cb, dir);
        return wait();
    };
    switch (id) {
    case 1:
        if (go(43, 279, 2)) changeScene(12, 3);
        return;
    case 2:
        if (go(475, 247, 4)) changeScene(2013, 2);
        return;
    case 3:
        if (go(393, 350, 0)) changeScene(1013, 2);
        return;
    default:
        // Abweichung: Original zeigt MessageBoxA("Unknown_Event____", "Rent_A_Hero") (modal)
        std::fprintf(stderr, "Loc02: Unknown_Event____ (%d)\n", id);
        return;
    }
}

void Loc02::onBackRoom(int id) {
    const auto go = [this](int x, int y, int dir) {
        const int cb = beginWait();
        if (hero_) hero_->walkTo(x, y, cb, dir);
        return wait();
    };
    if (id == 20) {
        if (go(260, 355, 0)) changeScene(15, 2);
    } else if (id == 21) {
        if (go(422, 279, 6)) changeScene(13, 2);
    }
}

void Loc02::onTaproom(int id) {
    const auto go = [this](int x, int y, int dir) {
        const int cb = beginWait();
        if (hero_) hero_->walkTo(x, y, cb, dir);
        return wait();
    };
    const auto line = [this](const char* key, AnimLayer* who, int mode, int from, int to) {
        const int cb = beginWait();
        speak("Loc02", key, who, mode, from, to, cb);
        wait();
    };
    const auto rod = [this](const char* key) {
        const int cb = beginWait();
        sayActor("Loc02", key, hero_, cb);
        wait();
    };
    switch (id) {
    case 10:
        if (go(434, 231, 5)) changeScene(14, 2);
        return;
    case 11: {
        if (!go(310, 258, 2)) return;
        if (flag(kPirates) == 0 || flag(kGuests) == 3) {
            switch (random(1, 4)) {
            case 1: sayActor("Loc02", "013_ROD_01", hero_, 0); return;
            case 2: sayActor("Loc02", "013_ROD_02", hero_, 0); return;
            case 3: sayActor("Loc02", "013_ROD_03", hero_, 0); return;
            case 4: sayActor("Loc02", "013_ROD_04", hero_, 0); return;
            default: return;
            }
        }
        if (flag(kGuests) == 1) {
            flag(kGuests) = 2;
            rod("013_ROD_05");
            line("013_GA1_01", guest1_, 0x0C, 0, 5);
            line("013_GA2_01", guest2_, 0x0C, 0, 4);
            rod("013_ROD_06");
            line("013_GA1_02", guest1_, 0x0C, 0, 5);
        } else if (flag(kGuests) == 2) {
            flag(kGuests) = 3;
            rod("013_ROD_07");
            line("013_GA1_03", guest1_, 0x0D, 0, 5);
            line("013_GA1_04", guest1_, 0x0C, 0, 5);
            delay(2000);
            line("013_GA1_05", guest1_, 0x0C, 0, 5);
            playWait(guest2_, 11, 18, 19);
            play(tock_.get());
            playWait(guest2_, 11, 20, 23);
            delay(2500);
            playWait(guest2_, 11, 20, 18);
            line("013_GA2_02", guest2_, 0x0C, 4, 12);
            setTimerHandler(*guestTimer_, static_cast<std::uint32_t>(random(10, 30) * 100), 0, 14);
        }
        return;
    }
    case 12:
        if (go(375, 356, 7)) changeScene(13, 2);
        return;
    case 13:
        if (sancho1013_) sancho1013_->start(12);
        setTimerHandler(*sanchoTimer_, static_cast<std::uint32_t>(random(5, 15) * 1000), 0, 13);
        return;
    case 14:
        // Gast trinkt: Schluck bei Bild 58, Abstellen bei Bild 63
        if (guest1_) {
            guest1_->playRangeNoEvent(12, 38, 70);
            guest1_->setStepCount(38);
            guest1_->setCountEvent(15, 58);
        }
        setTimerHandler(*guestTimer_, static_cast<std::uint32_t>(random(5, 15) * 1000), 0, 14);
        return;
    case 15:
        play(gluck_.get());
        if (guest1_) guest1_->setCountEvent(16, 63);
        return;
    case 16: play(tock_.get()); return;
    default: return;
    }
}

void Loc02::counterRod(std::string_view key) {
    const int cb = beginWait();
    speak("Loc02", key, sanAnim_, 10, 185, 193, cb);
    wait();
    scene_.invalidateAll();
}

void Loc02::counterSan(std::string_view key) {
    const int cb = beginWait();
    speak("Loc02", key, sanAnim_, 10, 150, 158, cb);
    wait();
    scene_.invalidateAll();
}

void Loc02::onCounter(int id) {
    scene_.disableInput();
    const auto sanSpeak = [this](const char* key, int mode, int from, int to) {
        const int cb = beginWait();
        speak("Loc02", key, sanAnim_, mode, from, to, cb);
        wait();
    };
    auto* bar = scene_.inventory();
    switch (id) {
    case 30:
        changeScene(1013, 2);
        scene_.enableInput();
        return;
    case 31: {
        if (flag(kSancho) == 1 || flag(kSanchoLate) == 1) {
            flag(kSancho) = 2;
            flag(kSanchoLate) = 2;
            counterRod("014_ROD_01");
            counterSan("014_SAN_02");
            scene_.enableInput();
            return;
        }
        if (flag(kStrangerHere) == 0) {
            switch (flag(kSancho)) {
            case 2:
                flag(kSancho) = 3;
                counterRod("014_ROD_02");
                counterSan("014_SAN_03");
                scene_.enableInput();
                return;
            case 3:
                flag(kSancho) = 4;
                counterRod("014_ROD_03");
                counterSan("014_SAN_04");
                scene_.enableInput();
                return;
            case 4:
                flag(kSancho) = 5;
                counterRod("014_ROD_04");
                playWait(sanAnim_, 11, 158, 170);
                sanSpeak("014_SAN_05", 10, 170, 178);
                playWait(sanAnim_, 11, 178, 185);
                scene_.invalidateAll();
                scene_.enableInput();
                return;
            case 5:
                flag(kSancho) = 6;
                playWait(sanAnim_, 11, 193, 205);
                sanSpeak("014_ROD_05", 13, 205, 213);
                playWait(sanAnim_, 11, 213, 220);
                playWait(sanAnim_, 11, 158, 170);
                sanSpeak("014_SAN_06", 13, 170, 178);
                playWait(sanAnim_, 11, 178, 185);
                counterRod("014_ROD_06");
                scene_.enableInput();
                return;
            case 6:
                if (flag(kKanStage4) != 0) flag(kSancho) = 7;
                break;
            case 7:
                if (!bar || !bar->has(13)) {
                    flag(kSancho) = 8;
                    counterRod("014_ROD_08");
                    counterSan("014_SAN_08");
                    counterRod("014_ROD_09");
                    counterSan("014_SAN_09");
                    scene_.enableInput();
                    return;
                }
                break;
            case 8:
                // Sancho gibt Gegenstand 13
                flag(kSancho) = 7;
                counterRod("014_ROD_10");
                counterSan("014_SAN_10");
                playWait(sanAnim_, 11, 220, 280);
                if (bar) {
                    bar->add(13);
                    bar->setItemEvent(13, 33);
                }
                scene_.enableInput();
                return;
            default:
                scene_.enableInput();
                return;
            }
            counterRod("014_ROD_07");
            counterSan("014_SAN_07");
            scene_.enableInput();
            return;
        }
        if (flag(kSanchoLate) == 2) {
            flag(kSanchoLate) = 3;
            counterRod("014_ROD_11");
            counterSan("014_SAN_11");
            counterRod("014_ROD_12");
            counterSan("014_SAN_12");
            scene_.enableInput();
            return;
        }
        if (flag(kSanchoLate) == 3) {
            counterRod("014_ROD_13");
            counterSan("014_SAN_13");
            scene_.enableInput();
            return;
        }
        break;
    }
    case 32: {
        const int ear = flag(kEar);
        if (ear > 0 && ear < 3) {
            // Rodrigo zieht an Sanchos Ohr
            flag(kEar) = ear + 1;
            playWait(sanAnim_, 11, 0, 21);
            play(boing_.get());
            playWait(sanAnim_, 11, 22, 55);
            play(boioing_.get());
            playWait(sanAnim_, 11, 56, 77);
            play(knockWet_.get());
            playWait(sanAnim_, 11, 78, 79);
            play(tock_.get());
            playWait(sanAnim_, 11, 80, 108);
            play(noseBroken_.get());
            playWait(sanAnim_, 11, 109, 129);
            play(knockWet_.get());
            playWait(sanAnim_, 11, 130, 150);
            counterRod("014_ROD_14");
            counterSan("014_SAN_14");
            counterRod("014_ROD_15");
            scene_.enableInput();
            return;
        }
        if (ear == 3) {
            playWait(sanAnim_, 11, 120, 123);
            delay(200);
            playWait(sanAnim_, 11, 123, 120);
            counterSan("014_SAN_16");
            scene_.enableInput();
            return;
        }
        break;
    }
    case 33:
        // Gegenstand 13 zurückgeben
        playWait(sanAnim_, 11, 280, 220);
        if (bar) bar->remove(13);
        counterRod("014_ROD_17");
        counterSan("014_SAN_17");
        break;
    default: break;
    }
    scene_.enableInput();
}

void Loc02::tableRod(std::string_view key) {
    const int cb = beginWait();
    speak("Loc02", key, rodTalk_, 10, 0, 8, cb);
    wait();
    scene_.invalidateAll();
}

void Loc02::tableExchange(std::string_view rod, std::string_view fre) {
    drink(4, 0);
    tableRod(rod);
    const int cb = beginWait();
    speak("Loc02", fre, freTalk_, 10, 0, 8, cb);
    wait();
    scene_.invalidateAll();
    drink(6, 0);
}

void Loc02::drink(int mode, int arg) {
    AnimLayer* L = freDrink_;
    if (!L || !drinkTimer_) return;  // Port-Schutz, Original ohne Prüfung; alle Namen in den Daten vorhanden (tools/name_check.py)
    int trans = 0;
    if (mode == 5) {
        if (L->running) {
            setTimerHandler(*drinkTimer_, 5000, 0, 44);
            return;
        }
    } else if (mode == 6) {
        setTimerHandler(*drinkTimer_, static_cast<std::uint32_t>(random(10, 20) * 100), 0, 44);
        return;
    }
    switch (mode) {
    case 1:
    case 2:
        drinkTimer_->enabled = false;
        drinkTimer_->pending = false;
        trans = arg;
        break;
    case 3:
    case 4:
        drinkTimer_->enabled = false;
        drinkTimer_->pending = false;
        break;
    case 5: {
        const int r = random(1, 4);
        trans = r == 1 ? 1 : (r > 1 && r < 5) ? 2 : 0;
        break;
    }
    default: break;
    }
    const auto finishRunning = [&] {
        const bool clicks = scene_.clicksEnabled;
        scene_.disableInput();
        const int cb = beginWait();
        L->setEndEvent(cb);  // 0x004434A0
        wait();
        scene_.enableInput();
        scene_.clicksEnabled = clicks;
    };
    if (mode == 2 || mode == 4) {
        if (L->running) finishRunning();
    } else if ((mode == 1 || mode == 3) && L->running) {
        if (trans == 0) return;
        finishRunning();
    }
    if (trans == 0) return;
    // 1: Glas zum Mund (Bilder 0–50), 2: absetzen (50–70)
    const int from = trans == 1 ? 0 : 50, to = trans == 1 ? 50 : 70;
    switch (mode) {
    case 1:
    case 3:
        L->userState2 = trans;
        L->userState = 0;
        L->playRangeNoEvent(11, from, to);
        return;
    case 2:
    case 4: {
        const bool clicks = scene_.clicksEnabled;
        scene_.disableInput();
        L->userState2 = trans;
        L->userState = 0;
        const int cb = beginWait();
        L->playRange(11, cb, from, to);
        wait();
        scene_.enableInput();
        scene_.clicksEnabled = clicks;
        return;
    }
    case 5:
        L->userState2 = trans;
        L->userState = 0;
        if (trans == 1)
            if (Area* a = scene_.getArea("S0015_Bier")) a->active = false;
        // Original-Fehler: Win32 SetEvent(EventValue(0x2B)) mit einer Ereignisnummer statt eines
        // Handles (0x0040A399) – schlägt fehl, keine Wirkung
        L->playRangeNoEvent(11, from, to);
        setTimerHandler(*drinkTimer_, static_cast<std::uint32_t>(random(5, 7) * 1000), 0, 44);
        if (trans == 1) {
            L->setStepCount(0);
            L->setCountEvent(45, 31);
        }
        return;
    default: return;
    }
}

void Loc02::onStranger(int id) {
    switch (id) {
    case 40:
        drink(4, 0);
        changeScene(2013, 2);
        return;
    case 41:
        switch (flag(kStranger)) {
        case 1:
            flag(kStranger) = 2;
            tableExchange("015_ROD_02", "015_FRE_02");
            return;
        case 2:
            flag(kStranger) = 3;
            tableExchange("015_ROD_03", "015_FRE_03");
            return;
        case 3:
            flag(kStranger) = 4;
            tableExchange("015_ROD_04", "015_FRE_04");
            return;
        case 4:
            // Der Fremde packt aus, die Piraten greifen an
            scene_.disableInput();
            drink(4, 0);
            tableRod("015_ROD_05");
            scene_.setModal(frePack_.get());
            speak("Loc02", "015_FRE_05", 0);  // CGame_SayNoActor
            playWait(frePack_.get(), 11, 0, 45);
            play(eat_.get());
            speak("Loc02", "015_ROD_06", 0);
            playWait(frePack_.get(), 11, 46, 54);
            play(eat_.get());
            playWait(frePack_.get(), 11, 55, 60);
            speak("Loc02", "015_FRE_06", 0);
            playWait(frePack_.get(), 11, 61, 69);
            play(dangerLow_.get());
            playWait(frePack_.get(), 10, 70, -1);
            scene_.setModal(pirates_.get());
            playWait(pirates_.get(), 11, 0, 14);
            play(danger_.get());
            speak("Loc02", "015_ST1_01", 0);
            playWait(pirates_.get(), 10, 15, -1);
            scene_.setModal(rodWonder_.get());
            play(dangerHigh_.get());
            {
                const int cb = beginWait();
                if (rodWonder_) rodWonder_->playEndEvent(11, cb);
                wait();
            }
            flag(kStrangerHere) = 0;
            flag(kStranger) = 5;
            flag(kSabMet) = 1;
            if (auto* bar = scene_.inventory()) bar->add(1);
            changeScene(12, 3);
            return;
        default: return;
        }
    case 42:
        if (flag(kChest) < 3) {
            ++flag(kChest);
            scene_.disableInput();
            drink(4, 0);
            scene_.setModal(freNeck_.get());
            playWait(freNeck_.get(), 11, 0, 12);
            play(boing2_.get());
            playWait(freNeck_.get(), 10, 13, -1);
            drink(6, 0);
            scene_.enableInput();
            if (flag(kChest) == 3)
                if (Area* a = scene_.getArea("S0015_FremderBrust")) a->active = false;
        }
        return;
    case 43:
        if (flag(kBeer) == 1) {
            flag(kBeer) = 2;
            scene_.disableInput();
            drink(4, 0);
            scene_.setModal(freBum_.get());
            playWait(freBum_.get(), 11, 0, 16);
            play(tableHit_.get());
            playWait(freBum_.get(), 10, 17, -1);
            drink(6, 0);
            scene_.enableInput();
        } else if (flag(kBeer) == 2) {
            flag(kBeer) = 3;
            scene_.disableInput();
            drink(4, 0);
            scene_.setModal(freKnife_.get());
            playWait(freKnife_.get(), 11, 0, 19);
            play(woosh_.get());
            playWait(freKnife_.get(), 10, 20, -1);
            drink(6, 0);
            scene_.enableInput();
            if (Area* a = scene_.getArea("S0015_Bier")) a->active = false;
        }
        return;
    case 44: drink(5, 0); return;
    case 45:
        play(gluck_.get());
        if (freDrink_) freDrink_->setCountEvent(46, 41);
        return;
    case 46:
        play(tock_.get());
        if (freDrink_) freDrink_->setCountEvent(47, 45);
        return;
    case 47:
        if (flag(kBeer) < 3)
            if (Area* a = scene_.getArea("S0015_Bier")) a->active = true;
        return;
    default: return;
    }
}

}  // namespace rah
