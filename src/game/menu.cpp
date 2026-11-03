// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/menu.hpp"

#include <cstdio>

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/help.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {

constexpr int kKnobLeft = 0x1EA, kKnobRight = 0x21E;  // 490…542
// „Leer...“ (0x00476A3C): Kennung eines leeren Platzes, auch für den Vergleich beim Laden
constexpr const char* kEmpty = "Leer...";

// orig: Rent-A-Hero.exe 0x004445A0 – lineare Abbildung p (0…100) auf lo…hi
int lerp(int lo, int hi, int p) { return (hi - lo) * p / 100 + lo; }

// orig: Rent-A-Hero.exe 0x00444550 – x auf 490…542 begrenzen, Prozent liefern
int knobPercent(int& x) {
    if (x < 0x21F) {
        if (x < 0x1EA) x = 0x1EA;
    } else {
        x = 0x21E;
    }
    return (x - 0x1EA) * 100 / 0x34;
}

// orig: Rent-A-Hero.exe 0x00436500 SetMouseDelay (100…2000)
void setMouseDelay(EngineGlobals& g, int v) {
    if (v < 100) v = 100;
    else if (v > 2000) v = 2000;
    g.mouseDelay = static_cast<std::uint32_t>(v);
}

}  // namespace

GameMenu::GameMenu(GameContext& ctx, Scene& menuScene) : Game(ctx, menuScene, 0, 0, 0) {
    getViewObjects();
    sceneNr_ = 0;
    scene_.load("Menu", 0xFFFF);  // CSzene_LoadAll(0x00476A08)
    getViewObjects();
    mainMenu_ = scene_.getObject("Menu_MainMenu");
    rightMenu_ = scene_.getObject("Menu_RightMenu");
    optHead_ = scene_.getObject("Menu_OptHead");
    subtitles_ = scene_.getObject("Menu_Untertitel");
    infoText_ = scene_.getObject("Menu_InfoText");
    music_ = scene_.getObject("Menu_Musik");
    effects_ = scene_.getObject("Menu_Effekte");
    speechL_ = scene_.getObject("Menu_Sprache");
    buttonOn_ = scene_.getObject("Menu_ButtonOn");
    buttonOff_ = scene_.getObject("Menu_ButtonOff");
    sliderBack_[0] = scene_.getObject("Menu_ReglerBack_InfoText");
    slider_[0] = scene_.getObject("Menu_Regler_InfoText");
    sliderBack_[1] = scene_.getObject("Menu_ReglerBack_Musik");
    slider_[1] = scene_.getObject("Menu_Regler_Musik");
    sliderBack_[2] = scene_.getObject("Menu_ReglerBack_Effekte");
    slider_[2] = scene_.getObject("Menu_Regler_Effekte");
    sliderBack_[3] = scene_.getObject("Menu_ReglerBack_Sprache");
    slider_[3] = scene_.getObject("Menu_Regler_Sprache");
    loadL_ = scene_.getObject("Menu_SpielLaden");
    saveL_ = scene_.getObject("Menu_SpielSpeichern");
    options_ = scene_.getArea("Optionen");
    saves_ = scene_.getArea("Spielstand");
    newGame_ = scene_.getArea("NeuesSpiel");
    quit_ = scene_.getArea("Beenden");
    for (int i = 0; i < 9; ++i) {
        char name[16];
        std::snprintf(name, sizeof name, "File%d", i + 1);  // "File%d" (0x0047685C)
        files_[static_cast<std::size_t>(i)] = scene_.getArea(name);
    }
    sfxLarge_ = loadSfx("Sfx_ButtonLarge");
    sfxSmall_ = loadSfx("Sfx_ButtonSmall");
    sfxFire_ = loadSfx("Sfx_Fire");
    rod_ = loadSfx("Menu_ROD_01");
    if (rod_) {
        rod_->kind = kAudioSpeech;  // +0x04 = 3
        rod_->applyVolume();        // 0x00437350
    }
    slideIn_ = true;  // +0x80
}

