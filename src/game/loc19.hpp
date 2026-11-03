// SPDX-License-Identifier: GPL-3.0-or-later
// Loc19 – Esragoth: Waldweg (Szene 68, nachts 101) und Zwergentor (Szene 69)
// (docs/locations/loc19.md)
#pragma once

#include <initializer_list>

#include "game/game.hpp"

namespace rah {

class Loc19 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0041F070 Loc19_ctor
    Loc19(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0041F810
    ~Loc19() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x0041F960 Loc19_Start
    void start() override;

private:
    void destroyLocal() override;
    void start69();
    // orig: Rent-A-Hero.exe 0x004202D0 – Szene 68
    void onEvent68(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x00420500 – Szene 101
    void onEvent101(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x00420D60 – Szene 69
    void onEvent69(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x00420600 – Nahaufnahme: alles ausblenden, sprechen (0 Ramil,
    // 1 Jasmin, 2 Wache mit Staunen), vorherige Sichtbarkeit wiederherstellen
    void closeup(const char* key, int who, std::uint32_t before, std::uint32_t after, bool invalidate);
    void ambient(int lo, int hi, std::initializer_list<AudioBuffer*> sounds);
    bool walk(int x, int y, int dir);
    bool walkActor(Actor* a, int x, int y, int dir);
    bool playEnd(AnimLayer* a, unsigned mode);
    bool playRange(AnimLayer* a, unsigned mode, int from, int to);
    bool moveWait(Layer* l, double x, double y, double z, double zoom, int steps);
    void say(AnimLayer* who, const char* key, int from, int to);  // CGame_Speak(..., 0x0C, von, bis) + warten
    void sayNoActor(const char* key);                              // CGame_SayNoActor + warten
    void enableAt(Layer* l, double x, double y, double z, double zoom);
    void setActive(Layer* l, bool on);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    AnimLayer* jasRun_ = nullptr;          // +0x7C S69_JasLauf
    AnimLayer* jasChain_ = nullptr;        // +0x80 S69_JasWalkKette
    AnimLayer* ramAngry_ = nullptr;        // +0x84 S69_RamBoese
    AnimLayer* ramWalk_ = nullptr;         // +0x88
    AnimLayer* ramRun_ = nullptr;          // +0x8C
    AnimLayer* rodStare_ = nullptr;        // +0x90 S69_RodStarrt
    AnimLayer* rodPoint_ = nullptr;        // +0x94 S69_RodZeig
    AnimLayer* thaTalk_ = nullptr;         // +0x98
    AnimLayer* thaWalk_ = nullptr;         // +0x9C
    AnimLayer* door_ = nullptr;            // +0xA0 S69_Tuer
    AnimLayer* hug_ = nullptr;             // +0xA4 S69_Umarm
    AnimLayer* guard_ = nullptr;           // +0xA8 S69_Wache
    AnimLayer* dwarf1_ = nullptr;          // +0xAC S69_Zwerg1
    AnimLayer* ramBack_ = nullptr;         // +0xB0
    AnimLayer* ramCloseup_ = nullptr;      // +0xB4 S69_RamTalkCloseup
    AnimLayer* jasBack_ = nullptr;         // +0xB8
    AnimLayer* jasCloseup_ = nullptr;      // +0xBC S69_JasTalkCloseup
    AnimLayer* jasTalk_ = nullptr;         // +0xC0
    AnimLayer* ramBackTalk_ = nullptr;     // +0xC4
    AnimLayer* ramTalkAnim_ = nullptr;     // +0xC8
    AnimLayer* guardAmazedBack_ = nullptr; // +0xCC S69_WacheStauntBack
    AnimLayer* guardTalk_ = nullptr;       // +0xD0
    std::unique_ptr<AnimLayer> guardAmazed_;  // +0xD4 S69_WacheStaunt
    Area* to71_ = nullptr;                 // +0xD8
    Area* to70_ = nullptr;                 // +0xDC
    Area* to68_ = nullptr;                 // +0xE0
    std::unique_ptr<AudioBuffer> stepLeft_, stepRight_, doorOpen_, doorClose_;  // +0xE4…+0xF0
    std::unique_ptr<AudioBuffer> chirp_, bird3_, bird_, bird2_, thunder2_, thunder_;  // +0xF4…+0x108
    std::unique_ptr<Event> ambientTimer_;  // +0x10C
    Area* toWest_ = nullptr;               // +0x110 S68_To67
    Area* toEast_ = nullptr;               // +0x114 S68_To69 / S101_To69
};

}  // namespace rah
