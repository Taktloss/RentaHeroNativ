// SPDX-License-Identifier: GPL-3.0-or-later
// Szene (CSzene, 0xACC Bytes): Hintergrund, Ebenen, Tiefen-/Wegkarte, Z-Verwaltung und der
// Compositor CSzene_Compose (docs/COMPOSITOR.md, docs/formats/szn.md).
// Noch nicht enthalten: Bereiche (CArea), Figuren (CActor), animierte Ebenen (CAnimLayer),
// Videoebenen, Vollbildobjekt +0x5DC, Inventarleiste +0x5B4.
#pragma once

#include <array>
#include <memory>
#include <optional>
#include <string_view>

#include "engine/animlayer.hpp"
#include "engine/area.hpp"
#include "engine/globals.hpp"
#include "engine/layer.hpp"
#include "engine/paths.hpp"
#include "engine/rectlist.hpp"
#include "formats/ini.hpp"
#include "formats/szn.hpp"
#include "formats/zbf.hpp"
#include "render/surface.hpp"

namespace rah {

class Scene {
public:
    static constexpr int kMaxLayers = 40;

    // orig: Rent-A-Hero.exe 0x0044A680 CSzene_SetDevice (Oberfläche, Farbschlüssel, Y-Versatz)
    Scene(const DataPaths& paths, EngineGlobals& globals, Surface565& target, std::uint16_t colorKey = 0,
          int yOffset = 0);
    ~Scene();
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // orig: Rent-A-Hero.exe 0x004492E0 CSzene_Load(name, loadMask) – "<Szenenordner><name>.szn",
    // sonst Szenenordner der CD
    bool load(std::string_view name, unsigned loadMask);
    // orig: Rent-A-Hero.exe 0x00449380 CSzene_LoadSzn
    bool loadSzn(const IniFile& szn, unsigned loadMask);

    // orig: Rent-A-Hero.exe 0x0044AF60 CSzene_GetObject – nur Ebenen, Groß-/Kleinschreibung egal
    Layer* getObject(std::string_view name);
    Layer* background() { return background_.get(); }
    Layer* layer(int i) { return layers_[static_cast<std::size_t>(i)].get(); }

    // orig: Rent-A-Hero.exe 0x0044B280 CSzene_Update – Animations-Takt (TIMING.md)
    void update(bool realtime);
    std::uint32_t tickWindowStart() const { return windowStart_; }

    static constexpr int kMaxAreas = 60;

    // orig: Rent-A-Hero.exe 0x0044ADD0 CSzene_AddArea – name/rect dürfen fehlen (NULL)
    Area* addArea(const std::string* name, Rect* rect, int cursor);
    // orig: Rent-A-Hero.exe 0x0044AFB0 CSzene_GetArea (Original: MessageBox bei Fehlschlag)
    Area* getArea(std::string_view name);
    Area* area(int i) { return areas_[static_cast<std::size_t>(i)].get(); }
    // orig: Rent-A-Hero.exe 0x0044C3C0 CSzene_HitTest (Szenenkoordinaten)
    Area* hitTest(int x, int y);
    // orig: Rent-A-Hero.exe 0x0044C950 CSzene_OnClick (Bildschirmkoordinaten)
    void onClick(int x, int y);
    Area* lastHit() const { return lastHit_; }
    // orig: Rent-A-Hero.exe 0x0044CBB0 CSzene_MouseMove – liefert den gewünschten Zeigertyp;
    // der Aufrufer setzt ihn, wenn er vom aktuellen abweicht (0x0043EA60/0x0043E960)
    int onMouseMove(int x, int y);
    bool showTexts = true;       // +0x5CC (Ctor 1)
    // Schauplatznummer +0x5D0 (0 = Menü), für die Bereichstexte aus LocNN.GER
    void setLocationNumber(int n) { location_ = n; }
    bool clicksEnabled = true;   // +0x514 (Ctor 1)
    bool clicksDuringFull = false;  // +0x518 (Ctor 0)
    bool globalClick = true;     // +0x510 (Ctor 1)

    // orig: Rent-A-Hero.exe 0x0044C690 CSzene_DispatchEvents – einmal pro Frame
    void dispatchEvents();
    Event& sceneEvent() { return sceneEvent_; }

    // orig: Rent-A-Hero.exe 0x0044C1E0 CSzene_InvalidateAll
    void invalidateAll();
    // orig: Rent-A-Hero.exe 0x0044B7D0 CSzene_Compose – true: Ausgaberechteck `output()` zeigen
    bool compose(bool force);
    const DirtyRegion& output() const { return listB_; }
    void clearOutput() { listB_.clear(); }

    int zMax() const { return zMax_; }
    int zMin() const { return zMin_; }
    int zCount(int z) const { return z >= 0 && z < 256 ? zCount_[static_cast<std::size_t>(z)] : 0; }
    int unsupportedLayers() const { return unsupported_; }

private:
    // orig: Rent-A-Hero.exe 0x00449E70 – Hintergrund (Z = 255) anlegen
    bool createBackground(const std::optional<FoundFile>& file);
    // orig: Rent-A-Hero.exe 0x0044AB10 CSzene_CreateLayer
    Layer* createLayer(const std::optional<FoundFile>& file, bool animated);
    // orig: Rent-A-Hero.exe 0x0044A1B0 CSzene_LoadDepth / 0x0044A260 CSzene_LoadPath
    void loadMap(const std::optional<FoundFile>& file, std::optional<ZMap>& out);

    const DataPaths& paths_;
    EngineGlobals& g_;
    Surface565& surface_;      // +0x2D4
    std::uint16_t colorKey_;   // +0x2D8
    int yOffset_;              // +0x2F0

    bool loaded_ = false;                                       // +0x00
    std::array<std::unique_ptr<Layer>, kMaxLayers> layers_{};   // +0x04
    std::unique_ptr<Layer> background_;                         // +0x5C0
    std::optional<ZMap> depth_, path_;                          // +0x2BC, +0x2C0
    int lastLayer_ = -1;                                        // +0x2B8 / +0x5D8
    DirtyRegion listA_;                                         // +0x2F4
    DirtyRegion listB_;                                         // +0xAC4
    int zMax_ = 0, zMin_ = 0xFF;                                // +0x2F8, +0x2FC
    std::array<short, 256> zCount_{};                           // +0x300
    std::array<std::unique_ptr<Area>, kMaxAreas> areas_{};     // +0xA4
    Area* lastHit_ = nullptr;                                   // +0x2C8
    int clickX_ = 0, clickY_ = 0;                               // +0x500, +0x504
    int textX_ = 0, textY_ = 0;                                 // +0x508, +0x50C
    int location_ = 0;                                          // +0x5D0
    bool barShown_ = false;                                     // +0x5BC
    Event sceneEvent_;                                          // +0x2C4 (Klick ins Leere, Typ 0x16)
    bool editor_ = false;                                       // +0xAC8
    std::uint32_t windowStart_ = 0;                             // +0x2DC
    std::array<int, 4> stageCount_{};                           // +0x2E0…+0x2EC
    int unsupported_ = 0;

    void moveZ(unsigned oldZ, unsigned newZ);
    bool textShown() const { return showTexts && lastHit_ && g_.textVisible; }
    void hideToggle();
};

}  // namespace rah
