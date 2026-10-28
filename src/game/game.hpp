// SPDX-License-Identifier: GPL-3.0-or-later
// Schauplatz-Basisklasse CGame und Schauplatzwechsel (docs/GAME.md, SCRIPTING.md).
#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string_view>

#include "engine/animlayer.hpp"
#include "engine/scene.hpp"
#include "engine/speech.hpp"

namespace rah {

class HelpOverlay;

// Schauplatzwechsel-Anforderung: g_NextLocation 0x0047D73C, g_NextScene 0x0047D740,
// g_PrevScene 0x0047D744, g_PrevLocation 0x0047D748, Abbruch 0x0047D74C, Flag 0x0047D750
struct LocationRequest {
    int nextLocation = 0, nextScene = 0, prevScene = 0, prevLocation = 0;
    bool abort = false;
    bool pending = false;
};

class Game;

// Was im Original globale Objekte sind (g_CurScene, g_CurGame, CHelp, Wechsel-Globals)
struct GameContext {
    EngineGlobals& g;
    DataPaths& paths;
    Scene& scene;
    HelpOverlay* help = nullptr;
    LocationRequest request;
    std::unique_ptr<Game> current;  // g_CurGame
};

class Game {
public:
    // orig: Rent-A-Hero.exe 0x0043F4D0 CGame_ctor (+ Felder aus den LocNN-Ctoren)
    Game(GameContext& ctx, int location, int scene, int prevScene);
    // orig: Rent-A-Hero.exe 0x0043F580 CGame_dtor
    virtual ~Game();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;
    // vtable +0x04 (Start), aus Game_SwitchLocation
    virtual void start() {}

    int location() const { return location_; }
    int sceneNumber() const { return sceneNr_; }
    int prevScene() const { return prevScene_; }

protected:
    using Handler = void (Game::*)(Event&, int);
    // orig: Rent-A-Hero.exe 0x0043FA30 CGame_SetEventProc
    void setEventProc(Handler h);
    template <class T>
    void setEventProc(void (T::*h)(Event&, int)) { setEventProc(static_cast<Handler>(h)); }
    // orig: Rent-A-Hero.exe 0x0043F6C0 CGame_GetViewObjects (ohne Figuren)
    void getViewObjects();
    // orig: Rent-A-Hero.exe 0x00440A80 CGame_ChangeScene(scene, loc)
    void changeScene(int scene, int location);
    // orig: Rent-A-Hero.exe 0x0043FFB0 CGame_LoadAnim
    std::unique_ptr<AnimLayer> loadAnim(std::string_view name) { return scene_.loadAnim(name); }
    // orig: Rent-A-Hero.exe 0x004404C0 CGame_Say(ger, name)
    bool say(std::string_view ger, std::string_view name);
    // orig: Rent-A-Hero.exe 0x004403E0 → CGame_Speak(ger, name, 0, 0, -1, -1, ereignis) 0x00440430
    bool speak(std::string_view ger, std::string_view name, int doneEvent);
    // orig: Rent-A-Hero.exe 0x004400C0 CGame_PlayMusic – Ton folgt (nur Protokoll)
    void playMusic(std::string_view name);
    // Ton-Aufrufe ohne Audio-Port (Puffer-Eigenschaften, Blenden): nur Protokoll
    void audio(std::string_view what);

    GameContext& ctx_;
    EngineGlobals& g_;
    Scene& scene_;            // +0x50
    int location_;            // +0x28
    int sceneNr_;             // +0x2C
    int prevScene_;           // +0x30
    std::array<int, 5> waits_{};  // +0x34 Warteplätze (SCRIPTING.md)
    std::uint32_t seed_;      // +0x20 timeGetTime()
    std::unique_ptr<Speech> speech_;  // +0x70

private:
    void stopSpeech();
};

// orig: Rent-A-Hero.exe 0x00404B20 RequestLocation(loc, scene, prevScene, abort)
void requestLocation(LocationRequest& r, int location, int scene, int prevScene, bool abort);
// orig: Rent-A-Hero.exe 0x004036E0 Game_SwitchLocation – false, wenn der Schauplatz fehlt
bool switchLocation(GameContext& ctx);
// orig: Rent-A-Hero.exe 0x004030C0 Game_Idle Schritt 7 (anstehenden Wechsel ausführen)
void processLocationChange(GameContext& ctx);
// Fabrik aus dem switch in Game_SwitchLocation; nullptr = (noch) nicht portiert
std::unique_ptr<Game> createLocation(GameContext& ctx, int location, int scene, int prevScene);

}  // namespace rah
