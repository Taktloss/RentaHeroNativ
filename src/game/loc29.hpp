// SPDX-License-Identifier: GPL-3.0-or-later
// Loc29 – Kneipe in Endavin, Szene 33 (docs/locations/loc29.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc29 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0042EE50 Loc29_ctor
    Loc29(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0042F030
    ~Loc29() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x0042F0E0 Loc29_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x0042F290 Loc29_OnEvent
    void onEvent(Event& e, int type);
    bool talk(const char* key, AnimLayer* speaker, int from, int to);
    bool playRange(AnimLayer* a, int from, int to);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<AnimLayer> enter_;      // +0x7C S33_Enter
    std::unique_ptr<AnimLayer> drinkLook_;  // +0x80 S33_RodKipptAndLook
    AnimLayer* pirate1_ = nullptr;          // +0x84
    Layer* jug_ = nullptr;                  // +0x88 S33_Krug1 (TCC)
    AnimLayer* pirate2_ = nullptr;          // +0x8C
    AnimLayer* pirate3_ = nullptr;          // +0x90
    AnimLayer* rodLeaves_ = nullptr;        // +0x94 S33_RodGeht
    AnimLayer* rodrigo_ = nullptr;          // +0x98 S33_Rodrigo
    Area* to32_ = nullptr;                  // +0x9C
    std::unique_ptr<AudioBuffer> drinking_, knock_, tock_, piano_;  // +0xA0…+0xAC
};

}  // namespace rah
