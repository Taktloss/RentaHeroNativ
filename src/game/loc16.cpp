// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc16.hpp"

#include <bit>
#include <cstdio>
#include <cstdlib>

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
float f32(std::uint32_t bits) { return std::bit_cast<float>(bits); }
constexpr std::size_t kNight = 0x3E80;
constexpr std::size_t kMegophiasHelm = 0x3E90;  // Megophias hat vom Helm erzählt (Loc27 0x0042C078); ≠ 0 → Hinweis bei der Landung
constexpr std::size_t kHelmHint = 0x3F40;    // Hinweis zum Helm gegeben
constexpr std::size_t k3F74 = 0x3F74;        // Loc12: 1 → 2 nach Gesprächsstufe 6 (Nacht)
constexpr std::size_t kFirstVisit = 0x3FA4;  // erster Besuch: Begrüßung (Start 1)
constexpr std::size_t kPirate = 0x3FA8;      // ein Pirat kommt aus 59 (Loc17) und wird verbrannt
constexpr std::size_t kDayTalk = 0x3FAC;     // Gesprächsstufe am Tag (1…11)
constexpr std::size_t kNightTalk = 0x3FB0;   // Gesprächsstufe in der Nacht (1…6)
constexpr std::size_t kFlyTalk = 0x3FB4;     // Gesprächsstufe vor dem Abflug (1…6)
constexpr std::size_t kTo59Open = 0x3FB8;    // Weg nach 59 freigegeben
constexpr std::size_t kCanFly = 0x3FBC;      // Drache fliegt Rodrigo (nachts)
constexpr std::size_t kHat = 0x3FC0;         // Hut liegt da (Gegenstand 4)
constexpr std::size_t kHand = 0x3FC4;        // Hand liegt da (Gegenstand 3)
constexpr std::size_t kVulturesGone = 0x3FC8;  // Aasfresser haben den Piraten (Loc17 0x0041D480, Rückkehr aus dem Schädel)
constexpr std::size_t kBeforeFirstFlight = 0x409C;  // Start 1, Loc26 setzt 0 bei der ersten Landung auf der Insel
constexpr int kHelm = 5;
constexpr int kPoseEvent = 6;
constexpr int kAmbient = 7;
constexpr int kFromLoc17 = 0x3B, kFromFlight = 0x5D;
}  // namespace

Loc16::Loc16(GameContext& ctx, int scene, int prevScene) : Game(ctx, 16, scene, prevScene) {
    sceneNr_ = 0x3A;
    getViewObjects();
    unsigned mask;
    if (flag(kNight) == 0) {
        mask = 1;
        if (flag(kPirate) != 0) pirateBurn_ = loadAnim("S58_PirateBurn");
    } else {
        mask = 2;
        if (flag(kCanFly) != 0) takeOff_ = loadAnim("S58_DragonTakeOff");
        if (prevScene_ == kFromFlight) landing_ = loadAnim("S58_DragonLanding");
    }
    scene_.load("S58", mask);  // DAT_0047399C
    getViewObjects();
    hat_ = scene_.getObject("S58_Hut");
    hand_ = scene_.getObject("S58_Hand");
    dragon_ = obj(flag(kNight) == 0 ? "S58_DragonTalk" : "S58_DragonTalk_nd");
    rodAttack_ = obj("S58_RodAngriff");
    rodLooksSky_ = obj("S58_RodLooksSky");
    rodPickUp_ = obj("S58_RodPickUp");
    rodSitDown_ = obj("S58_RodSitDown");
    to57_ = scene_.getArea("S58_To57");
    to59_ = scene_.getArea("S58_To59");
    poseTimer_ = std::make_unique<Event>();
    ambientTimer_ = std::make_unique<Event>();
    if (flag(kNight) == 0) {
        bird_ = loadSfx("Sfx_Bird");
        bird2_ = loadSfx("Sfx_Bird2");
        bird3_ = loadSfx("Sfx_Bird3");
        chirp_ = loadSfx("Sfx_Chirp");
    } else {
        if (hero_) hero_->setColor(-52, -11, 43);  // CLayer_SetColor 0x004473E0
        scene_.replaceBackground("S58_Back_nd");   // 0x0043FD90(…, 1)
        owl_ = loadSfx("Sfx_Owl");
        thunder_ = loadSfx("Sfx_Thunder");
        thunder2_ = loadSfx("Sfx_Thunder2");
    }
    woosh_ = loadSfx("Sfx_Woosh");
    clothes_ = loadSfxVol("Sfx_Clothes_Scratched", 0x46);
    wings_ = loadSfx("Sfx_DragonWings2");
    danger_ = loadSfx("Sfx_DangerStrings");
    dangerHigh_ = loadSfx("Sfx_DangerStringsHigh");
    knock_ = loadSfx("Sfx_Knock_Wet");
    nose_ = loadSfxVol("Sfx_Nose_BeingBroken", 0x46);
    fire_ = loadSfx("Sfx_DragonFire");
    music_ = playMusic("Loc16_DragonsLair");
    if (music_) {
        music_->setReverb(0x0F, f32(0x3DE353F8), f32(0x404947AE), f32(0x3EF1A9FC));
        music_->setReverbMix(f32(0x3DCCCCCD));  // 0x004375B0
    }
}

