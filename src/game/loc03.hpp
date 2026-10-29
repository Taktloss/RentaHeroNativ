// SPDX-License-Identifier: GPL-3.0-or-later
// Loc03 – Brücke vor Smashville (Szene 12) (docs/locations/loc03.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc03 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0040A4E0 Loc03_ctor
    Loc03(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0040A880
    ~Loc03() override;
    // orig: Rent-A-Hero.exe 0x0040A9C0 Loc03_Start
    void start() override;

    bool guardsLoaded() const { return guardTimer_ != nullptr; }

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x0040AE30 Loc03_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x0040ADC0 – Wachen-Timer neu (2–5 s)
    void restartGuardTimer();
    // orig: Rent-A-Hero.exe 0x0040AE10 – Wachen-Timer anhalten
    void stopGuardTimer();
    // Gespräch mit den Wachen und Auftritt der Säbelfrau (Ereignisse 6, 8, 9)
    void confrontGuards();
    bool playWait(AnimLayer* a, unsigned mode, int from, int to);
    void guardAreas(bool on);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<AnimLayer> pirateShip_;   // +0x7C S12_PirateShip (nur von Szene 15)
    std::unique_ptr<AnimLayer> takeOff_;      // +0x80 S12_Wegflug
    AnimLayer* approach_ = nullptr;           // +0x84 S12_Anflug
    AnimLayer* rab1Run_ = nullptr;            // +0x88
    AnimLayer* rab1Talk_ = nullptr;           // +0x8C
    AnimLayer* rab2_ = nullptr;               // +0x90
    AnimLayer* rodGlider_ = nullptr;          // +0x94 S12_RodGlider
    AnimLayer* sabInGlider_ = nullptr;        // +0x98
    AnimLayer* sabSword_ = nullptr;           // +0x9C
    AnimLayer* sabGlider_ = nullptr;          // +0xA0
    AnimLayer* sabWalk_ = nullptr;            // +0xA4 SabWalk
    Area* bridge_ = nullptr;                  // +0xA8
    Area* sign_ = nullptr;                    // +0xAC
    Area* to13_ = nullptr;                    // +0xB0
    Area* plant_ = nullptr;                   // +0xB4
    std::unique_ptr<Event> guardTimer_;       // +0xB8 (nur solange 0x3ECC == 0)
    std::unique_ptr<Event> ambientTimer_;     // +0xBC
    std::unique_ptr<AudioBuffer> ra2Line_;    // +0xC0 012_RA2_04
    std::unique_ptr<AudioBuffer> gliderPass_; // +0xC4
    std::unique_ptr<AudioBuffer> stepLeft_, stepRight_;      // +0xC8, +0xCC
    std::unique_ptr<AudioBuffer> swordPush_, swordPull_;     // +0xD0, +0xD4
    std::unique_ptr<AudioBuffer> bird_, bird3_, gliderPass3_, wind_, hack_;  // +0xD8…+0xE8
};

}  // namespace rah
