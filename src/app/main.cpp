// SPDX-License-Identifier: GPL-3.0-or-later
// Einstieg (SDL3). Vorerst ein Szenen-Betrachter: lädt eine Szene, schaltet zur Ansicht alle
// Ebenen und Bereiche ein und lässt Animationen in Schleife laufen (nicht Original – das tun
// später die Schauplätze). Klicks lösen Bereichs-Ereignisse aus, die ausgegeben werden; Überfahren zeigt Bereichstext
// und Zeiger wie im Original. Der Frame-Ablauf folgt Game_Idle (0x004030C0, docs/TIMING.md):
// CSzene_Update → Compose → Ausgabe (Szene bei y + 60) → CSzene_DispatchEvents.
#include <SDL3/SDL.h>

#include <cstdio>
#include <cstdlib>
#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

#include "audio/mixer.hpp"
#include "engine/animlayer.hpp"
#include "engine/audio.hpp"
#include "engine/help.hpp"
#include "engine/scene.hpp"
#include "game/game.hpp"
#include "formats/pe.hpp"
#include "render/font.hpp"
#include "video/ffvideo.hpp"
#include "vfs/dirfs.hpp"
#include "vfs/vfs.hpp"

using namespace rah;

namespace {

constexpr int kScreenW = 640, kScreenH = 480, kSceneTop = 60;

struct Options {
    std::vector<std::string> cd;  // Ordner, .iso oder .cue (mehrfach)
    std::string common;
    int location = 1;
    std::string scene = "7A";
    unsigned loadMask = 0xFFFF;
    int frames = -1;  // > 0: nach so vielen Frames beenden
    int frameMs = 1;  // Pause je Frame (Original: keine, TIMING.md)
    std::string dump; // PPM des Bildschirms am Ende
    bool hidden = false;
    std::string exe;   // installierte Rent-A-Hero.exe (Zeiger); sonst <Common>/../Rent-A-Hero.exe
    std::string font;  // Schriftdatei für die Bereichstexte (sonst Systemsuche)
    int mouseX = -1, mouseY = -1;  // Mausbewegung nach dem ersten Frame (kopflose Prüfung)
    int clickX = -1, clickY = -1;  // Klick im zweiten Frame (kopflose Prüfung)
    std::vector<int> items;        // Inventarleiste zeigen und diese Gegenstände hinzufügen
    std::string hint;              // Hilfe-Symbol zeigen; Klick darauf öffnet diese HLP-Datei
    int gameLoc = -1, gameScene = 0;  // Spielmodus: Schauplatz/Szene starten (Intro: 40 114)
    bool subtitles = false;           // rah.ini [MODES] SubTitles
    bool mute = false;                // ohne Tonausgabe
    bool audio3d = false;             // 3D-Ton wie mit Hardware-3D und EAX 1.0 (AUDIO.md „3D“)
    int musicVol = 100, sfxVol = 100, speechVol = 100;  // rah.ini [SOUNDS]
    std::string audioDump;            // Mischer-Ausgabe als WAV mitschneiden (Prüfung)
};

void usage() {
    std::fprintf(stderr,
                 "rentahero --cd <Ordner|.iso|.cue> [--cd …] [--common <Ordner>]\n"
                 "          [--loc <n>] [--scene <Name>] [--mask <n>]\n"
                 "          [--frames <n>] [--frame-ms <n>] [--dump <datei.ppm>] [--hidden]\n"
                 "          [--exe <Rent-A-Hero.exe>] [--font <datei.ttf>] [--mouse <x> <y>] [--click <x> <y>]\n"
                 "          [--items <id,id,…>] [--hint <HLP-Name, z. B. 04_006_A>]\n"
                 "          [--game <Schauplatz> <Szene> (Intro: --game 40 114)] [--subtitles]\n"
                 "          [--mute] [--music-vol <0-100>] [--sfx-vol <0-100>] [--speech-vol <0-100>]\n"
                 "          [--audio3d] [--audio-dump <datei.wav>]\n");
}

bool parse(int argc, char** argv, Options& o) {
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto val = [&]() -> const char* { return i + 1 < argc ? argv[++i] : nullptr; };
        const char* v = nullptr;
        if (a == "--hidden") {
            o.hidden = true;
            continue;
        }
        if (a == "--audio3d") {
            o.audio3d = true;
            continue;
        }
        if (a == "--mute") {
            o.mute = true;
            continue;
        }
        if (a == "--subtitles") {
            o.subtitles = true;
            continue;
        }
        if (!(v = val())) return false;
        if (a == "--cd") o.cd.emplace_back(v);
        else if (a == "--common") o.common = v;
        else if (a == "--loc") o.location = std::atoi(v);
        else if (a == "--scene") o.scene = v;
        else if (a == "--mask") o.loadMask = static_cast<unsigned>(std::strtoul(v, nullptr, 0));
        else if (a == "--frames") o.frames = std::atoi(v);
        else if (a == "--frame-ms") o.frameMs = std::atoi(v);
        else if (a == "--dump") o.dump = v;
        else if (a == "--font") o.font = v;
        else if (a == "--exe") o.exe = v;
        else if (a == "--hint") o.hint = v;
        else if (a == "--audio-dump") o.audioDump = v;
        else if (a == "--music-vol") o.musicVol = std::atoi(v);
        else if (a == "--sfx-vol") o.sfxVol = std::atoi(v);
        else if (a == "--speech-vol") o.speechVol = std::atoi(v);
        else if (a == "--items") {
            for (const char* p = v; *p;) {
                char* end = nullptr;
                o.items.push_back(static_cast<int>(std::strtol(p, &end, 10)));
                if (end == p) return false;
                p = *end == ',' ? end + 1 : end;
            }
        }
        else if (a == "--game" && i + 1 < argc) {
            o.gameLoc = std::atoi(v);
            o.gameScene = std::atoi(argv[++i]);
        }
        else if (a == "--click" && i + 1 < argc) {
            o.clickX = std::atoi(v);
            o.clickY = std::atoi(argv[++i]);
        }
        else if (a == "--mouse" && i + 1 < argc) {
            o.mouseX = std::atoi(v);
            o.mouseY = std::atoi(argv[++i]);
        }
        else return false;
    }
    return !o.cd.empty();
}

