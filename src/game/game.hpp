// SPDX-License-Identifier: GPL-3.0-or-later
// Schauplatz-Basisklasse CGame und Schauplatzwechsel (docs/GAME.md, SCRIPTING.md).
#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <vector>
#include <memory>
#include <string>
#include <string_view>

#include "game/gamestate.hpp"
#include "game/savegame.hpp"
#include "engine/animlayer.hpp"
#include "engine/audio.hpp"
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
    GameContext(EngineGlobals& g_, DataPaths& paths_, Scene& scene_) : g(g_), paths(paths_), scene(scene_) {}
    EngineGlobals& g;
    DataPaths& paths;
    Scene& scene;
    HelpOverlay* help = nullptr;
    LocationRequest request;
    std::unique_ptr<Game> current;  // g_CurGame
    // Musik CSzene+0x1A8 und ihr Name +0x1AC: bleibt über Schauplatzwechsel bestehen
    std::unique_ptr<AudioStream> music;
    std::string musicName;
    // Spielzustand CSzene+0x5B8 (CGame+0x78)
    GameState state;
    // Hauptschleife (WinMain 0x00401E00), re-entrant für CGame_Wait:
    // pumpMessage = PeekMessage(PM_REMOVE) + Übersetzen/Verteilen; Rückgabe kNoMessage,
    // kMessage, kQuit (WM_QUIT 0x12) oder kCommand (WM_COMMAND 0x111, nicht verteilt).
    // idle = Game_Idle 0x004030C0 (ein Bild).
    static constexpr int kNoMessage = 0, kMessage = 1, kQuit = 0x12, kCommand = 0x111;
    std::function<int()> pumpMessage;
    std::function<void()> idle;
    // Port: im verschachtelten Game_Idle gelöschte Schauplätze; das Original gibt den
    // Speicher sofort frei, die wartende Funktion liest danach freigegebenen Speicher.
    // Freigabe hier erst auf oberster Ebene (Tiefe 0).
    std::vector<std::unique_ptr<Game>> retired;
    void releaseRetired() { if (g.idleDepth == 0) retired.clear(); }
    // Spielstände (savegame.hpp)
    SaveGlobals save;
    // PostQuitMessage(0)
    std::function<void()> postQuit;
    // Spielmenü (MENU.md): g_MenuScene 0x0047D268, g_MenuGame 0x0047D270; Modus 0x00470808
    // (1 Optionen, 2 Spielstand). Bei offenem Menü bleibt `current` die gesicherte Spielklasse
    // (0x0047D76C) und `scene` die gesicherte Szene (0x0047D768).
    Scene* menuScene = nullptr;
    std::unique_ptr<Game> menu;
    int menuMode = 0;
    // g_CurScene / g_CurGame (bei offenem Menü die Menü-Objekte)
    Scene& curScene() { return g.gameMenuOpen && menuScene ? *menuScene : scene; }
    Game* curGame() { return g.gameMenuOpen ? menu.get() : current.get(); }
    // Plattform (MENU.md): MessageBoxA(MB_YESNO) → true bei IDYES; Namensdialog IDD_FILENAME an
    // der Bereichsposition (false = Abbrechen); Zeichenkette der EXE an einer VA (leer, wenn
    // unbekannt); GetAsyncKeyState(VK_LBUTTON); WM_COMMAND verteilen (0x00402870)
    std::function<bool(const std::string& text, const std::string& title)> askYesNo;
    std::function<bool(const Rect& slot, std::string& text)> editName;
    std::function<std::string(std::uint32_t va)> exeString;
    std::function<bool()> leftButtonDown;
    int lastCommand = 0;  // von pumpMessage mit kCommand geliefert
};

class Game {
public:
    // orig: Rent-A-Hero.exe 0x0043F4D0 CGame_ctor (+ Felder aus den LocNN-Ctoren)
    Game(GameContext& ctx, int location, int scene, int prevScene);
    // Menüklasse: eigene Szene g_MenuScene (0x00403520)
    Game(GameContext& ctx, Scene& scene, int location, int scene_, int prevScene);
    // orig: Rent-A-Hero.exe 0x0043F580 CGame_dtor (Rumpf in shutdown())
    virtual ~Game();
    // Destruktor-Rümpfe: erst der des Schauplatzes (destroyLocal), dann CGame_dtor. Game_Idle
    // Schritt 7 ruft das zum Zeitpunkt des Löschens; Unterklassen rufen es in ihrem Dtor.
    void shutdown();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;
    // orig: Rent-A-Hero.exe 0x0043F1B0 SetEventVariant(this) – diese Klasse empfängt die
    // Funktions-Handler (Wartestellen-Rückrufe)
    void bindEventVariant();
    // vtable +0x04 (Start), aus Game_SwitchLocation
    virtual void start() {}

protected:
    // Rumpf des LocNN-Destruktors (vor CGame_dtor)
    virtual void destroyLocal() {}

public:

