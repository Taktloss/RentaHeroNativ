// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc25.hpp"

#include <bit>

#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
float f32(std::uint32_t bits) { return std::bit_cast<float>(bits); }
constexpr std::size_t kRound = 0x4090;  // 0: Köpfe noch nicht da; 1…6 nächster richtiger Helfer
constexpr int kThunder = 200;
}  // namespace

Loc25::Loc25(GameContext& ctx, int scene, int prevScene) : Game(ctx, 25, scene, prevScene) {
    sceneNr_ = 0x69;
    getViewObjects();
    scene_.load("S105", 0xFFFF);  // CSzene_LoadAll(DAT_0047514C)
    getViewObjects();
    rod_ = scene_.getObject("S105_Rod");
    draHead_ = scene_.getObject("S105_DraHead");
    jas_ = scene_.getObject("S105_Jas");
    madHead_ = scene_.getObject("S105_MadHead");
    ran_ = scene_.getObject("S105_Ran");
    sabHead_ = scene_.getObject("S105_SabHead");
    rodMorph_ = obj("S105_RodMorph");
    wizAttack_ = obj("S105_WizAttack");
    sabAttack_ = obj("S105_SabAttack");
    draAttack_ = obj("S105_DraAttack");
    madAttack_ = obj("S105_MadAttack");
    louAttack_ = obj("S105_LouAttack");
    sanAttack_ = obj("S105_SanAttack");
    cynAttack_ = obj("S105_CynAttack");
    wizWalk_ = obj("S105_WizWalk");
    wizExplodes_ = obj("S105_WizExplodes");
    jasHead_ = scene_.getObject("S105_JasHead");
    louHead_ = scene_.getObject("S105_LouHead");
    ranHead_ = scene_.getObject("S105_RanHead");
    cynHead_ = scene_.getObject("S105_CynHead");
    sanHead_ = scene_.getObject("S105_SanHead");
    endTime_ = loadSfx("Sfx_EndTimeFeeling");
    dragonBreath_ = loadSfx("Sfx_DragonBreath");
    dragonFire_ = loadSfx("Sfx_DragonFire");
    explBass_ = loadSfx("Sfx_Explosion_Bass");
    explBass2_ = loadSfx("Sfx_Explosion_Bass2");
    gong_ = loadSfx("Sfx_Gong");
    knock_ = loadSfx("Sfx_Knock_Wet");
    mobileUp_ = loadSfx("Sfx_MobileUp");
    mobileDown_ = loadSfx("Sfx_MobileDown");
    horn_ = loadSfx("Sfx_SignalHorn_Modern");
    thunder_ = loadSfx("Sfx_Thunder");
    thunder2_ = loadSfx("Sfx_Thunder2");
    splash_ = loadSfx("Sfx_WaterSplash");
    woosh_ = loadSfx("Sfx_Woosh");
    lazer_ = loadSfx("Sfx_Lazer3");
    danger_ = loadSfx("Sfx_DangerStrings");
    explMisc_ = loadSfx("Sfx_Explosion_Misc2");
    const auto cue = [](AnimLayer* a, int frame, const std::unique_ptr<AudioBuffer>& b) {
        if (!a) return;
        AnimCue c;
        c.type = 1;
        c.frame = frame;
        c.buffer = b.get();
        a->addCue(c);  // CAnimLayer_AddCue 0x00442BB0
    };
    cue(draAttack_, 1, dragonBreath_);
    cue(draAttack_, 0x10, dragonFire_);
    cue(wizExplodes_, 5, explMisc_);
    cue(rodMorph_, 0, explBass_);
    cue(wizAttack_, 6, lazer_);
    cue(sabAttack_, 0x0B, woosh_);
    cue(sabAttack_, 0x0D, knock_);
    cue(madAttack_, 9, woosh_);
    cue(madAttack_, 10, knock_);
    cue(louAttack_, 6, splash_);
    cue(sanAttack_, 0x0F, horn_);
    cue(cynAttack_, 8, woosh_);
    cue(cynAttack_, 10, knock_);
    thunderTimer_ = std::make_unique<Event>();
    music_ = playMusic("Loc25_Cyberspace");
    if (music_) {
        music_->setReverb(10, f32(0x3F000000), f32(0x41200000), f32(0x3E99999A));
        music_->setReverbMix(f32(0x3DCCCCCD));  // 0x004375B0
    }
}

