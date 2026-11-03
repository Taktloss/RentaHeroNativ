// SPDX-License-Identifier: GPL-3.0-or-later
// Loc24 – Kapitänskajüte: Sabrina gegen den Kapitän, Szene 103 (docs/locations/loc24.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc24 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00427C10 Loc24_ctor
    Loc24(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00428180
    ~Loc24() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x004282D0 Loc24_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x00428820 Loc24_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x00428DB0 – nächste Runde des Kampfes je nach +0x110
    void fightRound();
    // orig: Rent-A-Hero.exe 0x00428F90 – Sabrina-Ebenen aus
    void hideSabrina();
    // orig: Rent-A-Hero.exe 0x00428FD0 – Kapitän-Ebenen aus
    void hideCaptain();
    void playFight(AnimLayer* a, int ev, int from, int to);
    void shiftFighters();
    bool walk2(int x, int y, int dir);
    bool moveWait(Layer* l, double x, double y, double z, double zoom, int steps);
    bool playRange(AnimLayer* a, unsigned mode, int from, int to);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    Layer* table_ = nullptr;              // +0x7C S103_Tisch
    AnimLayer* capComes_ = nullptr;       // +0x80
    AnimLayer* capAttack1_ = nullptr;     // +0x84
    AnimLayer* capAttack2_ = nullptr;     // +0x88
    AnimLayer* capAttack3_ = nullptr;     // +0x8C
    AnimLayer* capCameTalk_ = nullptr;    // +0x90
    AnimLayer* capCovers_ = nullptr;      // +0x94 S103_CapDecktUnten
    AnimLayer* rodTakesHelm_ = nullptr;   // +0x98
    Layer* helm_ = nullptr;               // +0x9C S103_Helm
    AnimLayer* sabAttack2_ = nullptr;     // +0xA0
    AnimLayer* sabCovers1_ = nullptr;     // +0xA4 S103_SabDeckt1
    AnimLayer* sabCovers2_ = nullptr;     // +0xA8 S103_SabDeckt2
    AnimLayer* sabDrawsSword_ = nullptr;  // +0xAC
    Layer* bottle_ = nullptr;             // +0xB0 S103_Flasche
    AnimLayer* rodPickUp_ = nullptr;      // +0xB4
    AnimLayer* sabWalk_ = nullptr;        // +0xB8 SabWalk
    AnimLayer* steam_ = nullptr;          // +0xBC Gen_SmallSteam
    Area* oven_ = nullptr;                // +0xC0 S103_Ofen
    Area* lever_ = nullptr;               // +0xC4 S103_Hebel
    Area* instruments_ = nullptr;         // +0xC8 S103_Instrumente
    Area* to102_ = nullptr;               // +0xCC
    Area* swords_ = nullptr;              // +0xD0 S103_Swords
    std::unique_ptr<Event> ambientTimer_; // +0xD4
    std::unique_ptr<AudioBuffer> sword1_, sword2_, swordPull_, workFix_, stepL_, stepR_, stepCyber_;  // +0xD8…+0xF0
    std::unique_ptr<AudioBuffer> ocean_, seagull_, woosh_, thunder_, thunder2_, danger_, piano_;     // +0xF4…+0x10C
    int round_ = 0;                       // +0x110 Kampfrunde: 0, 1, 2 zufällig; 3 Rodrigo hat den Helm
};

}  // namespace rah
