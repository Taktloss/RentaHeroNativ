// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc18.hpp"

#include <bit>

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
float f32(std::uint32_t bits) { return std::bit_cast<float>(bits); }
constexpr std::size_t kBrainTalk = 0x3FDC;  // Kommentar-Reihe am Gehirn (1→2→3→1)
constexpr std::size_t kAcid = 0x3FE0;       // Säure gestreut: Gehirn raucht, Kiefer lassen sich öffnen
constexpr std::size_t kFromSkull = 0x3FE4;  // Kiefer geöffnet → Loc17 Ankunft aus 61
constexpr std::size_t kFirstIn = 0x3FE8;    // erster Besuch: 061_ROD_01
constexpr int kItemAcid = 6;                // Gegenstand → Ereignis 4
constexpr int kItem8 = 8;                   // Gegenstand → Ereignis 5
constexpr int kScene60 = 0x3C;
}  // namespace

Loc18::Loc18(GameContext& ctx, int scene, int prevScene) : Game(ctx, 18, scene, prevScene) {
    getViewObjects();
    scene_.load("S61", 0);  // CSzene_Load(DAT_00473E4C, 0)
    getViewObjects();
    climbIn_ = loadAnim("S61_ClimbIn");
    climbOut_ = loadAnim("S61_ClimbOut");
    openJaws_ = loadAnim("S61_OpenJaws");
    strew_ = loadAnim("S61_Streu");
    smoke_ = dynamic_cast<AnimLayer*>(scene_.getObject("S61_Smoke"));
    smokeClone_ = dynamic_cast<AnimLayer*>(scene_.getObject("S61_SmokeClone"));
    mushroomL_ = scene_.getObject("S61_PilzLinks");
    mushroomR_ = scene_.getObject("S61_PilzRechts");
    to60_ = scene_.getArea("S61_To60");
    brain_ = scene_.getArea("S61_Brain");
    to60b_ = scene_.getArea("S61_To60_2");
    acid_ = loadSfx("Sfx_Acid");
    leaves_ = loadSfxVol("Sfx_Leafs", 0x32);
    fly1_ = loadSfx("Sfx_Fly1");
    fly2_ = loadSfx("Sfx_Fly2");
    clothes_ = loadSfxVol("Sfx_Clothes_Scratched", 0x32);
    creak_ = loadSfx("Sfx_Door_Creaking4");
    stepL_ = loadSfx("Gen_StepLeft");
    stepR_ = loadSfx("Gen_StepRight");
    const auto cue = [](AnimLayer* a, int frame, AudioBuffer* b) {
        if (!a) return;
        AnimCue c;
        c.type = 1;
        c.frame = frame;
        c.buffer = b;
        a->addCue(c);  // CAnimLayer_AddCue 0x00442BB0
    };
    cue(climbOut_.get(), 0x1E, clothes_.get());
    cue(climbIn_.get(), 0x1B, clothes_.get());
    cue(openJaws_.get(), 0x14, stepL_.get());
    cue(openJaws_.get(), 0x1B, stepR_.get());
    cue(openJaws_.get(), 0x26, stepL_.get());
    cue(openJaws_.get(), 0x32, creak_.get());
    cue(strew_.get(), 0x1D, leaves_.get());
    ambientTimer_ = std::make_unique<Event>();
    music_ = playMusic("Loc17_UglyAndDisgusting");
    if (music_) {
        music_->setReverb(2, f32(0x3ED58106), f32(0x3ECCCCCD), f32(0x3F2A7EFA));
        music_->setReverbMix(f32(0x3DCCCCCD));  // 0x004375B0
    }
}

void Loc18::destroyLocal() {
    stopSmoke();
    ambientTimer_.reset();
    climbIn_.reset();
    climbOut_.reset();
    openJaws_.reset();
    strew_.reset();
    for (auto* b : {&acid_, &leaves_, &fly1_, &fly2_, &clothes_, &creak_, &stepL_, &stepR_}) b->reset();
}

void Loc18::startSmoke() {
    smokeTimer_ = std::make_unique<Event>();
    setTimerHandler(*smokeTimer_, 1000, 0, 6);
}

void Loc18::rearmSmoke() {
    if (smokeTimer_) setTimerHandler(*smokeTimer_, 4000, 0, 6);
}

void Loc18::stopSmoke() {
    if (!smokeTimer_) return;
    smokeTimer_->enabled = false;
    smokeTimer_->pending = false;
    smokeTimer_.reset();
}

void Loc18::climbOut() {
    scene_.setModal(climbOut_.get());
    const int cb = beginWait();
    if (climbOut_) climbOut_->playEndEvent(11, cb);
    if (wait()) changeScene(kScene60, 0x11);
}

