// SPDX-License-Identifier: GPL-3.0-or-later
// Inventarleiste (CSzene+0x5B4, 0x42C Bytes, Ctor 0x00441750) und Statustext des Renderers
// (docs/INVENTORY.md).
#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>

#include "core/rect.hpp"
#include "engine/animlayer.hpp"
#include "engine/area.hpp"
#include "engine/globals.hpp"

namespace rah {

class Scene;

// orig: Rent-A-Hero.exe 0x0043E600 – Statustext setzen und ausgeben (Text nicht leer →
// Rechteck merken; leer → Rechteck −60 aus der Szenenoberfläche neu ausgeben)
void rendererSetStatusText(EngineGlobals& g, const Rect& rect, const std::string& text);
// orig: Rent-A-Hero.exe 0x0043E7C0 – Statustext löschen, Rechteck neu ausgeben
void rendererClearStatusText(EngineGlobals& g);
// orig: Rent-A-Hero.exe 0x0043E830 – Statustext erneut zeichnen, falls nicht leer
void rendererRedrawStatusText(EngineGlobals& g);

class InventoryBar {
public:
    static constexpr int kSlots = 50;
    static constexpr int kSlotWidth = 64;
    static constexpr int kTop = 360, kBottom = 420;

    struct Entry {                        // 0x14 Bytes
        int id = 0;                       // +0x00
        bool used = false;                // +0x04
        std::unique_ptr<AnimLayer> layer; // +0x08
        std::unique_ptr<Area> area;       // +0x0C (nur freigegeben, nie angelegt)
        int eventId = 0;                  // +0x10
    };

    // orig: Rent-A-Hero.exe 0x00441750 (+0x3F4 = Szene, gesetzt in CSzene_ctor)
    InventoryBar(Scene& scene, EngineGlobals& g);
    ~InventoryBar();
    InventoryBar(const InventoryBar&) = delete;
    InventoryBar& operator=(const InventoryBar&) = delete;

    // orig: Rent-A-Hero.exe 0x00441BF0 Inventory_Has
    bool has(int id) const;
    // orig: Rent-A-Hero.exe 0x00441960 Inventory_Add
    bool add(int id);
    // orig: Rent-A-Hero.exe 0x00441B00 Inventory_Remove
    bool remove(int id);
    // orig: Rent-A-Hero.exe 0x004418D0 Inventory_Clear
    void clear();
    // orig: Rent-A-Hero.exe 0x00442240 Inventory_SetEvent(id, ev)
    void setItemEvent(int id, int ev);
    // orig: Rent-A-Hero.exe 0x00442280 – alle Gegenstand-Ereignisse und den Benutzen-Handler
    // löschen (CGame_dtor beim Schauplatzwechsel)
    void clearItemEvents();
    // orig: Rent-A-Hero.exe 0x00441DB0 – Ereignis „benutzt“ (+0x424) mit Handler h
    void setUseHandler(int h);
    // orig: Rent-A-Hero.exe 0x00441E60(h, game) – game wird hier zum Aufruf von CGame_DescribeItem
    void setDescribe(int h, std::function<void(int id)> describeItem);
    // orig: Rent-A-Hero.exe 0x00441F50 CInvBar_Invalidate
    void invalidate() { redraw_ = true; }
    // orig: Rent-A-Hero.exe 0x00441CE0 – Takt der Gegenstände (aus CSzene_Update)
    void tick();
    // orig: Rent-A-Hero.exe 0x00441C30 – Gegenstände auf die Szenenoberfläche; true = ausgeben
    bool draw();
    // orig: Rent-A-Hero.exe 0x00441F00 – auszugebendes Rechteck (Szenenkoordinaten)
    Rect takeOutputRect();
    // orig: Rent-A-Hero.exe 0x00441D40(leiste, verlassen)
    void leave(bool left);
    // orig: Rent-A-Hero.exe 0x00441FC0 – Funktions-Handler des Bereichs (Typ 0x0D Maus, 0x0B Klick)
    void onAreaEvent(int type);

    Area& area() { return *area_; }
    Event* useEvent() { return useEvent_.get(); }
    const Entry& entry(int i) const { return entries_[static_cast<std::size_t>(i)]; }
    int count() const { return count_; }
    int first() const { return first_; }
    int hovered() const { return hovered_; }
    int lastClicked() const { return lastClicked_; }
    bool bright() const { return bright_; }
    bool needsRedraw() const { return redraw_; }

private:
    // orig: Rent-A-Hero.exe 0x00441EA0 – Ausgaberechteck der belegten Plätze
    void updateRect();
    // orig: Rent-A-Hero.exe 0x00441F60 – Leiste schwarz füllen, ganz ausgeben
    void blank();
    // orig: Rent-A-Hero.exe 0x004422C0 – Platz mit der ID (alle 50), sonst −1
    int find(int id) const;
    // orig: Rent-A-Hero.exe 0x00442160 – Text TEXT%03d aus DefInvent.GER als Statustext
    void showText(int id);

    Scene& scene_;                          // +0x3F4
    EngineGlobals& g_;
    std::array<Entry, kSlots> entries_{};   // +0x000
    int lastClicked_ = 0;                   // +0x3E8
    int count_ = 0;                         // +0x3EC
    int first_ = 0;                         // +0x3F0
    Rect rect_;                             // +0x3F8
    bool redraw_ = false;                   // +0x408
    bool bright_ = false;                   // +0x40C
    bool full_ = false;                     // +0x410
    int hovered_ = -1;                      // +0x414
    bool itemEvents_ = false;               // +0x418
    int useHandler_ = 0;                    // +0x41C
    std::function<void(int)> describe_;     // +0x420 (CGame*) → CGame_DescribeItem
    std::unique_ptr<Event> useEvent_;       // +0x424
    std::unique_ptr<Area> area_;            // +0x428
};

}  // namespace rah
