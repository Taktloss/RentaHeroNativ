// SPDX-License-Identifier: GPL-3.0-or-later
// Loc02 – Sanchos Kneipe (Szenen 13, 1013, 2013, 14, 15) (docs/locations/loc02.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc02 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00407E50 Loc02_ctor
    Loc02(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x004083E0
    ~Loc02() override;
    // orig: Rent-A-Hero.exe 0x00408620 Loc02_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x00408D50 – verteilt nach Szene
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x00408DD0 – Szene 13 (Eingang)
    void onEntrance(int id);
    // orig: Rent-A-Hero.exe 0x00408EB0 – Szene 1013 (Schankraum mit Gästen)
    void onTaproom(int id);
    // orig: Rent-A-Hero.exe 0x004093A0 – Szene 2013 (Weg zum Fremden)
    void onBackRoom(int id);
    // orig: Rent-A-Hero.exe 0x00409420 – Szene 14 (an der Theke bei Sancho)
    void onCounter(int id);
    // orig: Rent-A-Hero.exe 0x00409AE0 – Szene 15 (am Tisch des Fremden)
    void onStranger(int id);
    // orig: Rent-A-Hero.exe 0x0040A040 / 0x0040A080 – Rodrigo bzw. Sancho an der Theke
    void counterRod(std::string_view key);
    void counterSan(std::string_view key);
    // orig: Rent-A-Hero.exe 0x0040A0C0 – Rodrigo am Tisch des Fremden
    void tableRod(std::string_view key);
    // orig: Rent-A-Hero.exe 0x0040A100 – Rodrigo, dann der Fremde
    void tableExchange(std::string_view rod, std::string_view fre);
    // orig: Rent-A-Hero.exe 0x0040A190 – Trinkhaltung des Fremden (wie Loc05 0x0040E2A0)
    void drink(int mode, int arg);
    bool playWait(AnimLayer* a, unsigned mode, int from, int to);
    void play(AudioBuffer* b) {
        if (b) b->play();
    }
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<AnimLayer> from2013_;   // +0x7C S0013_From2013
    Layer* stranger13_ = nullptr;           // +0x80 S0013_Fre (BMP-Standbild)
    AnimLayer* guest1_ = nullptr;           // +0x84 S1013_Gast1
    AnimLayer* guest2_ = nullptr;           // +0x88 S1013_Gast2
    AnimLayer* sancho1013_ = nullptr;       // +0x8C S1013_San
    std::unique_ptr<Event> sanchoTimer_;    // +0x90
    std::unique_ptr<Event> guestTimer_;     // +0x94
    std::unique_ptr<AnimLayer> from0013_;   // +0x98 S2013_From0013
    AnimLayer* sanAnim_ = nullptr;          // +0x9C S0014_SanAnim
    std::unique_ptr<AnimLayer> freNeck_, freBum_, freKnife_, frePack_, pirates_, rodWonder_;  // +0xA0…+0xB4
    AnimLayer* freDrink_ = nullptr;         // +0xB8
    AnimLayer* freTalk_ = nullptr;          // +0xBC
    AnimLayer* rodTalk_ = nullptr;          // +0xC0
    std::unique_ptr<Event> drinkTimer_;     // +0xC4
    std::unique_ptr<AudioBuffer> boing_, boioing_, knockWet_, noseBroken_, tock_, gluck_;  // +0xC8…+0xDC
    std::unique_ptr<AudioBuffer> stepLeft_, stepRight_;                                    // +0xE0, +0xE4
    std::unique_ptr<AudioBuffer> danger_, eat_, woosh_, boing2_, dangerHigh_, dangerLow_, tableHit_;  // +0xE8…+0x100
};

}  // namespace rah
