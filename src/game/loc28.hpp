// SPDX-License-Identifier: GPL-3.0-or-later
// Loc28 – Endavin: Stadttor (Szene 31), Gasse (Szene 32), Kerkerfenster (Szene 35), docs/locations/loc28.md
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc28 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0042C6C0 Loc28_ctor
    Loc28(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0042CD10
    ~Loc28() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x0042CE30 Loc28_Start
    void start() override;

private:
    void destroyLocal() override;
    void start31();
    void start32();
    // orig: Rent-A-Hero.exe 0x0042D940 – Ereignisse Szene 31
    void onEvent31(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x0042E570 – Ereignisse Szene 32
    void onEvent32(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x0042EA20 – Ereignisse Szene 35
    void onEvent35(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x0042E420 – im Busch Piratenkleidung an- oder ausziehen
    void toggleDisguise();
    // 0x0042CE30: ein Fremder nennt der Wache die Parole
    void strangerSequence();
    void gateOrBush();
    bool hasAllItems();
    bool walk(int x, int y, int dir);
    bool playEnd(AnimLayer* a, unsigned mode);
    bool playRange(AnimLayer* a, unsigned mode, int from, int to);
    bool talk(const char* key, AnimLayer* speaker, int from = 0, int to = 8);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    // Szene 31
    std::unique_ptr<AnimLayer> enterCity_;  // +0x7C S31_EnterCity
    AnimLayer* lid_ = nullptr;              // +0x80 S31_Deckel
    AnimLayer* freArrives_ = nullptr;       // +0x84
    AnimLayer* freEnters_ = nullptr;        // +0x88
    AnimLayer* freParole_ = nullptr;        // +0x8C
    AnimLayer* freTalk_ = nullptr;          // +0x90
    AnimLayer* guard31_ = nullptr;          // +0x94 S31_Wache
    AnimLayer* jasClimbsOut_ = nullptr;     // +0x98
    AnimLayer* pirGlider_ = nullptr;        // +0x9C
    AnimLayer* rodBushDownUp_ = nullptr;    // +0xA0 S31_RodBuschRunterRauf
    AnimLayer* rodBushTalk_ = nullptr;      // +0xA4
    AnimLayer* rodClimbsOut_ = nullptr;     // +0xA8
    AnimLayer* rodEntersBush_ = nullptr;    // +0xAC
    AnimLayer* rodGlider_ = nullptr;        // +0xB0
    AnimLayer* rodLeavesBush_ = nullptr;    // +0xB4
    AnimLayer* pirBushDownUp_ = nullptr;    // +0xB8 S31_RodPirBushRunterRauf
    AnimLayer* pirBushTalk_ = nullptr;      // +0xBC
    AnimLayer* pirEntersBush_ = nullptr;    // +0xC0
    AnimLayer* pirLeavesBush_ = nullptr;    // +0xC4
    AnimLayer* pirParole_ = nullptr;        // +0xC8
    Area* to30_ = nullptr;                  // +0xCC S31_To30_113
    Area* to32_ = nullptr;                  // +0xD0
    Area* wall_ = nullptr;                  // +0xD4
    std::unique_ptr<Event> ambientTimer_;   // +0xD8
    std::unique_ptr<AudioBuffer> glider_, glider3_, bird_, bird2_, bird3_, chirp_, winding_, stepL_, stepR_;  // +0xDC…+0xFC
    // Szene 32
    AnimLayer* rodRummages_ = nullptr;      // +0x100 S32_RodKramtAnim
    AnimLayer* guard32_ = nullptr;          // +0x104 S32_Wache
    Area* areas32_[12] = {};                // +0x108…+0x134: To31, Wagen, FensterL/R, Kneipe, Treppen, SchuttL/R, FassL/R, Kerker
    // Szene 35
    AnimLayer* stranger_ = nullptr;         // +0x134 S35_Fremder
    AnimLayer* captain_ = nullptr;          // +0x138 S35_Captain
    Layer* window_ = nullptr;               // +0x13C
    Area* to34_ = nullptr;                  // +0x140
    Area* to34b_ = nullptr;                 // +0x144
    std::unique_ptr<Event> talkTimer_;      // +0x148
};

}  // namespace rah