void Loc25::destroyLocal() {
    thunderTimer_.reset();
    for (auto* b : {&endTime_, &dragonBreath_, &dragonFire_, &explBass_, &explBass2_, &gong_, &knock_, &mobileUp_,
                    &mobileDown_, &horn_, &thunder_, &thunder2_, &splash_, &woosh_, &lazer_, &danger_, &explMisc_})
        b->reset();
}

void Loc25::wizardApproach() {
    if (danger_) danger_->play();  // vtable +0x08
    if (!wizWalk_) return;
    wizWalk_->setPos(0x172, -0xA7);  // vtable +0x14
    wizWalk_->setZoom(0x50);         // CLayer_SetZoom
    wizWalk_->start(0x0D);
    const int cb = beginWait();
    wizWalk_->moveTo(357.0, 42.0, 0.0, 100.0, 0x14, cb);  // 0x004475B0
    wait();
}

void Loc25::start() {
    // Original: kein CMusic_FadeIn, sondern Schleife mit Lautstärke 100
    if (music_) {
        music_->playLoop();      // vtable +0x10
        music_->setVolume(100);  // CAudioBuffer_SetVolume
    }
    setEventProc(&Loc25::onEvent);
    scene_.disableAll();
    if (endTime_) {
        endTime_->setFreq(0x14);  // CAudioBuffer_SetFreq
        endTime_->playLoop();
    }
    if (auto* bg = dynamic_cast<AnimLayer*>(scene_.background())) bg->playRangeNoEvent(13, 0, 10);  // +0x60 (AVI)
    if (InventoryBar* bar = scene_.inventory()) {
        for (int id : {5, 8, 9, 10, 0x0B, 0x18, 0x1A}) bar->remove(id);
    }
    scene_.disableInput();
    wizardApproach();
    if (rod_) rod_->enable();
    {
        const int cb = beginWait();
        if (rodMorph_) rodMorph_->playEndEvent(10, cb);
        wait();
    }
    if (wizWalk_ && wizWalk_->area) wizWalk_->area->setEvent(1);
    scene_.enableInput();
    setTimerHandler(*thunderTimer_, static_cast<std::uint32_t>(random(3, 5) * 1000), 0, kThunder);
    scene_.enableInput();
}