    Actor* hero() const { return hero_; }
    AudioStream* musicStream() const { return music_; }
    void setMusic(AudioStream* m) { music_ = m; }
    int hintCountdown() const { return hintCountdown_; }
    const char* hintName() const { return hintName_; }
    // orig: Rent-A-Hero.exe 0x00440B80 CGame_Tick – frühestens alle 5001 ms (HINTS.md)
    void tick();
    // orig: Rent-A-Hero.exe 0x00440BE0 CGame_SetHint
    void setHint(const char* name, int ms);
    // orig: Rent-A-Hero.exe 0x00440C40 CGame_ShowHint
    void showHint();
    // orig: Rent-A-Hero.exe 0x004448D0 Hints_Select
    void selectHint();
    // +0x04 (Game_SwitchLocation: 1, wenn die Figur aus dem Spielstand gesetzt wurde)
    void setRestored(int v) { restored_ = v; }
    int location() const { return location_; }
    int sceneNumber() const { return sceneNr_; }
    int prevScene() const { return prevScene_; }

protected:
    using Handler = void (Game::*)(Event&, int);
    // orig: Rent-A-Hero.exe 0x0043FA30 CGame_SetEventProc
    void setEventProc(Handler h);
    template <class T>
    void setEventProc(void (T::*h)(Event&, int)) { setEventProc(static_cast<Handler>(h)); }
    // orig: Rent-A-Hero.exe 0x0043F880 CGame_BeginWait – freien Warteplatz belegen; liefert den
    // Rückruf CGame_WaitDone_i als Handler-Wert (0 und +0x4C = 5, wenn alle belegt)
    int beginWait();
    // orig: Rent-A-Hero.exe 0x0043F780 CGame_Wait – verschachtelte Nachrichtenschleife bis der
    // Platz ≠ 1; false = abgebrochen (bzw. WM_QUIT/WM_COMMAND)
    bool wait();
    // orig: Rent-A-Hero.exe 0x0043F360 CEvent_SetTimerHandler(ms, flags, handler)
    void setTimerHandler(Event& e, std::uint32_t ms, unsigned flags, int handler);
    // orig: Rent-A-Hero.exe 0x004409F0 CGame_Delay(ms)
    void delay(std::uint32_t ms);
    // orig: Rent-A-Hero.exe 0x00440890 CGame_WalkAndComment / 0x004408D0 CGame_Comment /
    // 0x00440960 CGame_HeroSay
    void walkAndComment(int x, int y, int dir, std::string_view key);
    void comment(std::string_view key);
    void heroSay(std::string_view key);
    // orig: Rent-A-Hero.exe 0x0043F6C0 CGame_GetViewObjects (ohne Figuren)
    void getViewObjects();
    // orig: Rent-A-Hero.exe 0x00440A80 CGame_ChangeScene(scene, loc)
    void changeScene(int scene, int location);
    // orig: Rent-A-Hero.exe 0x0043FFB0 CGame_LoadAnim
    std::unique_ptr<AnimLayer> loadAnim(std::string_view name) { return scene_.loadAnim(name); }
    // orig: Rent-A-Hero.exe 0x004404C0 CGame_Say(ger, name)
    bool say(std::string_view ger, std::string_view name);
    // orig: Rent-A-Hero.exe 0x00440430 CGame_Speak(ger, name, sprecher, modus, von, bis, ereignis):
    // sprecher == Held (+0x1C) → Figur-Variante (Modus −1)
    bool speak(std::string_view ger, std::string_view name, AnimLayer* speaker, int mode, int from, int to,
               int doneEvent);
    // orig: Rent-A-Hero.exe 0x004403E0 → CGame_Speak(ger, name, 0, 0, −1, −1, ereignis)
    bool speak(std::string_view ger, std::string_view name, int doneEvent) {
        return speak(ger, name, nullptr, 0, -1, -1, doneEvent);
    }
    // orig: Rent-A-Hero.exe 0x00440400 CGame_SayActor → CGame_Speak(ger, name, figur, 0x0C, −1, −1, ereignis)
    bool sayActor(std::string_view ger, std::string_view name, AnimLayer* speaker, int doneEvent) {
        return speak(ger, name, speaker, 0x0C, -1, -1, doneEvent);
    }
    // orig: Rent-A-Hero.exe 0x004409C0 CGame_FaceAndSay – Held dreht sich nach vorn, dann HeroSayWait
    void faceAndSay(std::string_view key);
    // orig: Rent-A-Hero.exe 0x004406E0 CGame_PlayAnimWait – als Vollbildobjekt bis zum Ende abspielen
    bool playAnimWait(AnimLayer* a);
    // orig: Rent-A-Hero.exe 0x00440AD0 CGame_DescribeItem – „DefInvent“, Schlüssel INVENT%03d
    // (Held sichtbar: anhalten, nach vorn drehen, warten) bzw. INVENT%03dH ohne Figur
    void describeItem(int id);
    // orig: Rent-A-Hero.exe 0x004400C0 CGame_PlayMusic – gleiche Musik läuft weiter, sonst die
    // alte ausblenden (2 s, dann löschen) und die neue laden (nicht abspielen)
    AudioStream* playMusic(std::string_view name);
    // orig: Rent-A-Hero.exe 0x004402F0 CGame_LoadSfx / 0x004403A0 CGame_LoadSfxVol
    std::unique_ptr<AudioBuffer> loadSfx(std::string_view name);
    std::unique_ptr<AudioBuffer> loadSfxVol(std::string_view name, int vol);
    // orig: Rent-A-Hero.exe 0x00440690 CGame_Random(von, bis) – +0x20 = +0x20 · 0x7F05 / 0xB2
    // (vorzeichenbehaftet, 32 Bit), Ergebnis |+0x20| mod (bis − von + 1) + von, höchstens bis
    int random(int lo, int hi);
    // orig: Rent-A-Hero.exe 0x00440290 CGame_PlaySfxRandom3D – z, dann x = Zufall(−100…100) · 0.01
    void playSfxRandom3D(AudioBuffer& b);

