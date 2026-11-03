// SPDX-License-Identifier: GPL-3.0-or-later
// Loc17 – Aasfresser-Lichtung (Szene 59) und Totenschädel (Szene 60), docs/locations/loc17.md
#pragma once

#include <array>

#include "game/game.hpp"

namespace rah {

class Loc17 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0041CF00 Loc17_ctor
    Loc17(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0041D350
    ~Loc17() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x0041D480 Loc17_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x0041D9F0 Loc17_OnEvent
    void onEvent(Event& e, int type);
    void start59();
    void start60();
    void showForeground60();
    bool walk(int x, int y, int dir);
    bool playRange(AnimLayer* a, unsigned mode, int from, int to);
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::array<AnimLayer*, 7> vultures_{};  // +0x7C Aasfresser1…7
    std::array<int, 7> homeX_{};            // +0x98 Startpositionen (Szene 59, __ftol)
    std::array<int, 7> homeY_{};            // +0xB4
    Area* to58_ = nullptr;                  // +0xD0
    Area* skeleton_ = nullptr;              // +0xD4
    Area* wall_ = nullptr;                  // +0xD8
    Area* head_ = nullptr;                  // +0xDC
    AnimLayer* bushAway_ = nullptr;         // +0xE0 S60_BuschWeg
    Layer* bush_ = nullptr;                 // +0xE4 S60_Busch
    Layer* leafLU_ = nullptr;               // +0xE8
    Layer* leafRO_ = nullptr;               // +0xEC
    Layer* light_ = nullptr;                // +0xF0
    AnimLayer* splash_ = nullptr;           // +0xF4 S60_Spritz
    std::unique_ptr<AnimLayer> rodOut_;     // +0xF8 S60_RodRaus
    std::unique_ptr<AnimLayer> tryBush_;    // +0xFC S60_TryOpenSkullMitBusch
    std::unique_ptr<AnimLayer> tryNoBush_;  // +0x100 S60_TryOpenSkullOhneBusch
    Area* to59_ = nullptr;                  // +0x104
    Area* skull_ = nullptr;                 // +0x108
    Area* mouth_ = nullptr;                 // +0x10C
    Area* ear_ = nullptr;                   // +0x110
    std::unique_ptr<AudioBuffer> smack_, burst_, ooh_, tableHit_, knock_;  // +0x114…+0x124
    std::unique_ptr<AudioBuffer> bird_, bird2_, bird3_, chirp_;            // +0x128…+0x134
    std::unique_ptr<Event> ambientTimer_;   // +0x138
};

}  // namespace rah
