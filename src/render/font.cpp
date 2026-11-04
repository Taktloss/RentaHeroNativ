// SPDX-License-Identifier: GPL-3.0-or-later
#include "render/font.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <system_error>
#include <vector>

#include "core/strings.hpp"
#include "engine/area.hpp"

namespace rah {

char32_t cp1252ToUnicode(unsigned char c) {
    static constexpr char16_t k80[32] = {
        0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160,
        0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD, 0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022,
        0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0x0178};
    return c >= 0x80 && c < 0xA0 ? k80[c - 0x80] : c;
}

struct Font::Impl {
    FT_Library lib = nullptr;
    FT_Face face = nullptr;
    int cell = 0, ascent = 0;
    ~Impl() {
        if (face) FT_Done_Face(face);
        if (lib) FT_Done_FreeType(lib);
    }
    bool load(unsigned char c) const {
        return FT_Load_Char(face, cp1252ToUnicode(c), FT_LOAD_RENDER | FT_LOAD_TARGET_MONO | FT_LOAD_MONOCHROME) == 0;
    }
    int advance() const { return static_cast<int>((face->glyph->advance.x + 32) >> 6); }
};

Font::Font() = default;
Font::~Font() = default;

bool Font::isOpen() const { return impl_ && impl_->face; }

bool Font::open(const std::string& file, int cellHeight, std::string* err) {
    auto fail = [&](const char* m) {
        if (err) *err = m;
        impl_.reset();
        return false;
    };
    impl_ = std::make_unique<Impl>();
    if (FT_Init_FreeType(&impl_->lib)) return fail("FreeType nicht initialisiert");
    if (FT_New_Face(impl_->lib, file.c_str(), 0, &impl_->face)) return fail("Schriftdatei nicht lesbar");
    // GDI: positive lfHeight = Zellenhöhe → em = Höhe · unitsPerEm / (usWinAscent + usWinDescent).
    // Pixel-em auf die nächste ganze Zahl: mit derselben TTF pixelgleich zu Wine-GDI (26 Texte,
    // Höhe 12 und 15, NONANTIALIASED; tools/fontcmp.cpp, WINEPROBE.md).
    const auto* os2 = static_cast<const TT_OS2*>(FT_Get_Sfnt_Table(impl_->face, FT_SFNT_OS2));
    double cell = os2 && os2->usWinAscent + os2->usWinDescent > 0
                      ? static_cast<double>(os2->usWinAscent + os2->usWinDescent)
                      : static_cast<double>(impl_->face->ascender - impl_->face->descender);
    const auto ppem = static_cast<FT_UInt>(std::lround(cellHeight * impl_->face->units_per_EM / cell));
    if (FT_Set_Pixel_Sizes(impl_->face, 0, ppem)) return fail("Schriftgröße nicht setzbar");
    impl_->cell = cellHeight;
    const double asc = os2 && os2->usWinAscent + os2->usWinDescent > 0 ? os2->usWinAscent
                                                                         : impl_->face->ascender;
    impl_->ascent = static_cast<int>(std::lround(cellHeight * asc / cell));
    return true;
}

int Font::cellHeight() const { return isOpen() ? impl_->cell : 0; }
int Font::ascent() const { return isOpen() ? impl_->ascent : 0; }

int Font::textWidth(std::string_view s) const {
    if (!isOpen()) return 0;
    int w = 0;
    for (unsigned char c : s)
        if (impl_->load(c)) w += impl_->advance();
    return w;
}

void Font::textOut(Surface565& s, int x, int y, std::string_view text, TextAlign align, std::uint16_t color,
                   const Rect* clip) const {
    if (!isOpen()) return;
    int pen = x;
    // TA_CENTER: x − floor(Breite/2) – Wine-GDI in allen 39 Messfällen (WINEPROBE.md)
    if (align == TextAlign::Center) pen = x - textWidth(text) / 2;
    else if (align == TextAlign::Right) pen = x - textWidth(text);
    for (unsigned char c : text) {
        if (!impl_->load(c)) continue;
        const FT_GlyphSlot g = impl_->face->glyph;
        const FT_Bitmap& b = g->bitmap;
        const int left = pen + g->bitmap_left, top = y - g->bitmap_top;
        for (unsigned r = 0; r < b.rows; ++r) {
            const int py = top + static_cast<int>(r);
            if (py < 0 || py >= s.height || (clip && (py < clip->top || py >= clip->bottom))) continue;
            const unsigned char* bits = b.buffer + static_cast<std::ptrdiff_t>(r) * b.pitch;
            for (unsigned col = 0; col < b.width; ++col) {
                const int px = left + static_cast<int>(col);
                if (px < 0 || px >= s.width || (clip && (px < clip->left || px >= clip->right))) continue;
                if (!(bits[col >> 3] & (0x80 >> (col & 7)))) continue;
                s.row(py)[px] = color;
            }
        }
        pen += impl_->advance();
    }
}

std::string Font::findSystemFont(bool bold, bool italic) {
    const int style = (bold ? 1 : 0) | (italic ? 2 : 0);
    static constexpr const char* kArial[4] = {"arial.ttf", "arialbd.ttf", "ariali.ttf", "arialbi.ttf"};
    static constexpr const char* kLib[4] = {"LiberationSans-Regular.ttf", "LiberationSans-Bold.ttf",
                                            "LiberationSans-Italic.ttf", "LiberationSans-BoldItalic.ttf"};
    namespace fs = std::filesystem;
    std::vector<fs::path> dirs;
#ifdef _WIN32
    if (const char* w = std::getenv("WINDIR")) dirs.emplace_back(fs::path(w) / "Fonts");
    if (const char* l = std::getenv("LOCALAPPDATA")) dirs.emplace_back(fs::path(l) / "Microsoft/Windows/Fonts");
#else
    if (const char* h = std::getenv("HOME")) {
        dirs.emplace_back(fs::path(h) / ".local/share/fonts");
        dirs.emplace_back(fs::path(h) / ".fonts");
    }
    dirs.emplace_back("/usr/local/share/fonts");
    dirs.emplace_back("/usr/share/fonts");
#endif
    for (const char* want : {kArial[style], kLib[style]}) {
        for (const auto& d : dirs) {
            std::error_code ec;
            for (fs::recursive_directory_iterator it(d, fs::directory_options::skip_permission_denied, ec), end;
                 !ec && it != end; it.increment(ec)) {
                if (it->is_regular_file(ec) && equalsNoCase(it->path().filename().string(), want))
                    return it->path().string();
            }
        }
    }
    return {};
}

TextAlign areaTextAlign(std::size_t len, int x) {
    // Vergleiche vorzeichenlos wie im Original
    const auto w = static_cast<unsigned>(len * 8 / 2);
    const auto ux = static_cast<unsigned>(x);
    if (ux < w) return TextAlign::Left;
    if (ux + w <= 640u) return TextAlign::Center;
    return TextAlign::Right;
}

// orig: Rent-A-Hero.exe 0x00436230 CArea_DrawText
void drawAreaText(const Font& font, Surface565& s, const Area& a, int x, int y) {
    const std::string& t = a.text.empty() ? a.name : a.text;
    if (t.empty()) return;
    const TextAlign al = areaTextAlign(t.size(), x);
    for (int dx : {-1, 1})
        for (int dy : {-1, 1}) font.textOut(s, x + dx, y + dy, t, al, 0x0000);
    font.textOut(s, x, y, t, al, 0xFFFF);
}

}  // namespace rah

