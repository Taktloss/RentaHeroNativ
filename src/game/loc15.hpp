// SPDX-License-Identifier: GPL-3.0-or-later
// Loc15 – Schlucht am Kanal, Szene 57 (docs/locations/loc15.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc15 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0041A330 Loc15_ctor
    Loc15(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0041A780
    ~Loc15() override;
    // orig: Rent-A-Hero.exe 0x0041A8B0 Loc15_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x0041ACF0 Loc15_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x0041B200 – Gleiter schaukelt (y 207 ↔ 209, je 2 Schritte)
    void bobGlider();
    bool walk(int x, int y, int dir);
    bool playEnd(AnimLayer* a, unsigned mode);
    bool playRange(AnimLayer* a, unsigned mode, int from, int to);
    bool moveWait(Layer* l, double x, double y, int steps);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<Event> ambientTimer_;                    // +0x7C
    std::unique_ptr<AudioBuffer> bird_, bird2_, bird3_, chirp_;  // +0x80…+0x8C (Tag)
    std::unique_ptr<AudioBuffer> owl_, thunder_, thunder2_;  // +0x90…+0x98 (Nacht)
    std::unique_ptr<AudioBuffer> passing_;                   // +0x9C Sfx_Glider_PassingBy
    std::unique_ptr<AudioBuffer> danger_;                    // +0xA0 Sfx_DangerStringsLow
    std::unique_ptr<AudioBuffer> scratch_;                   // +0xA4 Sfx_Clothes_Scratched
    AnimLayer* rodIn_ = nullptr;         // +0xA8 S57_RodRein_vd/_nd
    AnimLayer* gliderGone_ = nullptr;    // +0xAC S57_GleiterWeg
    AnimLayer* rodDismount_ = nullptr;   // +0xB0 S57_RodAbsteig
    AnimLayer* rodOut_ = nullptr;        // +0xB4 S57_RodRaus_vd/_nd
    Layer* glider_ = nullptr;            // +0xB8 S57_Gleiter
    Layer* gliderArrives_ = nullptr;     // +0xBC S57_GleiterHin
    AnimLayer* rodLooks_ = nullptr;      // +0xC0 S57_RodSchaut
    AnimLayer* pirate_ = nullptr;        // +0xC4 S57_PiratGuck
    AnimLayer* pickUp_ = nullptr;        // +0xC8 Gen_RodPickUp (Nacht)
    Layer* sack_ = nullptr;              // +0xCC S57_Sack (Nacht)
    Area* kanal_ = nullptr;              // +0xD0
    Area* to38_ = nullptr;               // +0xD4
    Area* to58_ = nullptr;               // +0xD8
    Area* gorge_ = nullptr;              // +0xDC S57_Schlucht
    int bob_ = 0;                        // +0xE0 (0/1 schaukeln, 2 aus)
};

}  // namespace rah
