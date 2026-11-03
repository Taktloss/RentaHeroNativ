// SPDX-License-Identifier: GPL-3.0-or-later
// Loc26 – Insel des Megophias: Strand mit Drachen (Szene 93) und Grube (Szene 98), docs/locations/loc26.md
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc26 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0042A070 Loc26_ctor
    Loc26(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0042A3C0
    ~Loc26() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x0042A490 Loc26_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x0042A8E0 – Ereignisse Szene 93
    void onEvent93(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x0042AC00 – Ereignisse Szene 98
    void onEvent98(Event& e, int type);
    void commentHere(const char* key);
    bool walk(int x, int y, int dir);
    bool walk2(int x, int y, int dir);
    bool playRange(AnimLayer* a, unsigned mode, int from, int to);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<Event> ambientTimer_;  // +0x7C
    std::unique_ptr<AudioBuffer> seagull_, wind_, wind2_, ocean_, wings_;  // +0x80…+0x90
    AnimLayer* seaLO_ = nullptr;           // +0x94 S93_MeerLO
    AnimLayer* seaLU_ = nullptr;           // +0x98
    AnimLayer* seaRO_ = nullptr;           // +0x9C
    AnimLayer* takeoff_ = nullptr;         // +0xA0 S93_DragonTakeoff
    AnimLayer* breath_ = nullptr;          // +0xA4 S93_DragonBreath
    AnimLayer* landing_ = nullptr;         // +0xA8 S93_DragonLanding
    Area* hut_ = nullptr;                  // +0xAC S93_Huette
    Area* sea_ = nullptr;                  // +0xB0 S93_Meer
    Area* forest_ = nullptr;               // +0xB4 S93_Wald
    Area* to98_ = nullptr;                 // +0xB8
    Area* dragon_ = nullptr;               // +0xBC S93_Dragon
    std::unique_ptr<Event> breathTimer_;   // +0xC0
    AnimLayer* pickUp_ = nullptr;          // +0xC4 Gen_RodPickUp
    AnimLayer* flag98_ = nullptr;          // +0xC8 S98_Fahne
    AnimLayer* seaL_ = nullptr;            // +0xCC S98_MeerL
    AnimLayer* seaR_ = nullptr;            // +0xD0 S98_MeerR
    Area* to93_ = nullptr;                 // +0xD4
    Area* pit_ = nullptr;                  // +0xD8 S98_Grube
};

}  // namespace rah