namespace rah {

std::vector<std::string> wrapText(const Font& font, std::string_view text, int width) {
    std::vector<std::string> lines;
    std::size_t i = 0;
    while (true) {
        // Absatz bis CR, LF oder CRLF
        std::size_t end = i;
        while (end < text.size() && text[end] != '\r' && text[end] != '\n') ++end;
        const std::string_view para = text.substr(i, end - i);
        std::string line;
        std::size_t w = 0;
        while (w <= para.size()) {
            std::size_t sp = para.find(' ', w);
            if (sp == std::string_view::npos) sp = para.size();
            const std::string_view word = para.substr(w, sp - w);
            std::string cand = line.empty() ? std::string(word) : line + ' ' + std::string(word);
            if (!line.empty() && font.textWidth(cand) > width) {
                lines.push_back(line);
                line = std::string(word);
            } else {
                line = std::move(cand);
            }
            w = sp + 1;
        }
        lines.push_back(line);
        if (end >= text.size()) break;
        i = end + ((text[end] == '\r' && end + 1 < text.size() && text[end + 1] == '\n') ? 2 : 1);
        if (i >= text.size()) break;  // abschließender Umbruch erzeugt keine Leerzeile
    }
    return lines;
}

void drawTextWordBreak(const Font& font, Surface565& s, const Rect& r, std::string_view text, std::uint16_t color) {
    int y = r.top;
    for (const std::string& line : wrapText(font, text, r.width())) {
        if (y >= r.bottom) break;
        // GDI: x = links + (Breite − Zeilenbreite) / 2
        const int x = r.left + (r.width() - font.textWidth(line)) / 2;
        font.textOut(s, x, y + font.ascent(), line, TextAlign::Left, color, &r);
        y += font.cellHeight();
    }
}

}  // namespace rah
