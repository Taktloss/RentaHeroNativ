// SPDX-License-Identifier: GPL-3.0-or-later
// Loc01 – Smashville: Platz (7), unterer Stock (1007), Abflug (8), Hebel oben (9), Kanal (46)
// (docs/locations/loc01.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc01 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00405530 Loc01_ctor
    Loc01(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00405B90
    ~Loc01() override;
    // orig: Rent-A-Hero.exe 0x00405DC0 Loc01_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x004065C0 Loc01_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x004070D0 – Kamerafahrten (_mk mit Kanalwesen, sonst _ok)
    void loadCamAnims();
    // orig: Rent-A-Hero.exe 0x00407220 – Liftfahrten
    void loadLiftAnims();
    // orig: Rent-A-Hero.exe 0x004072E0 – Landung mit dem Gleiter
    void arriveByGlider();
    // orig: Rent-A-Hero.exe 0x004073C0 – Treppe vom Lift herunter
    void stairsFromLift();
    // orig: Rent-A-Hero.exe 0x00407500 – Treppe zum Haus hinauf → 6/4
    void stairsHomeUp();
    // orig: Rent-A-Hero.exe 0x00407640 – Treppe zum Lift hinauf → 1007/1
    void stairsToLift();
    // orig: Rent-A-Hero.exe 0x00407790 – Abflug → 8/1
    void flyAway();
    // orig: Rent-A-Hero.exe 0x00407860 – Gegenstand 7 (Sack) ablegen
    void dropSack();
    // orig: Rent-A-Hero.exe 0x004079E0 – Gegenstand 13 am Kanal benutzen
    void useTroot();
    // orig: Rent-A-Hero.exe 0x00407B80 – Gespräch mit dem Kanalwesen
    void talkKanal();
    // orig: Rent-A-Hero.exe 0x00407DC0 – Gleiter schwebt auf und ab (Rückruf von CLayer_MoveTo)
    void bobGlider();
    // CAnimLayer_PlayRange + CGame_Wait (Schrittfolgen der Kamerafahrten)
    bool playWait(AnimLayer* a, unsigned mode, int from, int to);
    void playSfx(AudioBuffer* b) {
        if (b) b->play();  // vtable +0x08
    }
    // vtable +0x24 des Hintergrunds (Szene 8: Video anhalten)
    void stopBackground();

    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    Layer* glider_ = nullptr;                // +0x7C GliderLayer
    Layer* sack_ = nullptr;                  // +0x80 SackLayer
    AnimLayer* troot_ = nullptr;             // +0x84 TrootLayer
    AnimLayer* sackAnim_ = nullptr;          // +0x88 SackAnimLayer
    AnimLayer* kanalLowRun_ = nullptr;       // +0x8C KanalLowRun
    AnimLayer* kanalLowTalk_ = nullptr;      // +0x90 KanalLowTalk
    Layer* kanalGone_ = nullptr;             // +0x94 KanalWegLayer
    Area* gliderArea_ = nullptr;             // +0x98
    Area* door_ = nullptr;                   // +0x9C 007a_Tuer
    Area* stairs_ = nullptr;                 // +0xA0 007a_Treppe
    Area* plant_ = nullptr;                  // +0xA8 007a_Pflanze
    Area* sign_ = nullptr;                   // +0xAC 007a_Schild
    std::unique_ptr<AnimLayer> flyAway_;     // +0xB0 FlyWegCam
    std::unique_ptr<AnimLayer> returnHome_;  // +0xB8 ReturnHomeCam
    std::unique_ptr<AnimLayer> stairHomeDown_, stairHomeUp_;    // +0xBC, +0xC0
    std::unique_ptr<AnimLayer> stairLiftUp_, stairLiftDown_;    // +0xC4, +0xC8
    AnimLayer* pressLeverMid_ = nullptr;     // +0xCC PressLeverMitteLayer
    Layer* lever_ = nullptr;                 // +0xD0 Hebel / HebelOben
    Area* lowerFloor_ = nullptr;             // +0xD4 007b_UntererStock
    Area* liftDown_ = nullptr;               // +0xD8 007b_LiftRunter
    Area* plant2_ = nullptr;                 // +0xDC 007b_Pflanze
    Area* stairs2_ = nullptr;                // +0xE0 007b_Treppe
    Area* lift2_ = nullptr;                  // +0xE4 007b_Lift
    Area* sign2_ = nullptr;                  // +0xE8 007b_Schild
    std::unique_ptr<AnimLayer> liftMidUp_, liftMidDown_, liftUpMid_, liftDownMid_;  // +0xEC…+0xF8
    AnimLayer* pressLeverTop_ = nullptr;     // +0xFC PressLeverObenLayer
    std::unique_ptr<AnimLayer> flyAll_;      // +0x100 FlyGanzWegCam
    AnimLayer* kanalTalk_ = nullptr;         // +0x104 KanalTalk
    std::unique_ptr<AnimLayer> kanalOut_, kanalIn_;  // +0x108, +0x10C
    std::unique_ptr<AudioBuffer> bubble_, horn_, moneyBag_, woosh_, tock_, clickClack_;  // +0x110…+0x124
    std::unique_ptr<AudioBuffer> clothes_;   // +0x12C (vom Dtor nicht gelöscht)
    std::unique_ptr<AudioBuffer> lift_, stepLeft_, stepRight_, gliderPass_;  // +0x130…+0x13C
    std::unique_ptr<AudioBuffer> bird_, bird3_, gliderPass3_, wind_, hack_;  // +0x140…+0x150
    std::unique_ptr<Event> ambient_;         // +0x154
    int bobUp_ = 0;                          // +0x158
};

}  // namespace rah
