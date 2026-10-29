// SPDX-License-Identifier: GPL-3.0-or-later
// Loc12 – Brunnen vor Cynthias Haus, Szene 91 (docs/locations/loc12.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc12 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00418700 Loc12_ctor
    Loc12(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00418900
    ~Loc12() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x004189E0 Loc12_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x00418BF0 Loc12_OnEvent
    void onEvent(Event& e, int type);
    // Rodrigo beugt sich über den Brunnen, zwei Sätze, zurück (Muster in 0x00418BF0 Fall 4/7)
    void lookIntoWell(const char* first, const char* second, std::uint32_t pause);
    void wellDone();
    bool walk(int x, int y, int dir);
    bool playEnd(AnimLayer* a, unsigned mode);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    Layer* well_ = nullptr;                    // +0x7C S91_Brunnen
    AnimLayer* shake1_ = nullptr;              // +0x80 S91_RodRuettel1
    AnimLayer* shake2_ = nullptr;              // +0x84 S91_RodRuettel2
    AnimLayer* wellIn_ = nullptr;              // +0x88 S91_RodBrunnenHin
    AnimLayer* wellOut_ = nullptr;             // +0x8C S91_RodBrunnenWeg
    std::unique_ptr<AnimLayer> outCam_;        // +0x90 S91_RodRausCam
    std::unique_ptr<AnimLayer> inCam_;         // +0x94 S91_RodReinCam
    std::unique_ptr<Event> ambientTimer_;      // +0x98
    std::unique_ptr<AudioBuffer> owl_, thunder_, thunder2_, moneyBag_, bubble_;  // +0x9C…+0xAC
};

}  // namespace rah
