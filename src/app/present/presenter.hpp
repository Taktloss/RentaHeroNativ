// SPDX-License-Identifier: GPL-3.0-or-later
// Ausgabe des 640×480-Bildes ins Fenster, mit Bildfiltern (OpenGL) oder ohne (SDL_Renderer).
// Port, kein Original-Gegenstück: dort DirectDraw-Vollbild 640×480 (docs/FILTERS.md).
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include <SDL3/SDL.h>

#include "app/present/filters.hpp"

namespace rah {

class Presenter {
public:
    enum class Kind { OpenGL, SdlRenderer };
    // OpenGL 3.3 (Kompatibilitätsprofil) im Fenster (SDL_WINDOW_OPENGL) versuchen, sonst
    // SDL_Renderer. *log: warum OpenGL nicht verfügbar ist.
    static std::unique_ptr<Presenter> create(SDL_Window* win, bool tryGl, std::string* log);
    virtual ~Presenter() = default;

    virtual Kind kind() const = 0;
    virtual std::string info() const = 0;
    bool supportsFilters() const { return kind() == Kind::OpenGL; }

    // ImGui-Backends; den ImGui-Kontext legt der Aufrufer an
    virtual void imguiInit() = 0;
    virtual void imguiShutdown() = 0;
    virtual void imguiNewFrame() = 0;

    // Spielbild (RGB565, Zeilen ohne Lücke); markiert nur bei Änderung neu
    virtual void setFrame(const std::uint16_t* px, int w, int h) = 0;
    virtual void clearFrame() = 0;
    // Bildfilter; false (+*err): nicht verfügbar, es gilt dann „Original“ bzw. „Weich“
    virtual bool setDisplay(const DisplaySettings& d, std::string* err) = 0;
    const DisplaySettings& display() const { return display_; }

    // Zeichnen und anzeigen. Ohne Änderung (Bild, Fenstergröße, Einstellungen) und ohne ImGui
    // geschieht nichts. grab: Bild vor dem Anzeigen auslesen (Prüfmodus; Aufrufer gibt frei).
    virtual void present(bool withImGui, SDL_Surface** grab = nullptr) = 0;
    // Fensterkoordinaten eines SDL-Ereignisses → Bildkoordinaten (außerhalb möglich)
    virtual void windowToFrame(float wx, float wy, float* fx, float* fy) const = 0;

protected:
    DisplaySettings display_;
};

}  // namespace rah
