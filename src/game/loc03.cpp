// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc03.hpp"

#include "engine/area.hpp"

namespace rah {

namespace {
constexpr std::size_t kSabMet = 0x3ECC;     // Säbelfrau aufgetreten, Wachen weg
constexpr std::size_t kLouiWalks = 0x3EC8;  // Piratenschiff gesehen (Loc05: Ladebit 1)
constexpr int kGuardBlock[] = {407, 296, 538, 341};  // CActor_BlockRect (0x197, 0x128, 0x21A, 0x155)
// orig: DAT_0047D774 – zuletzt gesprochener Wachen-Spruch (global, .bss = 0)
int g_lastGuardLine = 0;
}  // namespace

Loc03::Loc03(GameContext& ctx, int scene, int prevScene) : Game(ctx, 3, scene, prevScene) {
    getViewObjects();
    if (prevScene_ == 15) pirateShip_ = loadAnim("S12_PirateShip");
    music_ = playMusic(prevScene_ == 15 ? "Loc09_SmashvilleAttack" : "Loc01_Smashville");
    if (music_) {
        music_->setReverb(16, 0.111f, 2.767f, 0.224f);  // 0x3DE353F8, 0x40311687, 0x3E656042
        music_->setReverbMix(0.1f);                     // 0x3DCCCCCD
    }
    gliderPass_ = loadSfx("Sfx_Glider_PassingBy");
    unsigned mask = 0;
    if (flag(kSabMet) == 0) {
        mask = 1;
        guardTimer_ = std::make_unique<Event>();
    }
    scene_.load("S12", mask);
    getViewObjects();
    takeOff_ = loadAnim("S12_Wegflug");
    approach_ = obj("S12_Anflug");
    rodGlider_ = scene_.getObject("S12_RodGlider");
    if (mask != 0) {
        rab1Run_ = obj("S12_Rab1Run");
        rab1Talk_ = obj("S12_Rab1Talk");
        rab2_ = obj("S12_Rab2");
        sabInGlider_ = obj("S12_SabInGlider");
        sabSword_ = obj("S12_SabSword");
        sabGlider_ = scene_.getObject("S12_SabGlider");
        sabWalk_ = obj("SabWalk");
        ra2Line_ = loadSfx("012_RA2_04");
        stepLeft_ = loadSfx("Gen_StepLeft");
        stepRight_ = loadSfx("Gen_StepRight");
    }
    ambientTimer_ = std::make_unique<Event>();
    bird_ = loadSfx("Sfx_Bird");
    bird3_ = loadSfx("Sfx_Bird3");
    gliderPass3_ = loadSfx("Sfx_Glider_PassingBy3");
    wind_ = loadSfx("Sfx_Wind2");
    hack_ = loadSfxVol("Sfx_Work_Hack2", 70);
    if (flag(kSabMet) == 0) {
        swordPush_ = loadSfx("Sfx_SwordPushShort");
        swordPull_ = loadSfx("Sfx_SwordPull");
    }
    bridge_ = scene_.getArea("S12_Bridge");
    sign_ = scene_.getArea("S12_Schild");
    to13_ = scene_.getArea("S12_To13");
    plant_ = scene_.getArea("S12_Pflanze");
}

Loc03::~Loc03() { shutdown(); }

void Loc03::destroyLocal() {
    guardTimer_.reset();
    ambientTimer_.reset();
    pirateShip_.reset();
    takeOff_.reset();
    ra2Line_.reset();
    gliderPass_.reset();
    bird_.reset();
    bird3_.reset();
    gliderPass3_.reset();
    wind_.reset();
    hack_.reset();
    swordPull_.reset();
    swordPush_.reset();
    // Original-Fehler: Schrittklänge +0xC8/+0xCC werden nie gelöscht
    (void)stepLeft_.release();
    (void)stepRight_.release();
}

bool Loc03::playWait(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

void Loc03::guardAreas(bool on) {
    for (AnimLayer* L : {rab1Talk_, rab2_, sabSword_})
        if (L && L->area) L->area->active = on;
}

void Loc03::restartGuardTimer() {
    if (guardTimer_ && flag(kSabMet) == 0)
        setTimerHandler(*guardTimer_, static_cast<std::uint32_t>(random(2, 5) * 1000), 0, 10);
}

void Loc03::stopGuardTimer() {
    if (!guardTimer_) return;
    guardTimer_->enabled = false;
    guardTimer_->pending = false;
}

void Loc03::start() {
    setEventProc(&Loc03::onEvent);
    scene_.disableAll();
    if (Layer* bg = scene_.background()) bg->enable();  // +0x60
    if (flag(kSabMet) == 0 && prevScene_ != 15) {
        scene_.clearFlag510();
        for (Layer* L : std::initializer_list<Layer*>{rab1Talk_, rab2_, sabSword_, sabGlider_})
            if (L) L->enable();
        const std::pair<Layer*, int> ev[] = {{sabSword_, 6}, {sabGlider_, 7}, {rab1Talk_, 8}, {rab2_, 9}};
        for (const auto& [L, id] : ev)
            if (L && L->area) L->area->setEvent(id);  // UNVERIFIED: Original ohne Prüfung
        if (hero_) hero_->blockRect(kGuardBlock[0], kGuardBlock[1], kGuardBlock[2], kGuardBlock[3]);
        if (guardTimer_) setTimerHandler(*guardTimer_, static_cast<std::uint32_t>(random(4, 9) * 333), 0, 10);
    }
    if (restored_ == 0) {
        if (prevScene_ == 8) {
            // Anflug mit dem Gleiter
            if (music_) music_->fadeIn(80, 2000);
            if (gliderPass_) {
                gliderPass_->setFreq(90);
                gliderPass_->play();
                gliderPass_->fadeFreq(35, 2000);
            }
            const int cb = beginWait();
            if (approach_) approach_->playEndEvent(10, cb);
            if (!wait()) return;
            if (rodGlider_) rodGlider_->enable();
            if (hero_) {
                hero_->setPosDir(635, 334, 2);
                hero_->walkToClick(581, 311);
            }
        } else if (prevScene_ == 13) {
            if (music_) music_->fadeIn(80, 2000);
            if (rodGlider_) rodGlider_->enable();
            if (hero_) {
                hero_->setPosDir(634, 270, 2);
                hero_->walkToClick(568, 280);
            }
        } else if (prevScene_ == 15) {
            // Piratenangriff
            if (music_) music_->play();  // vtable +0x08
            if (pirateShip_) {
                scene_.setModal(pirateShip_.get());
                if (!playWait(pirateShip_.get(), 11, 0, 58)) return;
                speak("Loc03", "012_GA1_01", 0);  // CGame_SayNoActor
                if (!playWait(pirateShip_.get(), 11, 59, 94)) return;
                speak("Loc03", "012_GA2_01", 0);
                if (!playWait(pirateShip_.get(), 10, 95, -1)) return;
                flag(kLouiWalks) = 1;
            }
            music_ = playMusic("Loc01_Smashville");
            if (music_) {
                music_->setReverbMix(0.1f);
                music_->fadeIn(80, 2000);
            }
            if (rodGlider_) rodGlider_->enable();
            if (hero_) {
                hero_->setPosDir(238, 294, 6);
                hero_->walkToClick(315, 348);
            }
        }
    } else {
        if (music_) music_->fadeIn(80, 2000);
        if (hero_) hero_->show(scene_.clicksEnabled);
        if (rodGlider_) rodGlider_->enable();
    }
    if (rodGlider_ && rodGlider_->area) rodGlider_->area->setEvent(5);
    const std::pair<Area*, int> areas[] = {{bridge_, 1}, {sign_, 2}, {to13_, 3}, {plant_, 4}};
    for (const auto& [a, id] : areas)
        if (a) a->setEvent(id);
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(3, 6) * 1000), 0, 14);
    scene_.enableInput();
}

