// SPDX-License-Identifier: GPL-3.0-or-later
// Spielmenü (Menüklasse 0x004436B0, 0x22C Bytes, vtable 0x0046B908; docs/MENU.md)
#pragma once

#include <array>
#include <memory>

#include "game/game.hpp"

namespace rah {

class GameMenu : public Game {
public:
    // orig: Rent-A-Hero.exe 0x004436B0 – auf der Menü-Szene g_MenuScene
    GameMenu(GameContext& ctx, Scene& menuScene);
    ~GameMenu() override;
    // orig: Rent-A-Hero.exe 0x00443A60 (vtable +0x04)
    void start() override;
    // orig: Rent-A-Hero.exe 0x00444830 – aus CSzene_Compose der Menü-Szene (+0xAC8)
    void draw();

    // Ereignis-IDs (0x00443A60)
    enum : int {
        kOptions = 1, kSaves = 2, kNewGame = 3, kQuit = 4, kSubOn = 7, kSubOff = 8,
        kSliderMouse = 0x0D, kSliderMusic = 0x0E, kSliderSfx = 0x0F, kSliderSpeech = 0x10,
        kLoad = 0x11, kSave = 0x12, kFile1 = 0x1F,
    };

private:
    // orig: Rent-A-Hero.exe 0x00444010
    void onEvent(Event& e, int type);
    // orig: Rent-A-Hero.exe 0x00443C40 – Seite des Modus 0x00470808 aufbauen
    void buildPage();
    // orig: Rent-A-Hero.exe 0x004445D0 – Regler ziehen, solange die linke Taste gedrückt ist
    int dragSlider(Layer* knob);
    // orig: Rent-A-Hero.exe 0x004446C0 – Namen der neun Plätze lesen
    void readSlots();
    // orig: Rent-A-Hero.exe 0x004447E0 – Namen auf die Szenenoberfläche
    void drawNames();
    // orig: Rent-A-Hero.exe 0x004448C0 – Bereich des aktuellen Platzes
    Area* currentSlot() const;
    void enableKnob(Layer* back, Layer* knob, int percent);

    Layer* mainMenu_ = nullptr;     // +0x198
    Layer* rightMenu_ = nullptr;    // +0x19C
    Layer* optHead_ = nullptr;      // +0x1A0
    Layer* subtitles_ = nullptr;    // +0x1A4
    Layer* infoText_ = nullptr;     // +0x1A8
    Layer* music_ = nullptr;        // +0x1AC
    Layer* effects_ = nullptr;      // +0x1B0
    Layer* speechL_ = nullptr;      // +0x1B4
    Layer* buttonOn_ = nullptr;     // +0x1B8
    Layer* buttonOff_ = nullptr;    // +0x1BC
    std::array<Layer*, 4> sliderBack_{};  // +0x1C0, +0x1C8, +0x1D0, +0x1D8 (Maus, Musik, Effekte, Sprache)
    std::array<Layer*, 4> slider_{};      // +0x1C4, +0x1CC, +0x1D4, +0x1DC
    Layer* loadL_ = nullptr;        // +0x1E0
    Layer* saveL_ = nullptr;        // +0x1E4
    Area* options_ = nullptr;       // +0x1E8
    Area* saves_ = nullptr;         // +0x1EC
    Area* newGame_ = nullptr;       // +0x1F0
    Area* quit_ = nullptr;          // +0x1F4
    std::array<Area*, 9> files_{};  // +0x1F8
    std::unique_ptr<AudioBuffer> sfxLarge_, sfxSmall_, sfxFire_, rod_;  // +0x21C … +0x228
    bool slideIn_ = true;           // +0x80
    bool baked_ = false;            // +0x84 Rahmen im Hintergrund
    bool drawNames_ = false;        // +0x88
    bool drawFrame_ = false;        // +0x8C
};

// orig: Rent-A-Hero.exe 0x00403520 – Menü-Szene und -Klasse beim Programmstart anlegen
void createGameMenu(GameContext& ctx, Scene& menuScene);
// orig: Rent-A-Hero.exe 0x00403370 GameMenu_Open (Modus vorher in ctx.menuMode)
void gameMenuOpen(GameContext& ctx);
// orig: Rent-A-Hero.exe 0x00403480 GameMenu_Close
void gameMenuClose(GameContext& ctx);
// orig: Rent-A-Hero.exe 0x00402870 – WM_COMMAND-Verteiler
void onCommand(GameContext& ctx, int id);

}  // namespace rah
