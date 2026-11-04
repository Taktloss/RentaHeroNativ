// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/inventory.hpp"

#include <cstdio>

#include "engine/scene.hpp"
#include "formats/ini.hpp"

namespace rah {

namespace {

Rect shiftedUp(Rect r) {
    r.top -= 60;
    r.bottom -= 60;
    return r;
}

}  // namespace

void rendererSetStatusText(EngineGlobals& g, const Rect& rect, const std::string& text) {
    g.statusText = text;
    if (!text.empty()) g.statusRect = rect;
    if (text.empty()) {
        if (g.outputSceneRect) g.outputSceneRect(shiftedUp(g.statusRect));
        return;
    }
    if (g.drawStatusText) g.drawStatusText(g.statusRect, g.statusText);
}

void rendererClearStatusText(EngineGlobals& g) {
    g.statusText.clear();  // Leerstring 0x0047D770
    if (g.outputSceneRect) g.outputSceneRect(shiftedUp(g.statusRect));
}

void rendererRedrawStatusText(EngineGlobals& g) {
    if (g.statusText.empty()) return;
    rendererSetStatusText(g, g.statusRect, g.statusText);
}

InventoryBar::InventoryBar(Scene& scene, EngineGlobals& g) : scene_(scene), g_(g) {
    area_ = std::make_unique<Area>();
    area_->rect = {0, kTop, 640, kBottom};  // direkt gesetzt, ohne Raster
    area_->setName("Inventory");
    // orig: Rent-A-Hero.exe 0x00436480 – Funktions-Handler, aktiviert, Bereich aktiv
    area_->event->setHandler([this](Event&, int type) { onAreaEvent(type); });
    area_->event->enabled = true;
    area_->active = true;
    updateRect();
}

InventoryBar::~InventoryBar() = default;

void InventoryBar::updateRect() {
    if (count_ == 0) {
        rect_ = {};
        return;
    }
    rect_ = {0, kTop, count_ < 10 ? count_ << 6 : 640, kBottom};
}

void InventoryBar::blank() {
    // Blt DDBLT_COLORFILL (Farbe 0) mit dem Bereichsrechteck
    const Rect& r = area_->rect;
    scene_.surface().fill(r.left, r.top, r.right, r.bottom, 0);
    redraw_ = true;
    full_ = true;
}

int InventoryBar::find(int id) const {
    for (int i = 0; i < kSlots; ++i)
        if (entries_[static_cast<std::size_t>(i)].id == id) return i;
    return -1;
}

bool InventoryBar::has(int id) const {
    for (int i = 0; i < count_; ++i) {
        const Entry& e = entries_[static_cast<std::size_t>(i)];
        if (e.id == id && e.used) return true;
    }
    return false;
}

bool InventoryBar::add(int id) {
    if (has(id)) return true;
    for (int i = 0; i < kSlots; ++i) {
        Entry& e = entries_[static_cast<std::size_t>(i)];
        if (e.layer) continue;  // erster Platz ohne Ebene (nicht Index = Anzahl)
        char name[16];
        std::snprintf(name, sizeof name, "Item%03d", id);
        const auto file = scene_.paths().findFile(name, "A16");
        if (!file) return false;
        auto L = std::make_unique<AnimLayer>(g_);
        // CAnimLayer Load (Slot 2) + Slot 3; 0x00441A32 PUSH 0 an der Stelle, an der CSzene_CreateBackground
        // den Farbschlüssel +0x2D8 übergibt (0x00449E70) → Farbschlüssel 0
        if (!L->load(*file, 0, nullptr)) return false;
        L->renderToSurface();  // 0x00446C90
        // 0x00447F00: Name = Dateiname ohne Pfad und Endung
        const auto slash = L->path.find_last_of("/\\:");
        L->name = slash == std::string::npos ? L->path : L->path.substr(slash + 1);
        if (const auto dot = L->name.rfind('.'); dot != std::string::npos) L->name.resize(dot);
        L->zoom = 100.0;
        L->fps = 10;
        L->enable();
        e.id = id;
        e.layer = std::move(L);
        e.used = true;
        e.eventId = 0;
        redraw_ = true;
        ++count_;
        updateRect();
        e.layer->start(12);
        return true;
    }
    return false;
}

bool InventoryBar::remove(int id) {
    for (int i = 0; i < count_; ++i) {
        if (entries_[static_cast<std::size_t>(i)].id != id) continue;
        // Ebene und Bereich löschen, Rest um einen Platz nach vorn (bis Platz 49), letzten leeren
        for (int j = i; j < kSlots - 1; ++j)
            entries_[static_cast<std::size_t>(j)] = std::move(entries_[static_cast<std::size_t>(j + 1)]);
        entries_[kSlots - 1] = Entry{};
        redraw_ = true;
        --count_;
        blank();
        updateRect();
        if (hovered_ == i) hovered_ = -1;  // Eigenheit: dahinter liegendes +0x414 rückt nicht mit
        return true;
    }
    return false;
}

void InventoryBar::clear() {
    for (int i = 0; i < count_; ++i) entries_[static_cast<std::size_t>(i)] = Entry{};
    count_ = 0;
    redraw_ = true;
    blank();
    updateRect();
}

void InventoryBar::setItemEvent(int id, int ev) {
    const int i = find(id);
    if (i == -1) return;
    entries_[static_cast<std::size_t>(i)].eventId = ev;
    itemEvents_ = true;
    setUseHandler(useHandler_);
}

void InventoryBar::clearItemEvents() {
    for (Entry& e : entries_) e.eventId = 0;  // +0x10 je Eintrag
    itemEvents_ = false;                      // +0x418
    useHandler_ = 0;                          // +0x41C
    describe_ = nullptr;                      // +0x420
}

void InventoryBar::setUseHandler(int h) {
    if (!useEvent_) useEvent_ = std::make_unique<Event>();
    useEvent_->setHandler(h);
    if (h != 0) {
        useEvent_->enabled = true;
        if (h != useHandler_) itemEvents_ = false;
    }
}

void InventoryBar::setDescribe(int h, std::function<void(int id)> describeItem) {
    useHandler_ = h;
    describe_ = std::move(describeItem);
    itemEvents_ = h != 0;
}

void InventoryBar::tick() {
    for (int i = 0; i < count_; ++i) {
        Entry& e = entries_[static_cast<std::size_t>(i)];
        if (!e.used || !e.layer) return;
        e.layer->update(g_);
        if (e.layer->dirty) redraw_ = true;
    }
}

bool InventoryBar::draw() {
    if (!redraw_) return false;
    int x = 0;
    for (int i = first_; i < count_; ++i, x += kSlotWidth) {
        Entry& e = entries_[static_cast<std::size_t>(i)];
        if (x > 0x27F || !e.used || !e.layer) break;
        // jedes Mal neu getönt und damit neu dekodiert (A16: kumuliert, tcc.md)
        const int c = bright_ ? 0 : -128;
        e.layer->setColor(c, c, c);
        e.layer->setPos(x, kTop);
        e.layer->draw(scene_.surface(), 0, nullptr);  // 0x00447150: flag 0, ganzes Bild
    }
    redraw_ = false;
    return true;
}

Rect InventoryBar::takeOutputRect() {
    if (!full_) return rect_;
    full_ = false;
    return area_->rect;
}

void InventoryBar::leave(bool left) {
    if (bright_ && left) {
        rendererClearStatusText(g_);
        bright_ = false;
        redraw_ = true;
    }
    if (hovered_ != -1) {
        // Slot 9 = CAnimLayer_Stop(0). Original prüft die Ebene nicht. Leer werden kann der Platz durch
        // die Eigenheit in remove() (+0x414 rückt nicht mit): überfahrener letzter Gegenstand, davor
        // einer entfernt → Original ruft über Zeiger 0 (Absturz). Port: Absturzschutz (Abweichung).
        if (auto& L = entries_[static_cast<std::size_t>(hovered_)].layer) L->stop(0);
        redraw_ = true;
        hovered_ = -1;
    }
}

void InventoryBar::onAreaEvent(int type) {
    const auto x = static_cast<unsigned>(scene_.mouseX());  // CSzene+0x500
    if (x < 5 && first_ > 0) {
        --first_;
        redraw_ = true;
        return;
    }
    if (x > 0x27A && count_ - first_ > 10) {
        ++first_;
        redraw_ = true;
        return;
    }
    const int i = static_cast<int>(x >> 6) + first_;
    if (i != hovered_ && hovered_ != -1) {
        leave(false);
        hovered_ = -1;
    }
    if (!bright_) {
        bright_ = true;
        redraw_ = true;
    }
    if (i > count_ - 1) return;
    Entry& e = entries_[static_cast<std::size_t>(i)];
    if (type == 0x0D) {
        if (hovered_ == -1) {
            if (e.layer) {  // Original prüft nicht; i < count_ hat immer eine Ebene (add() legt nur mit an)
                e.layer->ended = false;
                e.layer->start(13);  // 0x00443240: Schleife
            }
            hovered_ = i;
            showText(e.id);
        }
        return;
    }
    lastClicked_ = e.id;
    if (useEvent_) {
        if (!itemEvents_) {
            useEvent_->mark(g_, lastClicked_);
            return;
        }
        if (e.eventId != 0) {
            useEvent_->mark(g_, e.eventId);
            return;
        }
    }
    // CGame_DescribeItem(+0x420, id). +0x420 setzt nur CGame_SetEventProc (0x0043FA4A), CGame_dtor
    // nullt es (0x004422A8); jeder Schauplatz ruft setEventProc in start() – ohne CGame (Absturz im
    // Original) gibt es keinen Inventarklick
    if (describe_) describe_(lastClicked_);
}

void InventoryBar::showText(int id) {
    char key[16];
    std::snprintf(key, sizeof key, "TEXT%03d", id);
    std::string text = "NULL";
    if (const auto f = scene_.paths().findFile("DefInvent", "GER")) {
        if (const auto data = f->read()) text = IniFile(*data).getString("SPEECH", key, "NULL", 100);
    }
    if (text == "NULL") text = std::string("[") + key + "]";
    rendererClearStatusText(g_);
    rendererSetStatusText(g_, Rect{40, 424, 600, 475}, text);
}

}  // namespace rah