void Loc18::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    setEventProc(&Loc18::onEvent);
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    if (mushroomL_) mushroomL_->enable();
    if (mushroomR_) mushroomR_->enable();
    scene_.disableInput();
    if (to60_) to60_->setEvent(1);
    if (brain_) brain_->setEvent(2);
    if (to60b_) to60b_->setEvent(3);
    if (InventoryBar* bar = scene_.inventory()) {
        bar->setItemEvent(kItemAcid, 4);
        bar->setItemEvent(kItem8, 5);
    }
    if (restored_ == 0) {
        scene_.setModal(climbIn_.get());
        {
            const int cb = beginWait();
            if (climbIn_) climbIn_->playEndEvent(10, cb);
            wait();
        }
        if (hero_) {
            hero_->show(scene_.clicksEnabled);
            hero_->setPosDir(0x8B, 0xAD, 0);
        }
        {
            const int cb = beginWait();
            if (hero_) hero_->walkTo(0x91, 0xBE, cb, 0);  // CActor_WalkTo2
            wait();
        }
        if (flag(kFirstIn) != 0) {
            flag(kFirstIn) = 0;
            heroSay("061_ROD_01");  // CGame_SayActor("Loc18", …, Held)
        }
    } else if (hero_) {
        hero_->show(scene_.clicksEnabled);
    }
    if (flag(kAcid) != 0) startSmoke();
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(3, 6) * 1000), 0, 0x0E);
    scene_.enableInput();
}

void Loc18::onEvent(Event& e, int) {
    switch (e.param) {
    case 1: {  // hinaus → 60
        if (smokeTimer_) {
            smokeTimer_->enabled = false;
            smokeTimer_->pending = false;
        }
        const int cb = beginWait();
        if (hero_) hero_->walkTo(0x8C, 0xC0, cb, 3);
        if (!wait()) {
            if (flag(kAcid) != 0) rearmSmoke();
            return;
        }
        if (flag(kAcid) == 0) {
            climbOut();
            return;
        }
        scene_.disableInput();
        if (!cloneRunning_ && !smokeRunning_) {
            climbOut();
            return;
        }
        leaveWhenDone_ = true;  // erst nach dem laufenden Rauch (Ereignis 7/8)
        return;
    }
    case 2: {  // Gehirn
        const char* key = nullptr;
        switch (flag(kBrainTalk)) {
        case 1: flag(kBrainTalk) = 2; key = "061_ROD_04"; break;
        case 2: flag(kBrainTalk) = 3; key = "061_ROD_05"; break;
        case 3: flag(kBrainTalk) = 1; key = "061_ROD_06"; break;
        default: return;
        }
        walkAndComment(0x100, 0x123, 5, key);
        return;
    }
    case 3:  // Kiefer
        if (flag(kAcid) == 0) {
            walkAndComment(0x235, 0xF7, 5, "061_ROD_02");
            return;
        }
        scene_.disableInput();
        walkAndComment(0x235, 0xF7, 5, "061_ROD_03");
        scene_.setModal(openJaws_.get());
        {
            const int cb = beginWait();
            if (openJaws_) openJaws_->playEndEvent(11, cb);
            if (!wait()) return;
        }
        flag(kFromSkull) = 1;
        changeScene(kScene60, 0x11);
        return;
    case 4:  // Gegenstand 6 aufs Gehirn streuen
        scene_.disableInput();
        walkAndComment(0x100, 0x123, 5, "061_ROD_08");
        scene_.setModal(strew_.get());
        {
            const int cb = beginWait();
            if (strew_) strew_->playEndEvent(10, cb);
            wait();
        }
        if (acid_) acid_->play();  // vtable +0x08
        startSmoke();
        delay(3000);
        heroSay("061_ROD_09");
        flag(kAcid) = 1;
        if (InventoryBar* bar = scene_.inventory()) bar->remove(kItemAcid);
        scene_.enableInput();
        return;
    case 5:  // Gegenstand 8
        faceAndSay("061_ROD_10");
        return;
    case 6:  // Rauch abwechselnd aus beiden Ebenen
        if (nextSmoke_ == 1) {
            if (smoke_) smoke_->playEndEvent(10, 7);
            smokeRunning_ = true;
            nextSmoke_ = 2;
        } else if (nextSmoke_ == 2) {
            if (smokeClone_) smokeClone_->playEndEvent(10, 8);
            cloneRunning_ = true;
            nextSmoke_ = 1;
        }
        rearmSmoke();
        return;
    case 7:
    case 8: {
        bool other;
        if (e.param == 7) {
            other = cloneRunning_;
            smokeRunning_ = false;
        } else {
            other = smokeRunning_;
            cloneRunning_ = false;
        }
        if (!other && leaveWhenDone_) climbOut();
        return;
    }
    case 0x0E: {  // Fliegen
        const int r = random(1, 2);
        AudioBuffer* b = r == 1 ? fly1_.get() : r == 2 ? fly2_.get() : nullptr;
        if (b) {
            b->setVolume(random(0x32, 100));  // CAudioBuffer_SetVolume
            playSfxRandom3D(*b);
        }
        setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(3, 6) * 1000), 0, 0x0E);
        return;
    }
    default:
        return;
    }
}

}  // namespace rah
