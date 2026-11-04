// SPDX-License-Identifier: GPL-3.0-or-later
// Gemeinsame ImGui-Oberfläche des Ports: Einrichtung, Bild-Einstellungen, Overlay im Spiel
// (Alt-Tasten, Einblendung). Kein Original-Gegenstück (docs/LAUNCHER.md, docs/FILTERS.md).
#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include <SDL3/SDL.h>

#include "app/launchersettings.hpp"
#include "app/present/presenter.hpp"

namespace rah {

// ImGui-Kontext, Stil und Schrift (System-Arial) für das Fenster; danach presenter.imguiInit()
void uiCreate(SDL_Window* win);
void uiDestroy();

// Bild-Einstellungen als ImGui-Bedienelemente; true bei Änderung
bool displayControls(DisplaySettings& d, bool filtersAvailable, bool* fullscreen);

// Bild-Einstellungen übernehmen; Fehler als Text (leer: in Ordnung)
std::string applyDisplay(Presenter& p, const DisplaySettings& d);

// Ein Hotspot der aktuellen Szene für die Anzeige mit Tab: Zellen (4×4) in Bildkoordinaten 640×480
struct HotspotView {
    std::string label;  // UTF-8, leer: ohne Beschriftung
    std::vector<SDL_FRect> cells;
};

// Alt-Tasten und Overlay während des Spiels
class GameOverlay {
public:
    GameOverlay(SDL_Window* win, Presenter& p, LauncherSettings& s, std::filesystem::path iniFile)
        : win_(win), p_(p), s_(s), ini_(std::move(iniFile)) {}
    // true: Ereignis gehört dem Overlay (nicht an das Spiel weitergeben)
    bool handleEvent(const SDL_Event& ev);
    // Bild ausgeben (mit Overlay/Einblendung, wenn sichtbar)
    void present(SDL_Surface** grab = nullptr);
    void toast(std::string text);
    bool open() const { return open_; }
    // Hotspots der aktuellen Szene (Tab, docs/HOTSPOTS.md); force: immer zeigen (Prüfung)
    void setHotspotSource(std::function<std::vector<HotspotView>()> f) { hotspotSrc_ = std::move(f); }
    void forceHotspots(bool on) { forceHotspots_ = on; }

private:
    void changed(const std::string& label);
    SDL_Window* win_;
    Presenter& p_;
    LauncherSettings& s_;
    std::filesystem::path ini_;
    bool open_ = false;
    std::string toast_;
    std::uint64_t toastUntil_ = 0;
    void drawHotspots(const std::vector<HotspotView>& hs);
    std::function<std::vector<HotspotView>()> hotspotSrc_;
    bool hotspots_ = false, forceHotspots_ = false;
};

}  // namespace rah
