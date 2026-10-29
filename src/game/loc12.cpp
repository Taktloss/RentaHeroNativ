// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc12.hpp"

#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
constexpr std::size_t kWell = 0x3F74;         // Brunnen-Stufe (1 leer, 2 Geldsack fehlt, 3 bezahlt, 4)
constexpr std::size_t kWellTalk = 0x3F78;     // Gespräch mit dem Brunnen nach dem Bezahlen (1…4)
constexpr std::size_t kNoMoneyBag = 0x3FA0;   // Stufe 2 ohne Gegenstand 7 gesehen
constexpr std::size_t kWellAdvice = 0x3FBC;   // Brunnen-Rat vollständig gehört
constexpr std::size_t kUnlock = 0x409C;       // UNVERIFIED: Bedeutung; 0 → Brunnen-Stufe 4
constexpr std::size_t kBush42 = 0x3F60;       // Loc10: Busch in Szene 42 weg
constexpr int kAmbient = 6;
}  // namespace

Loc12::Loc12(GameContext& ctx, int scene, int prevScene) : Game(ctx, 12, 91, prevScene) {
    (void)scene;  // Original setzt +0x2C fest auf 0x5B
    scene_.load("S91", 0xFFFF);  // CSzene_LoadAll(DAT_00473384)
    getViewObjects();
    owl_ = loadSfx("Sfx_Owl");
    thunder_ = loadSfx("Sfx_Thunder");
    thunder2_ = loadSfx("Sfx_Thunder2");
    moneyBag_ = loadSfx("Sfx_MoneyBag");
    bubble_ = loadSfx("Sfx_Bubble");
    if (prevScene_ == 90) scene_.replaceBackground("S91_Back2");  // 0x0043FD90(…, 1)
    inCam_ = loadAnim("S91_RodReinCam");
    outCam_ = loadAnim("S91_RodRausCam");
    // UNVERIFIED: das Original prüft weder auf fehlende noch auf statische Ebenen
    well_ = scene_.getObject("S91_Brunnen");
    shake1_ = obj("S91_RodRuettel1");
    shake2_ = obj("S91_RodRuettel2");
    wellIn_ = obj("S91_RodBrunnenHin");
    wellOut_ = obj("S91_RodBrunnenWeg");
    ambientTimer_ = std::make_unique<Event>();
    music_ = playMusic("Loc10_ForestInDarkness");
    if (music_) {
        music_->setReverb(4, 0.208f, 0.478f, 0.0f);  // 0x3E54FDF4, 0x3EF4BC6A, 0
        music_->setReverbMix(0.1f);                  // 0x3DCCCCCD
    }
}

void Loc12::destroyLocal() {
    ambientTimer_.reset();  // 0x0043F160 (austragen) + Löschen
    inCam_.reset();
    outCam_.reset();
    for (auto* b : {&owl_, &thunder_, &thunder2_, &moneyBag_, &bubble_}) b->reset();
}

bool Loc12::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo
    return wait();
}

bool Loc12::playEnd(AnimLayer* a, unsigned mode) {
    const int cb = beginWait();
    if (a) a->playEndEvent(mode, cb);
    return wait();
}

void Loc12::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // CSzene+0x5C0
    setEventProc(&Loc12::onEvent);
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(5, 15) * 1000), 0, kAmbient);
    if (well_) well_->enable();
    if (Layer* top = scene_.getObject("S91_Top")) top->enable();
    if (auto* heaven = dynamic_cast<AnimLayer*>(scene_.getObject("S91_Heaven"))) heaven->start(13);
    for (const auto& [n, ev] : {std::pair{"S91_Door1", 1}, {"S91_Door2", 2}, {"S91_CynDoor", 3}, {"S91_Wald", 5}})
        if (Area* a = scene_.getArea(n)) a->setEvent(ev);
    if (well_ && well_->area) well_->area->setEvent(4);
    if (flag(kUnlock) == 0) flag(kWell) = 4;
    if (restored_ == 0) {
        if (prevScene_ == 42) {
            if (hero_) {
                hero_->setPosDir(4, 336, 6);
                hero_->walkTo(60, 336, 0, 7);
            }
        } else {
            // Rodrigo kommt aus Cynthias Haus
            scene_.setModal(outCam_.get());
            playEnd(outCam_.get(), 10);
            if (hero_) {
                hero_->setPosDir(353, 287, 0);
                hero_->walkTo(345, 310, 0, 1);
            }
        }
    } else if (hero_) {
        hero_->show(scene_.clicksEnabled);
    }
    if (flag(kWell) == 2)
        if (auto* bar = scene_.inventory()) bar->setItemEvent(7, 7);
    scene_.enableInput();
}

void Loc12::wellDone() {
    if (hero_) hero_->show(scene_.clicksEnabled);
    if (well_) {
        well_->enable();
        if (well_->area) well_->area->active = true;
    }
}

