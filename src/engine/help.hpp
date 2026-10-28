// SPDX-License-Identifier: GPL-3.0-or-later
// Hilfe-Objekt (CHelp, global 0x0047D150, CSzene+0x9F8): Hilfe-Symbol oben rechts und
// Hilfefenster mit HLP-Text (docs/HINTS.md, docs/formats/hlp.md).
#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "core/rect.hpp"
#include "engine/globals.hpp"
#include "engine/layer.hpp"

namespace rah {

class Scene;

// orig: Rent-A-Hero.exe 0x004416F0 Hlp_Decode – erstes Byte ≥ 0x80 → alles XOR 0x80
void hlpDecode(std::string& text);

class HelpOverlay {
public:
    // orig: Rent-A-Hero.exe 0x00440F70 (rechte Seite als Vorgabe, 0x00441080)
    explicit HelpOverlay(EngineGlobals& g);
    ~HelpOverlay();
    HelpOverlay(const HelpOverlay&) = delete;
    HelpOverlay& operator=(const HelpOverlay&) = delete;

    // orig: Rent-A-Hero.exe 0x004410C0(hwnd, renderer, szene, game) – Fenster und Renderer
    // liefert die Plattform; showHint steht für CGame_ShowHint(game) (0x00440C40)
    void setup(Scene* scene, std::function<void()> showHint, bool newRenderer);
    // orig: Rent-A-Hero.exe 0x00441110 – nullptr: gespeicherten Pfad erneut laden
    void load(const std::string* name);
    // orig: Rent-A-Hero.exe 0x004414A0 CHelp_IconOn
    void iconOn();
    // orig: Rent-A-Hero.exe 0x004414B0 CHelp_IconOff
    void iconOff();
    // orig: Rent-A-Hero.exe 0x00441490 – Symbol neu zeichnen, wenn sichtbar
    void redrawIcon();
    // orig: Rent-A-Hero.exe 0x004414C0 CHelp_Show
    void show(std::string_view name);
    // orig: Rent-A-Hero.exe 0x004416C0
    void close();
    // orig: Rent-A-Hero.exe 0x004416E0 – Klick auf das Symbol
    void onIconClick();
    // orig: Rent-A-Hero.exe 0x00440FF0 – Ebenen freigeben
    void release();

    bool iconShown() const { return iconShown_; }
    bool isOpen() const { return open_; }
    const Rect& textRect() const { return textRect_; }
    Layer* icon() { return icon_.get(); }

private:
    // orig: Rent-A-Hero.exe 0x004413E0 – Symbol direkt auf den Bildschirm
    void drawIcon();
    // orig: Rent-A-Hero.exe 0x00441450 – Symbolrechteck auf dem Bildschirm schwarz
    void clearIcon();
    // orig: Rent-A-Hero.exe 0x00441040 / 0x00441080 – Text und Maske links bzw. rechts
    void useLeft();
    void useRight();
    std::unique_ptr<Layer> loadLayer(const std::optional<FoundFile>& file, int x, int y);

    EngineGlobals& g_;
    bool iconShown_ = false;                 // +0x00
    bool open_ = false;                      // +0x04
    std::unique_ptr<Layer> icon_;            // +0x08
    std::unique_ptr<Layer> maskLeft_;        // +0x0C
    std::unique_ptr<Layer> maskRight_;       // +0x10
    Layer* mask_ = nullptr;                  // +0x14
    Scene* scene_ = nullptr;                 // +0x1C
    std::function<void()> showHint_;         // +0x20 (CGame*)
    std::optional<FoundFile> iconFile_;      // +0x28
    Rect textRect_;                          // +0xF0
};

}  // namespace rah