GameMenu::~GameMenu() {
    // orig: Rent-A-Hero.exe 0x004439D0 – Töne löschen, dann CGame_dtor
    sfxLarge_.reset();
    sfxSmall_.reset();
    sfxFire_.reset();
    rod_.reset();
    shutdown();
}

namespace {
void play(const std::unique_ptr<AudioBuffer>& b) {
    if (b) b->play();  // vtable +0x08
}
void setAreaEvent(Area* a, int id) {
    if (a) a->setEvent(id);
}
void setLayerEvent(Layer* l, int id) {
    if (l) setAreaEvent(l->area, id);
}
void enable(Layer* l) {
    if (l) l->enable();
}
void disable(Layer* l) {
    if (l) l->disable();
}
void activate(Area* a) {
    if (a) a->active = true;  // +0xB8 = 1
}
void activateLayerArea(Layer* l) {
    if (l) activate(l->area);
}
}  // namespace

void GameMenu::start() {
    slideIn_ = true;
    baked_ = false;
    drawNames_ = false;
    drawFrame_ = false;
    setEventProc(&GameMenu::onEvent);  // 0x00444010
    scene_.disableAll();
    setAreaEvent(options_, kOptions);
    setAreaEvent(saves_, kSaves);
    setAreaEvent(newGame_, kNewGame);
    setAreaEvent(quit_, kQuit);
    setLayerEvent(buttonOn_, kSubOn);
    setLayerEvent(buttonOff_, kSubOff);
    for (int i = 0; i < 4; ++i) setLayerEvent(slider_[static_cast<std::size_t>(i)], kSliderMouse + i);
    for (int i = 0; i < 9; ++i) setAreaEvent(files_[static_cast<std::size_t>(i)], kFile1 + i);
    setLayerEvent(loadL_, kLoad);
    setLayerEvent(saveL_, kSave);
    for (Layer* k : slider_)
        if (k) k->fps = 0x14;  // +0x54
    buildPage();
    scene_.invalidateAll();
    scene_.enableInput();
}

void GameMenu::enableKnob(Layer* back, Layer* knob, int percent) {
    enable(back);
    if (knob) knob->setX(lerp(kKnobLeft, kKnobRight, percent));  // 0x00446F10
    enable(knob);
    activateLayerArea(knob);
}

void GameMenu::buildPage() {
    scene_.disableAll();
    if (Layer* bg = scene_.background()) bg->enable();  // +0x60 = CSzene+0x5C0 (GetViewObjects)
    if (slideIn_ || !baked_) {
        enable(mainMenu_);
        enable(rightMenu_);
    }
    if (slideIn_) {
        slideIn_ = false;
        scene_.invalidateAll();
        if (mainMenu_) mainMenu_->setX(-0xFA);
        if (rightMenu_) rightMenu_->setX(0x26C);
        if (mainMenu_) mainMenu_->moveTo(50.0, 0, 0, 0, 5, 0);
        const int w = beginWait();
        if (rightMenu_) rightMenu_->moveTo(300.0, 0, 0, 0, 5, w);
        wait();
        play(sfxLarge_);
        if (scene_.snapshotBackground()) {  // 0x0044A0C0
            baked_ = true;
            disable(mainMenu_);
            disable(rightMenu_);
        }
    }
    activate(options_);
    activate(saves_);
    activate(newGame_);
    activate(quit_);
    if (ctx_.menuMode != 2) {
        enable(optHead_);
        enable(subtitles_);
        enable(infoText_);
        enable(music_);
        enable(effects_);
        enable(speechL_);
        if (!g_.subtitles) {
            enable(buttonOff_);
            activateLayerArea(buttonOff_);
        } else {
            enable(buttonOn_);
            activateLayerArea(buttonOn_);
        }
        const int delay = static_cast<int>(g_.mouseDelay);
        // UNVERIFIED (Port): ohne Ausgabegerät gibt es keinen Audio-Manager – Regler dann bei 100
        AudioManager* au = g_.audio;
        enableKnob(sliderBack_[0], slider_[0], (delay * 5 - 500) * 0x14 / 0x76C);
        enableKnob(sliderBack_[1], slider_[1], au ? au->musicVolume() : 100);
        enableKnob(sliderBack_[2], slider_[2], au ? au->sfxVolume() : 100);
        enableKnob(sliderBack_[3], slider_[3], au ? au->speechVolume() : 100);
        scene_.enableInput();
        return;
    }
    enable(loadL_);
    activateLayerArea(loadL_);
    if (!ctx_.save.canSave) {
        if (saveL_) saveL_->setColor(-128, -128, -128);  // 0xFFFFFF80
        enable(saveL_);
    } else {
        if (saveL_) saveL_->setColor(0, 0, 0);
        enable(saveL_);
        activateLayerArea(saveL_);
    }
    readSlots();
    drawNames_ = true;
    drawFrame_ = true;
    for (Area* a : files_) activate(a);
    scene_.enableInput();
}

