// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc41.hpp"

#include <bit>

namespace rah {

namespace {
float f32(std::uint32_t bits) { return std::bit_cast<float>(bits); }
}  // namespace

Loc41::Loc41(GameContext& ctx, int scene, int prevScene) : Game(ctx, 41, scene, prevScene) {
    getViewObjects();
    scene_.load("S112", 0xFFFF);  // CSzene_LoadAll(DAT_00476428)
    getViewObjects();
    credits_ = dynamic_cast<AnimLayer*>(scene_.getObject("S112_Credits"));
    ending_ = dynamic_cast<AnimLayer*>(scene_.getObject("S112_EndeSequenz"));
    seaFadeOut_ = dynamic_cast<AnimLayer*>(scene_.getObject("S112_MeerFadeOut"));
    seaLoop_ = dynamic_cast<AnimLayer*>(scene_.getObject("S112_MeerLoop"));
    explosion_ = loadSfxVol("Sfx_Explosion_Bass2", 0x41);
    bird3_ = loadSfx("Sfx_Bird3");
    bird2_ = loadSfx("Sfx_Bird2");
    bird_ = loadSfx("Sfx_Bird");
    chirp_ = loadSfx("Sfx_Chirp");
    thunder_ = loadSfx("Sfx_Thunder");
    thunder2_ = loadSfx("Sfx_Thunder2");
    if (ending_) {
        const auto cue = [this](int frame, AudioBuffer* b, int action) {
            AnimCue c;
            c.type = 1;
            c.frame = frame;
            c.buffer = b;
            c.action = action;
            ending_->addCue(c);  // 0x00442BB0 / 0x00442C10 CAnimLayer_AddCueAction
        };
        cue(0x1F, thunder2_.get(), 1);
        cue(0x25, thunder_.get(), 1);
        cue(0x67, explosion_.get(), 1);
        cue(0x78, explosion_.get(), 1);
        cue(0x8E, explosion_.get(), 1);
        cue(0x5A, bird_.get(), 0x32);
        cue(0x69, bird2_.get(), 0x32);
        cue(0x7B, bird3_.get(), 0x32);
        cue(0x91, bird2_.get(), 0x32);
        cue(0x9E, chirp_.get(), 0x32);
        cue(0xAB, bird3_.get(), 0x32);
        cue(0xB4, bird_.get(), 0x32);
        ending_->setFrameEvent(0x14, 0x82);  // CAnimLayer_SetFrameEvent
    }
    if (credits_) credits_->setFrameEvent(0x15, 0x672);
    music_ = playMusic("Loc41_Outro");
    if (music_) {
        music_->setReverb(0x10, f32(0x3DE353F8), f32(0x40311687), f32(0x3E656042));
        music_->setReverbMix(f32(0x3DCCCCCD));  // 0x004375B0
    }
}

void Loc41::destroyLocal() {
    for (auto* b : {&explosion_, &thunder_, &thunder2_, &bird3_, &bird2_, &bird_, &chirp_}) b->reset();
}

void Loc41::start() {
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60, falls vorhanden
    if (music_) music_->play();                          // vtable +0x08 (kein FadeIn)
    setEventProc(&Loc41::onEvent);
    const auto playEnd = [this](AnimLayer* a) {
        const int cb = beginWait();
        if (a) a->playEndEvent(10, cb);
        wait();
    };
    playEnd(ending_);
    if (seaLoop_) seaLoop_->start(0x0D);
    playEnd(credits_);
    {
        const int cb = beginWait();
        if (seaLoop_) seaLoop_->playRange(10, cb, -1, -1);
        wait();
    }
    playEnd(seaFadeOut_);
    ctx_.save.request = 5;  // g_SaveRequest: Beenden
}

void Loc41::switchMusic(std::string_view name, bool fade) {
    music_ = playMusic(name);  // CGame_PlayMusic
    if (!music_) return;
    if (fade)
        music_->fadeIn(100, 1000);  // 0x00437FD0
    else
        music_->play();
}

void Loc41::onEvent(Event& e, int) {
    if (e.param == 0x14 && ending_) {
        if (ending_->frame == 0x82) {  // +0x2C0
            switchMusic("Loc41_InALand", false);
            if (music_) {
                music_->setReverb(0x0F, f32(0x3DE353F8), f32(0x404947AE), f32(0x3EF1A9FC));
                music_->setReverbMix(f32(0x3DCCCCCD));
            }
            ending_->setFrameEvent(0x14, 0xA0);
        } else if (ending_->frame == 0xA0) {
            say("Loc41", "112_SPR_01");  // CGame_Say
        }
        return;
    }
    if (e.param != 0x15 || !credits_) return;
    int next = 0;
    switch (credits_->frame) {
    case 0x672: switchMusic("Loc35_RodrigoOnGlider", false); next = 0x76C; break;
    case 0x76C: switchMusic("Loc05_LouisOffice", true); next = 0x834; break;
    case 0x834: switchMusic("Loc06_Ranama", true); next = 0x8FC; break;
    case 0x8FC: switchMusic("Loc26_IslandOfMegophias", true); next = 0x9C4; break;
    case 0x9C4: switchMusic("Loc14_Cynthia", true); next = 0xB40; break;
    case 0xB40:
        if (music_) music_->fadeVolume(0, 5000);  // CAudioBuffer_FadeVolume
        return;
    default: return;
    }
    credits_->setFrameEvent(0x15, next);
}

}  // namespace rah