void Loc25::attack(int number, Layer* layer) {
    scene_.disableInput();
    if (layer) layer->enable();
    const bool dragon = layer == draAttack_;
    if (dragon && draAttack_) {
        draAttack_->setZoom(0x19);
        draAttack_->setPos(0xB9, 0xA9);
        draAttack_->moveTo(1.0, 1.0, 0.0, 100.0, 10, 0);
    }
    if (rod_) rod_->disable();
    {
        const int cb = beginWait();
        if (rodMorph_) rodMorph_->playEndEvent(10, cb);
        wait();
    }
    if (dragon && draAttack_) draAttack_->setPos(0, 0);
    if (flag(kRound) != number) {
        // falscher Helfer: der Zauberer schlägt zurück, alles von vorn
        if (wizWalk_) wizWalk_->disable();
        {
            const int cb = beginWait();
            if (wizAttack_) wizAttack_->playRange(11, cb, 0, 5);
            wait();
        }
        if (rodMorph_) rodMorph_->start(10);
        if (layer) layer->disable();
        if (rod_) rod_->enable();
        {
            const int cb = beginWait();
            if (wizAttack_) wizAttack_->playRange(10, cb, 6, -1);
            wait();
        }
        if (wizWalk_) {
            wizWalk_->start(0x0D);
            if (wizWalk_->area) wizWalk_->area->active = true;  // +0xB8
        }
        flag(kRound) = 1;
        scene_.enableInput();
        return;
    }
    // richtiger Helfer: Treffer
    auto* anim = dynamic_cast<AnimLayer*>(layer);
    if (wizWalk_) wizWalk_->setPos(0x165, 0x2A);
    if (wizExplodes_) wizExplodes_->setZoom(100);
    if (!dragon && wizWalk_) {
        const int cb = beginWait();
        wizWalk_->moveTo(247.0, 23.0, 0.0, 150.0, 10, cb);
        wait();
    }
    {
        const int cb = beginWait();
        if (anim) anim->playRange(11, cb, 0, number - 1);
        wait();
    }
    if (wizWalk_) wizWalk_->disable();
    if (wizExplodes_) {
        if (dragon) {
            wizExplodes_->setPos(0x10F, -0x15);
            wizExplodes_->setZoom(100);
        } else {
            wizExplodes_->setPos(0x76, -0x49);
            wizExplodes_->setZoom(0x96);
        }
        wizExplodes_->start(10);
    }
    {
        const int cb = beginWait();
        if (anim) anim->playRange(11, cb, number, -1);
        wait();
    }
    if (layer) layer->disable();
    if (rod_) rod_->enable();
    if (rodMorph_) rodMorph_->start(10);
    if (flag(kRound) < 6) {
        wizardApproach();
    } else {
        // Megophias ist besiegt
        if (gong_) gong_->play();
        delay(3000);
        if (mobileDown_) mobileDown_->play();
        for (Layer* h : {draHead_, madHead_, sabHead_, jasHead_, louHead_, ranHead_, cynHead_}) {
            if (h) h->disable();
            delay(100);
        }
        if (sanHead_) sanHead_->disable();
        delay(1000);
        if (rod_) rod_->disable();
        {
            const int cb = beginWait();
            if (rodMorph_) rodMorph_->playEndEvent(10, cb);
            wait();
        }
        delay(2000);
        changeScene(0x70, 0x29);
    }
    ++flag(kRound);
    scene_.enableInput();
}

void Loc25::onEvent(Event& e, int) {
    switch (e.param) {
    case 1: {  // Zauberer anklicken: er greift an, beim ersten Mal erscheinen die Helfer
        scene_.disableInput();
        if (wizWalk_) wizWalk_->disable();
        {
            const int cb = beginWait();
            if (wizAttack_) wizAttack_->playRange(11, cb, 0, 5);
            wait();
        }
        if (rodMorph_) rodMorph_->start(10);
        {
            const int cb = beginWait();
            if (wizAttack_) wizAttack_->playRange(10, cb, 6, -1);
            wait();
        }
        if (wizWalk_) {
            wizWalk_->start(0x0D);
            if (wizWalk_->area) wizWalk_->area->active = true;
        }
        speak("Loc25", "105_MEG_01", nullptr, 0x0C, -1, -1, 0);  // CGame_SayActor(…, 0, 0)
        if (flag(kRound) == 0) {
            if (mobileUp_) mobileUp_->play();
            Layer* heads[] = {draHead_, madHead_, sabHead_, jasHead_, louHead_, ranHead_, cynHead_, sanHead_};
            for (std::size_t i = 0; i < 8; ++i) {
                if (heads[i]) heads[i]->enable();
                if (i < 7) delay(100);
            }
            int ev = 0x0B;
            for (Layer* h : heads) {
                if (h && h->area) h->area->setEvent(ev);
                ++ev;
            }
            flag(kRound) = 1;
        }
        scene_.enableInput();
        return;
    }
    case 0x0B: attack(6, draAttack_); return;
    case 0x0C: attack(1, madAttack_); return;
    case 0x0D: attack(2, sabAttack_); return;
    case 0x0E: attack(0, jas_); return;
    case 0x0F: attack(4, louAttack_); return;
    case 0x10: attack(0, ran_); return;
    case 0x11: attack(3, cynAttack_); return;
    case 0x12: attack(5, sanAttack_); return;
    case kThunder: {
        const int r = random(1, 2);
        AudioBuffer* b = r == 1 ? thunder_.get() : r == 2 ? thunder2_.get() : nullptr;
        if (b) playSfxRandom3D(*b);
        setTimerHandler(*thunderTimer_, static_cast<std::uint32_t>(random(3, 5) * 1000), 0, kThunder);
        return;
    }
    default:
        return;
    }
}

}  // namespace rah
