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

    // orig: Rent-A-Hero.exe 0x004492E0 CSzene_Load(name, loadMask) – "<Szenenordner><name>.szn",
    // sonst Szenenordner der CD
    bool load(std::string_view name, unsigned loadMask);
    // orig: Rent-A-Hero.exe 0x00449380 CSzene_LoadSzn
    bool loadSzn(const IniFile& szn, unsigned loadMask);

    // orig: Rent-A-Hero.exe 0x0044AF60 CSzene_GetObject – nur Ebenen, Groß-/Kleinschreibung egal
    Layer* getObject(std::string_view name);
    Layer* background() { return background_.get(); }
    Layer* layer(int i) { return layers_[static_cast<std::size_t>(i)].get(); }

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
    int unsupported_ = 0;
};

}  // namespace rah
