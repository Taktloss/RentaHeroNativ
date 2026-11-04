// SPDX-License-Identifier: GPL-3.0-or-later
// Messprogramm (Win32, i686, ohne CRT): GDI-Schrift wie im Original (CreateFontA „Arial“,
// DEFAULT_CHARSET, 0x00436230 / 0x00440F70 / 0x0043E4C0), Qualität ohne Kantenglättung.
// stdout: Metriken und TA_CENTER-Versatz. font.out: je Fall u32 fall, u32 text, i32 cx, i32 cy,
// dann kW × kH Bytes (1 = Pixel gesetzt), Text mit TA_LEFT|TA_BASELINE bei (kX, kBase).
// Gegenstück: tools/fontcmp.cpp (Port-Darstellung, Pixelvergleich). docs/WINEPROBE.md.
typedef unsigned int u32;
typedef int i32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef void* HANDLE;
#define WINAPI __stdcall

__declspec(dllimport) HANDLE WINAPI CreateFileA(const char*, u32, u32, void*, u32, u32, HANDLE);
__declspec(dllimport) i32 WINAPI WriteFile(HANDLE, const void*, u32, u32*, void*);
__declspec(dllimport) i32 WINAPI CloseHandle(HANDLE);
__declspec(dllimport) void* WINAPI LoadLibraryA(const char*);
__declspec(dllimport) void* WINAPI GetProcAddress(void*, const char*);
__declspec(dllimport) void WINAPI ExitProcess(u32);
__declspec(dllimport) HANDLE WINAPI GetStdHandle(u32);

enum { kW = 200, kH = 28, kX = 10, kBase = 20 };

static void outs(const char* s) {
    u32 n = 0, w;
    while (s[n]) ++n;
    WriteFile(GetStdHandle((u32)-11), s, n, &w, 0);
}
static void outi(i32 v) {
    char b[16];
    int i = 15;
    u32 u = v < 0 ? (u32)-v : (u32)v;
    b[i] = 0;
    do { b[--i] = (char)('0' + u % 10); u /= 10; } while (u);
    if (v < 0) b[--i] = '-';
    outs(b + i);
}

typedef struct { u32 biSize; i32 biWidth, biHeight; u16 biPlanes, biBitCount; u32 biCompression, biSizeImage; i32 xppm, yppm; u32 clrUsed, clrImportant; } BmiHeader;
typedef struct {
    i32 tmHeight, tmAscent, tmDescent, tmInternalLeading, tmExternalLeading, tmAveCharWidth, tmMaxCharWidth,
        tmWeight, tmOverhang, tmDigitizedAspectX, tmDigitizedAspectY;
    u8 first, last, def, brk, italic, underlined, struck, pitch, charset;
} TextMetricA;
typedef struct { i32 cx, cy; } Size;
typedef struct { i32 left, top, right, bottom; } Rect;

typedef void*(WINAPI* CreateCompatibleDCFn)(void*);
typedef void*(WINAPI* CreateDIBSectionFn)(void*, const BmiHeader*, u32, void**, void*, u32);
typedef void*(WINAPI* SelectObjectFn)(void*, void*);
typedef void*(WINAPI* CreateFontAFn)(i32, i32, i32, i32, i32, u32, u32, u32, u32, u32, u32, u32, u32, const char*);
typedef i32(WINAPI* GetTextMetricsAFn)(void*, TextMetricA*);
typedef i32(WINAPI* GetTextExtentPoint32AFn)(void*, const char*, i32, Size*);
typedef i32(WINAPI* GetTextFaceAFn)(void*, i32, char*);
typedef u32(WINAPI* SetU32Fn)(void*, u32);
typedef i32(WINAPI* TextOutAFn)(void*, i32, i32, const char*, i32);
typedef i32(WINAPI* DeleteFn)(void*);

static const struct { i32 h, weight, italic; const char* name; } kCases[] = {
    {15, 700, 1, "Bereichsname 15 fett kursiv"},
    {15, 400, 0, "Hilfetext 15"},
    {12, 400, 0, "Renderer 12"},
};
// CP1252
static const char* const kTexts[] = {
    "Rodrigo", "Hallo Welt", "B\xFCro", "Wagenrad", "iiiii", "WWWW", "Kneipe zum Ochsen",
    "\xC4rger?! 1234567890", "Megophias", "Jasmin & Ramil", "A", "Il1|", "Tr\xE4nke",
};
enum { kNumTexts = sizeof kTexts / sizeof kTexts[0] };

