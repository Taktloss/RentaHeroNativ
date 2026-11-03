// SPDX-License-Identifier: GPL-3.0-or-later
// Loc25 – Cyberspace: Endkampf gegen Megophias, Szene 105 (docs/locations/loc25.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc25 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00429020 Loc25_ctor
    Loc25(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x004294E0
    ~Loc25() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x00429660 Loc25_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x00429810 Loc25_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x00429C80(nummer, angriff) – Helfer rufen; richtige Reihenfolge
    // (0x4090) trifft den Zauberer
    void attack(int number, Layer* layer);
    // Zauberer kommt von oben heran (Start und nach jedem Treffer < 6)
    void wizardApproach();
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    Layer* rod_ = nullptr;          // +0x7C S105_Rod
    Layer* draHead_ = nullptr;      // +0x80
    Layer* jas_ = nullptr;          // +0x84 (TCC)
    Layer* madHead_ = nullptr;      // +0x88
    Layer* ran_ = nullptr;          // +0x8C (TCC)
    Layer* sabHead_ = nullptr;      // +0x90
    AnimLayer* rodMorph_ = nullptr; // +0x94
    AnimLayer* wizAttack_ = nullptr;   // +0x98
    AnimLayer* sabAttack_ = nullptr;   // +0x9C
    AnimLayer* draAttack_ = nullptr;   // +0xA0
    AnimLayer* madAttack_ = nullptr;   // +0xA4
    AnimLayer* louAttack_ = nullptr;   // +0xA8
    AnimLayer* sanAttack_ = nullptr;   // +0xAC
    AnimLayer* cynAttack_ = nullptr;   // +0xB0
    AnimLayer* wizWalk_ = nullptr;     // +0xB4
    AnimLayer* wizExplodes_ = nullptr; // +0xB8
    Layer* jasHead_ = nullptr;      // +0xBC
    Layer* louHead_ = nullptr;      // +0xC0
    Layer* ranHead_ = nullptr;      // +0xC4
    Layer* cynHead_ = nullptr;      // +0xC8
    Layer* sanHead_ = nullptr;      // +0xCC
    std::unique_ptr<AudioBuffer> endTime_, dragonBreath_, dragonFire_, explBass_, explBass2_, gong_, knock_;
    std::unique_ptr<AudioBuffer> mobileUp_, mobileDown_, horn_, thunder_, thunder2_, splash_, woosh_, lazer_;
    std::unique_ptr<AudioBuffer> danger_, explMisc_;  // +0xD0…+0x110
    std::unique_ptr<Event> thunderTimer_;  // +0x114
};

}  // namespace rah
