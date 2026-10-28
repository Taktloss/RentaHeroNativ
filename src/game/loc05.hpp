// SPDX-License-Identifier: GPL-3.0-or-later
// Loc05 – Louis Büro (Szene 10) (docs/locations/loc05.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc05 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0040D530 Loc05_ctor
    Loc05(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0040D7D0
    ~Loc05() override;
    // orig: Rent-A-Hero.exe 0x0040D890 Loc05_Start
    void start() override;

    AnimLayer* louiTalk() { return louiTalk_; }

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x0040DB30 Loc05_OnGenericEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x0040E180 – Rodrigo vor Louis Tisch
    void stepToDesk();
    // orig: Rent-A-Hero.exe 0x0040E1B0 – Loui spricht
    void louiLine(std::string_view key);
    // orig: Rent-A-Hero.exe 0x0040E200 – Rodrigo, dann Loui
    void exchange(std::string_view rod, std::string_view lou);
    // orig: Rent-A-Hero.exe 0x0040E260 – Rodrigo, dann Loui beim Gang durchs Büro
    void walkExchange(std::string_view rod, std::string_view lou);
    // orig: Rent-A-Hero.exe 0x0040E2A0 – Louis Haltung (1/2 Übergang, 3/4 Zielhaltung, jeweils
    // ohne/mit Warten; 5 Zufallswechsel per Timer, 6 Timer neu)
    void pose(int mode, int arg);
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    AnimLayer* doorOpen_ = nullptr;           // +0x7C S10_DoorOpen
    AnimLayer* louiTalk_ = nullptr;           // +0x80 S10_LouiTalk
    AnimLayer* louiWalk_ = nullptr;           // +0x84 S10_LouiWalk
    std::unique_ptr<Event> poseTimer_;        // +0x88
    std::unique_ptr<AudioBuffer> doorCreak_;  // +0x8C
    std::unique_ptr<AudioBuffer> stepLeft_, stepRight_, splash_, tock_;  // +0x90…+0x9C
};

}  // namespace rah
