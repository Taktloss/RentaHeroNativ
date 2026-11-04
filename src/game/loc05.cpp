// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc05.hpp"

#include <cstdio>

#include "engine/area.hpp"

namespace rah {

namespace {
constexpr std::size_t kRamil = 0x3E84;        // Stufe 3 → 1: Ramil kommt zu Rodrigo (Loc04)
constexpr std::size_t kStrangerInPub = 0x3EA0;  // Stufe 3 → 1: der Fremde sitzt in der Kneipe (Loc02, Ladebit 1; Loc02 setzt 0)
constexpr std::size_t kLouiWalks = 0x3EC8;    // Ladebit 1 (S10_LouiWalk) und Gang-Gespräch frei
constexpr std::size_t kGreet = 0x3ED8;        // Begrüßungsgespräch beim nächsten Betreten
constexpr std::size_t kTalk = 0x3EDC;         // Gesprächsstufe mit Loui
constexpr std::size_t kFlowers = 0x3EE0;      // Blumen: 1 → ROD_05, dann 2 → ROD_06
constexpr std::size_t kWalkDone = 0x3EE4;     // Gang durchs Büro gesehen
constexpr int kPoseEvent = 20;
}  // namespace

Loc05::Loc05(GameContext& ctx, int scene, int prevScene) : Game(ctx, 5, scene, prevScene) {
    getViewObjects();
    const bool walk = flag(kLouiWalks) != 0;
    scene_.load("S10", walk ? 1u : 0u);
    getViewObjects();
    doorOpen_ = dynamic_cast<AnimLayer*>(scene_.getObject("S10_DoorOpen"));
    louiTalk_ = dynamic_cast<AnimLayer*>(scene_.getObject("S10_LouiTalk"));
    if (walk) {
        stepLeft_ = loadSfx("Gen_StepLeft");
        stepRight_ = loadSfx("Gen_StepRight");
        splash_ = loadSfxVol("Sfx_WaterSplash", 65);
        tock_ = loadSfxVol("Sfx_Tock", 50);
        louiWalk_ = dynamic_cast<AnimLayer*>(scene_.getObject("S10_LouiWalk"));
        if (louiWalk_) {
            // orig: CAnimLayer_AddCue(bild, puffer) 0x00442BB0
            const std::pair<int, AudioBuffer*> cues[] = {
                {42, stepLeft_.get()},  {47, stepRight_.get()}, {54, stepLeft_.get()},
                {58, stepRight_.get()}, {93, splash_.get()},    {112, splash_.get()},
                {129, tock_.get()},     {140, stepLeft_.get()}, {146, stepRight_.get()},
                {151, stepLeft_.get()}};
            for (const auto& [frame, buf] : cues) {
                AnimCue c;
                c.type = 1;
                c.frame = frame;
                c.buffer = buf;
                louiWalk_->addCue(c);
            }
        }
    }
    doorCreak_ = loadSfxVol("Sfx_Door_CreakShort", 50);
    poseTimer_ = std::make_unique<Event>();
    music_ = playMusic("Loc05_LouisOffice");
    if (music_) {
        music_->setReverb(4, 0.208f, 0.478f, 0.0f);  // 0x3E54FDF4, 0x3EF4BC6A, 0
        music_->setReverbMix(0.1f);                  // 0x3DCCCCCD
    }
}

Loc05::~Loc05() { shutdown(); }

void Loc05::destroyLocal() {
    poseTimer_.reset();  // FUN_0043F160 + Freigabe
    doorCreak_.reset();
    stepLeft_.reset();
    stepRight_.reset();
    splash_.reset();
    tock_.reset();
}

void Loc05::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // CSzene+0x5C0
    setEventProc(&Loc05::onEvent);
    if (restored_ == 0) {
        pose(3, 2);
        int cb = beginWait();
        if (doorOpen_) doorOpen_->playRange(11, cb, 0, 4);
        wait();
        if (doorCreak_) doorCreak_->play();
        cb = beginWait();
        if (doorOpen_) doorOpen_->playRange(11, cb, 5, -1);
        wait();
        if (hero_) {
            hero_->setPosDir(311, 218, 7);
            hero_->show(scene_.clicksEnabled);
        }
        if (flag(kGreet) == 0) {
            if (hero_) hero_->walkTo(419, 255, 0, 1);
        } else {
            flag(kGreet) = 0;
            stepToDesk();
            louiLine("010_LOU_01");
            exchange("010_ROD_01", "010_LOU_02");
            exchange("010_ROD_02", "010_LOU_03");
        }
    } else {
        if (louiTalk_) louiTalk_->enable();
        if (hero_) hero_->show(scene_.clicksEnabled);
    }
    pose(6, 0);
    const std::pair<const char*, int> areas[] = {
        {"S10_Exit", 1},        {"S10_Desk", 2},          {"S10_Flowers1", 3}, {"S10_Flowers2", 4},
        {"S10_Kitchen", 5},     {"S10_Loui", 6},          {"S10_Truhe", 7},    {"S10_KastenLinks", 10},
        {"S10_KastenRechts", 11}, {"S10_Ofen", 9},        {"S10_Krug", 8}};
    for (const auto& [name, ev] : areas)
        if (Area* a = scene_.getArea(name)) a->setEvent(ev);  // Port-Schutz, Original ohne Prüfung; alle Namen in den Daten vorhanden (tools/name_check.py)
    scene_.enableInput();
}

