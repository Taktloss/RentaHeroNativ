// SPDX-License-Identifier: GPL-3.0-or-later
// Loc10 – Pilzwald (Szenen 37–44, 66) (docs/locations/loc10.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc10 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00413FF0 Loc10_ctor
    Loc10(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00414850
    ~Loc10() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x00414A50 Loc10_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x00415E60 Loc10_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x00417EB0 – Wege aus Szene 37 (mit Jasmin: sie zeigt nach Norden)
    void leave37(int id);
    // orig: Rent-A-Hero.exe 0x00418380 – Wege aus Szene 38
    void leave38(int id);
    // Szene 38: Gespräch mit dem Mädchen (Stufen 1, 2, 4, 5 in 0x00415E60)
    void girlTalk(const char* rod, const char* girl, int next);
    // Szene 40: Jasmin auf dem Baum, Stufe 4 (0x00415E60, Sprungziel nach Fall 0x42)
    void jasminTree();
    bool walk(int x, int y, int dir);
    bool playWait(AnimLayer* a, unsigned mode, int from, int to);
    void play(AudioBuffer* b) {
        if (b) b->play();
    }
    void stopEvent(Event* e) {
        if (e) e->enabled = e->pending = false;
    }
    void hideHero() {
        if (hero_) hero_->hide(scene_.clicksEnabled);
    }
    void showHero() {
        if (hero_) hero_->show(scene_.clicksEnabled);
    }
    void enable(Layer* l) {
        if (l) l->enable();
    }
    void disable(Layer* l) {
        if (l) l->disable();
    }
    void setLayerEvent(Layer* l, int ev);
    void setAreaEvent(const char* name, int ev);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    Layer* bush_ = nullptr;                 // +0x7C Gen_Busch
    Layer* bushClone_ = nullptr;            // +0x80 Gen_BuschClone
    AnimLayer* rodPickUp_ = nullptr;        // +0x84 Gen_RodPickUp
    AnimLayer* rodFallDown_ = nullptr;      // +0x88
    Layer* glider_ = nullptr;               // +0x8C S37_Glider
    AnimLayer* gliderArrives_ = nullptr;    // +0x90
    AnimLayer* gliderLeaves_ = nullptr;     // +0x94
    AnimLayer* jasFallDown_ = nullptr;      // +0x98
    AnimLayer* jasPointN_ = nullptr;        // +0x9C
    std::unique_ptr<Event> gliderTimer_;    // +0xA0
    AnimLayer* rodOffersTeddy_ = nullptr;   // +0xA4
    AnimLayer* girl_ = nullptr;             // +0xA8 S38_Girl
    AnimLayer* girlJumps_ = nullptr;        // +0xAC
    AnimLayer* jasPointsNW_ = nullptr;      // +0xB0
    AnimLayer* rodStealsRope_ = nullptr;    // +0xB4
    std::unique_ptr<Event> girlTimer_;      // +0xB8
    Layer* mushroom39_ = nullptr;           // +0xBC S39_Pilz
    AnimLayer *climbsTree_ = nullptr, *dropsStraw_ = nullptr, *killsFire_ = nullptr,
              *looksRoots_ = nullptr, *setsFire_ = nullptr, *takesStraw_ = nullptr,
              *watchesNecklace_ = nullptr, *takesNecklace_ = nullptr, *necklaceDropped_ = nullptr;  // +0xC0…+0xE0
    Layer* necklace_ = nullptr;             // +0xE4
    Layer* straw_ = nullptr;                // +0xE8
    AnimLayer *fireStarts_ = nullptr, *fireLoops_ = nullptr, *fireStops_ = nullptr;  // +0xEC…+0xF4
    AnimLayer* branch_ = nullptr;           // +0xF8 S40_Branch
    std::unique_ptr<Event> jasminTimer_;    // +0xFC
    AnimLayer* rodTakesTeddy_ = nullptr;    // +0x100
    AnimLayer* rodTakesLeaf_ = nullptr;     // +0x104
    AnimLayer* slugExplodes_ = nullptr;     // +0x108
    Layer* leaf_ = nullptr;                 // +0x10C
    std::unique_ptr<AnimLayer> woodChase_;  // +0x110 S66_WoodChase
    std::unique_ptr<Event> ambientTimer_;   // +0x114
    std::unique_ptr<AudioBuffer> bird_, bird2_, bird3_, chirp_, owl_, thunder_, thunder2_;  // +0x118…+0x130
    std::unique_ptr<AudioBuffer> passing_, knockWet_, stepSoft_, clothes_, match_, fire_;   // +0x134…+0x148
    std::unique_ptr<AudioBuffer> explosion_, eat_, gliderFlying_, fly2_;                    // +0x14C…+0x158
    int bobX_ = 0, bobY_ = 0;               // +0x15C, +0x160 Ruhelage des Gleiters
    double bobUnused_ = 0;                  // +0x168 (nie verändert, wird trotzdem umgebrochen)
    double bobPhase_ = 0;                   // +0x170
};

}  // namespace rah
