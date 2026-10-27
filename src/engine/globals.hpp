// SPDX-License-Identifier: GPL-3.0-or-later
// Globale Zustände des Originals, die mehrere Objekte (Spiel- und Menüszene, Ebenen,
// Ereignisse) teilen.
#pragma once

#include <cstdint>
#include <array>
#include <functional>
#include <memory>
#include <vector>

#include "engine/area.hpp"
#include "engine/event.hpp"

namespace rah {

struct AnimCue;
class AnimLayer;
class Scene;
class Surface565;

struct EngineGlobals {
    bool sceneChanged = false;  // g_SceneChanged 0x00481F28
    // g_EventProc 0x0047E60C: OnGenericEvent des aktuellen Schauplatzes
    std::function<void(Event&, int type)> eventProc;
    HitGrid hitGrid;       // 0x0047D77C
    Event clickEvent;      // 0x00481EE8 (statisch, Typ 0x16; EVENTS.md)
    std::array<Event*, 10> timerEvents{};  // g_TimerEvents 0x0047E614
    std::vector<Event*> runningTimers;     // Port: statt timeSetEvent-Threads
    int eventVariant = 0;  // 0x0047E608 (aus 0x0047D26C gesetzt; Bedeutung offen)
    // Bereichstext unter der Maus (docs/MOUSE.md)
    bool textVisible = false;              // 0x00481F24
    std::unique_ptr<Event> toggleEvent;    // 0x00481F2C, Handler 0x0044C8D0
    Scene* toggleScene = nullptr;          // 0x00481F30
    std::uint32_t mouseDelay = 250;        // g_MouseDelay 0x004764AC
    // CArea_DrawText (0x00436230) auf die Szenenoberfläche; Schrift liefert die Plattform
    std::function<void(Surface565&, const Area&, int x, int y)> drawAreaText;
    // timeGetTime() – im Spiel die echte Uhr, in Tests steuerbar
    std::function<std::uint32_t()> now = [] { return std::uint32_t{0}; };
    // Ton-Cues einer Animation (AUDIO.md); bis zum Audio-Port nur ein Haken
    std::function<void(AnimLayer&, const AnimCue&)> runCue;
};

}  // namespace rah
