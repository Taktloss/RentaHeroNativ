// SPDX-License-Identifier: GPL-3.0-or-later
// Loc06 – bei Ranama: Zimmer (Szene 26) und Gespräch (Szene 27) (docs/locations/loc06.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc06 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0040E730 Loc06_ctor
    Loc06(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0040EAF0
    ~Loc06() override;
    // orig: Rent-A-Hero.exe 0x0040EBC0 Loc06_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x0040F040 Loc06_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x0040F4F0 – Ranama schwebt (Rückruf von CLayer_MoveTo), Stufe 3: fällt
    void ranamaBob();
    // orig: Rent-A-Hero.exe 0x0040F640 – Rodrigo spricht (Szene 26: HeroSayWait, sonst S27_RodTalk)
    void rodLine(std::string_view key);
    // orig: Rent-A-Hero.exe 0x0040F690 – Ranama spricht (S26_RanTalk bzw. S27_RanTalk)
    void ranLine(std::string_view key);
    // orig: Rent-A-Hero.exe 0x0040F6D0 – Zimmer verlassen
    void leave();
    bool playWait(AnimLayer* a, unsigned mode, int from, int to);
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<AnimLayer> earGrab_;  // +0x7C S27_OhrGreif
    AnimLayer* ranFalls_ = nullptr;       // +0x80 S26_RanFaellt
    AnimLayer* ranTalk_ = nullptr;        // +0x84 S26_RanTalk
    AnimLayer* ranBroom_ = nullptr;       // +0x88 S26_RanBesen
    AnimLayer* ranLooks_ = nullptr;       // +0x8C S27_RanSchaut
    AnimLayer* rodTalk_ = nullptr;        // +0x90 S27_RodTalk
    AnimLayer* ranTalk27_ = nullptr;      // +0x94 S27_RanTalk
    Layer* ranTalkRing_ = nullptr;        // +0x98 S27_RanTalkRing (TCC-Standbild)
    Layer* ranLooksRing_ = nullptr;       // +0x9C S27_RanSchautRing (TCC-Standbild)
    Area* exit_ = nullptr;                // +0xA0 S26_Exit
    Area* bottle_ = nullptr;              // +0xA4 S26_Bottle
    Area* telescope_ = nullptr;           // +0xA8 S26_Telescope
    Area* backArea_ = nullptr;            // +0xAC S26_Background
    Area* ranama_ = nullptr;              // +0xB0 S26_Ranama
    Area* exit27_ = nullptr;              // +0xB4 S27_Exit
    std::unique_ptr<Event> timer_;        // +0xB8
    int bob_ = 0;                         // +0xBC Schwebestufe (0/1 auf und ab, 2/3 fallen)
    int bottleCount_ = 0;                 // +0xC0
    int ringPending_ = 0;                 // +0xC4
    std::unique_ptr<AudioBuffer> boing_, knock_, gong_, ratsch_, tock_;  // +0xC8…+0xD8
};

}  // namespace rah