void Loc03::confrontGuards() {
    stopGuardTimer();
    int cb = beginWait();
    if (hero_) hero_->walkTo(497, 347, cb, 3);
    if (!wait()) return;
    scene_.disableInput();
    if (hero_) hero_->unblockRect(kGuardBlock[0], kGuardBlock[1], kGuardBlock[2], kGuardBlock[3]);
    const auto line = [this](const char* key, AnimLayer* who) {
        const int c = beginWait();
        speak("Loc03", key, who, 0x0C, 0, 4, c);
        wait();
    };
    cb = beginWait();
    sayActor("Loc03", "012_ROD_06", hero_, cb);
    wait();
    line("012_RA1_03", rab1Talk_);
    line("012_RA2_03", rab2_);
    line("012_RA1_04", rab1Talk_);
    if (ra2Line_) ra2Line_->play();
    if (rab2_) rab2_->playRange(13, 0, 0, 4);
    line("012_RA1_05", rab1Talk_);
    if (rab2_) {
        rab2_->stop(0);  // vtable +0x24
        rab2_->showFrame(0);
    }
    line("012_RA1_06", rab1Talk_);
    line("012_RA2_05", rab2_);
    // Die Wachen laufen davon
    if (rab1Talk_) rab1Talk_->disable();
    const auto cue = [](AnimLayer* L, int frame, AudioBuffer* b) {
        if (!L) return;
        AnimCue c;
        c.type = 1;
        c.frame = frame;
        c.buffer = b;
        L->addCue(c);  // 0x00442BB0
    };
    cue(rab1Run_, 4, stepRight_.get());
    cue(rab1Run_, 6, stepLeft_.get());
    cue(rab2_, 8, stepLeft_.get());
    cue(rab2_, 11, stepRight_.get());
    if (!playWait(rab1Run_, 11, 0, 3)) return;
    if (rab1Run_) {
        rab1Run_->playRangeNoEvent(13, 4, 8);
        rab1Run_->moveTo(625.0, 215.0, 0.0, 0.0, 15, 0);
    }
    if (!playWait(rab2_, 11, 5, 7)) return;
    if (hero_) hero_->walkTo(478, 338, 0, 5);
    if (rab2_) rab2_->playRange(13, 0, 8, 14);
    cb = beginWait();
    if (rab2_) rab2_->moveTo(615.0, 240.0, 0.0, 0.0, 15, cb);
    if (!wait()) return;
    for (AnimLayer* L : {rab1Run_, rab2_}) {
        if (!L) continue;
        L->disable();
        L->clearCues();
    }
    cb = beginWait();
    sayActor("Loc03", "012_ROD_07", hero_, cb);
    wait();
    // Auftritt der Säbelfrau
    sayActor("Loc03", "012_SAB_01", nullptr, 0);
    if (!playWait(sabSword_, 11, 0, 8)) return;
    if (swordPull_) swordPull_->play();
    if (!playWait(sabSword_, 11, 9, 30)) return;
    if (!playWait(sabSword_, 11, 29, 18)) return;
    if (swordPush_) swordPush_->play();
    if (!playWait(sabSword_, 11, 17, 0)) return;
    if (sabSword_) sabSword_->disable();
    if (sabWalk_) {
        cue(sabWalk_, 49, stepLeft_.get());
        cue(sabWalk_, 55, stepRight_.get());
        cue(sabWalk_, 61, stepLeft_.get());
        cue(sabWalk_, 67, stepRight_.get());
        sabWalk_->enable();
        sabWalk_->playRange(13, 0, 48, 59);
        sabWalk_->moveTo(411.0, 177.0, 164.0, 52.0, 52, 12);
    }
    scene_.enableInput();
    cb = beginWait();
    if (hero_) hero_->turn(0, cb);
    wait();
    sayActor("Loc03", "012_ROD_08", hero_, 0);
    flag(kSabMet) = 1;
}

