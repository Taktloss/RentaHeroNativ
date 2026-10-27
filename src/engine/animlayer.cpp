// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/animlayer.hpp"

namespace rah {

namespace {

void ensureEvent(std::unique_ptr<Event>& e) {
    if (!e) e = std::make_unique<Event>();
}

// Ereignis abschalten wie 0x00443030 bei ev = 0: Handler 0, aktiviert = ausstehend = 0
void clearEvent(std::unique_ptr<Event>& e) {
    if (!e) return;
    e->setHandler(0);
    e->enabled = false;
    e->pending = false;
}

}  // namespace

// orig: Rent-A-Hero.exe 0x004422E0 CAnimLayer_ctor
AnimLayer::AnimLayer(EngineGlobals& g) : g_(g) {
    fps = 10;
    isAnim = true;
}

bool AnimLayer::load(const FoundFile& file, std::uint16_t key, std::string* error) {
    path = file.path;
    auto data = file.read();
    if (!data) {
        if (error) *error = "Datei fehlt: " + file.path;
        return false;
    }
    kind = layerKindFromName(path);
    loaded = true;
    colorKey = key;
    if (active) dirty = true;
    if (kind != LayerKind::A16) {
        // Bildfolgen aus BMP/TCC (Typ < 2) kommen in den Daten nicht vor; AVI folgt gesondert
        if (error) *error = "Animationsart noch nicht portiert: " + path;
        return false;
    }
    frame = -1;
    if (!anim.load(std::move(*data), error)) return false;
    width = static_cast<int>(anim.planes().width);
    height = static_cast<int>(anim.planes().height);
    full = {0, 0, width, height};
    zoomed = full;
    hasImage = true;
    seenDecodes_ = anim.decodes();
    alpha = true;  // Slot 3 setzt +0x198 für TCC/A16/AVI unabhängig vom Alpha des Bilds
    nextTime = g_.now() + static_cast<std::uint32_t>(period);
    if (active) dirty = true;
    return true;
}

void AnimLayer::afterImageCall() {
    if (anim.decodes() != seenDecodes_) {
        seenDecodes_ = anim.decodes();
        tcc.afterDecode(anim.planes());
    }
}

bool AnimLayer::update(EngineGlobals& g) {
    Layer::update(g);
    if (endPending) {
        endPending = false;
        ended = true;
        if (endEvent) endEvent->mark(g, 1);
        return true;
    }
    if (!active || !running) return true;
    if (!nextFrame()) return false;
    if (fps != 0) period = 1000 / fps;
    if (period != 0) {
        nextTime += static_cast<std::uint32_t>(period);
        const std::uint32_t now = g.now();
        if (nextTime < now) {
            if (!catchUp()) return false;
            nextTime = static_cast<std::uint32_t>(period) + now;
        }
    }
    dirty = true;
    return true;
}

bool AnimLayer::nextFrame() {
    // Lippensync (+0x484) folgt mit der Sprachausgabe
    if (!reverse) {
        ++frame;
        if (!anim.next()) onEnd();
    } else {
        --frame;
        if (frame == 0) {
            onEnd();  // Bild 0 wird rückwärts nie gezeigt (Original)
        } else {
            anim.seek(frame);
            anim.next();
        }
    }
    afterImageCall();
    ++stepCount;
    if (stepCount == stepTarget && stepTarget != 0 && countEvent) countEvent->mark(g_, 4);
    if (frameEvent && frame == frameEventFrame) frameEvent->mark(g_, 3);
    runCues(frame);
    if (frame == rangeTo) onEnd();
    return true;
}

bool AnimLayer::catchUp() {
    // A16: Bild-Slot 5 (0x004387F0) liefert nur 1 → kein Bildwechsel, aber Bild-Ereignis und
    // Cues des aktuellen Bilds noch einmal
    if (frameEvent && frame == frameEventFrame) frameEvent->mark(g_, 3);
    runCues(frame);
    return true;
}

void AnimLayer::onEnd() {
    if (loop || endMode == 2) {
        if (rangeFrom == -1) {
            anim.rewind();
            frame = 0;
        } else {
            anim.seek(rangeFrom);
            anim.next();
            frame = rangeFrom;
        }
        afterImageCall();
    }
    if (!loop) {
        rangeTo = rangeFrom = -1;
        running = false;
        reverse = false;
        endPending = true;
    }
}

void AnimLayer::start(unsigned mode) {
    loop = false;
    active = true;
    running = true;
    endMode = 1;
    dirty = true;
    endPending = false;
    // Sprungtabelle 0x00443214: [0,0,5,5,5,5,5,5,5,5,1,2,3,4]
    static constexpr std::uint8_t kCase[14] = {0, 0, 5, 5, 5, 5, 5, 5, 5, 5, 1, 2, 3, 4};
    if (mode <= 13) {
        switch (kCase[mode]) {
        case 0: hideAtEnd = mode == 0; break;
        case 1: hideAtEnd = true; break;
        case 2: hideAtEnd = false; break;
        case 3:
            hideAtEnd = false;
            endMode = 2;
            break;
        case 4:
            loop = true;
            endMode = 1;
            active = running = true;
            hideAtEnd = false;
            dirty = true;
            break;
        default: break;
        }
    }
    if (ended) {
        anim.seek(0);
        frame = -1;
    }
    ended = false;
}

void AnimLayer::playEndEvent(unsigned mode, int ev) {
    if (ev == 0) {
        clearEvent(endEvent);
    } else {
        ensureEvent(endEvent);
        endEvent->setHandler(ev);
        endEvent->enabled = true;
    }
    start(mode);
}

namespace {

// Gemeinsamer Anfang von 0x00442E70 / 0x00442F90
template <class Seek>
void setRange(AnimLayer& a, int from, int to, Seek seek) {
    a.rangeFrom = from;
    a.rangeTo = to;
    if (from == -1) {
        a.reverse = false;
        return;
    }
    if (a.kind != LayerKind::Avi || from != 0) seek(from);
    a.reverse = to != -1 && to < from;
}

}  // namespace

void AnimLayer::playRange(unsigned mode, int ev, int from, int to) {
    setRange(*this, from, to, [this](int n) { seekFrame(n); });
    playEndEvent(mode, ev);
}

void AnimLayer::playRangeNoEvent(unsigned mode, int from, int to) {
    setRange(*this, from, to, [this](int n) { seekFrame(n); });
    clearEvent(endEvent);
    start(mode);
}

bool AnimLayer::seekFrame(int n) {
    if (n == -1) n = 0;
    if (!anim.seek(n)) return false;
    afterImageCall();
    frame = n < 1 ? -1 : n - 1;
    if (active) dirty = true;
    ended = false;
    return true;
}

void AnimLayer::showFrame(int n) {
    rangeTo = rangeFrom = -1;
    if (!seekFrame(n)) return;
    if (!nextFrame()) return;
    dirty = true;
}

void AnimLayer::setFrameEvent(int ev, int atFrame) {
    if (ev == 0) return;
    ensureEvent(frameEvent);
    frameEvent->setHandler(ev);
    frameEvent->enabled = true;
    frameEventFrame = atFrame;
}

void AnimLayer::stop(int x) {
    if (x != 0) {
        loop = false;
        endMode = x;
        return;
    }
    rangeTo = rangeFrom = -1;
    loop = false;
    running = false;
    ended = true;
    reverse = false;
    if (endEvent) endEvent->mark(g_, 1);
}

void AnimLayer::addCue(const AnimCue& c) {
    for (auto& slot : cues) {
        if (slot.type == 0) {
            slot = c;
            return;
        }
    }
    // mehr als 100 Cues werden still verworfen (AUDIO.md)
}

void AnimLayer::runCues(int atFrame) {
    for (const auto& c : cues) {
        if (c.type != 0 && c.frame == atFrame && g_.runCue) g_.runCue(*this, c);
    }
}

}  // namespace rah
