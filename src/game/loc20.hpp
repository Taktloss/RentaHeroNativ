// SPDX-License-Identifier: GPL-3.0-or-later
// Loc20 – Bibliothek hinter dem Zwergentor mit Tharain, Szene 70 (docs/locations/loc20.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc20 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x004223E0 Loc20_ctor
    Loc20(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00422630
    ~Loc20() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x00422690 Loc20_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x00422930 Loc20_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x00422D50 – Tharain spricht (Bilder 15–45) und warten
    void tharainSay(const char* key);
    bool walk(int x, int y, int dir);
    bool playEnd(AnimLayer* a, unsigned mode);
    bool playRange(AnimLayer* a, unsigned mode, int from, int to);
    void turn(int dir);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<AudioBuffer> mobileDown_;  // +0x7C Sfx_MobileDown
    Layer* desk_ = nullptr;                    // +0x80 S70_Pult
    Layer* book_ = nullptr;                    // +0x84 S70_Book
    AnimLayer* rodCatch_ = nullptr;            // +0x88 S70_RodCatchStone
    AnimLayer* tharain_ = nullptr;             // +0x8C S70_Tharain
    AnimLayer* stoneFlies_ = nullptr;          // +0x90 S70_StoneFlies
    AnimLayer* thaTakes_ = nullptr;            // +0x94 S70_ThaTakesStone
    Layer* stone_ = nullptr;                   // +0x98 S70_Stone
    Layer* colors_[6] = {};                    // +0x9C…+0xB0 Blue, Cyan, Green, Red, Violet, Yellow
    Area* to69_ = nullptr;                     // +0xB4
    Area* floor_ = nullptr;                    // +0xB8 S70_Boden
};

}  // namespace rah
