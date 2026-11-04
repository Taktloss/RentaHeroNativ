// SPDX-License-Identifier: GPL-3.0-or-later
// Loc30 – Kerker in Endavin, Szene 34 (docs/locations/loc30.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc30 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0042F750 Loc30_ctor
    Loc30(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0042F950
    ~Loc30() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x0042FA00 Loc30_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x0042FC70 Loc30_OnEvent
    void onEvent(Event& e, int type);
    void talkJasper();
    void jasSay(const char* key);
    bool walk(int x, int y, int dir);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<AnimLayer> rodComesIn_;  // +0x7C S34_RodReinCam
    AnimLayer* jasGetsUp_ = nullptr;         // +0x80 S34_JasAufsteh
    AnimLayer* jasTalk_ = nullptr;           // +0x84 S34_JasStehTalk
    AnimLayer* rodKnock_ = nullptr;          // +0x88
    Layer* straw_ = nullptr;                 // +0x8C S34_Straw (TCC)
    Layer* gridOpen_ = nullptr;              // +0x90 S34_GridOpen (TCC, ungenutzt)
    std::unique_ptr<AudioBuffer> winding_, tock_, boxPushed_, knock_, clothes_;  // +0x94…+0xA4
};

}  // namespace rah
