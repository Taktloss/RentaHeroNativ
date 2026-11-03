// SPDX-License-Identifier: GPL-3.0-or-later
// Loc14 – bei Cynthia, Szene 90 (docs/locations/loc14.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc14 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00419650 Loc14_ctor
    Loc14(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00419910
    ~Loc14() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x004199C0 Loc14_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x00419C90 Loc14_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x0041A300 – Cynthia spricht und warten
    void cynSay(const char* key);
    bool walk(int x, int y, int dir);
    void turn(int dir);
    void setMusicCynthia();
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<AudioBuffer> fire_, woosh_, hack_, clang_;  // +0x7C…+0x88
    std::unique_ptr<AnimLayer> wakeUp_;     // +0x8C S90_RodWachtAuf
    std::unique_ptr<AnimLayer> bed_;        // +0x90 S90_Bett
    AnimLayer* rodStandUp_ = nullptr;       // +0x94 S90_Rodaufsteh
    AnimLayer* cynSit_ = nullptr;           // +0x98 S90_Cynsitz
    AnimLayer* fireplace_ = nullptr;        // +0x9C S90_Kamin
    AnimLayer* rodSitUp_ = nullptr;         // +0xA0 S90_Rodaufsetz
    AnimLayer* rodTalk_ = nullptr;          // +0xA4 S90_RodTalk
    AnimLayer* cynTalk_ = nullptr;          // +0xA8 S90_CynTalk
    Area* sickBed_ = nullptr;               // +0xAC S90_Krankenbett
    Area* doubleBed_ = nullptr;             // +0xB0 S90_Doppelbett
    Area* flowers_ = nullptr;               // +0xB4 S90_Blumen
    Area* exit_ = nullptr;                  // +0xB8 S90_Exit
    Area* trough_ = nullptr;                // +0xBC S90_Trog
    Area* shelves_ = nullptr;               // +0xC0 S90_Regale
    Area* fireArea_ = nullptr;              // +0xC4 S90_KaminArea
};

}  // namespace rah
