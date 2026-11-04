// SPDX-License-Identifier: GPL-3.0-or-later
// Messprogramm (Win32, i686, ohne CRT): DirectSound-Fähigkeiten und DirectSound3D-Vorgaben.
// Das Original schaltet 3D nur bei dwFreeHw3DAllBuffers ≠ 0 ein (AUDIO.md, +0x74) und legt
// statische Puffer mit Flags 0xE2 (+0x10 bei 3D) an, Format 22 050 Hz mono 16 Bit.
// Ausgabe: Textzeilen. docs/WINEPROBE.md.
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

int _fltused = 0;  // ohne CRT: Marker für Gleitkomma-Nutzung

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
// float ohne CRT: Wert × 1000 gerundet
static void outf(float f) { outi((i32)(f * 1000.0f + (f < 0 ? -0.5f : 0.5f))); outs("e-3"); }

typedef struct { void** vt; } Com;
typedef i32 HR;
typedef struct { u32 d1; u16 d2, d3; u8 d4[8]; } Guid;
typedef struct { u16 tag, ch; u32 rate, avg; u16 align, bits, cb; } WaveFmt;
typedef struct { u32 dwSize, dwFlags, dwBufferBytes, dwReserved; WaveFmt* fmt; } BufDesc;
typedef struct { float x, y, z; } Vec;
typedef struct { u32 dwSize; Vec pos, vel; u32 inCone, outCone; Vec coneOrient; i32 coneOutVol; float minDist, maxDist; u32 mode; } Ds3dBuffer;
typedef struct { u32 dwSize; Vec pos, vel, front, top; float distFactor, rolloff, doppler; } Ds3dListener;

typedef HR(WINAPI* DSCreateFn)(void*, Com**, void*);
typedef HR(WINAPI* QIFn)(Com*, const Guid*, Com**);
typedef HR(WINAPI* GetCapsFn)(Com*, u32*);
typedef HR(WINAPI* SetCoopFn)(Com*, void*, u32);
typedef HR(WINAPI* CreateBufFn)(Com*, BufDesc*, Com**, void*);
typedef HR(WINAPI* GetAllFn)(Com*, void*);
typedef void*(WINAPI* GetDesktopWindowFn)(void);

static const Guid kIID3DBuffer = {0x279AFA86, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}};
static const Guid kIID3DListener = {0x279AFA84, 0x4981, 0x11CE, {0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60}};

void start(void) {
    void* dsound = LoadLibraryA("dsound.dll");
    void* user = LoadLibraryA("user32.dll");
    DSCreateFn create = (DSCreateFn)GetProcAddress(dsound, "DirectSoundCreate");
    GetDesktopWindowFn desk = (GetDesktopWindowFn)GetProcAddress(user, "GetDesktopWindow");
    Com* ds = 0;
    HR hr = create ? create(0, &ds, 0) : -1;
    outs("DirectSoundCreate: ");
    outx((u32)hr);
    outs("\n");
    if (hr != 0) ExitProcess(1);
    outs("SetCooperativeLevel(PRIORITY): ");
    outx((u32)((SetCoopFn)ds->vt[6])(ds, desk(), 2));
    outs("\n");
    u32 caps[24];
    for (int i = 0; i < 24; ++i) caps[i] = 0;
    caps[0] = 96;
    hr = ((GetCapsFn)ds->vt[4])(ds, caps);
    outs("GetCaps: ");
    outx((u32)hr);
    outs(" dwFlags=");
    outx(caps[1]);
    outs(" MaxHw3DAll=");
    outi((i32)caps[11]);
    outs(" FreeHw3DAll=");
    outi((i32)caps[14]);
    outs(" FreeHwMixingAll=");
    outi((i32)caps[8]);
    outs("\n");

    WaveFmt wf = {1, 1, 22050, 44100, 2, 16, 0};
    BufDesc bd = {20, 0xE2 | 0x10, 22050 * 2, 0, &wf};
    Com* buf = 0;
    hr = ((CreateBufFn)ds->vt[3])(ds, &bd, &buf, 0);
    outs("CreateSoundBuffer(0xF2, 3D): ");
    outx((u32)hr);
    outs("\n");
    if (hr == 0) {
        Com* b3 = 0;
        hr = ((QIFn)buf->vt[0])(buf, &kIID3DBuffer, &b3);
        Ds3dBuffer p;
        u8* q = (u8*)&p;
        for (u32 i = 0; i < sizeof p; ++i) q[i] = 0;
        p.dwSize = sizeof p;
        HR g = b3 ? ((GetAllFn)b3->vt[3])(b3, &p) : -1;
        outs("3D-Puffer: qi=");
        outx((u32)hr);
        outs(" get=");
        outx((u32)g);
        outs(" minDist=");
        outf(p.minDist);
        outs(" maxDist=");
        outi((i32)(p.maxDist / 1000.0f));
        outs("e3");
        outs(" mode=");
        outi((i32)p.mode);
        outs(" cone=");
        outi((i32)p.inCone);
        outs("/");
        outi((i32)p.outCone);
        outs(" coneOutVol=");
        outi(p.coneOutVol);
        outs(" pos=");
        outf(p.pos.x); outs(","); outf(p.pos.y); outs(","); outf(p.pos.z);
        outs("\n");
    }
    BufDesc pd = {20, 0x01 | 0x10, 0, 0, 0};  // PRIMARYBUFFER | CTRL3D
    Com* prim = 0;
    hr = ((CreateBufFn)ds->vt[3])(ds, &pd, &prim, 0);
    if (hr == 0) {
        Com* l = 0;
        HR q = ((QIFn)prim->vt[0])(prim, &kIID3DListener, &l);
        Ds3dListener p;
        u8* b = (u8*)&p;
        for (u32 i = 0; i < sizeof p; ++i) b[i] = 0;
        p.dwSize = sizeof p;
        HR g = l ? ((GetAllFn)l->vt[3])(l, &p) : -1;
        outs("Listener: qi=");
        outx((u32)q);
        outs(" get=");
        outx((u32)g);
        outs(" distFactor=");
        outf(p.distFactor);
        outs(" rolloff=");
        outf(p.rolloff);
        outs(" doppler=");
        outf(p.doppler);
        outs(" front=");
        outf(p.front.x); outs(","); outf(p.front.y); outs(","); outf(p.front.z);
        outs(" top=");
        outf(p.top.x); outs(","); outf(p.top.y); outs(","); outf(p.top.z);
        outs("\n");
    } else {
        outs("Primärpuffer: ");
        outx((u32)hr);
        outs("\n");
    }
    ExitProcess(0);
}
