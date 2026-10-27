// SPDX-License-Identifier: GPL-3.0-or-later
// Ereignis (CEvent, 0x24 Bytes, Ctor 0x0043F130) und Auslösen (docs/EVENTS.md).
#pragma once

#include <cstdint>
#include <functional>

namespace rah {

struct EngineGlobals;
struct Event;

// Funktionszeiger-Handler des Originals (Handler ≥ 0xFFF), z. B. CGame_WaitDone_i
using EventFn = std::function<void(Event&, int type)>;

struct Event {
    Event() = default;
    Event(const Event&) = delete;
    Event& operator=(const Event&) = delete;
    ~Event();  // meldet einen laufenden Timer ab

    bool enabled = false;    // +0x00
    bool pending = false;    // +0x04
    int unk08 = 1;           // +0x08 (mit g_EventVariant: Param = Typ bei Funktions-Handlern)
    bool immediate = false;  // +0x0C Sofort-Flag
    int type = 0;            // +0x10
    int handlerId = 0;       // +0x1C, < 0xFFF: Ereignis-ID für g_EventProc
    EventFn handlerFn;       // +0x1C, ≥ 0xFFF: Funktionszeiger
    int param = 0;           // +0x20

    // CTimer +0x18 (docs/TIMING.md „Multimedia-Timer“): Fälligkeit wie timeSetEvent
    bool hasTimer = false;        // +0x18 != 0
    bool timerRunning = false;
    bool timerPeriodic = false;   // flags & TIME_PERIODIC
    std::uint32_t timerDelay = 0;
    std::uint32_t timerDue = 0;
    EngineGlobals* timerOwner = nullptr;

    // orig: Rent-A-Hero.exe 0x0043F260 CEvent_SetHandler
    void setHandler(int id) {
        pending = false;
        handlerId = id;
        handlerFn = nullptr;
    }
    void setHandler(EventFn fn) {
        pending = false;
        handlerId = 0;
        handlerFn = std::move(fn);
    }
    bool hasHandler() const { return handlerId != 0 || handlerFn; }

    // Muster der Auslöser (z. B. CAnimLayer_Update 0x00442AA0): ausstehend markieren,
    // Typ und Param = Typ; Sofort-Flag → direkt auslösen.
    void mark(EngineGlobals& g, int t);
};

// orig: Rent-A-Hero.exe 0x0043F3C0 CEvent_Fire
void fireEvent(EngineGlobals& g, Event& e, int type);
// orig: Rent-A-Hero.exe 0x0043F2A0 CEvent_SetTimer(ms, flags) – flags 1 = periodisch
void setEventTimer(EngineGlobals& g, Event& e, std::uint32_t ms, unsigned flags);
// Ersatz für den Timer-Thread (Entscheidung 2026-10-27): in jedem Hauptschleifendurchlauf
// fällige Timer auslösen wie CEvent_Timer 0x0043F390 (Sofort-Flag → direkt CEvent_Fire)
void pollTimers(EngineGlobals& g);
// orig: Rent-A-Hero.exe 0x0043F4A0 CEvent_FireTimers
void fireTimers(EngineGlobals& g);

}  // namespace rah
