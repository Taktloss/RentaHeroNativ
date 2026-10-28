// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc40.hpp"

#include "game/loc01.hpp"
#include "game/loc04.hpp"
#include "game/loc05.hpp"
#include "game/loc06.hpp"
#include "game/loc35.hpp"
#include "game/loc37.hpp"

#include <bit>
#include <cstdint>

namespace rah {

namespace {
constexpr int kFrameEvent = 1;  // EventValue(1): Zielbild erreicht
constexpr int kDoneEvent = 2;   // EventValue(2): Animationsende oder Klick
constexpr int kSpeechEvent = 3; // EventValue(3): Sprache fertig (S116)
float f32(std::uint32_t bits) { return std::bit_cast<float>(bits); }
}  // namespace

AudioBuffer* Loc40::obj(int off) { return sfx_[static_cast<std::size_t>((off - 0x80) / 4)].get(); }

void Loc40::load(int off, std::string_view name) { sfx_[static_cast<std::size_t>((off - 0x80) / 4)] = loadSfx(name); }

void Loc40::loadVol(int off, std::string_view name, int vol) {
    sfx_[static_cast<std::size_t>((off - 0x80) / 4)] = loadSfxVol(name, vol);
}

// orig: Rent-A-Hero.exe 0x00442BB0 CAnimLayer_AddCue(bild, puffer)
void Loc40::cue(int frame, int off) { cueAction(frame, off, 1); }

// orig: Rent-A-Hero.exe 0x00442C10 CAnimLayer_AddCueAction(bild, puffer, aktion)
void Loc40::cueAction(int frame, int off, int action) {
    AnimCue c;
    c.type = 1;
    c.frame = frame;
    c.buffer = obj(off);
    c.action = action;
    anim_->addCue(c);
}

// orig: Rent-A-Hero.exe 0x00442C70 CAnimLayer_AddCueEx(bild, puffer, aktion, vol, volMs, freq, freqMs, x, y, z)
void Loc40::cueEx(int frame, int off, int action, int vol, int volMs, int freq, int freqMs, float x, float y, float z) {
    AnimCue c;
    c.type = 2;
    c.frame = frame;
    c.buffer = obj(off);
    c.action = action;
    c.vol = vol;
    c.volMs = volMs;
    c.freq = freq;
    c.freqMs = freqMs;
    c.x = x;
    c.y = y;
    c.z = z;
    anim_->addCue(c);
}

Loc40::Loc40(GameContext& ctx, int scene, int prevScene) : Game(ctx, 40, scene, prevScene) {
    scene_.load("xxx", 0xFFFF);  // CSzene_LoadAll: SZN fehlt → leere Szene
    getViewObjects();
    // Zeiger +0x7C…+0xFC = 0
    if (sceneNr_ == 114) {
        anim_ = loadAnim("S114_IntroPart1");
        playMusic("Loc40_IntroPart1");
        loadVol(0x80, "Sfx_Ocean", 0);
        load(0x84, "Sfx_Seagull");
        load(0x88, "Sfx_Bird");
        load(0x8C, "Sfx_Bird2");
        load(0x90, "Sfx_Bird3");
        load(0x94, "Sfx_Work_Hack");
        load(0x98, "Sfx_Work_Hack2");
        load(0x9C, "Sfx_Sword2");
        load(0xA0, "Sfx_MobileDown");
        load(0xA4, "Sfx_Owl");
        load(0xA8, "Sfx_Cricket");
        load(0xAC, "Sfx_Gong");
        load(0xB0, "Sfx_CannonFire");
        load(0xB4, "Sfx_CannonFire2");
        load(0xB8, "Sfx_Explosion_Bass");
        load(0xBC, "Sfx_Explosion_Bass2");
        load(0xC0, "Sfx_Explosion_Misc2");
        load(0xC4, "Sfx_Fire");
        load(0xC8, "Sfx_BombApproaching");
        load(0xCC, "Sfx_BombApproaching2");
        if (!anim_) return;  // Port: Original würde über den Nullzeiger abstürzen
        cueEx(120, 0x80, 0, 45, 1500, 0, 0, 0, 0, 0);
        cueEx(140, 0x80, 0, 100, 16000, 0, 0, 0, 0, 0);
        cueEx(340, 0x80, 0, 1, 8000, 0, 0, 0, 0, 0);
        cueEx(255, 0x84, 1, 80, 0, 0, 0, 0, 0, 0.1f);
        cueEx(265, 0x84, 1, 60, 0, 0, 0, 0.2f, 0, 0.1f);
        cueEx(410, 0x88, 1, 50, 0, 0, 0, -0.5f, 0, 0.2f);
        cueEx(420, 0x8C, 1, 50, 0, 0, 0, 0.4f, 0, 0.2f);
        cueAction(425, 0x80, 3);
        cueEx(440, 0x90, 1, 50, 0, 0, 0, -0.3f, 0, -0.2f);
        cueEx(478, 0x94, 1, 30, 0, 0, 0, -0.1f, 0, 0.1f);
        cueEx(494, 0x94, 1, 50, 0, 0, 0, -0.1f, 0, 0.1f);
        cueEx(506, 0x94, 1, 70, 0, 0, 0, -0.1f, 0, 0.1f);
        cueEx(525, 0x94, 1, 90, 0, 0, 0, -0.1f, 0, 0.1f);
        cueEx(549, 0x94, 1, 100, 0, 0, 0, -0.1f, 0, 0.1f);
        cueEx(565, 0x94, 1, 100, 0, 0, 0, -0.1f, 0, 0.1f);
        cueEx(577, 0x9C, 1, 100, 0, 0, 0, -0.1f, 0, 0.1f);
        cueEx(484, 0x98, 1, 40, 0, 0, 0, -0.2f, 0, 0.1f);
        cueEx(498, 0x98, 1, 70, 0, 0, 0, -0.2f, 0, 0.1f);
        cueEx(516, 0x98, 1, 100, 0, 0, 0, -0.1f, 0, 0.1f);
        cueEx(534, 0x98, 1, 80, 0, 0, 0, 0, 0, -0.1f);
        cueEx(552, 0x98, 1, 60, 0, 0, 0, 0, 0, -0.1f);
        cueEx(570, 0x98, 1, 60, 0, 0, 0, 0, 0, -0.1f);
        cueEx(598, 0x98, 1, 60, 0, 0, 0, 0, 0, -0.1f);
        cueEx(616, 0x98, 1, 60, 0, 0, 0, -0.1f, 0, -0.1f);
        cueEx(624, 0xA0, 1, 100, 0, 0, 0, 0, 0, 0);
        cueEx(670, 0xA4, 1, 70, 0, 0, 0, -0.2f, 0, 0.2f);
        cueEx(700, 0xA8, 1, 70, 0, 0, 0, 0.2f, 0, 0.2f);
        cueEx(710, 0xA8, 1, 60, 0, 0, 0, 0.4f, 0, -0.1f);
        cueEx(960, 0x80, 2, 70, 1170, 0, 0, 0.4f, 0, -0.1f);
        cueEx(1120, 0x84, 1, 70, 0, 0, 0, -0.3f, 0, 0.2f);
        cueEx(1140, 0x84, 1, 50, 0, 0, 0, -0.1f, 0, 0.2f);
        cueAction(1158, 0x80, 3);
        cueAction(1158, 0xC4, 2);
        cue(1142, 0xB0);
        cue(1152, 0xC8);
        cueEx(1168, 0xBC, 1, 100, 0, 0, 0, -0.2f, 0, 0.2f);
        cueEx(1168, 0xC0, 1, 60, 0, 0, 0, -0.2f, 0, 0.2f);
        cueEx(1179, 0xB4, 1, 100, 0, 0, 0, 0.2f, 0, 0.2f);
        cueEx(1189, 0xCC, 1, 100, 0, 0, 0, 0.2f, 0, 0.2f);
        cueEx(1207, 0xC0, 1, 100, 0, 0, 0, 0.2f, 0, 0.1f);
        cueEx(1234, 0xB0, 1, 100, 0, 0, 0, -0.1f, 0, -0.2f);
        cueEx(1244, 0xC8, 1, 100, 0, 0, 0, -0.1f, 0, -0.2f);
        cueEx(1260, 0xB8, 1, 100, 0, 0, 0, -0.1f, 0, -0.2f);
        cueEx(1270, 0xC4, 0, 1, 1000, 0, 0, 0, 0, 0);
        cueEx(1270, 0xC4, 0, 1, 1000, 0, 0, 0, 0, 0);
    } else if (sceneNr_ == 115) {
        anim_ = loadAnim("S115_IntroPart2");
        playMusic("Loc36_IntroDragonAttack");
        load(0xBC, "Sfx_Explosion_Bass2");
        load(0xB8, "Sfx_Explosion_Bass");
        loadVol(0xD0, "Sfx_Glider_Flying", 0);
        loadVol(0xFC, "Sfx_DragonCrash", 70);
        loadVol(0xD8, "Sfx_ServoMotor_ShortClang2", 60);
        loadVol(0xE8, "Sfx_DragonFire", 90);
        loadVol(0xDC, "Sfx_ServoMotor_ShortClang", 65);
        loadVol(0xE0, "Sfx_Rubberband", 90);
        loadVol(0xE4, "Sfx_WaterSplash", 70);
        load(0xEC, "Sfx_DragonWings2");
        load(0xF0, "Sfx_DragonBreath");
        load(0xF4, "Sfx_DragonBreath2");
        load(0xF8, "Sfx_DragonBreath3");
        load(0x88, "Sfx_Bird");
        load(0x8C, "Sfx_Bird2");
        load(0x90, "Sfx_Bird3");
        if (!anim_) return;
        cueEx(140, 0xD0, 0, 20, 0, 0, 0, 0, 0, 0);
        cueEx(191, 0xD0, 0, 75, 0, 0, 0, 0, 0, 0);
        cueEx(230, 0xD0, 0, 100, 800, 0, 0, 0, 0, 0);
        cueEx(240, 0xD0, 0, 50, 2000, 100, 2000, 0, 0, 0);
        cueEx(263, 0xD0, 0, 100, 2800, 0, 0, 0, 0, 0);
        cueEx(300, 0xD0, 0, 50, 2200, 0, 0, 0, 0, 0);
        cueEx(350, 0xD0, 0, 100, 2800, 0, 0, 0, 0, 0);
        cueEx(380, 0xD0, 0, 0, 0, 50, 1000, 0, 0, 0);
        cueEx(393, 0xD0, 0, 30, 0, 0, 0, 0, 0, 0);
        cueEx(464, 0xD0, 0, 100, 2800, 0, 0, 0, 0, 0);
        cueEx(510, 0xD0, 0, 0, 0, 100, 800, 0, 0, 0);
        cueEx(520, 0xD0, 0, 50, 1800, 0, 0, 0, 0, 0);
        cueEx(541, 0xD0, 0, 0, 0, 50, 0, 0, 0, 0);
        cueEx(541, 0xD0, 0, 100, 1400, 100, 1400, 0, 0, 0);
        cueEx(560, 0xD0, 0, 50, 1500, 50, 1500, 0, 0, 0);
        cueEx(578, 0xD0, 0, 100, 0, 100, 0, 0, 0, 0);
        cueEx(640, 0xD0, 0, 1, 2000, 50, 2000, 0, 0, 0);
        cue(403, 0xBC);
        cue(506, 0xBC);
        cue(645, 0xBC);
        cue(645, 0xB8);
        cue(506, 0xFC);
        cue(645, 0xFC);
        cue(614, 0xD8);
        cue(329, 0xE8);
        cue(221, 0xDC);
        cue(624, 0xE0);
        cue(656, 0xE4);
        cueEx(405, 0xEC, 1, 0, 0, 0, 0, 0, 0, 0);
        cueEx(420, 0xEC, 1, 0, 0, 0, 0, 0, 0, 0.1f);
        cueEx(437, 0xEC, 1, 0, 0, 0, 0, 0, 0, 0);
        cueEx(455, 0xEC, 1, 0, 0, 0, 0, 0.1f, 0, 0.1f);
        cueEx(473, 0xEC, 1, 0, 0, 0, 0, -0.1f, 0, -0.3f);
        cueEx(491, 0xEC, 1, 0, 0, 0, 0, -0.1f, 0, -0.2f);
        cueEx(529, 0xEC, 1, 0, 0, 0, 0, -0.1f, 0, 0.1f);
        cueEx(543, 0xEC, 1, 0, 0, 0, 0, 0.3f, 0, 0.3f);
        cueEx(555, 0xEC, 1, 0, 0, 0, 0, 0.2f, 0, 0.2f);
        cueEx(568, 0xEC, 1, 0, 0, 0, 0, 0, 0, 0.1f);
        cueEx(581, 0xEC, 1, 0, 0, 0, 0, 0, 0, -0.3f);
        cueEx(594, 0xEC, 1, 0, 0, 0, 0, 0, 0, -0.2f);
        cueEx(612, 0xEC, 1, 0, 0, 0, 0, 0, 0, -0.1f);
        cueEx(630, 0xEC, 1, 0, 0, 0, 0, -0.2f, 0, 0.2f);
        cue(150, 0xF0);
        cue(294, 0xF0);
        cue(325, 0xF4);
        cue(393, 0xF4);
        cue(522, 0xF4);
        cue(413, 0xF8);
        cue(508, 0xF8);
        cueEx(37, 0x88, 1, 20, 0, 0, 0, 0.2f, 0, 0.3f);
        cueEx(208, 0x8C, 1, 20, 0, 0, 0, -0.1f, 0, -0.2f);
        cueEx(390, 0x90, 1, 20, 0, 0, 0, 0.3f, 0, 0.1f);
    } else if (sceneNr_ == 116) {
        anim_ = loadAnim("S116_IntroPart3");
        playMusic("Loc36_IntroDragonAttack");
        loadVol(0xD0, "Sfx_Glider_Flying", 0);
        load(0x88, "Sfx_Bird");
        load(0x8C, "Sfx_Bird2");
        load(0x90, "Sfx_Bird3");
        if (!anim_) return;
        cueEx(88, 0xD0, 2, 100, 800, 0, 0, 0, 0, 0);
        cueEx(100, 0x8C, 1, 20, 0, 0, 0, 0.1f, 0, 0.2f);
        cueEx(113, 0x88, 1, 20, 0, 0, 0, -0.2f, 0, -0.3f);
        cueEx(123, 0x90, 1, 20, 0, 0, 0, 0.2f, 0, 0.3f);
    }
}

Loc40::~Loc40() { shutdown(); }

void Loc40::destroyLocal() {
    // +0x7C … +0xFC löschen (Animation zuerst), danach CGame_dtor (Szene hält +0x5DC noch)
    anim_.reset();
    for (auto& b : sfx_) b.reset();
}

void Loc40::start() {
    // Port: ohne AVI fehlt +0x7C; das Original würde hier abstürzen
    if (!anim_) return;
    if (sceneNr_ == 114) {
        if (music_) {
            music_->play();
            music_->setReverb(0xF, f32(0x3DE353F8), f32(0x404947AE), f32(0x3EF1A9FC));
            music_->setReverbMix(f32(0x3DCCCCCD));
        }
        setEventProc(&Loc40::onEvent114);
        scene_.setClickEvent(kDoneEvent);
        scene_.setModal(anim_.get());
        anim_->setFrameEvent(kFrameEvent, 10);
        anim_->playEndEvent(10, kDoneEvent);
        if (AudioBuffer* ocean = obj(0x80)) {
            ocean->playLoop();
            ocean->fadeVolume(30, 5000);
        }
        scene_.clicksDuringFull = true;
    } else if (sceneNr_ == 115) {
        if (music_) {
            music_->play();
            music_->setReverb(0xF, f32(0x3DE353F8), f32(0x404947AE), f32(0x3EF1A9FC));
            music_->setReverbMix(f32(0x3DCCCCCD));
        }
        setEventProc(&Loc40::onEvent115);
        scene_.setClickEvent(kDoneEvent);
        scene_.setModal(anim_.get());
        anim_->playEndEvent(10, kDoneEvent);
        if (AudioBuffer* glider = obj(0xD0)) {
            glider->playLoop();
            glider->fadeVolume(75, 1200);
            glider->fadeFreq(50, 1200);
        }
        scene_.clicksDuringFull = true;
    } else if (sceneNr_ == 116) {
        if (music_) {
            music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
            music_->setReverb(0x10, f32(0x3DE353F8), f32(0x40311687), f32(0x3E656042));
            music_->setReverbMix(f32(0x3DCCCCCD));
        }
        setEventProc(&Loc40::onEvent116);
        scene_.setClickEvent(kDoneEvent);
        scene_.setModal(anim_.get());
        anim_->setFrameEvent(kFrameEvent, 1);
        anim_->playRangeNoEvent(11, 0, 120);
        scene_.clicksDuringFull = true;
    }
    scene_.clearFlag510();
    scene_.enableInput();
}

void Loc40::onEvent114(Event& e, int) {
    if (e.param == kFrameEvent) {
        int next = 0;
        switch (anim_->frame) {
        case 10: say("Loc40", "016_SPR_01"); next = 0x96; break;
        case 0x96:
            say("Loc40", "016_SPR_02");
            if (music_) {
                music_->setReverb(0x10, f32(0x3DE353F8), f32(0x40311687), f32(0x3E656042));
                music_->setReverbMix(f32(0x3DCCCCCD));
            }
            next = 0x212;
            break;
        case 0x212: say("Loc40", "016_SPR_03"); next = 0x2E4; break;
        case 0x2E4: say("Loc40", "021_SPR_01"); next = 0x352; break;
        case 0x352: say("Loc40", "021_SPR_02"); next = 0x3C0; break;
        case 0x3C0:
            say("Loc40", "023_SPR_01");
            if (music_) {
                music_->setReverb(0xF, f32(0x3DE353F8), f32(0x404947AE), f32(0x3EF1A9FC));
                music_->setReverbMix(f32(0x3DCCCCCD));
            }
            next = 0x435;
            break;
        case 0x435:
            playMusic("Loc09_SmashvilleAttack");
            if (music_) {
                music_->setReverb(0x10, f32(0x3DE353F8), f32(0x40311687), f32(0x3E656042));
                music_->setReverbMix(f32(0x3DCCCCCD));
                music_->play();
            }
            next = 0x4E0;
            break;
        case 0x4E0: say("Loc40", "024_ST2_01"); return;
        default: return;
        }
        anim_->setFrameEvent(kFrameEvent, next);
    } else if (e.param == kDoneEvent) {
        scene_.clicksDuringFull = false;
        changeScene(1, 35);
    }
}

void Loc40::onEvent115(Event& e, int) {
    if (e.param == kDoneEvent) {
        scene_.clicksDuringFull = false;
        changeScene(3, 37);
    }
}

void Loc40::onEvent116(Event& e, int) {
    if (e.param == kFrameEvent) {
        int next = 0;
        switch (anim_->frame) {
        case 1: say("Loc40", "003_PRI_05"); next = 0x15; break;
        case 0x15: say("Loc40", "003_PRI_06"); next = 0x57; break;
        case 0x57: speak("Loc40", "004_PRI_01", kSpeechEvent); next = 0x62; break;
        case 0x62:
            playMusic("Loc35_RodrigoOnGlider");
            if (music_) {
                music_->fadeIn(100, 2000);  // 0x00437FD0
                music_->setReverbMix(f32(0x3DCCCCCD));
            }
            if (AudioBuffer* glider = obj(0xD0)) {
                glider->fadeVolume(1, 5000);
                glider->fadeFreq(50, 5000);
            }
            return;
        default: return;
        }
        anim_->setFrameEvent(kFrameEvent, next);
    } else if (e.param == kDoneEvent) {
        scene_.clicksDuringFull = false;
        sceneNr_ = 5;  // Original: vorherige Szene wird 5 (loc40.md)
        changeScene(7, 1);
    } else if (e.param == kSpeechEvent) {
        anim_->playRange(10, kDoneEvent, 0x79, -1);
    }
}

std::unique_ptr<Game> createLocation(GameContext& ctx, int location, int scene, int prevScene) {
    switch (location) {
    case 1: return std::make_unique<Loc01>(ctx, scene, prevScene);
    case 4: return std::make_unique<Loc04>(ctx, scene, prevScene);
    case 5: return std::make_unique<Loc05>(ctx, scene, prevScene);
    case 6: return std::make_unique<Loc06>(ctx, scene, prevScene);
    case 35: return std::make_unique<Loc35>(ctx, scene, prevScene);
    case 37: return std::make_unique<Loc37>(ctx, scene, prevScene);
    case 40: return std::make_unique<Loc40>(ctx, scene, prevScene);
    default: return nullptr;  // weitere Schauplätze folgen
    }
}

}  // namespace rah
