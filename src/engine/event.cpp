// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/event.hpp"

#include "engine/globals.hpp"

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

}  // namespace rah
