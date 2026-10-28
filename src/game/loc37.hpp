// SPDX-License-Identifier: GPL-3.0-or-later
// Loc37 – Landung vor der Höhle, Szene 3 (docs/locations/loc35-37.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc37 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00431260 Loc37_ctor
    Loc37(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00431470
    ~Loc37() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x00431540 Loc37_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x004316F0 Loc37_OnEvent
    void onEvent(Event& e, int type);

    AnimLayer* landing_ = nullptr;  // +0x7C S3_Landen
    AnimLayer* pointing_ = nullptr; // +0x80 S3_Zeigen
    Layer* glider_ = nullptr;       // +0x84 S3_Gleiter
    Area* cave_ = nullptr;          // +0x88 S3_Hoehle
    std::unique_ptr<Event> nagTimer_, ambientTimer_;  // +0x8C, +0x90
    std::unique_ptr<AudioBuffer> passing_, bird_, bird2_, bird3_, chirp_;  // +0x94…+0xA4
    bool commenting_ = false;       // +0xA8 Ereignis 2 läuft
};

}  // namespace rah
