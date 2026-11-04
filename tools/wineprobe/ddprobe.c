// SPDX-License-Identifier: GPL-3.0-or-later
// Messprogramm (Win32, i686, ohne CRT) für DirectDraw-/GDI-Verhalten, auf das sich der Port
// stützt (docs/WINEPROBE.md). Legt 16-Bit-565-Oberflächen wie das Original an (Flags 7 +
// Pixelformat, Caps OFFSCREENPLAIN|VIDEOMEMORY bzw. |SYSTEMMEMORY, 0x00446D46/0x00446D69)
// und gibt Messwerte als Textzeilen „schlüssel: werte“ aus. Keine Originaldaten.
typedef unsigned int u32;
typedef int i32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef void* HANDLE;
#define WINAPI __stdcall

__declspec(dllimport) i32 WINAPI WriteFile(HANDLE, const void*, u32, u32*, void*);
__declspec(dllimport) void* WINAPI LoadLibraryA(const char*);
__declspec(dllimport) void* WINAPI GetProcAddress(void*, const char*);
__declspec(dllimport) void WINAPI ExitProcess(u32);
__declspec(dllimport) HANDLE WINAPI GetStdHandle(u32);

// ---------- Ausgabe ----------
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
static void outx(u32 v) {
    char b[11] = "0x00000000";
    for (int i = 0; i < 8; ++i) b[9 - i] = "0123456789ABCDEF"[(v >> (4 * i)) & 15];
    outs(b);
}

// ---------- DirectDraw (eigene Deklarationen, Layout v1) ----------
typedef struct { u32 dwSize, dwFlags, dwFourCC, dwRGBBitCount, r, g, b, a; } PixFmt;
typedef struct {
    u32 dwSize, dwFlags, dwHeight, dwWidth;
    i32 lPitch;
    u32 back, refresh, alphaBits, reserved;
    void* lpSurface;
    u32 colorKeys[8];
    PixFmt pf;
    u32 caps;
} SurfDesc;  // 108 Bytes (DDSURFACEDESC)
typedef char sdSizeCheck[sizeof(SurfDesc) == 108 ? 1 : -1];
typedef struct { i32 left, top, right, bottom; } Rect;
typedef struct { void** vt; } Com;
typedef i32 HR;

#define VT(o, i) ((o)->vt[i])
typedef HR(WINAPI* DDCreateFn)(void*, Com**, void*);
typedef HR(WINAPI* SetCoopFn)(Com*, void*, u32);
typedef HR(WINAPI* CreateSurfFn)(Com*, SurfDesc*, Com**, void*);
typedef u32(WINAPI* ReleaseFn)(Com*);
typedef HR(WINAPI* LockFn)(Com*, Rect*, SurfDesc*, u32, void*);
typedef HR(WINAPI* UnlockFn)(Com*, void*);
typedef HR(WINAPI* BltFastFn)(Com*, u32, u32, Com*, Rect*, u32);
typedef HR(WINAPI* BltFn)(Com*, Rect*, Com*, Rect*, u32, void*);
typedef HR(WINAPI* GetDCFn)(Com*, void**);
typedef HR(WINAPI* ReleaseDCFn)(Com*, void*);

enum { kDDSCL_NORMAL = 8, kLockWait = 1, kBltFastWait = 0x10, kBltWait = 0x01000000 };
enum { kCapsVideo = 0x4040, kCapsSystem = 0x840 };

static Com* dd;