// Szenenausschnitt `r` (Szenenkoordinaten) in den Bildschirmpuffer, Ziel y + 60
// (orig: Ausgabe des Vollbild-Renderers 0x0043E210, COMPOSITOR.md)
void present(const Surface565& s, const Rect& r, Surface565& screen) {
    for (int y = r.top; y < r.bottom; ++y) {
        const int sy = y + kSceneTop;
        if (y < 0 || y >= s.height || sy >= kScreenH) continue;
        for (int x = r.left; x < r.right; ++x) {
            if (x < 0 || x >= s.width || x >= kScreenW) continue;
            screen.row(sy)[x] = s.row(y)[x];
        }
    }
}

void writePpm(const Surface565& screen, const std::string& path) {
    const auto& px = screen.pixels;
    std::ofstream o(path, std::ios::binary);
    o << "P6\n" << kScreenW << " " << kScreenH << "\n255\n";
    for (auto p : px) {
        std::uint8_t c[3];
        unpack565To888(p, c[0], c[1], c[2]);
        o.write(reinterpret_cast<const char*>(c), 3);
    }
}

// Mauszeiger aus den Ressourcen der installierten EXE (MOUSE.md)
class Cursors {
public:
    ~Cursors() {
        for (auto* c : cursors_)
            if (c) SDL_DestroyCursor(c);
    }
    bool load(const std::string& exe) {
        auto f = readFile(exe);
        PeResources pe;
        if (!f || !pe.parse(std::move(*f))) return false;
        int n = 0;
        for (std::size_t t = 0; t < kGroup.size(); ++t) {
            if (!kGroup[t]) continue;
            auto c = loadCursor(pe, kGroup[t]);
            if (!c || c->width % 8) continue;
            // SDL: Daten 1 + Maske 1 = schwarz, 0/1 = weiß, 0/0 = durchsichtig, 1/0 = invertiert
            const std::size_t stride = static_cast<std::size_t>(c->width / 8);
            std::vector<Uint8> data(stride * static_cast<std::size_t>(c->height)), mask(data.size());
            for (int y = 0; y < c->height; ++y)
                for (int x = 0; x < c->width; ++x) {
                    const auto p = c->pixels[static_cast<std::size_t>(y * c->width + x)];
                    const auto i = static_cast<std::size_t>(y) * stride + static_cast<std::size_t>(x / 8);
                    const auto bit = static_cast<Uint8>(0x80 >> (x % 8));
                    if (p == MonoCursor::Black || p == MonoCursor::Invert) data[i] |= bit;
                    if (p == MonoCursor::Black || p == MonoCursor::White) mask[i] |= bit;
                }
            cursors_[t] = SDL_CreateCursor(data.data(), mask.data(), c->width, c->height, c->hotX, c->hotY);
            n += cursors_[t] != nullptr;
        }
        return n > 0;
    }
    // orig: Rent-A-Hero.exe 0x0043EA60 Renderer_GetCursor – unbekannt → 99
    int get() const { return current_; }
    // orig: Rent-A-Hero.exe 0x0043E960 Renderer_SetCursor – Typ 5 und unbekannte: keine Änderung
    void set(int type) {
        if (type < 0 || static_cast<std::size_t>(type) >= cursors_.size() || !cursors_[static_cast<std::size_t>(type)])
            return;
        SDL_SetCursor(cursors_[static_cast<std::size_t>(type)]);
        current_ = type;
    }

private:
    // Typ → RT_GROUP_CURSOR (Tabelle in MOUSE.md)
    static constexpr std::array<std::uint32_t, 12> kGroup = {0x95, 0x97, 0x98, 0x99, 0x9A, 0,
                                                             0x9B, 0x9C, 0x9D, 0x9E, 0x9F, 0x96};
    std::array<SDL_Cursor*, 12> cursors_{};
    int current_ = 99;
};

}  // namespace