void Loc16::destroyLocal() {
    poseTimer_.reset();
    ambientTimer_.reset();
    pirateBurn_.reset();
    takeOff_.reset();
    landing_.reset();
    for (auto* b : {&bird_, &bird2_, &bird3_, &chirp_, &owl_, &thunder_, &thunder2_, &woosh_, &clothes_, &wings_,
                    &danger_, &dangerHigh_, &knock_, &nose_, &fire_})
        b->reset();
}

bool Loc16::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo
    return wait();
}

bool Loc16::playRange(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

void Loc16::blockSky(bool on) {
    if (!hero_) return;
    if (on)
        hero_->blockRect(0, 0, 0x27F, 0x106);
    else
        hero_->unblockRect(0, 0, 0x27F, 0x106);
}

void Loc16::dragonSay(const char* key) {
    pose(4, 0);
    const int cb = beginWait();
    speak("Loc16", key, dragon_, 0x0C, 0, 8, cb);
    wait();
    pose(6, 0);
}

void Loc16::exchange(const char* rod, const char* dra) {
    pose(3, 0);
    heroSay(rod);
    dragonSay(dra);
}

void Loc16::exchangeN(int n) {
    char rod[16], dra[16];
    std::snprintf(rod, sizeof rod, "058_ROD_%02d", n);
    std::snprintf(dra, sizeof dra, "058_DRA_%02d", n);
    exchange(rod, dra);
}

void Loc16::pose(int mode, int arg) {
    AnimLayer* L = dragon_;
    if (!L) return;  // UNVERIFIED: Original ohne Prüfung
    if (mode == 5) {
        if (L->running) {  // +0x1A8
            setTimerHandler(*poseTimer_, static_cast<std::uint32_t>(random(30, 60) * 100), 0, kPoseEvent);
            return;
        }
    } else if (mode == 6) {
        setTimerHandler(*poseTimer_, static_cast<std::uint32_t>(random(10, 20) * 100), 0, kPoseEvent);
        return;
    }
    // Haltungen (+0x2B4): 0 Ruhe, 1…5 Gesten; Übergang (+0x2B8): ungerade 0→n, gerade n→0
    int trans = 0;
    const int st = L->userState;
    switch (mode) {
    case 1:
    case 2:
        poseTimer_->enabled = false;
        poseTimer_->pending = false;
        trans = arg;
        break;
    case 3:
    case 4:
        poseTimer_->enabled = false;
        poseTimer_->pending = false;
        if (st == 0) trans = (arg >= 1 && arg <= 5) ? 2 * arg - 1 : 0;
        else if (st >= 1 && st <= 5) trans = 2 * st;
        break;
    case 5:
        if (st == 0) {
            const int r = random(1, 2);
            trans = r == 1 ? 1 : r == 2 ? 3 : 0;
        } else if (st == 1) {
            trans = 2;
        } else if (st == 2) {
            trans = 4;
        }
        break;
    default: break;
    }
    const auto finishRunning = [&] {
        const bool clicks = scene_.clicksEnabled;  // +0x514
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
    static constexpr int kFrom[] = {0, 0x55, 0x5A, 0x81, 0x84, 0x30, 0x4D, 0x56, 0x76, 0x81, 0x8B};
    static constexpr int kTo[] = {0, 0x5A, 0x55, 0x84, 0x81, 0x49, 0x56, 0x6E, 0x81, 0x8B, 0x81};
    static constexpr int kPose[] = {0, 1, 0, 2, 0, 3, 0, 4, 0, 5, 0};
    const int from = kFrom[trans], to = kTo[trans];
    switch (mode) {
    case 1:
    case 3:
        L->userState2 = trans;
        L->userState = kPose[trans];
        L->playRangeNoEvent(11, from, to);
        return;
    case 2:
    case 4: {
        const bool clicks = scene_.clicksEnabled;
        scene_.disableInput();
        L->userState2 = trans;
        L->userState = kPose[trans];
        const int cb = beginWait();
        L->playRange(11, cb, from, to);
        wait();
        scene_.enableInput();
        scene_.clicksEnabled = clicks;
        return;
    }
    case 5:
        L->userState2 = trans;
        L->userState = kPose[trans];
        L->playRangeNoEvent(11, from, to);
        setTimerHandler(*poseTimer_, static_cast<std::uint32_t>(random(5, 7) * 1000), 0, kPoseEvent);
        return;
    default: return;
    }
}

void Loc16::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    setEventProc(&Loc16::onEvent);
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(5, 15) * 1000), 0, kAmbient);
    if (dragon_) dragon_->enable();
    const auto showItem = [](Layer* l, int ev) {
        if (!l) return;
        l->enable();
        if (l->area) l->area->setEvent(ev);
    };
    if (flag(kHand) != 0) showItem(hand_, 5);
    if (flag(kHat) != 0) showItem(hat_, 4);
    if (restored_ == 0) {
        if (prevScene_ == kFromLoc17) {
            if (hero_) hero_->setPosDir(500, 0xDA, 0);
            if (flag(kPirate) == 0) {
                scene_.disableInput();
                if (!walk(0x1E0, 0x13B, 1)) return;  // Original: Rückkehr ohne EnableInput
                scene_.enableInput();
            } else {
                // ein Pirat ist Rodrigo gefolgt: der Drache verbrennt ihn
                scene_.disableInput();
                walk(0x15D, 0x138, 5);
                dragonSay("058_DRA_03");
                playSfx(dangerHigh_.get());
                heroSay("058_ROD_03");
                scene_.setModal(pirateBurn_.get());
                playSfx(danger_.get());
                playRange(pirateBurn_.get(), 11, 0, 0x13);
                playSfx(knock_.get());
                playRange(pirateBurn_.get(), 11, 0x14, 0x1F);
                playSfx(knock_.get());
                playRange(pirateBurn_.get(), 11, 0x20, 0x28);
                playSfx(nose_.get());
                playRange(pirateBurn_.get(), 11, 0x29, 0x31);
                playSfx(knock_.get());
                playRange(pirateBurn_.get(), 11, 0x32, 0x57);
                playSfx(fire_.get());
                playRange(pirateBurn_.get(), 10, 0x58, -1);
                flag(kHand) = 1;
                showItem(hand_, 5);
                flag(kHat) = 1;
                showItem(hat_, 4);
                if (hero_) hero_->setPosDir(0x15D, 0x138, 2);
                walk(0x178, 0x13D, 4);
                exchange("058_ROD_04", "058_DRA_04");
                flag(kPirate) = 0;
                scene_.enableInput();
            }
        } else if (prevScene_ == kFromFlight) {
            // Landung nach dem Flug
            scene_.disableInput();
            scene_.setModal(landing_.get());
            playRange(landing_.get(), 11, 0, 0x21);
            playSfx(clothes_.get());
            playRange(landing_.get(), 10, 0x22, -1);
            if (hero_) hero_->setPosDir(0x1AB, 0x11E, 4);
            walk(0x11D, 0x140, 6);
            auto* bar = scene_.inventory();
            if ((!bar || !bar->has(kHelm)) && flag(kMegophiasHelm) != 0 && flag(kHelmHint) == 0) {
                exchange("058_ROD_05", "058_DRA_05");
                exchange("058_ROD_06", "058_DRA_06");
                flag(kHelmHint) = 1;
            }
            scene_.enableInput();
        } else {
            if (hero_) hero_->setPosDir(0x10, 300, 6);
            if (flag(kFirstVisit) == 0) {
                if (hero_) hero_->walkTo(0xC0, 0x125, 0, 6);
            } else {
                scene_.disableInput();
                walk(0xC0, 0x125, 6);
                dragonSay("058_DRA_01");
                exchange("058_ROD_01", "058_DRA_02");
                flag(kFirstVisit) = 0;
                scene_.enableInput();
            }
        }
    } else if (hero_) {
        hero_->show(scene_.clicksEnabled);
    }
    if (flag(kTo59Open) == 0 || flag(kVulturesGone) != 0) blockSky(true);
    if (to57_) to57_->setEvent(1);
    if (to59_) to59_->setEvent(2);
    if (Area* a = scene_.getArea("S58_Dragon")) a->setEvent(3);
    pose(6, 0);
    scene_.enableInput();
}