static Com* create16(u32 w, u32 h, u32 caps, HR* hrOut) {
    SurfDesc sd;
    u8* p = (u8*)&sd;
    for (u32 i = 0; i < sizeof sd; ++i) p[i] = 0;
    sd.dwSize = 108;
    sd.dwFlags = 7 | 0x1000;  // CAPS|HEIGHT|WIDTH (wie Original) + PIXELFORMAT (statt Bildschirmmodus)
    sd.dwWidth = w;
    sd.dwHeight = h;
    sd.pf.dwSize = 32;
    sd.pf.dwFlags = 0x40;  // DDPF_RGB
    sd.pf.dwRGBBitCount = 16;
    sd.pf.r = 0xF800;
    sd.pf.g = 0x07E0;
    sd.pf.b = 0x001F;
    sd.caps = caps;
    Com* s = 0;
    HR hr = ((CreateSurfFn)VT(dd, 6))(dd, &sd, &s, 0);
    if (hrOut) *hrOut = hr;
    return hr == 0 ? s : 0;
}
static void release(Com* o) { if (o) ((ReleaseFn)VT(o, 2))(o); }
static u16* lock(Com* s, Rect* r, i32* pitch, HR* hrOut) {
    SurfDesc sd;
    u8* p = (u8*)&sd;
    for (u32 i = 0; i < sizeof sd; ++i) p[i] = 0;
    sd.dwSize = 108;
    HR hr = ((LockFn)VT(s, 25))(s, r, &sd, kLockWait, 0);
    if (hrOut) *hrOut = hr;
    if (hr != 0) return 0;
    if (pitch) *pitch = sd.lPitch;
    return (u16*)sd.lpSurface;
}
static void unlock(Com* s) { ((UnlockFn)VT(s, 32))(s, 0); }
static void fill(Com* s, u32 w, u32 h, u16 v) {
    i32 pitch;
    u16* px = lock(s, 0, &pitch, 0);
    if (!px) return;
    for (u32 y = 0; y < h; ++y)
        for (u32 x = 0; x < w; ++x) ((u16*)((u8*)px + y * pitch))[x] = v;
    unlock(s);
}
static u32 countValue(Com* s, u32 w, u32 h, u16 v) {
    i32 pitch;
    u16* px = lock(s, 0, &pitch, 0);
    if (!px) return 0xFFFFFFFF;
    u32 n = 0;
    for (u32 y = 0; y < h; ++y)
        for (u32 x = 0; x < w; ++x) n += ((u16*)((u8*)px + y * pitch))[x] == v;
    unlock(s);
    return n;
}

// ---------- Messungen ----------
static void probePitch(u32 caps, const char* name) {
    static const u32 ws[] = {1, 2, 3, 4, 5, 7, 9, 13, 319, 320, 321, 639, 640, 641};
    outs("pitch_");
    outs(name);
    outs(":");
    for (u32 i = 0; i < sizeof ws / sizeof ws[0]; ++i) {
        HR hr;
        Com* s = create16(ws[i], 4, caps, &hr);
        i32 pitch = -1;
        if (s && lock(s, 0, &pitch, 0)) unlock(s);
        outs(" ");
        outi((i32)ws[i]);
        outs("=");
        if (s) outi(pitch); else outx((u32)hr);
        release(s);
    }
    outs("\n");
}

static void probeNewContent(u32 caps, const char* name) {
    // frische Oberfläche; dann gefüllt freigeben und gleich große neu anlegen
    Com* a = create16(64, 64, caps, 0);
    u32 zero = a ? countValue(a, 64, 64, 0) : 0;
    if (a) fill(a, 64, 64, 0xAAAA);
    release(a);
    Com* b = create16(64, 64, caps, 0);
    u32 zero2 = b ? countValue(b, 64, 64, 0) : 0;
    u32 old2 = b ? countValue(b, 64, 64, 0xAAAA) : 0;
    release(b);
    outs("new_content_");
    outs(name);
    outs(": first_zero=");
    outi((i32)zero);
    outs("/4096 reuse_zero=");
    outi((i32)zero2);
    outs(" reuse_old=");
    outi((i32)old2);
    outs("\n");
}

static void probeLockRect(void) {
    static const Rect rs[] = {{0, 0, 64, 64}, {-1, 0, 4, 4}, {0, -1, 4, 4}, {60, 0, 65, 4},
                              {0, 60, 4, 65}, {70, 70, 80, 80}, {10, 10, 5, 5}, {10, 10, 10, 10}};
    Com* s = create16(64, 64, kCapsSystem, 0);
    outs("lock_rect (64x64):");
    for (u32 i = 0; i < sizeof rs / sizeof rs[0]; ++i) {
        Rect r = rs[i];
        HR hr;
        u16* p = lock(s, &r, 0, &hr);
        if (p) unlock(s);
        outs(" [");
        outi(r.left); outs(","); outi(r.top); outs(","); outi(r.right); outs(","); outi(r.bottom);
        outs("]=");
        outx((u32)hr);
    }
    outs("\n");
    release(s);
}

