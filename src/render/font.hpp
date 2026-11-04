// SPDX-License-Identifier: GPL-3.0-or-later
// GDI-Schrift (CreateFontA + TextOutA) über FreeType, einfarbig gerastert (docs/RENDERER.md,
// docs/MOUSE.md). Bewusste Abweichung (Entscheidung 2026-10-27): System-Arial, sonst Liberation
// Sans; GDI-Hinting wird nicht nachgebildet.
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "core/rect.hpp"
#include "render/surface.hpp"

namespace rah {

struct Area;

// Ausrichtung wie SetTextAlign, immer mit TA_BASELINE
enum class TextAlign { Left, Center, Right };

class Font {
public:
    Font();
    ~Font();
    Font(const Font&) = delete;
    Font& operator=(const Font&) = delete;

    // CreateFontA mit positiver Höhe = Zellenhöhe (usWinAscent + usWinDescent)
    bool open(const std::string& file, int cellHeight, std::string* err = nullptr);
    bool isOpen() const;
    // GDI tmHeight / tmAscent: Zellenhöhe und round(Zellenhöhe · usWinAscent / (usWinAscent +
    // usWinDescent)) – Wine-GDI: 15/12 und 12/10 wie hier (abgerundet wäre 12/9; WINEPROBE.md)
    int cellHeight() const;
    int ascent() const;

    // Vorschubbreite des Textes (GetTextExtentPoint32A-Breite, ohne Unterschneidung)
    int textWidth(std::string_view cp1252) const;
    // TextOutA mit TA_BASELINE und transparentem Hintergrund; Text in Codepage 1252
    void textOut(Surface565& s, int x, int y, std::string_view cp1252, TextAlign align, std::uint16_t color,
                 const Rect* clip = nullptr) const;

    // Schriftdatei suchen: System-Arial (arial*.ttf), sonst Liberation Sans; leer = keine
    static std::string findSystemFont(bool bold, bool italic);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// orig: Rent-A-Hero.exe 0x00436230 CArea_DrawText – Ausrichtung nach Schätzbreite len·8/2
TextAlign areaTextAlign(std::size_t len, int x);
// orig: Rent-A-Hero.exe 0x00436230 CArea_DrawText (Schrift: Arial 15, fett, kursiv)
void drawAreaText(const Font& font, Surface565& s, const Area& a, int x, int y);

// DrawTextA(…, 0x15 = DT_CENTER | DT_VCENTER | DT_WORDBREAK), transparenter Hintergrund, ohne
// DT_NOCLIP (am Rechteck abgeschnitten). DT_VCENTER wirkt nur mit DT_SINGLELINE → Text oben.
// Zeilen an CR/LF/CRLF, Umbruch an Leerzeichen; ein zu langes Wort bleibt ungebrochen.
// UNVERIFIED: Feinheiten des GDI-Umbruchs (mehrere Leerzeichen, Tabs – in den Daten nicht vorhanden)
void drawTextWordBreak(const Font& font, Surface565& s, const Rect& r, std::string_view cp1252, std::uint16_t color);
// Zeilen wie drawTextWordBreak (für Tests)
std::vector<std::string> wrapText(const Font& font, std::string_view cp1252, int width);

// Codepage 1252 → Unicode (ANSI-Codepage des deutschen Windows)
char32_t cp1252ToUnicode(unsigned char c);
// Texte der EXE (CP1252) für SDL (UTF-8) und zurück; nicht darstellbare Zeichen entfallen
std::string cp1252ToUtf8(std::string_view in);
std::string utf8ToCp1252(std::string_view in);

}  // namespace rah
