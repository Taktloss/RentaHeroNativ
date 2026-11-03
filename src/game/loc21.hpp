// SPDX-License-Identifier: GPL-3.0-or-later
// Loc21 – Zwergenbergwerk, Szenen 71–80 (docs/locations/loc21.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc21 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00422D90 Loc21_ctor
    Loc21(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00423780
    ~Loc21() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x00423880 Loc21_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x00424600 Loc21_OnEvent
    void onEvent(Event& e, int type);
    // Ereignisse 0x3E/0x5D: Fahrhebel am Bohrer (76 bzw. 79)
    void driveLever(bool in79);
    // Ereignisse 0x3F/0x5E: Bohrhebel – Schutt wegbohren (76 bzw. 79)
    void drillLever(bool in79);
    void addDrillCues(AnimLayer* drill);
    void area(const char* name, int ev);
    void areaActive(const char* name, bool on);
    void arrive(int x, int y, int dir, int tx, int ty, int tdir);
    void go(int x, int y, int dir, int scene);
    bool walk(int x, int y, int dir);
    bool playEnd(AnimLayer* a, unsigned mode);
    bool playRange(AnimLayer* a, unsigned mode, int from, int to);
    void showSchutt(int state);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    AnimLayer* rodTakesShovel_ = nullptr;  // +0x7C S71_RodTakesShovel
    Layer* shovel_ = nullptr;              // +0x80 S71_Shovel
    AnimLayer* rodFeelsWall_ = nullptr;    // +0x84 S72_RodFeelsWall
    AnimLayer* rodUsesGloom_ = nullptr;    // +0x88 S73_RodUsesGloom
    AnimLayer* drill73_ = nullptr;         // +0x8C S73_Drill
    Layer* lever73_ = nullptr;             // +0x90 S73_Hebel
    AnimLayer* lever74_ = nullptr;         // +0x94 S74_Hebel
    AnimLayer* rodUsesSwitch_ = nullptr;   // +0x98 S74_RodUsesWeiche
    AnimLayer* rodLooksSwitch_ = nullptr;  // +0x9C S74_RodLooksWeiche
    Layer* tinder_ = nullptr;              // +0xA0 S76_Zunder
    AnimLayer* rodPickUp_ = nullptr;       // +0xA4 Gen_RodPickUp
    AnimLayer* drillLeaves_ = nullptr;     // +0xA8 Gen_DrillLeaves
    AnimLayer* drillDrills_ = nullptr;     // +0xAC Gen_DrillDrills
    AnimLayer* schutt_ = nullptr;          // +0xB0 Gen_Schutt
    AnimLayer* schutt2_ = nullptr;         // +0xB4 Gen_Schutt2
    AnimLayer* rodGrab_ = nullptr;         // +0xB8 Gen_RodGreif
    AnimLayer* smallSteam_ = nullptr;      // +0xBC Gen_SmallSteam
    AnimLayer* rodHacks_ = nullptr;        // +0xC0 S80_RodHacks
    Layer* gloomOnFloor_ = nullptr;        // +0xC4 S80_GloomOnFloor
    AnimLayer* rodTakesPickAxe_ = nullptr; // +0xC8 S80_RodTakesPickAxe
    AnimLayer* rodTakesStone_ = nullptr;   // +0xCC S80_RodTakesStone
    AnimLayer* rodTriesTake_ = nullptr;    // +0xD0 S80_RodTriesTake
    Layer* pickAxe_ = nullptr;             // +0xD4 S80_PickAxe
    int feltWall_ = 0;                     // +0xD8 Wand in 80 abgetastet (nur in dieser Szene)
    std::unique_ptr<AudioBuffer> splash_;  // +0xDC Sfx_WaterSplash (Vol. 60)
    std::unique_ptr<AudioBuffer> winding_, wings_, door_, drill_, explosion_, hack_, sizzle_, drip_;  // +0xE0…+0xFC
    std::unique_ptr<Event> dripTimer_;     // +0x100
};

}  // namespace rah
