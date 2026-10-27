// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/event.hpp"

#include "engine/globals.hpp"

#include <algorithm>

namespace rah {

void Event::mark(EngineGlobals& g, int t) {
    pending = true;
    type = t;
    param = t;
    if (immediate) fireEvent(g, *this, t);
}

void fireEvent(EngineGlobals& g, Event& e, int type) {
    if (e.enabled && e.hasHandler()) {
        const bool was = e.pending;
        e.pending = false;
        if (!was) return;
        if (e.handlerFn) {
            if (g.eventVariant != 0 && e.unk08 != 0) e.param = type;
            EventFn fn = e.handlerFn;  // Handler darf das Ereignis neu setzen
            fn(e, type);
        } else if (e.handlerId < 0xFFF && g.eventProc) {
            e.param = e.handlerId;
            g.eventProc(e, type);
        }
    }
    e.pending = false;
}

Event::~Event() {
    if (!timerOwner) return;
    for (auto& slot : timerOwner->timerEvents) {
        if (slot == this) slot = nullptr;  // UNVERIFIED: Abmelden im Original-Dtor
    }
    std::erase(timerOwner->runningTimers, this);
}

void setEventTimer(EngineGlobals& g, Event& e, std::uint32_t ms, unsigned flags) {
    e.pending = false;
    if (!e.hasTimer) {
        // erstmals: ersten freien Platz der Liste belegen (voll → nicht eingetragen, der Timer
        // läuft trotzdem; ohne Sofort-Flag wird das Ereignis dann nie ausgelöst)
        for (auto& slot : g.timerEvents) {
            if (!slot) {
                slot = &e;
                break;
            }
        }
    }
    // alten Timer stoppen (CTimer_Stop) und neu anlegen
    e.hasTimer = true;
    e.enabled = true;
    e.timerOwner = &g;
    e.timerDelay = ms;
    e.timerPeriodic = (flags & 1) != 0;
    e.timerDue = g.now() + ms;
    e.timerRunning = true;
    if (std::find(g.runningTimers.begin(), g.runningTimers.end(), &e) == g.runningTimers.end()) {
        g.runningTimers.push_back(&e);
    }
}

void pollTimers(EngineGlobals& g) {
    const std::vector<Event*> snapshot = g.runningTimers;  // Handler dürfen Timer ändern
    for (Event* e : snapshot) {
        if (std::find(g.runningTimers.begin(), g.runningTimers.end(), e) == g.runningTimers.end()) continue;
        if (!e->timerRunning) continue;
        const std::uint32_t now = g.now();
        if (static_cast<std::int32_t>(now - e->timerDue) < 0) continue;
        if (e->timerPeriodic) {
            e->timerDue += e->timerDelay;
        } else {
            e->timerRunning = false;  // CTimer_Callback: einmalig → Kennung 0
            std::erase(g.runningTimers, e);
        }
        // orig: Rent-A-Hero.exe 0x0043F390 CEvent_Timer
        e->pending = true;
        e->type = 0x15;
        e->param = 0x15;
        if (e->immediate) fireEvent(g, *e, 0x15);
    }
}

void fireTimers(EngineGlobals& g) {
    for (Event* e : g.timerEvents) {
        if (e) fireEvent(g, *e, e->type);
    }
}

}  // namespace rah