void Loc16::takeOff() {
    scene_.disableInput();
    pose(4, 0);
    walk(0x1AB, 0x11E, 4);
    if (hero_) hero_->hide(scene_.clicksEnabled);
    scene_.setModal(takeOff_.get());
    playRange(takeOff_.get(), 11, 0, 7);
    playSfx(clothes_.get());
    playRange(takeOff_.get(), 11, 8, 0x43);
    playSfx(wings_.get());
    playRange(takeOff_.get(), 11, 0x44, 0x57);
    playSfx(wings_.get());
    playRange(takeOff_.get(), 10, 0x58, -1);
    changeScene(0x5D, 0x1A);
    scene_.enableInput();
}

void Loc16::talkToDragon() {
    // Original: CGame_Wait auch ohne vorherigen Weg (nachts mit Flugerlaubnis)
    if (flag(kNight) == 0 || flag(kCanFly) == 0 || flag(kBeforeFirstFlight) != 0) {
        const int cb = beginWait();
        if (hero_) hero_->walkTo(0x11D, 0x140, cb, 6);
    }
    if (!wait()) return;
    if (flag(kVulturesGone) == 0) {
        // Tag: Gesprächsstufen 1…11 (0x3FAC)
        const int t = flag(kDayTalk);
        if (t == 1) {
            scene_.disableInput();
            heroSay("058_ROD_07");
            if (hero_) hero_->hide(scene_.clicksEnabled);
            playRange(rodAttack_, 11, 0, 4);
            playSfx(woosh_.get());
            playRange(rodAttack_, 11, 5, 10);
            dragonSay("058_DRA_07");
            playRange(rodAttack_, 10, 0x0B, -1);
            if (hero_) hero_->show(scene_.clicksEnabled);
            flag(kDayTalk) = 2;
            scene_.enableInput();
            return;
        }
        if (t == 9) {
            scene_.disableInput();
            pose(3, 0);
            heroSay("058_ROD_15");
            pose(4, 0);
            pose(4, 4);
            {
                const int cb = beginWait();
                speak("Loc16", "058_DRA_15", dragon_, 0x0C, 0x6E, 0x76, cb);
                wait();
            }
            pose(3, 0);
            flag(kTo59Open) = 1;
            blockSky(false);
            walk(500, 0xDA, 4);
            scene_.enableInput();
            changeScene(0x3B, 0x11);
        } else {
            exchangeN(t + 6);
        }
        // Original: zählt auch nach dem Szenenwechsel (Stufe 9) weiter
        if (flag(kDayTalk) < 0x0B) ++flag(kDayTalk);
        return;
    }
    if (flag(kCanFly) != 0) {
        // vor dem Abflug (0x3FB4)
        switch (flag(kFlyTalk)) {
        case 1:
        case 2:
        case 3:
            exchangeN(flag(kFlyTalk) + 0x18);
            break;
        case 4:
            exchange("058_ROD_28", "058_DRA_28");
            heroSay("058_ROD_29");
            break;
        case 5:
            scene_.disableInput();
            walk(0x1AB, 0x11E, 4);
            dragonSay("058_DRA_30");
            exchange("058_ROD_30", "058_DRA_31");
            ++flag(kFlyTalk);
            takeOff();
            scene_.enableInput();
            return;
        case 6:
            if (hero_ && std::abs(hero_->footX - 0x1AB) < 10 && std::abs(hero_->footY - 0x11E) < 10) {
                takeOff();
                return;
            }
            exchange("058_ROD_32", "058_DRA_32");
            walk(0x1AB, 0x11E, 4);
            return;
        default:
            return;
        }
        ++flag(kFlyTalk);
        return;
    }
    if (flag(kNight) == 0) {
        exchange("058_ROD_18", "058_DRA_18");
        return;
    }
    // Nacht: Gesprächsstufen 1…6 (0x3FB0)
    const int t = flag(kNightTalk);
    if (t == 1) {
        pose(3, 0);
        heroSay("058_ROD_19");
        pose(4, 0);
        pose(4, 5);
        {
            const int cb = beginWait();
            speak("Loc16", "058_DRA_19", dragon_, 0x0C, 0x8B, 0x94, cb);
            wait();
        }
        pose(3, 0);
        pose(6, 0);
    } else if (t == 4) {
        scene_.disableInput();
        {
            const int cb = beginWait();
            if (hero_) hero_->turn(5, cb);  // CActor_Turn
            wait();
        }
        if (hero_) hero_->hide(scene_.clicksEnabled);
        {
            const int cb = beginWait();
            if (rodLooksSky_) rodLooksSky_->playEndEvent(10, cb);
            wait();
        }
        if (hero_) hero_->show(scene_.clicksEnabled);
        heroSay("058_ROD_22");
        walk(0x160, 0x155, 5);
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playRange(rodSitDown_, 11, 0, 10);
        dragonSay("058_DRA_22");
        playRange(rodSitDown_, 10, 0x0B, -1);
        if (hero_) hero_->show(scene_.clicksEnabled);
        scene_.enableInput();
    } else if (t == 6) {
        exchange("058_ROD_24", "058_DRA_24");
        if (flag(k3F74) == 1) flag(k3F74) = 2;
    } else {
        exchangeN(t + 0x12);
    }
    if (flag(kNightTalk) < 6) ++flag(kNightTalk);
}

