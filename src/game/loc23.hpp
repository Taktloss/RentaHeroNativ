// SPDX-License-Identifier: GPL-3.0-or-later
// Loc23 – auf dem Piratenschiff: Sabrina in der Kajüte, Szene 102 (docs/locations/loc23.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc23 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00426A20 Loc23_ctor
    Loc23(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00426FE0
    ~Loc23() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x00427100 Loc23_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x004275D0 Loc23_OnEvent
    void onEvent(Event& e, int type);
    bool walk(int x, int y, int dir);
    bool playEnd(AnimLayer* a, unsigned mode);
    bool moveWait(Layer* l, double x, double y, double z, double zoom, int steps);
    void sabSay(const char* key);
    void sabAtDoor();
    void setActive(Layer* l, bool on);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<AnimLayer> escape_;   // +0x7C S102_Escape
    std::unique_ptr<AnimLayer> enter_;    // +0x80 S102_EnterShip
    Layer* boxLeft_ = nullptr;            // +0x84 S102_KisteLinks
    AnimLayer* boxToLeft_ = nullptr;      // +0x88 S102_KisteNachLinks
    AnimLayer* boxToRight_ = nullptr;     // +0x8C S102_KisteNachRechts
    Layer* boxRight_ = nullptr;           // +0x90 S102_KisteRechts
    Layer* doorClosed_ = nullptr;         // +0x94 S102_TuerZu
    Layer* doorOpen_ = nullptr;           // +0x98 S102_TuerOffen
    AnimLayer* rodShake_ = nullptr;       // +0x9C S102_RodRuettel
    AnimLayer* sabDoor_ = nullptr;        // +0xA0 S102_SabTuer
    AnimLayer* sabWalk_ = nullptr;        // +0xA4 SabWalk
    std::unique_ptr<AudioBuffer> doorSlam_, stepL_, stepR_, boxPushed_, explBass_, explMisc_;  // +0xA8…+0xBC
    std::unique_ptr<AudioBuffer> creak_, splash_, boxShort_, metalHit_, ocean_;               // +0xC0…+0xD0
    Area* to100_ = nullptr;               // +0xD4
    Area* to103_ = nullptr;               // +0xD8
};

}  // namespace rah
