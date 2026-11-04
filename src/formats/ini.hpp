// SPDX-License-Identifier: GPL-3.0-or-later
// INI-Leser mit der Semantik von GetPrivateProfileStringA/GetPrivateProfileIntA, über die
// das Original SZN-, GER- und Konfigurationsdateien liest (docs/formats/szn.md, ger.md).
#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace rah {

class IniFile {
public:
    IniFile() = default;
    explicit IniFile(std::span<const std::uint8_t> text) { parse(text); }
    void parse(std::span<const std::uint8_t> text);

    bool hasSection(std::string_view section) const;
    std::optional<std::string> find(std::string_view section, std::string_view key) const;

    // GetPrivateProfileStringA: Wert oder Standard, auf maxLen−1 Zeichen gekürzt (Puffergröße
    // des Originals), umschließende gleiche Anführungszeichen entfernt.
    std::string getString(std::string_view section, std::string_view key, std::string_view def,
                          std::size_t maxLen = 256) const;
    // GetPrivateProfileIntA: Standard nur, wenn der Schlüssel fehlt; sonst dezimal wie atoi
    // (Leerraum, Vorzeichen, Ziffern bis zum ersten Nicht-Ziffer-Zeichen; keine Ziffer → 0).
    // Hex-Präfixe ("0x") kommen in den Daten nicht vor (tools/data_scan.py, tests/test_ini.cpp).
    int getInt(std::string_view section, std::string_view key, int def) const;

    // Für Prüfungen: Sektionen/Schlüssel, die mehrfach vorkommen (es gilt jeweils der erste).
    int duplicateSections() const { return dupSections_; }
    int duplicateKeys() const { return dupKeys_; }
    std::vector<std::string> sections() const;
    std::vector<std::string> keys(std::string_view section) const;

private:
    struct Section {
        std::string name;
        std::vector<std::pair<std::string, std::string>> entries;  // Reihenfolge der Datei
        std::map<std::string, std::size_t> index;                  // Schlüssel groß → erster Eintrag
    };
    const Section* section(std::string_view name) const;

    std::vector<Section> sections_;
    std::map<std::string, std::size_t> sectionIndex_;  // Name groß → erste Sektion
    int dupSections_ = 0, dupKeys_ = 0;
};

}  // namespace rah
