// SPDX-License-Identifier: GPL-3.0-or-later
// Loc04 – Rodrigos Büro (Szene 6) mit Ramil (docs/locations/loc04.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc04 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0040B8C0 Loc04_ctor
    Loc04(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0040BE80
    ~Loc04() override;
    // orig: Rent-A-Hero.exe 0x0040C010 Loc04_Start
    void start() override;

    bool atDesk() const { return atDesk_ != 0; }
    int ramTalkStage() const { return ramStage_; }

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x0040C570 Loc04_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x0040CF00 – Kommentar am Schreibtisch bzw. hingehen
    void deskComment(std::string_view key);
    // orig: Rent-A-Hero.exe 0x0040CF60 – Ansicht wechseln (Hintergrund, Tiefenkarte, Bereiche)
    void showView(Layer* bg, int desk);
    // orig: Rent-A-Hero.exe 0x0040D180 – an den Schreibtisch setzen (0: Nahaufnahme, 1: Tisch, 2: Ramil)
    void sitDown(int mode);
    // orig: Rent-A-Hero.exe 0x0040D300 – aufstehen
    void standUp();
    // orig: Rent-A-Hero.exe 0x0040D3B0 – hingehen und kommentieren bzw. vom Tisch aus
    void comment3(int x, int y, int dir, std::string_view key);
    // orig: Rent-A-Hero.exe 0x0040D400 – Rodrigo spricht (0: Nahaufnahme, 1: Tisch, 2: mit Ramil)
    void rodLine(std::string_view key, int view);
    // orig: Rent-A-Hero.exe 0x0040D4E0 – Ramil spricht
    void ramLine(std::string_view key);
    // Held als Ebene durch den Raum bewegen (Eintreten/Hinausgehen)
    bool playWait(AnimLayer* a, unsigned mode, int from, int to);
    void playSfx(AudioBuffer* b) {
        if (b) b->play();
    }
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<AnimLayer> rodIn_, rodOut_, rodSit_;            // +0x7C, +0x80, +0x84
    std::unique_ptr<AnimLayer> ramilIn_, ramilOut_, ramilMoney_;    // +0x88, +0x8C, +0x90
    AnimLayer* rodTalk_ = nullptr;      // +0x94 RodTalk (Nahaufnahme)
    AnimLayer* rodDeskTalk_ = nullptr;  // +0x98 RodDeskTalk
    AnimLayer* rodTake_ = nullptr;      // +0x9C RodTake
    AnimLayer* ramTalk_ = nullptr;      // +0xA0 RamTalk
    AnimLayer* rodSitLayer_ = nullptr;  // +0xA4 RodSetzenLayer
    AnimLayer* rodSitAct1_ = nullptr;   // +0xA8 RodSitAct1
    AnimLayer* rodSitAct2_ = nullptr;   // +0xAC RodSitAct2
    Area* desktop_ = nullptr;           // +0xB0
    Area* boar_ = nullptr;              // +0xB4 006_Eber
    Area* box_ = nullptr;               // +0xB8 006_Kiste
    Area* elephant_ = nullptr;          // +0xBC
    Area* cabinet_ = nullptr;           // +0xC0 006_Kasten
    Area* door_ = nullptr;              // +0xC4
    Area* doorRamil_ = nullptr;         // +0xC8
    Area* newsRamil_ = nullptr;         // +0xCC
    Area* barrelRamil_ = nullptr;       // +0xD0
    Area* barrel_ = nullptr;            // +0xD4 006_Fass
    Area* news_ = nullptr;              // +0xD8
    Area* drawer2_ = nullptr;           // +0xDC
    Area* drawer3_ = nullptr;           // +0xE0
    Area* drawer4_ = nullptr;           // +0xE4
    std::unique_ptr<AudioBuffer> rummage_, moneyBag_, doorCreak_, tock_, stepLeft_, stepRight_, tockSoft_;  // +0xE8…+0x100
    int atDesk_ = 0;                    // +0x104
    int ramilShown_ = 0;                // +0x108
    int boarToggle_ = 0;                // +0x10C
    int desktopToggle_ = 0;             // +0x110
    int elephantToggle_ = 0;            // +0x114
    int ramStage_ = 0;                  // +0x118
    Layer* bgMain_ = nullptr;           // +0x11C
    Layer* bgRodTalk_ = nullptr;        // +0x120
    Layer* bgRamTalk_ = nullptr;        // +0x124
    ZMap* depthRam_ = nullptr;          // +0x128
    ZMap* depthMain_ = nullptr;         // +0x12C
};

}  // namespace rah
