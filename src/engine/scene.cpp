// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/scene.hpp"

#include "engine/help.hpp"

#include <cmath>
#include <cstdio>

#include "core/strings.hpp"

namespace rah {

namespace {

inline int ftol(double v) {
    return static_cast<int>(std::trunc(v));
}

}  // namespace

Scene::Scene(const DataPaths& paths, EngineGlobals& globals, Surface565& target, std::uint16_t colorKey, int yOffset)
    : paths_(paths), g_(globals), surface_(target), colorKey_(colorKey), yOffset_(yOffset) {
    g_.toggleScene = this;  // orig: 0x004483DE
    if (!editor_) bar_ = std::make_unique<InventoryBar>(*this, g_);  // orig: 0x00448638
    if (!g_.toggleEvent && !editor_) {
        // orig: 0x004484E8 – einmalig, deaktiviert, Funktions-Handler 0x0044C8D0
        g_.toggleEvent = std::make_unique<Event>();
        EngineGlobals* g = &g_;
        g_.toggleEvent->setHandler([g](Event&, int) {
            // orig: Rent-A-Hero.exe 0x0044C8D0 Scene_ToggleTimerEvent
            if (!g->textVisible) {
                g->textVisible = true;
                g->toggleEvent->enabled = true;
                setEventTimer(*g, *g->toggleEvent, 2000, 0);
                g->sceneChanged = true;
                return;
            }
            g->textVisible = false;
            g->toggleEvent->enabled = false;
            g->toggleEvent->pending = false;
            if (g->toggleScene) {
                g->toggleScene->invalidateAll();
                g->toggleScene->lastHit_ = nullptr;
            }
        });
    }
    windowStart_ = g_.now();  // orig: 0x004485FD in CSzene_ctor
    g_.hitGrid.clear();       // orig: 0x00448603
}

Scene::~Scene() {
    if (g_.toggleScene == this) g_.toggleScene = nullptr;  // Original lässt den Zeiger stehen
}

void Scene::hideToggle() {
    if (g_.toggleEvent) {
        g_.toggleEvent->enabled = false;
        g_.toggleEvent->pending = false;
    }
}

namespace {

// _splitpath(path, 0, 0, fname, 0) – orig: Rent-A-Hero.exe 0x00447F00
std::string fileStem(const std::string& path) {
    const auto slash = path.find_last_of("/\\:");
    std::string f = slash == std::string::npos ? path : path.substr(slash + 1);
    const auto dot = f.rfind('.');
    if (dot != std::string::npos) f.resize(dot);
    return f;
}

void markEvent(EngineGlobals& g, Event& e, int type) {
    e.mark(g, type);
}

// Stufe = min((fps − 1) / 5, 3), Ganzzahldivision (fps ≤ 0 → Stufe 0 bzw. negativ → 0)
int stageOf(int fps) {
    const int st = (fps - 1) / 5;
    return st > 3 ? 3 : st < 0 ? 0 : st;  // UNVERIFIED: fps < 0 kommt nicht vor (Index wäre negativ)
}

}  // namespace

void Scene::moveZ(unsigned oldZ, unsigned newZ) {
    // Zähler je Z (+0x300) und Grenzen +0x2F8/+0x2FC, Vergleiche vorzeichenlos (jbe/jae)
    auto cnt = [this](unsigned z) -> short& { return zCount_[z & 0xFFu]; };  // UNVERIFIED: Z > 255
    --cnt(oldZ);
    ++cnt(newZ);
    if (newZ > static_cast<unsigned>(zMax_)) zMax_ = static_cast<int>(newZ);
    if (newZ < static_cast<unsigned>(zMin_)) zMin_ = static_cast<int>(newZ);
    if (oldZ == static_cast<unsigned>(zMax_)) {
        while (cnt(static_cast<unsigned>(zMax_)) == 0 && static_cast<unsigned>(zMax_) > 1) --zMax_;
    }
    if (oldZ == static_cast<unsigned>(zMin_)) {
        while (cnt(static_cast<unsigned>(zMin_)) == 0 && static_cast<unsigned>(zMin_) < 0xFE) ++zMin_;
    }
}

void Scene::dispatchEvents() {
    fireEvent(g_, sceneEvent_, sceneEvent_.type);
    if (!editor_ && g_.toggleEvent) fireEvent(g_, *g_.toggleEvent, g_.toggleEvent->type);
    // 2. Vollbildobjekt: Ende-Ereignis (vorher abmelden, wenn beendet und auszublenden),
    // dann Bild-Ereignis, solange es noch angemeldet ist
    if (modal_ && modal_->endEvent) {
        Event& e = *modal_->endEvent;
        if (modal_->ended && modal_->hideAtEnd) {
            modal_->stop(0);
            modal_ = nullptr;
            invalidateAll();
        }
        fireEvent(g_, e, e.type);
    }
    if (modal_ && modal_->frameEvent) fireEvent(g_, *modal_->frameEvent, modal_->frameEvent->type);
    // Figuren +0x5E0/+0x5E4 folgen mit CActor.
    for (auto& slot : layers_) {
        Layer* L = slot.get();
        if (!L) break;
        if (L->ended && L->hideAtEnd) L->disable();  // vor dem Ende-Ereignis
        for (Event* e : {L->endEvent.get(), L->frameEvent.get(), L->countEvent.get(), L->tweenEvent.get()}) {
            if (e) fireEvent(g_, *e, e->type);
        }
    }
    for (auto& a : areas_) {
        if (!a) break;
        fireEvent(g_, *a->event, a->event->type);
    }
    if (!editor_) {
        fireEvent(g_, g_.clickEvent, g_.clickEvent.type);
        fireTimers(g_);
        if (bar_) {
            fireEvent(g_, *bar_->area().event, bar_->area().event->type);
            if (Event* e = bar_->useEvent()) fireEvent(g_, *e, e->type);
        }
    }
}

void Scene::update(bool realtime) {
    if (!loaded_) return;
    std::array<bool, 4> tick{};
    if (!realtime) {
        tick.fill(true);  // Original zusätzlich Sleep(0)
    } else {
        const std::uint32_t now = g_.now();
        const std::uint32_t dt = now - windowStart_;  // vorzeichenlos (jb)
        if (dt >= 200) {
            windowStart_ += 200;
            if (windowStart_ < now) windowStart_ = now;  // kein Nachholen
            stageCount_.fill(0);
            tick.fill(true);
        } else if (dt >= 150) {
            tick[3] = stageCount_[3] < 4;
        } else if (dt >= 134) {
            tick[2] = stageCount_[2] < 3;
        } else if (dt >= 100) {
            if (stageCount_[1] < 2) tick[1] = tick[3] = true;  // Stufe 3 hängt an Zähler 1
        } else if (dt >= 67) {
            tick[2] = stageCount_[2] < 2;
        } else if (dt >= 50) {
            tick[3] = stageCount_[3] < 2;
        }
        for (int i = 0; i < 4; ++i) {
            if (tick[static_cast<std::size_t>(i)]) ++stageCount_[static_cast<std::size_t>(i)];
        }
    }

    if (modal_) {
        // Vollbildobjekt verdrängt alle übrigen Updates (auch Figuren und Ebenen); Update liefert
        // bei CAnimLayer immer 1, das Löschen bei 0 (danach Nullzeiger-Zugriff) tritt nie ein.
        if (modal_->dirty || tick[static_cast<std::size_t>(stageOf(modal_->fps))]) {
            modal_->update(g_);
            if (modal_->dirty) g_.sceneChanged = true;
        }
        if (bar_ && (bar_->needsRedraw() || tick[1])) bar_->tick();  // 0x00441CE0
        return;
    }
    if (background_ && background_->isAnim) {
        if (background_->dirty || tick[static_cast<std::size_t>(stageOf(background_->fps))]) {
            background_->update(g_);
            Rect all{0, 0, 640, 360};  // CRectList_Add(NULL)
            listA_.add(all);
            if (background_->dirty) g_.sceneChanged = true;
        }
    }
    // Figuren +0x5E0/+0x5E4 folgen mit CActor.

    // Schleife bricht beim ersten leeren Platz ab (0x0044B544)
    for (auto& slot : layers_) {
        Layer* L = slot.get();
        if (!L) break;
        if (L->prevZ != -1.0) {
            moveZ(static_cast<unsigned>(ftol(L->prevZ)), static_cast<unsigned>(ftol(L->z)));
            L->prevZ = -1.0;
        }
        bool run;
        if (L->fps == 0) run = L->tweenSteps != 0 && tick[1];
        else run = L->dirty || tick[static_cast<std::size_t>(stageOf(L->fps))];
        if (run) {
            const auto before = static_cast<unsigned>(ftol(L->z));
            L->update(g_);
            const auto after = static_cast<unsigned>(ftol(L->z));
            if (before != after) moveZ(before, after);
        }
        if (L->dirty) g_.sceneChanged = true;
    }
    if (bar_ && (bar_->needsRedraw() || tick[1])) bar_->tick();  // 0x00441CE0
}

bool Scene::load(std::string_view name, unsigned loadMask) {
    const auto tryDir = [&](const DataPaths::Dir& d) -> std::optional<Bytes> {
        if (!d.fs) return std::nullopt;
        return d.fs->read(d.prefix + std::string(name) + ".szn");
    };
    auto data = tryDir(paths_.sceneDir());
    if (!data) data = tryDir(paths_.sceneCdDir());
    // Fehlt die Datei, liest GetPrivateProfileString nur Standardwerte: leere Szene, Rückgabe 1
    const IniFile ini = data ? IniFile(*data) : IniFile();
    return loadSzn(ini, loadMask);
}

bool Scene::loadSzn(const IniFile& szn, unsigned loadMask) {
    const SceneDesc d = readSceneDesc(szn, loadMask);
    if (d.background) {
        createBackground(paths_.findPath(d.background->filename));
        if (background_) background_->name = d.background->section;
        if (d.background->zBuf) loadMap(paths_.findPath(*d.background->zBuf), depth_);
        if (d.background->wBuf) loadMap(paths_.findPath(*d.background->wBuf), path_);
        if (background_) background_->fps = d.background->fps;
    }
    for (const SznLayer& l : d.layers) {
        // Ohne Treffer nutzt das Original den zuletzt probierten Pfad; CLayer_Load scheitert dann
        Layer* L = createLayer(paths_.findPath(l.filename), l.fps != 0);
        if (!L) continue;
        if (l.dirtyAlpha != 0) {
            L->dirtyAlpha = true;
            if (L->hasImage) {
                L->tcc.dirtyAlpha = true;
                L->tcc.calcAlphaBounds(L->planes());  // vtable +0x24
            }
        }
        L->setPos(l.x, yOffset_ + l.y);
        L->fps = l.fps;
        L->z = l.z;
        L->zBuffered = l.zBuffered != 0;
        L->name = l.section;
        const int zi = ftol(L->z);
        if (zMax_ < zi) zMax_ = zi;
        if (zi < zMin_) zMin_ = zi;
        if (zi >= 0 && zi < 256) ++zCount_[static_cast<std::size_t>(zi)];  // UNVERIFIED: Z außerhalb 0…255
        if (l.zoomVal != 100) L->setZoom(l.zoomVal);
    }
    for (const SznArea& a : d.areas) {
        Rect r{a.x1, a.y1, a.x2, a.y2};
        addArea(&a.section, &r, a.cursorType);
    }
    // Figuren folgen mit CActor.
    loaded_ = true;
    g_.sceneChanged = true;
    return true;
}

bool Scene::createBackground(const std::optional<FoundFile>& file) {
    background_.reset();
    if (!file) return false;
    // CLayer_Load setzt bei AVI +0x1BC → Ebene verwerfen, als CAnimLayer neu laden (VIDEO.md)
    std::unique_ptr<Layer> bg;
    if (layerKindFromName(file->path) == LayerKind::Avi) bg = std::make_unique<AnimLayer>(g_);
    else bg = std::make_unique<Layer>();
    std::string err;
    if (!bg->load(*file, colorKey_, &err)) return false;
    bg->z = 255.0;
    bg->renderToSurface();  // 0x00449FF0
    bg->setPos(0, yOffset_);
    background_ = std::move(bg);
    loaded_ = true;
    g_.sceneChanged = true;
    Rect r = background_->getRect();
    clipToScene(r);
    listA_.add(r);
    background_->name = fileStem(file->path);  // 0x00447F00, danach vom Lader überschrieben
    return true;
}

Layer* Scene::createLayer(const std::optional<FoundFile>& file, bool animated) {
    int slot = 0;
    while (layers_[static_cast<std::size_t>(slot)]) {
        if (++slot >= kMaxLayers) return nullptr;
    }
    std::unique_ptr<Layer> L;
    if (animated) L = std::make_unique<AnimLayer>(g_);
    else L = std::make_unique<Layer>();
    std::string err;
    if (!file || !L->load(*file, colorKey_, &err)) {
        if (animated && file && layerKindFromName(file->path) != LayerKind::A16) ++unsupported_;  // ohne Dekoder
        return nullptr;
    }
    L->renderToSurface();  // 0x0044AC05 / 0x0044AC17
    L->setPos(0, yOffset_);
    lastLayer_ = slot;
    g_.sceneChanged = true;
    // Ebenen werden nicht aktiviert (Bedingung „Dateiname == NULL“, 0x0044AC48); das tun die
    // Schauplätze.
    layers_[static_cast<std::size_t>(slot)] = std::move(L);
    Layer* layer = layers_[static_cast<std::size_t>(slot)].get();
    const std::string stem = fileStem(layer->path);
    if (Area* a = addArea(&stem, nullptr, 0)) a->attach(*layer);
    layer->name = stem;
    if (!barShown_) layer->update(g_);  // vtable +0x18, wenn +0x5BC == 0
    return layer;
}

void Scene::loadMap(const std::optional<FoundFile>& file, std::optional<ZMap>& out) {
    out.reset();
    if (!file) return;
    const auto data = file->read();
    if (!data) return;
    ZMap m;
    std::string err;
    if (loadZMap(*data, m, &err)) out = std::move(m);
}

Layer* Scene::getObject(std::string_view name) {
    for (auto& L : layers_) {
        if (!L) return nullptr;
        if (equalsNoCase(name, L->name)) return L.get();
    }
    return nullptr;
}

std::unique_ptr<AnimLayer> Scene::loadAnim(std::string_view name) {
    const auto file = paths_.findFile(name, "AVI");
    auto L = std::make_unique<AnimLayer>(g_);
    L->direct = true;
    // UNVERIFIED: ohne Treffer lädt das Original den zuletzt probierten Pfad – scheitert ebenso
    if (!file || !L->load(*file, colorKey_, nullptr)) return nullptr;
    L->alpha = true;  // +0x198
    L->setPos(0, yOffset_);
    L->name = fileStem(L->path);
    return L;
}

AnimLayer* Scene::setModal(AnimLayer* obj) {
    AnimLayer* old = modal_;
    modal_ = obj;
    if (!obj) {
        if (old) old->stop(0);  // UNVERIFIED: Original ruft Slot 9 ohne Nullprüfung
        invalidateAll();
    }
    return old;
}

void Scene::setClickEvent(int handler) {
    sceneEvent_.setHandler(handler);
    if (handler == 0) {
        sceneEvent_.enabled = false;
        sceneEvent_.pending = false;
    } else {
        sceneEvent_.enabled = true;
    }
}

void Scene::invalidateAll() {
    listB_.addAll();
    listA_.addAll();
    g_.sceneChanged = true;
}

bool Scene::compose(bool force) {
    if (!loaded_) return false;
    if (!g_.sceneChanged && !force) return false;
    // 2. Vollbildobjekt +0x5DC: zeichnen (flag 0 → bei AVI mit +0x30 direkt auf den Bildschirm)
    // oder, wenn beendet und auszublenden und ohne Ende-Ereignis, Stop(0) und abmelden
    if (modal_) {
        if (!(modal_->ended && modal_->hideAtEnd)) {
            if (modal_->dirty && modal_->active) modal_->draw(surface_, 0, nullptr);  // 0x00447150
        } else if (!modal_->endEvent) {
            modal_->stop(0);
            modal_ = nullptr;
            invalidateAll();
        }
    }
    if (modal_ && modal_->active) {
        g_.sceneChanged = false;  // Schritt 12: Rückgabe 0
        return false;
    }
    // Figuren (Schritte 4, 8, 11) folgen mit CActor.
    if (!background_) return false;
    // 5. Bereichstext unter der Maus: Rechteck in A und B (Editor-Vollbild +0x5C8 folgt)
    if (textShown()) {
        Rect t{textX_ - 160, textY_ - 90, textX_ + 160, textY_ - 50};
        if (clipToScene(t)) {
            listA_.add(t);
            listB_.add(t);
        }
    }

    // 6. Rechteck aus A clippen (in A selbst) und nach B
    if (listA_.first()) {
        Rect a = listA_.rect();
        clipToScene(a);
        listA_.clear();
        listA_.add(a);
        listB_.add(a);
    }
    // 7. dirty Ebenen
    for (auto& L : layers_) {
        if (!L) break;
        if (!L->dirty) continue;
        Rect r = L->getRect();
        clipToScene(r);
        listB_.add(r);
        if (!isRectEmpty(L->old)) {
            Rect t = L->old;
            if (L->z != 255.0) listA_.add(t);
            listB_.add(t);
        }
    }
    // 8. Ausbreitung über Überlappung mit B, danach von vorn
    for (int i = 0; i < kMaxLayers; ++i) {
        Layer* L = layers_[static_cast<std::size_t>(i)].get();
        if (!L) break;
        if (L->dirty || !L->active) continue;
        Rect r = L->getRect();
        clipToScene(r);
        if (!listB_.intersects(r)) continue;
        L->dirty = true;
        listB_.add(r);
        if (!isRectEmpty(L->old)) {
            Rect t = L->old;
            if (L->z != 255.0) listA_.add(t);
            listB_.add(t);
        }
        i = -1;
    }
    // 9. Hintergrund für A, flag 0
    if (auto a = listA_.first()) {
        Rect r = *a;
        background_->draw(surface_, 0, r, nullptr);
        listB_.add(r);
    }
    listA_.clear();
    // 10. Ebenen mit Z = 255.0, flag 0, ohne Tiefe
    for (auto& L : layers_) {
        if (!L) break;
        if (L->z == 255.0 && L->dirty) L->draw(surface_, 0, nullptr);
    }
    // 11. Z-Stufen von hinten nach vorn
    const ZMap* zmap = depth_ ? &*depth_ : nullptr;
    for (int lvl = zMax_ + 1; zMin_ <= lvl; --lvl) {
        if (lvl == 0) break;
        if (zCount(lvl) != 0) {  // zCount[256] liest im Original +0x500 (Klick-X), ohne Wirkung
            for (auto& L : layers_) {
                if (!L) break;
                if (ftol(L->z) == lvl && L->dirty && L->z != 255.0) L->draw(surface_, 1, zmap);
            }
        }
    }
    // 12. Bereichstext (CArea_DrawText über GetDC); Editor-Rahmen und Slot +0x10 der letzten
    // Ebene (Debug-Text) entfallen
    if (textShown() && g_.drawAreaText) g_.drawAreaText(surface_, *lastHit_, textX_, textY_ - 60);
    g_.sceneChanged = false;
    return true;
}

}  // namespace rah