void Loc03::onEvent(Event& e, int) {
    switch (e.param) {
    case 1:
        stopGuardTimer();
        walkAndComment(129, 291, 3, "012_ROD_03");
        restartGuardTimer();
        return;
    case 2:
        stopGuardTimer();
        walkAndComment(505, 279, 4, "012_ROD_01");
        restartGuardTimer();
        return;
    case 3: {
        const int cb = beginWait();
        if (hero_) hero_->walkTo(634, 270, cb, 6);
        if (wait()) {
            stopGuardTimer();
            changeScene(13, 2);
        }
        return;
    }
    case 4:
        stopGuardTimer();
        walkAndComment(554, 291, 4, "012_ROD_02");
        restartGuardTimer();
        return;
    case 5: {
        // Abflug mit dem Gleiter
        stopGuardTimer();
        const int cb = beginWait();
        if (hero_) hero_->walkTo(635, 334, cb, 6);
        if (!wait()) return;
        scene_.disableInput();
        if (rodGlider_) rodGlider_->disable();
        scene_.setModal(takeOff_.get());
        if (!playWait(takeOff_.get(), 11, 0, 30)) return;
        if (gliderPass_) {
            gliderPass_->setFreq(20);
            gliderPass_->play();
            gliderPass_->fadeFreq(30, 1500);
        }
        if (!playWait(takeOff_.get(), 11, 31, 68)) return;
        if (gliderPass_) gliderPass_->fadeFreq(60, 1000);
        if (!playWait(takeOff_.get(), 11, 69, -1)) return;
        changeScene(7, 1);
        return;
    }
    case 6:
    case 8:
    case 9: confrontGuards(); return;
    case 7: {
        stopGuardTimer();
        const int r = random(1, 2);
        if (r == 1) walkAndComment(415, 264, 4, "012_ROD_04");
        else if (r == 2) walkAndComment(415, 264, 4, "012_ROD_05");
        restartGuardTimer();
        return;
    }
    case 10: {
        // Wachen reden miteinander; nie zweimal derselbe Spruch
        int r = random(1, 4);
        while (r == g_lastGuardLine) r = random(1, 4);
        g_lastGuardLine = r;
        switch (r) {
        case 1: speak("Loc03", "012_RA1_01", rab1Talk_, 0x0C, 0, 4, 11); break;
        case 2:
            speak("Loc03", "012_RA2_01", rab2_, 0x0C, 0, 4, 11);
            if (rab1Talk_) rab1Talk_->playRangeNoEvent(12, 5, -1);
            break;
        case 3: speak("Loc03", "012_RA1_02", rab1Talk_, 0x0C, 0, 4, 11); break;
        case 4: speak("Loc03", "012_RA2_02", rab2_, 0x0C, 0, 4, 11); break;
        default: break;
        }
        guardAreas(false);
        return;
    }
    case 11:
        guardAreas(true);
        restartGuardTimer();
        return;
    case 12:
        // Säbelfrau steigt in ihren Gleiter
        if (sabWalk_) {
            sabWalk_->disable();
            sabWalk_->clearCues();
        }
        if (sabGlider_) sabGlider_->disable();
        if (sabInGlider_) sabInGlider_->enable();
        if (gliderPass_) {
            gliderPass_->setFreq(20);
            gliderPass_->play();
        }
        if (sabInGlider_) sabInGlider_->playRange(11, 13, 0, 57);
        return;
    case 13:
        if (gliderPass_) gliderPass_->fadeFreq(50, 2000);
        if (sabInGlider_) sabInGlider_->playRange(10, 0, 58, -1);
        return;
    case 14: {
        AudioBuffer* b = nullptr;
        switch (random(1, 9)) {
        case 1: b = bird_.get(); break;
        case 2: b = bird3_.get(); break;
        case 3:
        case 4: b = gliderPass3_.get(); break;
        case 5:
        case 6:
        case 7:
            if (wind_) playSfxRandom3D(*wind_);
            break;
        case 8:
        case 9: b = hack_.get(); break;
        default: break;
        }
        if (b) {
            b->setVolume(random(50, 100));
            playSfxRandom3D(*b);
        }
        setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(3, 6) * 1000), 0, 14);
        return;
    }
    default: return;
    }
}

}  // namespace rah
