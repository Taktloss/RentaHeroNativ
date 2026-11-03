// SPDX-License-Identifier: GPL-3.0-or-later
// Loc27 – Hütte (Szene 95) und Megophias' Geheimraum (Szene 96), docs/locations/loc27.md
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc27 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x0042AE20 Loc27_ctor
    Loc27(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0042B400
    ~Loc27() override;
    // orig: Rent-A-Hero.exe 0x0042B570 Loc27_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x0042BB70 Loc27_OnEvent
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x0042C590 – Gegenstand benutzen (Helm, Gegenstand 5)
    void useItem(int item);
    // orig: Rent-A-Hero.exe 0x0042C690 – Megophias spricht (Mikrofon)
    void megSay(const char* key);
    void talkMegophias();
    void heroSayKey(const char* key) { heroSay(key); }
    bool walk(int x, int y, int dir);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<AnimLayer> helmOn_;      // +0x9C S96_HelmAuf
    std::unique_ptr<AnimLayer> enterHut_;    // +0x7C S95_EnterHut
    std::unique_ptr<AnimLayer> enterSecret_; // +0x80 S95_EnterSecretRoom
    Area* windowW_ = nullptr;                // +0x84
    Area* windowN_ = nullptr;                // +0x88
    Area* windowE_ = nullptr;                // +0x8C
    Area* chair_ = nullptr;                  // +0x90
    Area* floor_ = nullptr;                  // +0x94
    Area* exit93_ = nullptr;                 // +0x98
    std::unique_ptr<AnimLayer> rodEnters_;   // +0xA0 S96_RodEntersRoom
    AnimLayer* rodAndEye_ = nullptr;         // +0xA4
    AnimLayer* rodDropsHelm_ = nullptr;      // +0xA8
    Layer* helm_ = nullptr;                  // +0xAC S96_Helm
    AnimLayer* rodWaves_ = nullptr;          // +0xB0 S96_RodWinkt
    Layer* eye_ = nullptr;                   // +0xB4 S96_Eye
    Layer* eyeDown_ = nullptr;               // +0xB8 S96_EyeDown
    AnimLayer* steam_ = nullptr;             // +0xBC
    AnimLayer* bubble_ = nullptr;            // +0xC0
    AnimLayer* microphone_ = nullptr;        // +0xC4
    AnimLayer* pump_ = nullptr;              // +0xC8
    Area* exit95_ = nullptr;                 // +0xCC
    Area* megophias_ = nullptr;              // +0xD0
    Area* socket_ = nullptr;                 // +0xD4 S96_Steckkontakt
    Area* machines_ = nullptr;               // +0xD8 S96_Anlagen
    std::unique_ptr<Event> ambientTimer_;    // +0xDC
    std::unique_ptr<AudioBuffer> stepL_, stepR_, mobileUp_, ocean_, seagull_;           // +0xE0…+0xF0
    std::unique_ptr<AudioBuffer> wings_, wings2_, bigStone_, bubbleSfx_, doorOpened_;   // +0xF4…+0x104
    std::unique_ptr<AudioBuffer> servo_, endTime_;                                      // +0x108, +0x10C
};

}  // namespace rah
