// SPDX-License-Identifier: GPL-3.0-or-later
// Loc18 – im Totenschädel, Szene 61 (docs/locations/loc18.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc18 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0041E4F0 Loc18_ctor
    Loc18(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0041E840
    ~Loc18() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x0041E960 Loc18_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x0041EC80 Loc18_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x0041EB20 – Rauch-Timer neu anlegen, 1 s → Ereignis 6
    void startSmoke();
    // orig: Rent-A-Hero.exe 0x0041EBC0 – Rauch-Timer (falls vorhanden) 4 s → Ereignis 6
    void rearmSmoke();
    // orig: Rent-A-Hero.exe 0x0041EBF0 – Rauch-Timer anhalten und löschen
    void stopSmoke();
    // orig: Rent-A-Hero.exe 0x0041EC40 – hinausklettern → 17/60
    void climbOut();
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<AnimLayer> climbIn_;   // +0x7C S61_ClimbIn
    std::unique_ptr<AnimLayer> climbOut_;  // +0x80 S61_ClimbOut
    std::unique_ptr<AnimLayer> openJaws_;  // +0x84 S61_OpenJaws
    std::unique_ptr<AnimLayer> strew_;     // +0x88 S61_Streu
    Layer* mushroomL_ = nullptr;           // +0x8C S61_PilzLinks
    Layer* mushroomR_ = nullptr;           // +0x90 S61_PilzRechts
    AnimLayer* smoke_ = nullptr;           // +0x94 S61_Smoke
    AnimLayer* smokeClone_ = nullptr;      // +0x98 S61_SmokeClone
    Area* to60_ = nullptr;                 // +0x9C
    Area* brain_ = nullptr;                // +0xA0
    Area* to60b_ = nullptr;                // +0xA4 S61_To60_2 (Kiefer)
    std::unique_ptr<Event> smokeTimer_;    // +0xA8
    std::unique_ptr<Event> ambientTimer_;  // +0xAC
    std::unique_ptr<AudioBuffer> acid_, leaves_, fly1_, fly2_, clothes_, creak_, stepL_, stepR_;  // +0xB0…+0xCC
    bool smokeRunning_ = false;            // +0xD0
    bool cloneRunning_ = false;            // +0xD4
    bool leaveWhenDone_ = false;           // +0xD8
    int nextSmoke_ = 1;                    // +0xDC (Ctor 1)
};

}  // namespace rah
