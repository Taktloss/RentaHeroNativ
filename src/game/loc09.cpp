// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc09.hpp"

#include <bit>
#include <cstdio>

namespace rah {

namespace {
float f32(std::uint32_t bits) { return std::bit_cast<float>(bits); }
constexpr int kDone = 1;   // Ende der Animation
constexpr int kFrame = 2;  // Bildereignis
}  // namespace

// orig: Rent-A-Hero.exe 0x00442BB0 CAnimLayer_AddCue
void Loc09::cue(int frame, int off) {
    if (!anim_) return;
    AnimCue c;
    c.type = 1;
    c.frame = frame;
    c.buffer = sfx(off);
    anim_->addCue(c);
}

// orig: Rent-A-Hero.exe 0x00442C70 CAnimLayer_AddCueEx(bild, puffer, aktion, vol, volMs, 0, 0, x, 0, z)
void Loc09::cueEx(int frame, int off, int action, int vol, int volMs, float x, float z) {
    if (!anim_) return;
    AnimCue c;
    c.type = 2;
    c.frame = frame;
    c.buffer = sfx(off);
    c.action = action;
    c.vol = vol;
    c.volMs = volMs;
    c.x = x;
    c.z = z;
    anim_->addCue(c);
}

Loc09::Loc09(GameContext& ctx, int scene, int prevScene) : Game(ctx, 9, scene, prevScene) {
    sceneNr_ = 0x59;
    scene_.load("xxx", 0xFFFF);  // DAT_00472A58, CSzene_LoadAll: SZN fehlt → leere Szene
    getViewObjects();
    anim_ = loadAnim("Attack");
    static constexpr struct { int off; const char* name; int vol; } kSfx[] = {
        {0x80, "Sfx_CannonFire", -1},        {0x84, "Sfx_CannonFire2", -1},     {0x88, "Sfx_Thunder", -1},
        {0x8C, "Sfx_Thunder2", -1},          {0x90, "Sfx_Explosion_Bass", -1},  {0x94, "Sfx_Explosion_Bass2", -1},
        {0x98, "Sfx_Explosion_Misc2", -1},   {0x9C, "Sfx_BombApproaching", -1}, {0xA0, "Sfx_BombApproaching2", -1},
        {0xA4, "Sfx_Ocean", 0x46},           {0xA8, "Sfx_BigStone", -1},        {0xAC, "Sfx_Seagull", 0x46},
        {0xB0, "Sfx_WaterSplash", -1},       {0xB4, "Sfx_ServoMotor_ShortClang2", -1},
        {0xB8, "Sfx_EndTimeFeeling", 0},
    };
    for (const auto& s : kSfx)
        sfx_[static_cast<std::size_t>((s.off - 0x80) / 4)] = s.vol < 0 ? loadSfx(s.name) : loadSfxVol(s.name, s.vol);
    if (anim_) {  // Port: Original prüft nicht
        const float p1 = f32(0x3DCCCCCD), p2 = f32(0x3E4CCCCD), p3 = f32(0x3E99999A);
        cueEx(0x3B, 0xAC, 1, 0x50, 0, p1, p3);
        cueEx(0x7B, 0xAC, 1, 100, 0, p1, p1);
        cueEx(0x197, 0xAC, 1, 0x50, 0, 0, p1);
        cueEx(0x1A5, 0xAC, 1, 0x3C, 0, -p1, p3);
        cue(0xA8, 0xB4);
        cue(0xAF, 0x80);
        for (int f : {0xBB, 0xBD, 0xBF, 0xC1}) cue(f, 0x84);
        cueEx(0xB0, 0x9C, 1, 0, 0, p1, p1);
        cueEx(0xC2, 0xA0, 1, 0, 0, p2, p1);
        cueEx(0xD3, 0x9C, 1, 0, 0, p1, p3);
        cueEx(199, 0x90, 1, 0, 0, p3, p1);
        cueEx(0xCE, 0x98, 1, 0, 0, p2, p1);
        cueEx(0xD5, 0x90, 1, 0, 0, p1, p3);
        cueEx(0xD9, 0x94, 1, 0, 0, p2, p2);
        cueEx(0xDD, 0x98, 1, 0, 0, p3, p1);
        cueEx(0xEE, 0xA4, 0, 1, 0, 0, 0);
        cue(0xFC, 0x88);
        cue(0x105, 0x8C);
        cue(0xFC, 0x88);  // Original: doppelt
        cue(0x105, 0x8C);
        cue(0xEF, 0xA8);
        for (int f = 0xF5; f <= 0x14F; f += 5) cue(f, 0xA8);  // Bilder 245, 250 … 335
        cueEx(0x151, 0xA4, 0, 0x46, 0, 0, 0);
        cue(0x177, 0x88);
        cue(0x17A, 0x8C);
        cue(0x18E, 0x88);
        cue(0x105, 0x8C);  // Original: Bild 261 ein drittes Mal
        cueEx(0x163, 0xB8, 2, 0x3C, 2000, 0, 0);
        cue(400, 0xB0);
        cue(0x194, 0xB0);
        cue(0x19D, 0xB0);
        cueEx(0x1D9, 0xA4, 0, 1, 0, 0, 0);
        cue(0x23A, 0x88);
        cue(0x23F, 0x8C);
        cue(0x25F, 0x98);
        cueEx(0x26A, 0xB8, 0, 1, 1000, 0, 0);
    }
    music_ = playMusic("Loc09_SmashvilleAttack");
    if (music_) {
        music_->setReverb(0x10, f32(0x3DE353F8), f32(0x40311687), f32(0x3E656042));
        music_->setReverbMix(f32(0x3DCCCCCD));  // 0x004375B0
    }
}

void Loc09::destroyLocal() {
    anim_.reset();
    for (auto& s : sfx_) s.reset();
}

void Loc09::start() {
    if (music_) music_->play();  // vtable +0x08
    setEventProc(&Loc09::onEvent);
    if (!anim_) return;  // Port: ohne AVI würde das Original abstürzen
    scene_.setModal(anim_.get());
    anim_->playEndEvent(10, kDone);
    anim_->setFrameEvent(kFrame, 0x105);
    if (AudioBuffer* ocean = sfx(0xA4)) ocean->playLoop();  // vtable +0x10
    flag(0x3E80) = 1;  // Nacht (Loc15 u. a.)
    flag(0x3F04) = 1;  // Gitter in der Kanalisation wieder unten (Loc07)
    flag(0x3F48) = 0;
    flag(0x3F54) = 4;  // Loc10: Stufe nach dem Angriff (UNVERIFIED Bedeutung)
}

void Loc09::onEvent(Event& e, int) {
    if (e.param == kDone) {
        flag(0x3E80) = 1;
        changeScene(0x5A, 0x0E);  // → Loc14, Szene 90
        return;
    }
    if (e.param != kFrame) {
        std::fprintf(stderr, "Loc09: Unknown_Event____ %d\n", e.param);  // Original: MessageBoxA
        return;
    }
    int next = 0;
    switch (anim_ ? anim_->frame : 0) {
    case 0x105:
        if (music_) {
            music_->setReverb(10, f32(0x3F000000), f32(0x41200000), f32(0x3E99999A));
            music_->setReverbMix(f32(0x3DCCCCCD));
        }
        say("Loc09", "084_ROD_01");
        next = 0x123;
        break;
    case 0x123:
        say("Loc09", "084_ROD_02");
        next = 0x14B;
        break;
    case 0x14B:
        say("Loc09", "084_ROD_03");
        next = 0x152;
        break;
    case 0x152:
        if (music_) {
            music_->setReverb(0x10, f32(0x3DE353F8), f32(0x40311687), f32(0x3E656042));
            music_->setReverbMix(f32(0x3DCCCCCD));
        }
        return;
    default:
        return;
    }
    anim_->setFrameEvent(kFrame, next);
}

}  // namespace rah