void Loc05::stepToDesk() {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(283, 336, cb, 1);
    wait();
}

void Loc05::louiLine(std::string_view key) {
    pose(4, 2);
    const int cb = beginWait();
    speak("Loc05", key, louiTalk_, 0x0C, 15, 23, cb);
    wait();
    pose(6, 0);
}

void Loc05::exchange(std::string_view rod, std::string_view lou) {
    pose(3, 2);
    heroSay(rod);  // CGame_HeroSayWait
    pose(4, 2);
    const int cb = beginWait();
    speak("Loc05", lou, louiTalk_, 0x0C, 15, 23, cb);
    wait();
    pose(6, 0);
}

void Loc05::walkExchange(std::string_view rod, std::string_view lou) {
    heroSay(rod);
    const int cb = beginWait();
    speak("Loc05", lou, louiWalk_, 0x0C, 96, 122, cb);
    wait();
}

void Loc05::pose(int mode, int arg) {
    AnimLayer* L = louiTalk_;
    if (!L) return;  // Port-Schutz, Original ohne Prüfung; alle Namen in den Daten vorhanden (tools/name_check.py)
    int trans = 0;
    if (mode == 5) {
        if (L->running) {
            setTimerHandler(*poseTimer_, static_cast<std::uint32_t>(random(30, 60) * 100), 0, kPoseEvent);
            return;
        }
    } else if (mode == 6) {
        setTimerHandler(*poseTimer_, static_cast<std::uint32_t>(random(10, 20) * 100), 0, kPoseEvent);
        return;
    }
    // Übergänge: 1 (0→2), 2 (2→0), 3 (0→1), 4 (1→0), 5 (2→1), 6 (1→2), 7 (2→2 Geste)
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
        if (L->userState == 0) trans = arg == 1 ? 3 : arg == 2 ? 1 : 0;
        else if (L->userState == 1) trans = arg == 0 ? 4 : arg == 2 ? 6 : 0;
        else if (L->userState == 2) trans = arg == 0 ? 2 : arg == 1 ? 5 : 0;
        break;
    case 5:
        if (L->userState == 0) {
            const int r = random(1, 4);
            trans = r == 1 ? 1 : (r > 1 && r < 5) ? 3 : 0;
        } else if (L->userState == 1) {
            const int r = random(1, 4);
            trans = r == 1 ? 6 : (r > 1 && r < 5) ? 4 : 0;
        } else if (L->userState == 2) {
            const int r = random(1, 3);
            trans = r == 1 ? 2 : r == 2 ? 5 : r == 3 ? 7 : 0;
        }
        break;
    default: break;
    }
    // laufenden Übergang abwarten (Klickzustand +0x514 bleibt erhalten)
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
    static constexpr int kFrom[] = {0, 1, 15, 1, 4, 15, 3, 69};
    static constexpr int kTo[] = {0, 15, 1, 3, 1, 3, 15, 122};
    static constexpr int kPose[] = {0, 2, 0, 1, 0, 1, 2, 2};
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

