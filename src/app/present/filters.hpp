// SPDX-License-Identifier: GPL-3.0-or-later
// Auswahl der Bildfilter (Port; docs/FILTERS.md)
#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>

#include "app/present/preset.hpp"

namespace rah {

struct FilterInfo {
    const char* id;
    const char* name;
    const char* preset;  // nullptr: direkt ohne Shader (nächster Nachbar bzw. bilinear)
    bool linear;         // nur ohne Preset: bilineare Abtastung
    const char* hint;
};

std::span<const FilterInfo> scalerFilters();  // erster Eintrag: "original"
std::span<const FilterInfo> crtFilters();     // erster Eintrag: "off"
const FilterInfo* findFilter(std::span<const FilterInfo> list, std::string_view id);

// Bild-Einstellungen (launcher.ini, Overlay, Alt-Tasten)
struct DisplaySettings {
    std::string filter = "original";
    std::string crt = "off";
    bool deband = false;
    bool integerScale = true;  // ganzzahlig skalieren (bisherige Darstellung); sonst 4:3 füllen
    bool operator==(const DisplaySettings&) const = default;
};

// Nächster Filter in der Liste (Alt+F), umlaufend; unbekannte id → erster
std::string nextFilterId(std::span<const FilterInfo> list, std::string_view id, int dir = 1);

// Gesamte Kette: [Entbänderung] + (CRT ? CRT : Filter). nullopt: kein Shader nötig (direkt).
// Liefert in *linear die Abtastung für den direkten Weg.
std::optional<PresetDesc> buildChain(const DisplaySettings& d, bool* linear, std::string* err);

struct OutRect {
    int x = 0, y = 0, w = 0, h = 0;  // Pixel, y von oben
};
// Ganzzahlig (größter Faktor k ≥ 1, sonst einpassen) bzw. 4:3 einpassen, zentriert
OutRect computeOutRect(int drawW, int drawH, int frameW, int frameH, bool integerScale);

// Kurzname der aktuellen Auswahl für die Einblendung
std::string displayLabel(const DisplaySettings& d);

}  // namespace rah