int main(int argc, char** argv) {
    Options opt;
    if (!parse(argc, argv, opt)) {
        usage();
        return 2;
    }

    Vfs cd;
    for (const auto& p : opt.cd) {
        std::string err;
        if (!cd.mountPath(p, &err)) {
            std::fprintf(stderr, "Datenquelle %s: %s\n", p.c_str(), err.c_str());
            return 1;
        }
    }
    std::unique_ptr<DirFileSystem> common;
    if (!opt.common.empty()) common = std::make_unique<DirFileSystem>(opt.common);
    DataPaths paths(&cd, common.get());
    paths.setLocation(opt.location);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    EngineGlobals g;
    // timeGetTime(): Millisekunden als 32-Bit-Zahl (läuft wie das Original nach ~49 Tagen über)
    g.now = [] { return static_cast<std::uint32_t>(SDL_GetTicks()); };

    g.openVideo = [](const FoundFile& f, std::string* err) -> std::unique_ptr<VideoDecoder> {
        auto data = f.read();
        if (!data) return nullptr;
        return openFfmpegVideo(std::move(*data), err);
    };
    // CArea_DrawText: Arial 15, fett, kursiv (MOUSE.md)
    Font areaFont;
    {
        const std::string file = opt.font.empty() ? Font::findSystemFont(true, true) : opt.font;
        std::string err;
        if (file.empty() || !areaFont.open(file, 15, &err))
            std::fprintf(stderr, "Schrift %s: %s – Bereichstexte aus\n", file.c_str(),
                         file.empty() ? "nicht gefunden" : err.c_str());
        else
            g.drawAreaText = [&areaFont](Surface565& s, const Area& a, int x, int y) { drawAreaText(areaFont, s, a, x, y); };
    }

    // Statustext des Renderers: Arial 12 normal (RENDERER.md)
    Font statusFont;
    {
        const std::string file = Font::findSystemFont(false, false);
        std::string err;
        if (file.empty() || !statusFont.open(file, 12, &err))
            std::fprintf(stderr, "Schrift %s: %s – Statustext aus\n", file.c_str(),
                         file.empty() ? "nicht gefunden" : err.c_str());
    }

    Surface565 surf(640, 420);  // UNVERIFIED: Größe der Szenenoberfläche (COMPOSITOR.md)
    Surface565 screen(kScreenW, kScreenH);
    g.screen = &screen;
    // Hilfetext: Arial 15 normal (0x00440F70)
    Font helpFont;
    if (const std::string file = Font::findSystemFont(false, false); !file.empty()) helpFont.open(file, 15);
    g.drawHelpText = [&helpFont](Surface565& s, const Rect& r, const std::string& t) {
        if (helpFont.isOpen()) drawTextWordBreak(helpFont, s, r, t, 0xFFFF);
    };
    // Renderer-Slot 0 (Szenenrechteck → Bildschirm) und Statustext (0x0043E600) direkt auf den Bildschirm
    g.outputSceneRect = [&](const Rect& r) { present(surf, r, screen); };
    g.drawStatusText = [&](const Rect& r, const std::string& t) {
        if (statusFont.isOpen()) drawTextWordBreak(statusFont, screen, r, t, 0xFFFF);
    };
    // Ton: DirectSound (Primärpuffer 22 050 Hz, 16 Bit, Stereo) → Mischer → SDL-Audiostrom
    struct AudioSink {
        Mixer mixer;
        bool record = false;
        std::vector<std::int16_t> recorded;  // nur im Audio-Thread beschrieben
    } sink;
    Mixer& mixer = sink.mixer;
    sink.record = !opt.audioDump.empty();
    SDL_AudioStream* audioOut = nullptr;
    if (!opt.mute && SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        const SDL_AudioSpec spec{SDL_AUDIO_S16, 2, Mixer::kRate};
        audioOut = SDL_OpenAudioDeviceStream(
            SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec,
            [](void* user, SDL_AudioStream* st, int additional, int) {
                if (additional <= 0) return;
                std::vector<std::int16_t> buf(static_cast<std::size_t>(additional / 2 + 2));
                const int pairs = additional / 4;
                auto* k = static_cast<AudioSink*>(user);
                k->mixer.mix(buf.data(), pairs);
                if (k->record) k->recorded.insert(k->recorded.end(), buf.begin(), buf.begin() + pairs * 2);
                SDL_PutAudioStreamData(st, buf.data(), pairs * 4);
            },
            &sink);
        if (audioOut) SDL_ResumeAudioStreamDevice(audioOut);
        else std::fprintf(stderr, "Ton: %s – stumm\n", SDL_GetError());
    }
    // CSzene+0x5C4 = g_Audio; ohne Ausgabegerät kein Manager (Sprache dann über den Timer-Weg)
    std::unique_ptr<AudioManager> audio;
    if (audioOut) {
        audio = std::make_unique<AudioManager>(g, &mixer, opt.audio3d);
        audio->setMusicVolume(opt.musicVol);
        audio->setSfxVolume(opt.sfxVol);
        audio->setSpeechVolume(opt.speechVol);
        g.audio = audio.get();
    }
    Scene scene(paths, g, surf);
    const bool game = opt.gameLoc >= 0;
    g.subtitles = opt.subtitles;
    GameContext ctx(g, paths, scene);
    scene.setLocationNumber(opt.location);
    if (!game && (!scene.load(opt.scene, opt.loadMask) || !scene.background())) {
        std::fprintf(stderr, "Szene %s nicht geladen\n", opt.scene.c_str());
        SDL_Quit();
        return 1;
    }
    // Betrachter: alles sichtbar, Animationen in Schleife (Modus 13)
    if (!game) scene.background()->enable();
    if (auto* a = dynamic_cast<AnimLayer*>(scene.background())) a->start(13);
    int anims = 0;
    for (int i = 0; scene.layer(i); ++i) {
        scene.layer(i)->enable();
        if (auto* a = dynamic_cast<AnimLayer*>(scene.layer(i))) {
            a->start(13);
            ++anims;
        }
    }
    // Betrachter: jeder Bereich bekommt Ereignis-ID = Platz + 1; ausgelöste Ereignisse ausgeben
    int areas = 0;
    for (; !game && areas < Scene::kMaxAreas && scene.area(areas); ++areas) scene.area(areas)->setEvent(areas + 1);
    if (!game) g.eventProc = [&scene](Event& e, int type) {
        const Area* a = e.param >= 1 && e.param <= Scene::kMaxAreas ? scene.area(e.param - 1) : nullptr;
        std::fprintf(stderr, "Ereignis %d Typ 0x%02X %s %s\n", e.param, type, a ? a->name.c_str() : "",
                     a ? a->text.c_str() : "");
    };
    if (!game) {
        scene.sceneEvent().setHandler(1000);
        scene.sceneEvent().enabled = true;
    }
    if (!game) std::fprintf(stderr, "LOC%02d/%s: %d animierte Ebenen, %d nicht unterstützt, %d Bereiche\n",
                 opt.location, opt.scene.c_str(), anims, scene.unsupportedLayers(), areas);

    Cursors cursors;
    {
        std::string exe = opt.exe;
        if (exe.empty() && !opt.common.empty()) exe = (std::filesystem::path(opt.common) / ".." / "Rent-A-Hero.exe").string();
        if (exe.empty() || !cursors.load(exe)) std::fprintf(stderr, "Zeiger: %s nicht lesbar – Systemzeiger\n", exe.c_str());
    }
    auto mouseMove = [&](int x, int y) {
        // CSzene_MouseMove Schritt 3: neu ≠ alt → Renderer_SetCursor
        const int c = scene.onMouseMove(x, y);
        if (c != cursors.get()) cursors.set(c);
    };

    SDL_Window* win = nullptr;
    SDL_Renderer* ren = nullptr;
    const SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | (opt.hidden ? SDL_WINDOW_HIDDEN : 0);
    if (!SDL_CreateWindowAndRenderer("Rent-A-Hero", kScreenW * 2, kScreenH * 2, flags, &win, &ren)) {
        std::fprintf(stderr, "Fenster: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderLogicalPresentation(ren, kScreenW, kScreenH, SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
    SDL_Texture* tex =
        SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, kScreenW, kScreenH);
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);

    // WinMain: 0x004410C0(…), 0x00441110("HelpIcon.tcc"), CSzene+0x9F8 = Hilfe
    HelpOverlay help(g);
    help.setup(&scene, [&help, &opt] {
        // CGame_ShowHint: Hinweis gesetzt und Countdown abgelaufen (Betrachter: --hint)
        if (!opt.hint.empty()) help.show(opt.hint);
    }, true);
    {
        const std::string icon = "HelpIcon.tcc";
        help.load(&icon);
    }
    scene.setHelp(&help);
    ctx.help = &help;
    if (game) {
        // Spielstart wie „Neues Spiel“: RequestLocation, Ausführung im ersten Game_Idle
        requestLocation(ctx.request, opt.gameLoc, opt.gameScene, 0, false);
        processLocationChange(ctx);
        if (!ctx.current) std::fprintf(stderr, "Schauplatz %d/%d nicht verfügbar\n", opt.gameLoc, opt.gameScene);
    }
    if (!opt.hint.empty()) help.iconOn();
    if (!opt.items.empty() && scene.inventory()) {
        scene.setBarShown(true);  // Game_SwitchLocation: +0x5BC = g_CurGame
        for (int id : opt.items)
            if (!scene.inventory()->add(id)) std::fprintf(stderr, "Gegenstand %d nicht geladen\n", id);
        scene.inventory()->setDescribe(0, [](int id) { std::fprintf(stderr, "Beschreibung %d\n", id); });
    }
    bool quit = false;
    for (int frame = 0; !quit && (opt.frames < 0 || frame < opt.frames); ++frame) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_EVENT_QUIT) quit = true;
            if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_ESCAPE) quit = true;
            // WndProc 0x00402350: WM_LBUTTONDOWN/UP, WM_LBUTTONDBLCLK, WM_RBUTTONDOWN und
            // WM_RBUTTONDBLCLK → OnMouseButton → CSzene_OnClick (Art wird nicht weitergegeben,
            // EVENTS.md). Rechte Taste loslassen (0x205) gehört nicht dazu.
            const bool down = ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN;
            const bool up = ev.type == SDL_EVENT_MOUSE_BUTTON_UP;
            if ((down && (ev.button.button == SDL_BUTTON_LEFT || ev.button.button == SDL_BUTTON_RIGHT)) ||
                (up && ev.button.button == SDL_BUTTON_LEFT)) {
                SDL_ConvertEventToRenderCoordinates(ren, &ev);
                scene.onClick(static_cast<int>(ev.button.x), static_cast<int>(ev.button.y));
            }
            // WM_MOUSEMOVE → OnMouseMove 0x00402790 → CSzene_MouseMove (MOUSE.md); Zeiger folgen
            if (ev.type == SDL_EVENT_MOUSE_MOTION) {
                SDL_ConvertEventToRenderCoordinates(ren, &ev);
                mouseMove(static_cast<int>(ev.motion.x), static_cast<int>(ev.motion.y));
            }
        }
        if (frame == 1 && opt.mouseX >= 0) mouseMove(opt.mouseX, opt.mouseY);
        if (frame == 2 && opt.clickX >= 0) scene.onClick(opt.clickX, opt.clickY);
        pollTimers(g);  // Port: Timer im Hauptthread je Schleifendurchlauf (TIMING.md)
        scene.update(true);
        // Game_Idle: Compose, Inventarleiste (0x00441C30), Ausgabe, dann Statustext (0x0043E830)
        if (scene.compose(false)) {
            if (auto r = scene.output().first()) present(surf, *r, screen);
            scene.clearOutput();
        }
        if (scene.inventory() && scene.inventory()->draw()) {
            present(surf, scene.inventory()->takeOutputRect(), screen);
            rendererRedrawStatusText(g);
        }
        // Statustext und Löschen schreiben auch außerhalb davon in den Bildschirm
        SDL_UpdateTexture(tex, nullptr, screen.pixels.data(), kScreenW * 2);
        SDL_RenderClear(ren);
        SDL_RenderTexture(ren, tex, nullptr, nullptr);
        SDL_RenderPresent(ren);
        scene.dispatchEvents();
        if (game) {
            const int before = ctx.current ? ctx.current->location() * 1000 + ctx.current->sceneNumber() : -1;
            processLocationChange(ctx);  // Game_Idle Schritt 7
            const int after = ctx.current ? ctx.current->location() * 1000 + ctx.current->sceneNumber() : -1;
            if (ctx.current && before != after)
                std::fprintf(stderr, "Schauplatz %d / Szene %d\n", ctx.current->location(), ctx.current->sceneNumber());
        }
        if (opt.frameMs > 0) SDL_Delay(static_cast<Uint32>(opt.frameMs));
    }
    if (!opt.dump.empty()) writePpm(screen, opt.dump);
    ctx.current.reset();  // vor Hilfe und Szene abbauen (CGame_dtor greift auf beide zu)
    ctx.music.reset();
    if (audioOut) SDL_DestroyAudioStream(audioOut);  // vor dem Mischer
    audioOut = nullptr;
    if (sink.record) {
        std::ofstream o(opt.audioDump, std::ios::binary);
        auto u32 = [&](std::uint32_t v) { o.write(reinterpret_cast<const char*>(&v), 4); };
        auto u16 = [&](std::uint16_t v) { o.write(reinterpret_cast<const char*>(&v), 2); };
        const auto bytes = static_cast<std::uint32_t>(sink.recorded.size() * 2);
        o.write("RIFF", 4), u32(36 + bytes), o.write("WAVEfmt ", 8), u32(16), u16(1), u16(2);
        u32(Mixer::kRate), u32(Mixer::kRate * 4), u16(4), u16(16), o.write("data", 4), u32(bytes);
        o.write(reinterpret_cast<const char*>(sink.recorded.data()), bytes);
    }
    g.audio = nullptr;
    audio.reset();

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