void Loc05::onEvent(Event& e, int) {
    auto& st = ctx_.state;
    switch (e.param) {
    case 1: {
        const int cb = beginWait();
        if (hero_) hero_->walkTo(311, 218, cb, 2);
        if (wait()) changeScene(9, 1);
        return;
    }
    case 2: walkAndComment(251, 321, 2, "010_ROD_04"); return;
    case 3:
    case 4: {
        const bool left = e.param == 3;
        const int x = left ? 342 : 472, y = left ? 273 : 328, dir = left ? 3 : 0;
        if (flag(kFlowers) == 1) {
            walkAndComment(x, y, dir, "010_ROD_05");
            flag(kFlowers) = 2;
        } else if (flag(kFlowers) == 2) {
            walkAndComment(x, y, dir, "010_ROD_06");
        }
        return;
    }
    case 5: walkAndComment(452, 240, 4, "010_ROD_07"); return;
    case 6:
        stepToDesk();
        switch (st.at(kTalk)) {
        case 1:
            st.at(kTalk) = 2;
            exchange("028_ROD_01", "028_LOU_01");
            return;
        case 2:
            st.at(kTalk) = 3;
            exchange("028_ROD_02", "028_LOU_02");
            return;
        case 3:
            st.at(kTalk) = 4;
            exchange("028_ROD_03", "028_LOU_03");
            flag(kRamil) = 1;
            flag(kStrangerInPub) = 1;
            return;
        case 4:
            st.at(kTalk) = 5;
            exchange("028_ROD_04", "028_LOU_04");
            return;
        case 5: {
            if (st.at(kLouiWalks) != 0 && st.at(kWalkDone) == 0) {
                st.at(kTalk) = 6;
                exchange("028_ROD_08", "028_LOU_08");
                return;
            }
            const int r = random(1, 3);
            if (r == 1) exchange("028_ROD_05", "028_LOU_05");
            else if (r == 2) exchange("028_ROD_06", "028_LOU_06");
            else if (r == 3) exchange("028_ROD_07", "028_LOU_07");
            return;
        }
        case 6:
            st.at(kTalk) = 7;
            exchange("028_ROD_09", "028_LOU_09");
            return;
        case 7:
            st.at(kTalk) = 8;
            exchange("028_ROD_10", "028_LOU_10");
            return;
        case 8:
            if (st.at(kRamil) == 2) {
                exchange("028_ROD_11", "028_LOU_11");
                flag(kTalk) = 9;
                return;
            }
            st.at(kTalk) = 9;
            [[fallthrough]];
        case 9: {
            // Loui geht durchs Büro (S10_LouiWalk)
            scene_.disableInput();
            Area* louiArea = scene_.getArea("S10_Loui");
            if (louiArea) louiArea->active = false;
            exchange("028_ROD_12", "028_LOU_12");
            sayActor("Loc05", "029_ROD_01", hero_, 0);
            pose(4, 2);
            if (louiTalk_) louiTalk_->disable();
            if (louiWalk_) louiWalk_->enable();
            const auto play = [this](unsigned m, int from, int to) {
                const int cb = beginWait();
                if (louiWalk_) louiWalk_->playRange(m, cb, from, to);
                wait();
            };
            play(11, 1, 43);
            if (hero_) hero_->turnTo(0);
            play(11, 44, 60);
            if (hero_) hero_->turnTo(7);
            play(11, 61, 96);
            const int cb = beginWait();
            speak("Loc05", "029_LOU_01", louiWalk_, 0x0C, 96, 122, cb);
            wait();
            walkExchange("029_ROD_02", "029_LOU_02");
            walkExchange("029_ROD_03", "029_LOU_03");
            walkExchange("029_ROD_04", "029_LOU_04");
            play(10, 124, 140);
            if (hero_) hero_->turnTo(0);
            play(10, 141, 150);
            if (hero_) hero_->turnTo(1);
            play(10, 151, 174);
            if (louiTalk_) {
                louiTalk_->enable();
                louiTalk_->showFrame(1);
                louiTalk_->userState = 0;
            }
            pose(6, 0);
            flag(kWalkDone) = 1;
            flag(kTalk) = 5;
            if (louiArea) louiArea->active = true;
            scene_.enableInput();
            return;
        }
        default: return;
        }
    case 7: walkAndComment(457, 287, 6, "010_ROD_08"); return;
    case 8: walkAndComment(370, 234, 3, "010_ROD_12"); return;
    case 9: walkAndComment(432, 240, 4, "010_ROD_11"); return;
    case 10: walkAndComment(382, 239, 4, "010_ROD_09"); return;
    case 11: walkAndComment(463, 241, 5, "010_ROD_10"); return;
    case kPoseEvent: pose(5, 0); return;
    default:
        // Abweichung: Original zeigt MessageBoxA("Unknown_Event____", "Rent_A_Hero") (modal)
        std::fprintf(stderr, "Loc05: Unknown_Event____ (%d)\n", e.param);
        return;
    }
}

}  // namespace rah
