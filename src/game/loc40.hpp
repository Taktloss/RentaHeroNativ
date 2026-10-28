// SPDX-License-Identifier: GPL-3.0-or-later
// Loc40 – Intro, Szenen 114/115/116 (docs/locations/loc40.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc40 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00431920 Loc40_ctor
    Loc40(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00432A40 Loc40_dtor
    ~Loc40() override;
    // orig: Rent-A-Hero.exe 0x00432CA0 Loc40_Start
    void start() override;
    AnimLayer* anim() { return anim_.get(); }

private:
    // orig: Rent-A-Hero.exe 0x00432EF0 Loc40_OnEvent114
    void onEvent114(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x004330D0 Loc40_OnEvent115
    void onEvent115(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x00433100 Loc40_OnEvent116
    void onEvent116(Event& e, int type);

    std::unique_ptr<AnimLayer> anim_;  // +0x7C
};

}  // namespace rah
