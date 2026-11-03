// SPDX-License-Identifier: GPL-3.0-or-later
// Loc09 – Zwischensequenz „Angriff auf Smashville“, Szene 89 (docs/locations/loc09.md)
#pragma once

#include <array>

#include "game/game.hpp"

namespace rah {

class Loc09 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x004134B0 Loc09_ctor
    Loc09(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00413C70
    ~Loc09() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x00413DC0 Loc09_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x00413E70 Loc09_OnEvent
    void onEvent(Event& e, int type);
    AudioBuffer* sfx(int off) { return sfx_[static_cast<std::size_t>((off - 0x80) / 4)].get(); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }
    void cue(int frame, int off);
    void cueEx(int frame, int off, int action, int vol, int volMs, float x, float z);

    std::unique_ptr<AnimLayer> anim_;                     // +0x7C Attack
    std::array<std::unique_ptr<AudioBuffer>, 15> sfx_{};  // +0x80 … +0xB8
};

}  // namespace rah