static i32 len(const char* s) { i32 n = 0; while (s[n]) ++n; return n; }

void start(void) {
    void* gdi = LoadLibraryA("gdi32.dll");
#define F(T, n) T n = (T)GetProcAddress(gdi, #n)
    F(CreateCompatibleDCFn, CreateCompatibleDC);
    F(CreateDIBSectionFn, CreateDIBSection);
    F(SelectObjectFn, SelectObject);
    F(CreateFontAFn, CreateFontA);
    F(GetTextMetricsAFn, GetTextMetricsA);
    F(GetTextExtentPoint32AFn, GetTextExtentPoint32A);
    F(GetTextFaceAFn, GetTextFaceA);
    F(SetU32Fn, SetTextColor);
    F(SetU32Fn, SetBkMode);
    F(SetU32Fn, SetTextAlign);
    F(TextOutAFn, TextOutA);
    F(DeleteFn, DeleteObject);
#undef F
    BmiHeader bh = {40, kW, -kH, 1, 32, 0, 0, 0, 0, 0, 0};
    u32* bits = 0;
    void* dc = CreateCompatibleDC(0);
    void* bmp = CreateDIBSection(dc, &bh, 0, (void**)&bits, 0, 0);
    SelectObject(dc, bmp);
    SetTextColor(dc, 0xFFFFFF);
    SetBkMode(dc, 1);  // TRANSPARENT
    HANDLE out = CreateFileA("font.out", 0x40000000, 0, 0, 2, 0x80, 0);
    static u8 mask[kW * kH];
    for (u32 c = 0; c < sizeof kCases / sizeof kCases[0]; ++c) {
        // Qualität 3 = NONANTIALIASED (Projektentscheidung: ohne Kantenglättung)
        void* font = CreateFontA(kCases[c].h, 0, 0, 0, kCases[c].weight, (u32)kCases[c].italic, 0, 0, 1, 0, 0, 3, 0, "Arial");
        void* old = SelectObject(dc, font);
        TextMetricA tm;
        GetTextMetricsA(dc, &tm);
        char face[64];
        face[0] = 0;
        GetTextFaceA(dc, 64, face);
        outs(kCases[c].name);
        outs(": face=");
        outs(face);
        outs(" tmHeight=");
        outi(tm.tmHeight);
        outs(" tmAscent=");
        outi(tm.tmAscent);
        outs(" tmDescent=");
        outi(tm.tmDescent);
        outs(" tmInternalLeading=");
        outi(tm.tmInternalLeading);
        outs(" tmAveCharWidth=");
        outi(tm.tmAveCharWidth);
        outs(" tmOverhang=");
        outi(tm.tmOverhang);
        outs("\n  center_versatz:");
        for (i32 t = 0; t < kNumTexts; ++t) {
            Size sz;
            GetTextExtentPoint32A(dc, kTexts[t], len(kTexts[t]), &sz);
            // links bei kX und zentriert bei kX + 100: linke Tintenspalte vergleichen
            i32 lx[2];
            for (int pass = 0; pass < 2; ++pass) {
                for (i32 i = 0; i < kW * kH; ++i) bits[i] = 0;
                SetTextAlign(dc, pass == 0 ? 24u : 24u | 6u);  // TA_BASELINE [| TA_CENTER]
                TextOutA(dc, pass == 0 ? kX : kX + 100, kBase, kTexts[t], len(kTexts[t]));
                lx[pass] = -1;
                for (i32 x = 0; x < kW && lx[pass] < 0; ++x)
                    for (i32 y = 0; y < kH; ++y)
                        if (bits[y * kW + x] & 0xFFFFFF) { lx[pass] = x; break; }
                if (pass == 0) {
                    for (i32 i = 0; i < kW * kH; ++i) mask[i] = (bits[i] & 0xFFFFFF) ? 1 : 0;
                    u32 hdr[4] = {c, (u32)t, (u32)sz.cx, (u32)sz.cy}, w;
                    WriteFile(out, hdr, 16, &w, 0);
                    WriteFile(out, mask, kW * kH, &w, 0);
                }
            }
            // Zentriert-Ursprung = kX + 100 − (Versatz); Versatz = um wie viel nach links
            outs(" ");
            outi(sz.cx);
            outs(":");
            outi(100 - (lx[1] - lx[0]));
        }
        outs("\n");
        SelectObject(dc, old);
        DeleteObject(font);
    }
    CloseHandle(out);
    ExitProcess(0);
}
