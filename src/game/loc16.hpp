// SPDX-License-Identifier: GPL-3.0-or-later
// Loc16 – Drachenhorst, Szene 58 (docs/locations/loc16.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc16 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0041B290 Loc16_ctor
    Loc16(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0041B690
    ~Loc16() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x0041B830 Loc16_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x0041BDC0 Loc16_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x0041BDC0 Ereignis 3 – mit dem Drachen sprechen
    void talkToDragon();
    // orig: Rent-A-Hero.exe 0x0041C860 – Sperrrechteck (0,0)–(639,262) an/aus
    void blockSky(bool on);
    // orig: Rent-A-Hero.exe 0x0041C890 – Rodrigo steigt auf, der Drache fliegt ab → 26/93
    void takeOff();
    // orig: Rent-A-Hero.exe 0x0041C9A0 – Drache spricht (Bilder 0–8)
    void dragonSay(const char* key);
    // orig: Rent-A-Hero.exe 0x0041C9F0 – Rodrigo, dann Drache
    void exchange(const char* rod, const char* dra);
    void exchangeN(int n);
    // orig: Rent-A-Hero.exe 0x0041CA50(modus, arg) – Haltung des Drachen (wie Loc05 0x0040A190)
    void pose(int mode, int arg);
    void playSfx(AudioBuffer* b) {
        if (b) b->play();  // vtable +0x08
    }
    bool walk(int x, int y, int dir);
    bool playRange(AnimLayer* a, unsigned mode, int from, int to);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<AnimLayer> pirateBurn_;  // +0x7C S58_PirateBurn
    std::unique_ptr<AnimLayer> takeOff_;     // +0x80 S58_DragonTakeOff
    std::unique_ptr<AnimLayer> landing_;     // +0x84 S58_DragonLanding
    Layer* hat_ = nullptr;                   // +0x88 S58_Hut
    Layer* hand_ = nullptr;                  // +0x8C S58_Hand
    AnimLayer* dragon_ = nullptr;            // +0x90 S58_DragonTalk(_nd)
    AnimLayer* rodAttack_ = nullptr;         // +0x94 S58_RodAngriff
    AnimLayer* rodLooksSky_ = nullptr;       // +0x98
    AnimLayer* rodPickUp_ = nullptr;         // +0x9C
    AnimLayer* rodSitDown_ = nullptr;        // +0xA4
    Area* to57_ = nullptr;                   // +0xA8
    Area* to59_ = nullptr;                   // +0xAC
    std::unique_ptr<Event> poseTimer_;       // +0xB0
    std::unique_ptr<Event> ambientTimer_;    // +0xB4
    std::unique_ptr<AudioBuffer> bird_, bird2_, bird3_, chirp_, owl_, thunder_, thunder2_;  // +0xB8…+0xD0
    std::unique_ptr<AudioBuffer> woosh_, clothes_, wings_, danger_, dangerHigh_, knock_, nose_, fire_;  // +0xD4…+0xF0
};

}  // namespace rah
