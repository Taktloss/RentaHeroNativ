// SPDX-License-Identifier: GPL-3.0-or-later
// Loc07 – Kanalisation unter Smashville, Szenen 47–56 (docs/locations/loc07.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc07 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0040F7B0 Loc07_ctor
    Loc07(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0040F9F0
    ~Loc07() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x0040FAD0 Loc07_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x00410870 Loc07_OnEvent
    void onEvent(Event& e, int type);
    // orig: 0x00410870 Ereignis 0x4A – Kurbel in Szene 52
    void crank();
    void startBack(const char* name);
    void area(const char* name, int ev);
    // Ankunft: Position, Richtung, Ziel, Endrichtung (CActor_SetPos + CActor_WalkTo2 ohne Warten)
    void arrive(int x, int y, int dir, int tx, int ty, int tdir);
    // Hingehen und bei Erfolg in die Szene wechseln
    void go(int x, int y, int dir, int scene);
    bool walk(int x, int y, int dir);
    bool playEnd(AnimLayer* a, unsigned mode);
    bool playRange(AnimLayer* a, unsigned mode, int from, int to);
    bool moveWait(Layer* l, double x, double y, int steps);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    Layer* crankL_ = nullptr;        // +0x7C 52_Kurbel
    Layer* grid_ = nullptr;          // +0x80 52_Gitter
    Layer* rope_ = nullptr;          // +0x84 52_Seil
    AnimLayer* ropeTears_ = nullptr;   // +0x88 52_SeilReisst
    Layer* sack_ = nullptr;          // +0x8C 52_Sack
    AnimLayer* pirate_ = nullptr;      // +0x90 51_PiratGuck
    std::unique_ptr<Event> bubbleTimer_;  // +0x94
    std::unique_ptr<AudioBuffer> bubble_, winding_, gridDrops_, rubberband_, fixBelt_, danger_, piano_;  // +0x98…+0xB0
};

}  // namespace rah