int GameMenu::dragSlider(Layer* knob) {
    scene_.disableInput();
    int p = 0;
    while (true) {
        int x = scene_.mouseX() - 0x0C;  // CSzene+0x500
        p = knobPercent(x);
        if (knob) knob->setX(x);
        if (AudioManager* au = g_.audio) {
            if (knob == slider_[1]) au->setMusicVolume(p);
            else if (knob == slider_[2]) au->setSfxVolume(p);
            else if (knob == slider_[3]) au->setSpeechVolume(p);
        }
        if (!ctx_.leftButtonDown || !ctx_.leftButtonDown()) break;  // GetAsyncKeyState(1)
        // PeekMessage(PM_REMOVE) → Translate/Dispatch (auch WM_COMMAND), dann Game_Idle
        if (ctx_.pumpMessage && ctx_.pumpMessage() == GameContext::kCommand) onCommand(ctx_, ctx_.lastCommand);
        if (ctx_.idle) ctx_.idle();
    }
    scene_.enableInput();
    return p;
}

Area* GameMenu::currentSlot() const { return files_[static_cast<std::size_t>(ctx_.save.slot)]; }

void GameMenu::readSlots() {
    SaveGlobals& s = ctx_.save;
    for (int i = 0; i < 9; ++i) {
        Area* a = files_[static_cast<std::size_t>(i)];
        if (a) a->text = kEmpty;  // strcpy ohne Kürzen
        const auto h = readSaveHeader(slotPath(s, i));  // _access + CreateFileA
        if (!h) continue;
        // ReadFile(4) nach 0x0047080C, ReadFile(30) nach g_SaveName – bei kurzer Datei nur teilweise
        if (h->versionRead) s.version = h->version;
        for (std::size_t k = 0; k < h->nameBytes; ++k) s.name[k] = h->name[k];
        if (s.version == 1 && a) a->text = saveName(s.name);
    }
}

void GameMenu::drawNames() {
    // GetDC der Szenenoberfläche, je Platz 0x00436370: Text in die Rechteckmitte
    for (Area* a : files_) {
        if (!a || !g_.drawAreaText) continue;
        const Rect& r = a->rect;
        g_.drawAreaText(scene_.surface(), *a, (r.right - r.left) / 2 + r.left, (r.bottom - r.top) / 2 + r.top);
    }
}

void GameMenu::draw() {
    Area* slot = currentSlot();
    if (ctx_.menuMode != 2) return;
    if (drawNames_) {
        drawNames_ = false;
        drawNames();
        scene_.invalidateAll();
    }
    if (drawFrame_ && slot) {
        Rect r = slot->rect;
        r.top -= 5;
        r.bottom -= 3;
        scene_.frameRect(r, 0x596FA);
        scene_.invalidateAll();
    }
}

