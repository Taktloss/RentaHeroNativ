// SPDX-License-Identifier: GPL-3.0-or-later
// Loc08 – Hafen mit Sabrina, Szenen 99 (Anlegeplatz) und 100 (Strand mit Boot)
// (docs/locations/loc08.md)
#pragma once

#include "game/game.hpp"

namespace rah {

class Loc08 final : public Game {
public:
    // orig: Rent-A-Hero.exe 0x00411640 Loc08_ctor
    Loc08(GameContext& ctx, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x00411AD0
    ~Loc08() override { shutdown(); }
    // orig: Rent-A-Hero.exe 0x00411C20 Loc08_Start
    void start() override;

private:
    void destroyLocal() override;
    // orig: Rent-A-Hero.exe 0x00411CD0 – Szene 99 aufbauen
    void start99();
    // orig: Rent-A-Hero.exe 0x00412150 – Szene 100 aufbauen
    void start100();
    // orig: Rent-A-Hero.exe 0x004124B0 Loc08_OnEvent (Umgebungston, sonst je Szene)
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x004125A0
    void onEvent99(int id);
    // orig: Rent-A-Hero.exe 0x00412FD0
    void onEvent100(int id);
    // Sabrina steht (Bild 4) an der Stelle (x, y, zoom) und ist anklickbar (Ereignis 13)
    void sabStand(double x, double y, double z, double zoom);
    bool walk(int x, int y, int dir);
    bool walk2(int x, int y, int dir);
    bool playEnd(AnimLayer* a, unsigned mode);
    bool playRange(AnimLayer* a, unsigned mode, int from, int to);
    bool moveWait(Layer* l, double x, double y, double z, double zoom, int steps);
    void sabSpeak(std::string_view key, int mode, bool waitDone);
    void heroSayGer(std::string_view key);
    AnimLayer* obj(std::string_view name) { return dynamic_cast<AnimLayer*>(scene_.getObject(name)); }
    std::int32_t& flag(std::size_t off) { return ctx_.state.at(off); }

    std::unique_ptr<AnimLayer> rodInCam_;  // +0x7C RodReinCam (Szene 99, erste Ankunft)
    AnimLayer* bush_ = nullptr;            // +0x80 Busch
    AnimLayer* hatch_ = nullptr;           // +0x84 Klappe
    AnimLayer* rodClimbIn_ = nullptr;      // +0x88
    AnimLayer* rodClimbOut_ = nullptr;     // +0x8C
    AnimLayer* rodScared_ = nullptr;       // +0x90 RodSchreck
    AnimLayer* sabJump_ = nullptr;         // +0x94
    AnimLayer* sabPoint_ = nullptr;        // +0x98
    AnimLayer* sabClimbIn_ = nullptr;      // +0x9C
    AnimLayer* sabClimbOut_ = nullptr;     // +0xA0
    AnimLayer* sabLever_ = nullptr;        // +0xA4
    AnimLayer* sabWalk_ = nullptr;         // +0xA8
    AnimLayer* sabStand_ = nullptr;        // +0xAC
    AnimLayer* surf_ = nullptr;            // +0xB0 Brandung
    AnimLayer* boatAway_ = nullptr;        // +0xB4
    AnimLayer* boatReturns_ = nullptr;     // +0xB8
    AnimLayer* boat_ = nullptr;            // +0xBC Boot
    Layer* boatBack_ = nullptr;            // +0xC0
    AnimLayer* boatFlag_ = nullptr;        // +0xC4 BootFahne
    AnimLayer* rodFlag_ = nullptr;         // +0xC8 RodFahne
    Area* landing_ = nullptr;              // +0xCC 099_AnlegePlatz
    Area* town_ = nullptr;                 // +0xD0 099_Stadt
    Area* barrel_ = nullptr;               // +0xD4 099_Fass
    Area* forest_ = nullptr;               // +0xD8 099_Wald
    Area* passage_ = nullptr;              // +0xDC 099_Gang
    Area* wayBack_ = nullptr;              // +0xE0 100_WayBack
    std::unique_ptr<Event> bushTimer_;     // +0xE4
    std::unique_ptr<Event> windTimer_;     // +0xE8
    std::unique_ptr<Event> ambientTimer_;  // +0xEC
    std::unique_ptr<AudioBuffer> wind_;    // +0xF0
    std::unique_ptr<AudioBuffer> seagull_, knockWet_, woosh_, fixBelt_, creak_, paddling_, thunder_,
        thunder2_, ocean_;                 // +0xF8…+0x118
};

}  // namespace rah