namespace rah {

Area* Scene::addArea(const std::string* name, Rect* rect, int cursor) {
    if (rect) {
        if (rect->right < rect->left) std::swap(rect->left, rect->right);
        if (rect->bottom < rect->top) std::swap(rect->top, rect->bottom);
    }
    for (auto& slot : areas_) {
        if (slot) continue;
        slot = std::make_unique<Area>();
        Area& a = *slot;
        a.grid = &g_.hitGrid;
        // Rechteck vor dem Namen (0x0044AE91): beim Markieren ist der Name noch leer, die
        // „Rodrigo“-Ausnahme greift hier also nie
        if (rect) a.setRect(*rect, 0);
        if (name) {
            a.setName(*name);
            a.setCursor(cursor);
            if (location_ != 0) {
                char loc[16];
                std::snprintf(loc, sizeof loc, "Loc%02d", location_);
                if (auto f = paths_.findFile(loc, "GER")) {
                    if (auto data = f->read()) {
                        const IniFile ger(*data);
                        const std::string t = ger.getString("AREA_TEXT", *name, "NULL", 100);
                        if (t != "NULL") a.setText(t);
                    }
                }
            }
        }
        a.active = false;
        return &a;
    }
    return nullptr;
}

Area* Scene::getArea(std::string_view name) {
    for (auto& a : areas_) {
        if (!a) break;
        if (equalsNoCase(name, a->name)) return a.get();
    }
    return nullptr;  // Original: MessageBox(name)
}

Area* Scene::hitTest(int x, int y) {
    auto hits = [x, y](Area* a) {
        return a && a->active && ptInRect(a->rect, x, y) && (!a->mask || !a->mask->isTransparentAt(x, y));
    };
    if (y >= 360 && y <= 420) {
        if (barShown_ && bar_ && hits(&bar_->area())) return &bar_->area();
        return nullptr;
    }
    if (!(editor_ || g_.hitGrid.test(x, y))) return nullptr;
    for (unsigned z = static_cast<unsigned>(zMin_); z <= static_cast<unsigned>(zMax_); ++z) {
        if (zCount(static_cast<int>(z)) == 0 || z == 0xFF) continue;
        for (auto& slot : layers_) {
            Layer* L = slot.get();
            if (!L) break;
            if (L->active && static_cast<unsigned>(ftol(L->z)) == z && hits(L->area)) return L->area;
        }
    }
    for (auto& slot : layers_) {
        Layer* L = slot.get();
        if (!L) break;
        if (L->active && L->z == 255.0 && hits(L->area)) return L->area;
    }
    for (auto& a : areas_) {
        if (!a) return nullptr;
        if (!a->mask && hits(a.get())) return a.get();
    }
    return nullptr;
}

void Scene::onClick(int x, int y) {
    clickX_ = x;
    clickY_ = y;
    // +0x5BC == 0: Figur läuft zum Klick (0x0044CB60) – folgt mit CActor.
    if (help_ && help_->isOpen()) {
        help_->close();
        return;
    }
    if (!clicksEnabled) return;
    if (!clicksDuringFull && modal_ && modal_->active && modal_->running) return;
    if (globalClick && !editor_ && g_.clickEvent.enabled) {
        markEvent(g_, g_.clickEvent, 0x16);
        return;
    }
    if (y < 60 && help_ && !editor_ && static_cast<unsigned>(x) >= 0x245) {
        help_->onIconClick();  // 0x004416E0
        return;
    }
    if (y < 60 || y >= 480) {
        if (!g_.textVisible) hideToggle();
        return;
    }
    lastHit_ = hitTest(x, y - 60);
    if (lastHit_) markEvent(g_, *lastHit_->event, 0x0B);
    if (bar_ && lastHit_ == &bar_->area()) {
        bar_->leave(false);
        return;
    }
    if (lastHit_) return;  // nicht der Bereich der Figur (Figuren folgen)
    // Figur aktiv → CActor_WalkToClick folgt.
    markEvent(g_, sceneEvent_, 0x16);
}

}  // namespace rah

