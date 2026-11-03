// SPDX-License-Identifier: GPL-3.0-or-later
// Ressourcen einer PE-Datei lesen (nur das, was LoadCursorA braucht; docs/MOUSE.md).
#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

#include "core/bytes.hpp"

namespace rah {

class PeResources {
public:
    bool parse(Bytes file);
    // Daten der Ressource (Typ, numerische ID), erste Sprache
    std::optional<std::span<const std::uint8_t>> find(std::uint32_t type, std::uint32_t id) const;
    // NUL-terminierte Zeichenkette an einer virtuellen Adresse (ImageBase + RVA), roh (CP1252)
    std::optional<std::string> stringAt(std::uint32_t va) const;

private:
    Bytes d_;
    struct Section {
        std::uint32_t va, size, raw;
    };
    std::vector<Section> secs_;
    std::size_t base_ = 0;
    std::uint32_t imageBase_ = 0;  // Dateiversatz des Ressourcenverzeichnisses
    std::optional<std::size_t> offset(std::uint32_t rva) const;
    std::optional<std::uint32_t> child(std::size_t dir, std::optional<std::uint32_t> id) const;
};

// Schwarzweiß-Zeiger, Pixel von oben nach unten
struct MonoCursor {
    enum Pixel : std::uint8_t { Transparent, Black, White, Invert };
    int width = 0, height = 0, hotX = 0, hotY = 0;
    std::vector<Pixel> pixels;
};

// LoadCursorA(hInstance, MAKEINTRESOURCE(group)): RT_GROUP_CURSOR → RT_CURSOR.
// UNVERIFIED: Auswahl bei mehreren Einträgen (die Gruppen der EXE haben je genau einen 32×32-Eintrag;
// hier: erster 1-Bit-Eintrag).
std::optional<MonoCursor> loadCursor(const PeResources& pe, std::uint32_t group);

}  // namespace rah