static void probeBltFast(void) {
    struct { i32 x, y; Rect src; } cs[] = {
        {48, 48, {0, 0, 16, 16}}, {56, 0, {0, 0, 16, 16}}, {0, 56, {0, 0, 16, 16}},
        {-4, 0, {0, 0, 16, 16}}, {0, 0, {0, 0, 20, 16}}, {0, 0, {-2, 0, 16, 16}},
        {64, 0, {0, 0, 16, 16}}, {0, 0, {4, 4, 4, 8}},
    };
    Com* dst = create16(64, 64, kCapsSystem, 0);
    Com* src = create16(16, 16, kCapsSystem, 0);
    fill(src, 16, 16, 0x1234);
    outs("bltfast (Ziel 64x64, Quelle 16x16):");
    for (u32 i = 0; i < sizeof cs / sizeof cs[0]; ++i) {
        fill(dst, 64, 64, 0);
        Rect r = cs[i].src;
        HR hr = ((BltFastFn)VT(dst, 7))(dst, (u32)cs[i].x, (u32)cs[i].y, src, &r, kBltFastWait);
        outs(" (");
        outi(cs[i].x); outs(","); outi(cs[i].y); outs(" src ");
        outi(r.left); outs(","); outi(r.top); outs(","); outi(r.right); outs(","); outi(r.bottom);
        outs(")=");
        outx((u32)hr);
        outs("/px");
        outi((i32)countValue(dst, 64, 64, 0x1234));
    }
    outs("\n");
    release(src);
    release(dst);
}

static void probeStretch(void) {
    // Quelle 8x1 mit Werten 0…7; Ziel 1 Zeile in verschiedenen Breiten → Quellindex je Zielpixel
    static const i32 dw[] = {3, 5, 7, 9, 12, 16};
    Com* src = create16(8, 8, kCapsSystem, 0);
    {
        i32 pitch;
        u16* p = lock(src, 0, &pitch, 0);
        for (u32 y = 0; y < 8; ++y)
            for (u32 x = 0; x < 8; ++x) ((u16*)((u8*)p + y * pitch))[x] = (u16)(y * 8 + x + 1);
        unlock(src);
    }
    Com* dst = create16(32, 32, kCapsSystem, 0);
    for (u32 i = 0; i < sizeof dw / sizeof dw[0]; ++i) {
        fill(dst, 32, 32, 0);
        Rect sr = {0, 0, 8, 8}, dr = {0, 0, dw[i], dw[i]};
        HR hr = ((BltFn)VT(dst, 5))(dst, &dr, src, &sr, kBltWait, 0);
        outs("stretch 8->");
        outi(dw[i]);
        outs(" hr=");
        outx((u32)hr);
        outs(" x:");
        i32 pitch;
        u16* p = lock(dst, 0, &pitch, 0);
        for (i32 x = 0; x < dw[i]; ++x) { outs(" "); outi((i32)p[x] - 1); }
        outs(" y:");
        for (i32 y = 0; y < dw[i]; ++y) { outs(" "); outi(((i32)((u16*)((u8*)p + y * pitch))[0] - 1) / 8); }
        unlock(dst);
        outs("\n");
    }
    release(dst);
    release(src);
}

// GDI: 24-Bit-DIB per BitBlt in die 16-Bit-Oberfläche (wie DDCopyBitmap, 0x0043D130)
typedef struct { u32 biSize; i32 biWidth, biHeight; u16 biPlanes, biBitCount; u32 biCompression, biSizeImage; i32 xppm, yppm; u32 clrUsed, clrImportant; } BmiHeader;
typedef void*(WINAPI* CreateCompatibleDCFn)(void*);
typedef void*(WINAPI* CreateDIBSectionFn)(void*, const BmiHeader*, u32, void**, void*, u32);
typedef void*(WINAPI* SelectObjectFn)(void*, void*);
typedef i32(WINAPI* BitBltFn)(void*, i32, i32, i32, i32, void*, i32, i32, u32);
typedef i32(WINAPI* DeleteFn)(void*);

