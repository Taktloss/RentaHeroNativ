// SPDX-License-Identifier: GPL-3.0-or-later
// Loc41 – Schlusssequenz und Abspann, Szene 112 (docs/locations/loc41.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc41 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x004332A0 Loc41_ctor
    Loc41(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00433550
    ~Loc41() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x00433610 Loc41_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x004336E0 Loc41_OnEvent – Musikwechsel nach Bildnummern
    void onEvent(Event& e, int type);
    void switchMusic(std::string_view name, bool fade);

    AnimLayer* ending_ = nullptr;     // +0x7C S112_EndeSequenz
    AnimLayer* seaFadeOut_ = nullptr; // +0x80 S112_MeerFadeOut
    AnimLayer* seaLoop_ = nullptr;    // +0x84 S112_MeerLoop
    AnimLayer* credits_ = nullptr;    // +0x88 S112_Credits
    std::unique_ptr<AudioBuffer> explosion_, thunder_, thunder2_, bird3_, bird2_, bird_, chirp_;  // +0x8C…+0xA4
};

}  // namespace rah
