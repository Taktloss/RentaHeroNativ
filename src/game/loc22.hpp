// SPDX-License-Identifier: GPL-3.0-or-later
// Loc22 – Kristallhöhle: Szene 81 und Steinrätsel 1081 (docs/locations/loc22.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc22 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00425CF0 Loc22_ctor
    Loc22(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x004261B0
    ~Loc22() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x00426270 Loc22_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x00426510 Loc22_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x00426790 – Kristall von Platz `slot` zurücknehmen
    void takeCrystal(int slot);
    // orig: Rent-A-Hero.exe 0x00426840 – Kristall `item` auf den ersten freien Platz legen
    void placeCrystal(int item);
    // orig: Rent-A-Hero.exe 0x004269A0 – Kristalle 17–22 lösen Ereignis = Gegenstandsnummer aus
    void setCrystalEvents();
    // orig: Rent-A-Hero.exe 0x004269F0 – liegt mindestens ein Kristall auf der Steinscheibe?
    bool anyPlaced();
    bool playEnd(AnimLayer* a, unsigned mode);
    bool playRange(AnimLayer* a, unsigned mode, int from, int to);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<AnimLayer> enter_, exit_, to1081_;  // +0x7C…+0x84 S81_Enter/Exit/To1081
    Layer* stones81_[6] = {};        // +0x88…+0x9C S81_Stone12Uhr … 10Uhr
    Area* to1081_Area_ = nullptr;    // +0xA0
    Area* to79_ = nullptr;           // +0xA4
    AnimLayer* bigStone_ = nullptr;  // +0xA8 S1081_BigStone
    AnimLayer* rod_[6] = {};         // +0xAC…+0xC0 S1081_Rod12Uhr … 10Uhr
    Layer* stones_[6] = {};          // +0xC4…+0xD8 S1081_Stone12Uhr … 10Uhr
    AnimLayer* rodStroke_ = nullptr; // +0xE0 S1081_RodStreich
    Area* to81_ = nullptr;           // +0xE4
    std::unique_ptr<AudioBuffer> danger_, door_, mobileDown_, bigStoneSfx_;  // +0xE8…+0xF4
    // +0xF8…+0x124: Drehbereich der Steinscheibe je Kristall 17…22 (von, bis)
    int turnFrom_[6] = {0x1E, 10, 0x14, 0x28, 0, 0x32};
    int turnTo_[6] = {0x28, 0x14, 0x1E, 0x32, 10, 0x3C};
    int firstTake_ = 1;              // +0x128 Kommentar beim Zurücknehmen noch offen
};

}  // namespace rah
