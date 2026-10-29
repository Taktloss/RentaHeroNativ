// SPDX-License-Identifier: GPL-3.0-or-later
// Loc31 – Flugkarte des Gleiters, Szene 113 (docs/locations/loc31.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc31 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00430360 Loc31_ctor
    Loc31(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00430550
    ~Loc31() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x00430670 Loc31_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x00430770 Loc31_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x00430990 – Schleife beenden, Gleiter startet, Abflug-Animation
    void takeOff(AnimLayer* flight);

    std::unique_ptr<AnimLayer> to30_, to37_, to57_, to8_;  // +0x7C…+0x88
    AnimLayer* backLoop_ = nullptr;                         // +0x8C S113_BackLoop
    Area* areaTo8_ = nullptr;                               // +0x90
    Area* areaTo37_ = nullptr;                              // +0x94
    Area* areaTo57_ = nullptr;                              // +0x98
    Area* areaTo30_ = nullptr;                              // +0x9C
    std::unique_ptr<AudioBuffer> seagull_, wind_, ocean_, passing_, door_;  // +0xA0…+0xB0
    std::unique_ptr<Event> ambientTimer_;                   // +0xB4
};

}  // namespace rah