namespace rah {

int Scene::onMouseMove(int x, int y) {
    clickX_ = x;
    clickY_ = y;
    int cursor = 0;
    if ((help_ && help_->isOpen()) || (modal_ && modal_->active && modal_->running)) return cursor;
    if (y < 60 || y > 479) {
        if (!g_.textVisible) hideToggle();
        return cursor;
    }
    Area* before = lastHit_;
    lastHit_ = hitTest(x, y - 60);
    if (lastHit_) cursor = lastHit_->cursor;
    if (!editor_ && bar_) {
        if (lastHit_ == &bar_->area()) {
            markEvent(g_, *lastHit_->event, 0x0D);
            lastHit_ = nullptr;
        } else {
            bar_->leave(true);
        }
    }
    if (lastHit_ != before || !g_.textVisible) {
        const bool redraw = showTexts && g_.textVisible && before && before != lastHit_;
        g_.textVisible = false;
        hideToggle();
        if (lastHit_) {
            cursor = lastHit_->cursor;
            if (showTexts) {
                textX_ = clickX_;
                textY_ = static_cast<unsigned>(clickY_) < 390u ? clickY_ + 25 : clickY_ - 20;
                if (g_.toggleEvent) {
                    g_.toggleEvent->enabled = true;
                    setEventTimer(g_, *g_.toggleEvent, g_.mouseDelay, 0);
                }
            }
        } else {
            cursor = 0;
        }
        if (redraw) invalidateAll();
    }
    return cursor;
}

}  // namespace rah
