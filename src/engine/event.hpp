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
    bool enabled = false;    // +0x00
    bool pending = false;    // +0x04
    int unk08 = 1;           // +0x08 (mit g_EventVariant: Param = Typ bei Funktions-Handlern)
    bool immediate = false;  // +0x0C Sofort-Flag
    int type = 0;            // +0x10
    int handlerId = 0;       // +0x1C, < 0xFFF: Ereignis-ID für g_EventProc
    EventFn handlerFn;       // +0x1C, ≥ 0xFFF: Funktionszeiger
    int param = 0;           // +0x20

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

}  // namespace rah