void Loc16::onEvent(Event& e, int) {
    switch (e.param) {
    case 1:  // → Schlucht 15/57
        if (walk(0x10, 300, 2)) changeScene(0x39, 0x0F);
        return;
    case 2:  // → 59 (Loc17)
        if (flag(kVulturesGone) != 0) {
            dragonSay("058_DRA_34");
            heroSay("058_ROD_35");
            return;
        }
        if (flag(kTo59Open) == 0) {
            // der Drache lässt Rodrigo nicht vorbei
            pose(3, 0);
            if (!walk(0x201, 0x11B, 2)) return;
            pose(4, 0);
            pose(4, 3);
            {
                const int cb = beginWait();
                speak("Loc16", "058_DRA_33", dragon_, 0x0C, 0x49, 0x4D, cb);
                wait();
            }
            if (hero_) hero_->walkTo(0x1CA, 0x138, 0, 3);  // CActor_WalkTo2
            pose(3, 0);
            pose(6, 0);
            return;
        }
        if (walk(500, 0xDA, 4)) changeScene(0x3B, 0x11);
        return;
    case 3:
        talkToDragon();
        return;
    case 4:  // Hut (Gegenstand 4)
    case 5: {  // Hand (Gegenstand 3)
        const bool hat = e.param == 4;
        if (!walk(hat ? 0x115 : 0x137, hat ? 0x15E : 0x15B, 4)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        if (rodPickUp_) {
            if (hat) rodPickUp_->setPos(0xF8, 0xF6);  // vtable +0x14
            else rodPickUp_->setPos(0x11A, 0xF3);
        }
        playRange(rodPickUp_, 11, 0, 9);
        if (hat) {
            if (hat_) hat_->disable();
        } else if (hand_) {
            hand_->disable();
        }
        playRange(rodPickUp_, 10, 10, -1);
        flag(hat ? kHat : kHand) = 0;
        if (auto* bar = scene_.inventory()) bar->add(hat ? 4 : 3);
        if (hero_) hero_->show(scene_.clicksEnabled);
        return;
    }
    case kPoseEvent:
        pose(5, 0);
        return;
    case kAmbient: {
        AudioBuffer* b = nullptr;
        if (flag(kNight) != 0) {
            const int r = random(1, 4);
            b = (r == 1 || r == 2) ? owl_.get() : r == 3 ? thunder_.get() : r == 4 ? thunder2_.get() : nullptr;
        } else {
            const int r = random(1, 4);
            AudioBuffer* birds[] = {bird_.get(), bird2_.get(), bird3_.get(), chirp_.get()};
            if (r >= 1 && r <= 4) {
                b = birds[r - 1];
                const int v = random(0x32, 100);
                if (b) b->setVolume(v);  // CAudioBuffer_SetVolume
            }
        }
        if (b) playSfxRandom3D(*b);
        setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(5, 10) * 1000), 0, kAmbient);
        return;
    }
    default:
        return;
    }
}

}  // namespace rah