void GameMenu::onEvent(Event& e, int) {
    SaveGlobals& s = ctx_.save;
    const auto post = [this](int id) {
        if (g_.postCommand) g_.postCommand(id);
    };
    const auto ask = [this](std::uint32_t textVa) {
        if (!ctx_.askYesNo) return false;
        const auto str = [this](std::uint32_t va) { return ctx_.exeString ? ctx_.exeString(va) : std::string(); };
        return ctx_.askYesNo(str(textVa), str(0x00470914));  // MessageBoxA(…, MB_YESNO) == IDYES
    };
    const int id = e.param;  // +0x20
    switch (id) {
    case kOptions:
    case kSaves:
        ctx_.menuMode = id;
        buildPage();
        play(sfxLarge_);
        return;
    case kNewGame:
        play(sfxLarge_);
        if (ask(0x00476AE4)) {
            s.request = 3;
            post(40092);
        }
        return;
    case kQuit:
        play(sfxLarge_);
        if (ask(s.canSave ? 0x00476AB8 : 0x00476A44)) {
            s.request = 4;
            post(40092);
        }
        return;
    case kSubOn:
        disable(buttonOn_);
        enable(buttonOff_);
        activateLayerArea(buttonOff_);
        g_.subtitles = false;
        play(sfxSmall_);
        return;
    case kSubOff:
        disable(buttonOff_);
        enable(buttonOn_);
        activateLayerArea(buttonOn_);
        g_.subtitles = true;
        play(sfxSmall_);
        return;
    case kSliderMouse:
        setMouseDelay(g_, lerp(100, 2000, dragSlider(slider_[0])));
        return;
    case kSliderMusic:
        dragSlider(slider_[1]);
        return;
    case kSliderSfx:
        if (sfxFire_) sfxFire_->playLoop();  // vtable +0x10
        dragSlider(slider_[2]);
        if (sfxFire_) sfxFire_->stop();  // vtable +0x14
        return;
    case kSliderSpeech:
        play(rod_);
        dragSlider(slider_[3]);
        return;
    case kLoad: {
        Area* slot = currentSlot();
        copySaveName(s.name, slot ? slot->text : std::string());
        if (saveName(s.name) == kEmpty) return;
        s.path = slotPath(s, s.slot);
        s.request = 1;
        post(40092);
        return;
    }
    case kSave: {
        Area* slot = currentSlot();
        if (!slot || !ctx_.editName) return;
        // DialogBoxParamA(IDD_FILENAME): OK schreibt höchstens 29 Zeichen in den Bereichstext
        std::string text = slot->text;
        if (!ctx_.editName(slot->rect, text)) return;
        if (text.size() > 29) text.resize(29);
        slot->text = text;
        copySaveName(s.name, slot->text);
        if (saveName(s.name) == kEmpty) {
            char buf[16];
            std::snprintf(buf, sizeof buf, "Game %d", s.slot);  // 0x00476A10
            copySaveName(s.name, buf);
            slot->text = buf;
        }
        s.path = slotPath(s, s.slot);
        s.request = 2;
        drawNames_ = true;
        scene_.invalidateAll();
        post(40092);
        return;
    }
    default:
        if (id >= kFile1 && id < kFile1 + 9) {
            s.slot = id - kFile1;
            play(sfxSmall_);
            drawNames_ = true;
            drawFrame_ = true;
            scene_.invalidateAll();
        }
        return;
    }
}

// orig: Rent-A-Hero.exe 0x00403520 – Menü-Szene und -Klasse beim Programmstart
void createGameMenu(GameContext& ctx, Scene& menuScene) {
    ctx.menuScene = &menuScene;
    ctx.menu = std::make_unique<GameMenu>(ctx, menuScene);
    auto* m = static_cast<GameMenu*>(ctx.menu.get());
    menuScene.setMenuScene([m] { m->draw(); });  // +0x5BC = Menüklasse, +0xAC8 = 1
    menuScene.setBarShown(true);                  // +0x5BC ≠ 0
    ctx.g.hitGrid.clear();                        // HitGrid_Clear
}

