// SPDX-License-Identifier: GPL-3.0-or-later
// Messprogramm (Win32, ohne CRT): dekodiert Indeo-5-Bilder mit dem Original-Codec
// (ir50_32.dll, VfW-DriverProc direkt) zur Referenz für den eigenen Transparenz-Leser.
// Eingabe in.bin (tools/iv5ref/avi2bin.py): u32 bpp, u32 strf-Länge, strf (BITMAPINFOHEADER),
// u32 Bildanzahl, je Bild u32 Länge + Daten. Ausgabe out.bin: je Bild biSizeImage Bytes
// (DIB, unten beginnend), davor u32 Rückgabewert von ICM_DECOMPRESS.
typedef unsigned int u32;
typedef int i32;
typedef void* HANDLE;
#define WINAPI __stdcall
__declspec(dllimport) HANDLE WINAPI CreateFileA(const char*, u32, u32, void*, u32, u32, HANDLE);
__declspec(dllimport) i32 WINAPI ReadFile(HANDLE, void*, u32, u32*, void*);
__declspec(dllimport) i32 WINAPI WriteFile(HANDLE, const void*, u32, u32*, void*);
__declspec(dllimport) i32 WINAPI CloseHandle(HANDLE);
__declspec(dllimport) void* WINAPI LoadLibraryA(const char*);
__declspec(dllimport) void* WINAPI GetProcAddress(void*, const char*);
__declspec(dllimport) void* WINAPI VirtualAlloc(void*, u32, u32, u32);
__declspec(dllimport) void WINAPI ExitProcess(u32);
__declspec(dllimport) HANDLE WINAPI GetStdHandle(u32);

typedef i32(WINAPI* DriverProcFn)(u32 id, HANDLE drv, u32 msg, i32 p1, i32 p2);

typedef struct {
    u32 biSize; i32 biWidth, biHeight; unsigned short biPlanes, biBitCount;
    u32 biCompression, biSizeImage; i32 biXPelsPerMeter, biYPelsPerMeter; u32 biClrUsed, biClrImportant;
} BIH;
typedef struct {
    u32 dwSize, fccType, fccHandler, dwVersion, dwFlags; i32 dwError; void *r1, *r2; u32 dnDevNode;
} ICOPEN;
typedef struct {
    u32 dwFlags; BIH* lpbiInput; void* lpInput; BIH* lpbiOutput; void* lpOutput; u32 ckid;
} ICDECOMPRESS;

void* memset(void* d, int c, unsigned n) {
    unsigned char* p = d;
    while (n--) *p++ = (unsigned char)c;
    return d;
}
void* memcpy(void* d, const void* s, unsigned n) {
    unsigned char* p = d;
    const unsigned char* q = s;
    while (n--) *p++ = *q++;
    return d;
}

static void say(const char* s) {
    u32 n = 0, w;
    while (s[n]) ++n;
    WriteFile(GetStdHandle((u32)-11), s, n, &w, 0);
}
static void fail(const char* s) { say(s); say("\n"); ExitProcess(1); }

static unsigned char* buf;
static u32 pos;
static u32 rd(void) { u32 v; memcpy(&v, buf + pos, 4); pos += 4; return v; }

void start(void) {
    HANDLE f = CreateFileA("in.bin", 0x80000000u, 1, 0, 3, 0x80, 0);
    if (f == (HANDLE)-1) fail("in.bin fehlt");
    buf = VirtualAlloc(0, 64u << 20, 0x3000, 4);
    u32 got = 0;
    ReadFile(f, buf, 64u << 20, &got, 0);
    CloseHandle(f);
    const u32 bpp = rd();
    const u32 strfLen = rd();
    BIH in;
    memset(&in, 0, sizeof in);
    memcpy(&in, buf + pos, strfLen < sizeof in ? strfLen : sizeof in);
    pos += strfLen;
    const u32 frames = rd();

    void* dll = LoadLibraryA("ir50_32.dll");
    if (!dll) fail("ir50_32.dll nicht geladen");
    DriverProcFn dp = (DriverProcFn)GetProcAddress(dll, "DriverProc");
    if (!dp) fail("DriverProc fehlt");
    dp(0, (HANDLE)1, 1, 0, 0);  // DRV_LOAD
    dp(0, (HANDLE)1, 2, 0, 0);  // DRV_ENABLE
    ICOPEN op;
    memset(&op, 0, sizeof op);
    op.dwSize = sizeof op;
    op.fccType = 0x63646976;  // 'vidc'
    op.fccHandler = in.biCompression;
    op.dwFlags = 2;  // ICMODE_DECOMPRESS
    const u32 id = (u32)dp(0, (HANDLE)1, 3, 0, (i32)&op);  // DRV_OPEN
    if (!id) fail("DRV_OPEN fehlgeschlagen");

    BIH out;
    memset(&out, 0, sizeof out);
    out.biSize = sizeof out;
    out.biWidth = in.biWidth;
    out.biHeight = in.biHeight;
    out.biPlanes = 1;
    out.biBitCount = (unsigned short)bpp;
    const u32 stride = ((u32)in.biWidth * bpp / 8 + 3) & ~3u;
    out.biSizeImage = stride * (u32)in.biHeight;
    i32 r = dp(id, (HANDLE)1, 0x400C, (i32)&in, (i32)&out);  // ICM_DECOMPRESS_BEGIN
    if (r != 0) fail("ICM_DECOMPRESS_BEGIN abgelehnt");

    HANDLE o = CreateFileA("out.bin", 0x40000000u, 0, 0, 2, 0x80, 0);
    unsigned char* pix = VirtualAlloc(0, out.biSizeImage, 0x3000, 4);
    for (u32 i = 0; i < frames; ++i) {
        const u32 len = rd();
        in.biSizeImage = len;
        // Vorbelegung 0xA5: unveränderte (transparente?) Pixel bleiben erkennbar
        memset(pix, 0xA5, out.biSizeImage);
        ICDECOMPRESS d;
        memset(&d, 0, sizeof d);
        d.lpbiInput = &in;
        d.lpInput = buf + pos;
        d.lpbiOutput = &out;
        d.lpOutput = pix;
        r = dp(id, (HANDLE)1, 0x400D, (i32)&d, sizeof d);  // ICM_DECOMPRESS
        pos += len;
        u32 w;
        WriteFile(o, &r, 4, &w, 0);
        WriteFile(o, pix, out.biSizeImage, &w, 0);
    }
    CloseHandle(o);
    dp(id, (HANDLE)1, 0x400E, 0, 0);  // ICM_DECOMPRESS_END
    dp(id, (HANDLE)1, 4, 0, 0);       // DRV_CLOSE
    say("ok\n");
    ExitProcess(0);
}