void Loc12::lookIntoWell(const char* first, const char* second, std::uint32_t pause) {
    if (hero_) hero_->hide(scene_.clicksEnabled);
    if (well_) well_->disable();
    playEnd(wellIn_, 11);
    {
        const int cb = beginWait();
        speak("Loc12", first, cb);  // CGame_SayNoActor
        wait();
    }
    if (second) {
        const int cb = beginWait();
        speak("Loc12", second, cb);
        wait();
    } else {
        delay(pause);
    }
    if (wellIn_) wellIn_->disable();
    playEnd(wellOut_, 10);
}

void Loc12::onEvent(Event& e, int) {
    // input: Eingabe danach wieder freigeben (nur Stufe 1 sperrt sie vorher)
    const auto backFromWell = [this](const char* say, bool input) {
        wellDone();
        const int cb = beginWait();
        if (hero_) hero_->walkTo(365, 337, cb, 0);  // CActor_WalkTo2
        wait();
        if (input) scene_.enableInput();
        heroSay(say);
    };
    switch (e.param) {
    case 1:
        // an der ersten Tür rütteln
        if (!walk(246, 317, 4)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playEnd(shake1_, 10);
        if (hero_) hero_->show(scene_.clicksEnabled);
        walk(246, 320, 0);
        heroSay("091_ROD_02");
        return;
    case 2:
        if (!walk(454, 300, 6)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playEnd(shake2_, 10);
        if (hero_) hero_->show(scene_.clicksEnabled);
        walk(454, 305, 0);
        heroSay("091_ROD_03");
        return;
    case 3:
        // in Cynthias Haus
        if (!walk(353, 287, 4)) return;
        if (prevScene_ != 90) {
            scene_.setModal(inCam_.get());
            playEnd(inCam_.get(), 11);
        }
        changeScene(90, 14);
        return;
    case 4:
        switch (flag(kWell)) {
        case 1:
            if (!walk(369, 337, 0)) return;
            scene_.disableInput();
            lookIntoWell("091_ROD_04", "091_ROD_05", 0);
            backFromWell("091_ROD_06", true);
            return;
        case 2:
            if (!walk(369, 337, 0)) return;
            heroSay("091_ROD_07");
            if (auto* bar = scene_.inventory(); !bar || !bar->has(7)) flag(kNoMoneyBag) = 1;
            return;
        case 3: {
            static constexpr const char* kLines[][2] = {
                {"091_ROD_11", "091_ROD_12"}, {"091_ROD_13", "091_ROD_14"},
                {"091_ROD_15", "091_ROD_16"}, {"091_ROD_17", "091_ROD_18"}};
            const int stage = flag(kWellTalk);
            if (stage < 1 || stage > 4) return;
            if (!walk(369, 337, 0)) return;
            lookIntoWell(kLines[stage - 1][0], kLines[stage - 1][1], 0);
            if (stage == 4) {
                flag(kWellTalk) = 3;
                flag(kWellAdvice) = 1;
                backFromWell("091_ROD_20", false);
                return;
            }
            flag(kWellTalk) = stage + 1;
            wellDone();
            return;
        }
        case 4:
            if (!walk(369, 337, 0)) return;
            lookIntoWell("091_ROD_04", nullptr, 1500);  // CGame_Delay(1500) statt zweitem Satz
            backFromWell("091_ROD_19", false);
            return;
        default: return;
        }
    case 5:
        if (!walk(4, 336, 2)) return;
        flag(kBush42) = 1;
        changeScene(42, 10);
        return;
    case kAmbient: {
        AudioBuffer* b = nullptr;
        switch (random(1, 4)) {
        case 1:
        case 2: b = owl_.get(); break;
        case 3: b = thunder_.get(); break;
        case 4: b = thunder2_.get(); break;
        default: break;
        }
        if (b) playSfxRandom3D(*b);
        setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(5, 10) * 1000), 0, kAmbient);
        return;
    }
    case 7:
        // Geldsack (Gegenstand 7) in den Brunnen
        if (!walk(369, 337, 0)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        if (well_) well_->disable();
        playEnd(wellIn_, 11);
        if (moneyBag_) moneyBag_->play();
        if (auto* bar = scene_.inventory()) bar->remove(7);
        flag(kWell) = 3;
        delay(3000);
        if (bubble_) bubble_->play();
        {
            int cb = beginWait();
            speak("Loc12", "091_ROD_08", cb);
            wait();
            cb = beginWait();
            speak("Loc12", "091_ROD_09", cb);
            wait();
        }
        if (wellIn_) wellIn_->disable();
        playEnd(wellOut_, 10);
        wellDone();
        {
            const int cb = beginWait();
            if (hero_) hero_->walkTo(365, 337, cb, 0);
            wait();
        }
        heroSay("091_ROD_10");
        return;
    default: return;
    }
}

}  // namespace rah