// orig: Rent-A-Hero.exe 0x00403370 GameMenu_Open
void gameMenuOpen(GameContext& ctx) {
    EngineGlobals& g = ctx.g;
    if (g.gameMenuOpen || !ctx.menu || !ctx.menuScene) return;
    if (g.idleDepth != 0) {  // g_FrameCount 0x0047D754
        dbg(g, "Menü nicht geöffnet: Game_Idle-Tiefe %d", g.idleDepth);
        return;
    }
    dbg(g, "Menü öffnen (Modus %d)", ctx.menuMode);
    g.gameMenuOpen = true;
    Game* game = ctx.current.get();
    ctx.save.canSave = game && game->hero() && game->hero()->active;  // 0x0047D694
    ctx.menu->bindEventVariant();  // SetEventVariant(g_MenuGame)
    if (game) {
        ctx.menuScene->snapshotBackground();  // 0x0044A0C0: Spielbild einfrieren
        ctx.menu->setMusic(game->musicStream());  // +0x08
    } else {
        ctx.menuScene->clearBackground();
    }
    ctx.menu->start();
}

// orig: Rent-A-Hero.exe 0x00403480 GameMenu_Close
void gameMenuClose(GameContext& ctx) {
    EngineGlobals& g = ctx.g;
    if (!g.gameMenuOpen) return;
    // g_CurScene/g_CurGame zurück (Port: curScene()/curGame() folgen aus gameMenuOpen)
    if (g.prevEventProc) {  // 0x0043F1E0
        g.eventProc = g.prevEventProc;
        g.prevEventProc = nullptr;
    }
    if (ctx.current) ctx.current->bindEventVariant();  // SetEventVariant(g_CurGame)
    else g.eventVariant = 0;
    if (auto* bar = ctx.scene.inventory()) bar->invalidate();  // CInvBar_Invalidate
    ctx.scene.invalidateAll();
    // ohne Spiel: Renderer-vtable +0x04(0) – Bildschirm leeren (Port: entfällt, Szene zeichnet neu)
    if (ctx.help) ctx.help->redrawIcon();  // 0x00441490
    g.gameMenuOpen = false;
}

// orig: Rent-A-Hero.exe 0x00402870 – WM_COMMAND (nur die Befehle mit Code; CONTROLS.md)
void onCommand(GameContext& ctx, int id) {
    EngineGlobals& g = ctx.g;
    AudioManager* au = g.audio;
    switch (id) {
    case 40079:  // 0x9C8F Spielmenü
    case 40093:  // 0x9C9D Ctrl+F1
        ctx.menuMode = 1;
        gameMenuOpen(ctx);
        return;
    case 40094:  // 0x9C9E Ctrl+F2
        ctx.menuMode = 2;
        gameMenuOpen(ctx);
        return;
    case 40092:  // 0x9C9C Menü schließen
        gameMenuClose(ctx);
        return;
    case 40081:
        if (au) au->setMusicVolume(au->musicVolume() + 10);
        return;
    case 40082:
        if (au) au->setMusicVolume(au->musicVolume() - 10);
        return;
    case 40083:
        if (au) au->setSfxVolume(au->sfxVolume() - 10);
        return;
    case 40084:
        if (au) au->setSfxVolume(au->sfxVolume() + 10);
        return;
    case 40085:
        if (au) au->setSpeechVolume(au->speechVolume() - 10);
        return;
    case 40086:
        if (au) au->setSpeechVolume(au->speechVolume() + 10);
        return;
    case 40087:
        setMouseDelay(g, static_cast<int>(g.mouseDelay) - 200);
        return;
    case 40088:
        setMouseDelay(g, static_cast<int>(g.mouseDelay) + 200);
        return;
    case 40090:
        g.subtitles = true;
        return;
    case 40091:
        g.subtitles = false;
        return;
    default:
        return;
    }
}

}  // namespace rah
