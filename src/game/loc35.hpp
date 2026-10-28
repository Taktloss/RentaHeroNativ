// SPDX-License-Identifier: GPL-3.0-or-later
// Loc35 – Rodrigo auf dem Gleiter, Szene 1 (docs/locations/loc35-37.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc35 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00430A20 Loc35_ctor (setzt CGame+0x28 = 1)
    Loc35(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00430C30
    ~Loc35() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x00430CD0 Loc35_Start
    void start() override;

    int timeStage() const { return stage_; }
    int nextAction() const { return action_; }

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x00430D70 Loc35_OnEvent
    void onEvent(Event& e, int type);
    AnimLayer* obj(std::string_view name);

    AnimLayer* back_ = nullptr;          // +0x7C S1_Back
    AnimLayer* back2_ = nullptr;         // +0x80 S1_Back2
    AnimLayer* toCloseup_ = nullptr;     // +0x84 S1_BackToCloseup
    AnimLayer* fromCloseup_ = nullptr;   // +0x88 S1_CloseupToBack
    AnimLayer* checksItems_ = nullptr;   // +0x8C S1_RodChecksItems
    AnimLayer* sign_ = nullptr;          // +0x90 S1_Schild
    AnimLayer* drawsBlaster_ = nullptr;  // +0x94 S1_RodDrawsBlaster
    AnimLayer* onGlider_ = nullptr;      // +0x98 S1_RodOnGlider
    AnimLayer* closeup_ = nullptr;       // +0x9C S1_RodToCloseup (Sprecher)
    std::unique_ptr<Event> timer_;       // +0xA0
    std::unique_ptr<AudioBuffer> glider_, signSfx_, door_;  // +0xA4, +0xA8, +0xAC
    int stage_ = 1;                      // +0xB0 Zeitstufe
    int lastAction_ = 1;                 // +0xB4
    int action_ = 1;                     // +0xB8 nächste Aktion (10 = Gegenstand benutzt)
};

}  // namespace rah