static void probeGdi(void* gdi) {
    CreateCompatibleDCFn ccdc = (CreateCompatibleDCFn)GetProcAddress(gdi, "CreateCompatibleDC");
    CreateDIBSectionFn cds = (CreateDIBSectionFn)GetProcAddress(gdi, "CreateDIBSection");
    SelectObjectFn sel = (SelectObjectFn)GetProcAddress(gdi, "SelectObject");
    BitBltFn bitblt = (BitBltFn)GetProcAddress(gdi, "BitBlt");
    DeleteFn deldc = (DeleteFn)GetProcAddress(gdi, "DeleteDC");
    DeleteFn delobj = (DeleteFn)GetProcAddress(gdi, "DeleteObject");
    // 256 × 8: Zeile 0 R=x, 1 G=x, 2 B=x, 3 grau x, 4…7 einfarbig 0x84/0x42/0x21 (Dithering?)
    BmiHeader bh = {40, 256, -8, 1, 24, 0, 0, 0, 0, 0, 0};
    u8* bits = 0;
    void* bmp = cds(0, &bh, 0, (void**)&bits, 0, 0);
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 256; ++x) {
            u8* q = bits + y * 768 + x * 3;  // B, G, R
            q[0] = q[1] = q[2] = 0;
            if (y == 0) q[2] = (u8)x;
            if (y == 1) q[1] = (u8)x;
            if (y == 2) q[0] = (u8)x;
            if (y == 3) q[0] = q[1] = q[2] = (u8)x;
            if (y >= 4) { q[2] = 0x84; q[1] = 0x42; q[0] = 0x21; }
        }
    Com* s = create16(256, 8, kCapsSystem, 0);
    void* sdc = 0;
    HR hr = ((GetDCFn)VT(s, 17))(s, &sdc);
    void* mdc = ccdc(sdc);
    sel(mdc, bmp);
    i32 ok = bitblt(sdc, 0, 0, 256, 8, mdc, 0, 0, 0x00CC0020);
    ((ReleaseDCFn)VT(s, 26))(s, sdc);
    deldc(mdc);
    i32 pitch;
    u16* p = lock(s, 0, &pitch, 0);
    u32 trunc = 0, round = 0, other = 0, ditherDiff = 0;
    i32 firstOther = -1;
    for (int y = 0; y < 4; ++y)
        for (int x = 0; x < 256; ++x) {
            u16 v = ((u16*)((u8*)p + y * pitch))[x];
            u32 r = y == 0 || y == 3 ? (u32)x : 0, g = y == 1 || y == 3 ? (u32)x : 0, b = y == 2 || y == 3 ? (u32)x : 0;
            u16 t = (u16)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
            u32 rr = (r * 31 + 127) / 255, rg = (g * 63 + 127) / 255, rb = (b * 31 + 127) / 255;
            u16 o = (u16)((rr << 11) | (rg << 5) | rb);
            if (v == t) ++trunc;
            if (v == o) ++round;
            if (v != t && v != o) { ++other; if (firstOther < 0) firstOther = y * 256 + x; }
        }
    u16 ref = ((u16*)((u8*)p + 4 * pitch))[0];
    for (int y = 4; y < 8; ++y)
        for (int x = 0; x < 256; ++x) ditherDiff += ((u16*)((u8*)p + y * pitch))[x] != ref;
    unlock(s);
    outs("gdi_24to16: getdc=");
    outx((u32)hr);
    outs(" bitblt=");
    outi(ok);
    outs(" abschneiden=");
    outi((i32)trunc);
    outs("/1024 runden=");
    outi((i32)round);
    outs(" weder=");
    outi((i32)other);
    outs(" erstes_weder=");
    outi(firstOther);
    outs(" einfarbig(84,42,21)=");
    outx(ref);
    outs(" abweichend=");
    outi((i32)ditherDiff);
    outs("/1024\n");
    release(s);
    delobj(bmp);
}

void start(void) {
    void* ddraw = LoadLibraryA("ddraw.dll");
    void* gdi = LoadLibraryA("gdi32.dll");
    DDCreateFn create = (DDCreateFn)GetProcAddress(ddraw, "DirectDrawCreate");
    HR hr = create ? create(0, &dd, 0) : -1;
    outs("DirectDrawCreate: ");
    outx((u32)hr);
    outs("\n");
    if (hr != 0) ExitProcess(1);
    outs("SetCooperativeLevel(NORMAL): ");
    outx((u32)((SetCoopFn)VT(dd, 20))(dd, 0, kDDSCL_NORMAL));
    outs("\n");
    probePitch(kCapsSystem, "system");
    probePitch(kCapsVideo, "video");
    probeNewContent(kCapsSystem, "system");
    probeNewContent(kCapsVideo, "video");
    probeLockRect();
    probeBltFast();
    probeStretch();
    probeGdi(gdi);
    release(dd);
    ExitProcess(0);
}