    GameContext& ctx_;
    EngineGlobals& g_;
    Scene& scene_;            // +0x50
    int location_;            // +0x28
    int sceneNr_;             // +0x2C
    int prevScene_;           // +0x30
    AudioStream* music_ = nullptr;  // +0x08 (gehört GameContext::music)
    Actor* hero_ = nullptr;         // +0x1C Held (CSzene+0x5E0)
    Actor* second_ = nullptr;       // +0x58 zweite Figur (CSzene+0x5E4)
    std::array<int, 5> waits_{};  // +0x34 Warteplätze (SCRIPTING.md)
    int lastWait_ = 0;            // +0x4C
    std::uint32_t seed_;      // +0x20 timeGetTime()
    // +0x04: 1, wenn die Figur aus dem Spielstand gesetzt wurde (SCRIPTING.md) – folgt mit dem Laden
    int restored_ = 0;
    // +0x10 Hinweis-Countdown (GetViewObjects lädt state[Szene], Save_Write schreibt zurück; HINTS.md)
    int hintCountdown_ = 0;
    std::uint32_t hintTickTime_ = 0;  // +0x14 timeGetTime() des letzten Ticks
    const char* hintName_ = nullptr;  // +0x18 HLP-Name (Zeiger auf Literal wie im Original)
    std::unique_ptr<Speech> speech_;  // +0x70

private:
    void stopSpeech();
    // orig: Rent-A-Hero.exe 0x0043F910 / 0x0043F940 / 0x0043F970 / 0x0043F9B0 / 0x0043F9E0
    // CGame_WaitDone_i(ereignis, typ)
    void waitDone(int i, Event& e, int type);
    bool shutDown_ = false;

public:
    // Port: Handler-Wert für „Ereignisprozedur des Schauplatzes“ (im Original deren Adresse,
    // von CGame_SetEventProc 0x0043FA30 als Benutzen-Handler an die Leiste gegeben)
    static constexpr int kEventProcHandler = 0x0043FA30;
    static constexpr std::array<int, 5> kWaitDone = {0x0043F910, 0x0043F940, 0x0043F970, 0x0043F9B0, 0x0043F9E0};
    const std::array<int, 5>& waitSlots() const { return waits_; }
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
