// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/ftol.hpp"
#include "engine/scene.hpp"

#include "engine/help.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "core/strings.hpp"

namespace rah {

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
    // Figuren: nur das Laufereignis +0x1C8, sonst das abgebrochene +0x16F0 (Ebenen-Ereignisse
    // der Figur werden hier nicht ausgelöst)
    for (auto& a : actors_) {
        if (!a) continue;
        Event* e = a->walkEvent ? a->walkEvent.get() : a->oldWalkEvent.get();
        if (e) fireEvent(g_, *e, e->type);
    }
    for (Layer* L : layers_) {
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
    for (auto& a : actors_) {
        if (!a) continue;
        if (a->alwaysUpdate) {
            a->dirty = true;
            a->update(g_);
        } else {
            if (!a->dirty && !tick[static_cast<std::size_t>(stageOf(a->fps))]) continue;
            a->update(g_);
            if (!a->dirty) continue;
        }
        g_.sceneChanged = true;
    }

    // Schleife bricht beim ersten leeren Platz ab (0x0044B544)
    for (auto& slot : layers_) {
        Layer* L = slot;
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
            // +0x1BC: CAnimLayer_Update direkt (0x0044B6A6), sonst vtable +0x18 – eine Figur
            // in der Ebenenliste läuft so als reine Animation (Loc04: Held als freie Ebene)
            if (auto* an = L->isAnim ? dynamic_cast<AnimLayer*>(L) : nullptr) an->AnimLayer::update(g_);
            else L->update(g_);
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
    for (int n = 0; n < 2; ++n) {
        const auto& oc = d.characters[static_cast<std::size_t>(n)];
        if (!oc) continue;
        const SznCharacter& c = *oc;
        auto& slot = ownedActors_[static_cast<std::size_t>(n)];
        if (slot && c.type != slot->type) {  // anderer Typ → neu anlegen
            // Abweichung: Original lässt den Bereich mit hängendem Masken-Zeiger stehen
            if (slot->area) slot->area->mask = nullptr;
            slot.reset();
            actors_[static_cast<std::size_t>(n)] = nullptr;
        }
        const bool fresh = !slot;
        Actor* a = createActor(n, paths_.findPath(c.filename), fresh);
        if (!a) continue;  // UNVERIFIED: Original arbeitet mit Zeiger 0 weiter
        if (c.sfxLeft) {
            const auto r = c.sfxRight ? paths_.findPath(*c.sfxRight) : std::nullopt;
            const auto al = c.sfxAlt ? paths_.findPath(*c.sfxAlt) : std::nullopt;
            setStepSounds(a, paths_.findPath(*c.sfxLeft), r, al);
        }
        a->setType(c.type);
        a->perCor = c.perCor;
        a->setColor(c.redAdd, c.greenAdd, c.blueAdd);
        for (std::size_t i = 0; i < 10; ++i) {
            const SznColorAdd& t = c.colorAdd[i];
            a->tintR[i] = t.red;
            a->tintG[i] = t.green;
            a->tintB[i] = t.blue;
            a->tintX[i] = t.x;
            a->tintY[i] = t.y;
            a->tintRange[i] = t.range;
            if (t.range != 0) a->tintZones = true;
        }
        a->zpos = c.zpos;
        a->zBuffered = c.zBuffered != 0;
        if (path_) a->zScale = static_cast<double>(a->zpos - path_->vMin) * path_->k + path_->sMin;
        a->zoomVal = c.zoomVal;
        a->name = c.section;
        a->setPos(c.x, c.y);  // vtable +0x14 → CActor_SetPosition(x, y, 0)
        a->stop(0);           // vtable +0x24
        a->setDirection(c.direction);
    }
    loaded_ = true;
    g_.sceneChanged = true;
    return true;
}

bool Scene::createBackground(const std::optional<FoundFile>& file) {
    background_ = nullptr;  // UNVERIFIED: ob 0x00449E70 den alten löscht – Port: bleibt im Besitz
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
    ownedBackgrounds_.push_back(std::move(bg));
    background_ = ownedBackgrounds_.back().get();
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
    lastCreated_ = L.get();
    // Ebenen werden nicht aktiviert (Bedingung „Dateiname == NULL“, 0x0044AC48); das tun die
    // Schauplätze.
    ownedLayers_.push_back(std::move(L));
    Layer* layer = ownedLayers_.back().get();
    layers_[static_cast<std::size_t>(slot)] = layer;
    const std::string stem = fileStem(layer->path);
    if (Area* a = addArea(&stem, nullptr, 0)) a->attach(*layer);
    layer->name = stem;
    if (!barShown_) layer->update(g_);  // vtable +0x18, wenn +0x5BC == 0
    return layer;
}

Actor* Scene::createActor(int slotNr, const std::optional<FoundFile>& file, bool fresh) {
    auto& slot = ownedActors_[static_cast<std::size_t>(slotNr)];
    if (!fresh) {
        slot->reset();  // 0x00433B70
    } else {
        if (slot) {
            removeArea(slot->area);
            slot.reset();
            actors_[static_cast<std::size_t>(slotNr)] = nullptr;
        }
        auto a = std::make_unique<Actor>(g_);
        std::string err;
        if (!file || !a->load(*file, colorKey_, &err)) return nullptr;
        a->renderToSurface();  // 0x00446C90
        slot = std::move(a);
    }
    Actor* a = slot.get();
    actors_[static_cast<std::size_t>(slotNr)] = a;
    if (path_) a->setPathMap(path_);
    a->setPos(0, yOffset_);  // vtable +0x14
    g_.sceneChanged = true;
    lastCreated_ = a;
    lastLayer_ = 40;
    const std::string name = slotNr == 0 ? "Rodrigo" : "Girlfriend";
    if (Area* ar = addArea(&name, nullptr, 0)) ar->attach(*a);
    a->name = fileStem(a->path);
    return a;
}

bool Scene::setStepSounds(Actor* a, const std::optional<FoundFile>& left, const std::optional<FoundFile>& right,
                          const std::optional<FoundFile>& alt) {
    if (!a || !g_.audio || !left) return false;
    a->setStepSounds(g_.audio, &*left, right ? &*right : nullptr, alt ? &*alt : nullptr);
    return true;
}

bool Scene::removeArea(Area* a) {
    for (std::size_t i = 0; i < areas_.size(); ++i) {
        if (areas_[i].get() != a || !a) continue;
        removedAreas_.push_back(std::move(areas_[i]));
        for (std::size_t j = i; j + 1 < areas_.size(); ++j) areas_[j] = std::move(areas_[j + 1]);
        areas_.back().reset();
        if (a->mask) a->mask->area = nullptr;  // CArea_DetachLayer 0x00436170
        a->mask = nullptr;
        return true;
    }
    return false;
}

void Scene::loadMap(const std::optional<FoundFile>& file, ZMap*& out) {
    out = nullptr;
    if (!file) return;
    const auto data = file->read();
    if (!data) return;
    ZMap m;
    std::string err;
    if (loadZMap(*data, m, &err)) {
        ownedMaps_.push_back(std::make_unique<ZMap>(std::move(m)));
        out = ownedMaps_.back().get();
    }
}

Layer* Scene::getObject(std::string_view name) {
    for (auto& L : layers_) {
        if (!L) return nullptr;
        if (equalsNoCase(name, L->name)) return L;
    }
    return nullptr;
}

void Scene::reset() {
    loaded_ = false;
    // +0x2B0 und Hintergrund +0x5C0 setzt das Original nur auf 0 (ohne Löschen)
    background_ = nullptr;
    layers_.fill(nullptr);
    ownedLayers_.clear();
    ownedBackgrounds_.clear();
    for (auto& a : areas_) a.reset();
    actors_.fill(nullptr);
    for (auto& a : ownedActors_) a.reset();
    g_.hitGrid.clear();
    lastLayer_ = -1;   // +0x5D8 = 0, +0x2B8 = 0 (Port: kein Platz)
    lastCreated_ = nullptr;
    depth_ = nullptr;  // +0x2BC = 0 (ohne Löschen)
    path_ = nullptr;   // +0x2C0 = 0 (ohne Löschen)
    ownedMaps_.clear();
    modal_ = nullptr;  // +0x5DC = 0 ohne Löschen
    g_.sceneChanged = false;
    zMax_ = 1;         // +0x2F8 = 1
    zMin_ = 0xFF;      // +0x2FC; zCount_ +0x300 bleibt stehen (erst CSzene_Clear leert es)
    // +0xC08 = 1, +0x194…+0x1A4 = 0: Bedeutung offen, im Port nicht vorhanden
    listB_.clear();
    listA_.clear();
    g_.textVisible = false;  // 0x00481F24
    if (g_.toggleEvent) {    // *g_ToggleEvent = 0, [1] = 0
        g_.toggleEvent->enabled = false;
        g_.toggleEvent->pending = false;
    }
    lastHit_ = nullptr;
    disableInput();
    setFlag510();
    setClickEvent(0);
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
    // 4. Figuren klassifizieren: 1 = hinter Zmax, 2 = im Bereich, 3 = vor Zmin (vorzeichenlos)
    std::array<int, 2> cls{};
    std::array<unsigned, 2> zc{};
    for (std::size_t i = 0; i < 2; ++i) {
        Actor* a = actors_[i];
        if (!a || (!a->dirty && !a->active)) continue;
        zc[i] = static_cast<unsigned>(ftol(a->z));
        if (zc[i] < static_cast<unsigned>(zMin_)) cls[i] = static_cast<unsigned>(zMax_) < zc[i] ? 1 : 3;
        else cls[i] = static_cast<unsigned>(zMax_) < zc[i] ? 1 : 2;
    }
    const auto actorRect = [&](std::size_t i) {
        Rect r = actors_[i]->getRect();
        clipToScene(r);
        return r;
    };
    const auto addOld = [&](Layer& L) {
        if (isRectEmpty(L.old)) return;
        Rect t = L.old;
        listA_.add(t);
        listB_.add(t);
    };
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
    // 8. Figuren (zweiter Durchgang nur, wenn Figur 1 dirty ist und Figur 0 noch nicht)
    for (int pass = 0; pass < 2; ++pass) {
        for (std::size_t i = 0; i < 2; ++i) {
            if (cls[i] == 0) continue;
            Actor& a = *actors_[i];
            const Rect r = actorRect(i);
            if (!a.dirty && a.active && listB_.intersects(r)) a.dirty = true;
            if (!a.dirty) {
                if (i == 1 && pass == 0) pass = 2;
                continue;
            }
            Rect rr = r;
            listB_.add(rr);
            addOld(a);
            if (i == 1 && actors_[0] && actors_[0]->dirty) pass = 2;
        }
    }
    // Ausbreitung über Überlappung mit B (Figuren erneut prüfen), danach von vorn
    for (int i = 0; i < kMaxLayers; ++i) {
        Layer* L = layers_[static_cast<std::size_t>(i)];
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
        for (std::size_t k = 0; k < 2; ++k) {
            if (cls[k] == 0) continue;
            Actor& a = *actors_[k];
            if (a.dirty || !a.active) continue;
            Rect ar = actorRect(k);
            if (!listB_.intersects(ar)) continue;
            a.dirty = true;
            listB_.add(ar);
            addOld(a);
            k = static_cast<std::size_t>(-1);  // Figuren von vorn
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
    for (std::size_t i = 0; i < 2; ++i)
        if (cls[i] != 0 && !actors_[i]->dirty) cls[i] = 0;  // nicht dirty → nicht zeichnen
    // 10. Ebenen mit Z = 255.0, flag 0, ohne Tiefe
    for (auto& L : layers_) {
        if (!L) break;
        if (L->z == 255.0 && L->dirty) L->draw(surface_, 0, nullptr);
    }
    // 11. Z-Stufen von hinten nach vorn; Figuren-Z vorübergehend mitzählen
    const ZMap* zmap = depth_;
    const auto zslot = [this](unsigned z) -> short* { return z < 256 ? &zCount_[z] : nullptr; };  // UNVERIFIED: Z > 255
    for (std::size_t i = 0; i < 2; ++i)
        if (short* c = zslot(zc[i])) ++*c;
    // Reihenfolge: z0 ≤ z1 → erst Figur 1, dann Figur 0
    const std::array<std::size_t, 2> order = zc[0] <= zc[1] ? std::array<std::size_t, 2>{1, 0} : std::array<std::size_t, 2>{0, 1};
    for (int lvl = zMax_ + 1; zMin_ <= lvl; --lvl) {
        if (lvl == 0) break;
        if (zCount(lvl) != 0) {  // zCount[256] liest im Original +0x500 (Klick-X), ohne Wirkung
            for (auto& L : layers_) {
                if (!L) break;
                if (ftol(L->z) == lvl && L->dirty && L->z != 255.0) L->draw(surface_, 1, zmap);
            }
        }
        for (const std::size_t i : order) {
            const int c = cls[i];
            if (c == 1 || (c == 2 && zc[i] == static_cast<unsigned>(lvl)) || (c == 3 && lvl == zMin_)) {
                actors_[i]->drawActor(surface_, 1, zmap);
                cls[i] = 0;
            }
        }
    }
    for (const std::size_t i : order) {
        if (short* c = zslot(zc[i])) --*c;
        if (cls[i] != 0) actors_[i]->drawActor(surface_, 1, zmap);
    }
    // 12. Bereichstext (CArea_DrawText über GetDC); Editor-Rahmen und Slot +0x10 der letzten
    // Ebene (Debug-Text) entfallen
    // (Editor-Vollbild +0x5C8 entfällt); ohne Text in der Menü-Szene das Menü-Zeichnen
    if (textShown()) {
        if (g_.drawAreaText) g_.drawAreaText(surface_, *lastHit_, textX_, textY_ - 60);
    } else if (editor_ && menuDraw_) {
        menuDraw_();
    }
    g_.sceneChanged = false;
    return true;
}

void Scene::onKeyDown(int vk) {
    if (help_ && help_->isOpen()) {
        help_->close();  // 0x004416C0
        return;
    }
    if (!clicksEnabled) {  // +0x514
        dbg(g_, "Taste %02X verworfen: Eingabe gesperrt", vk);
        return;
    }
    if (modal_ && modal_->active && modal_->running) {  // +0x5DC, +0x1A4, +0x1A8
        dbg(g_, "Taste %02X verworfen: Vollbildobjekt läuft", vk);
        return;
    }
    if (vk == 0x1B || (vk > 0x6F && vk < 0x7C)) {
        // PostMessage(+0x2CC, WM_COMMAND, 40079 bzw. 40092)
        if (g_.postCommand) g_.postCommand(g_.gameMenuOpen ? 40092 : 40079);
        return;
    }
    // else if (+0x5BC == 0) 0x0044CF00(vk) – Tastatur ohne laufendes Spiel (Editor) folgt
}

bool Scene::snapshotBackground() {
    Layer* bg = background_;
    if (!bg || bg->isAnim || !bg->hasSurface) return false;  // +0x1BC, +0x08
    // vtable +0x14 der Oberfläche +0x08: Blt(&{0,0,640,360}, Szenenoberfläche, &{0,0,640,360}, DDBLT_WAIT)
    Image565& dst = bg->bmp;
    const int w = std::min({640, dst.width, surface_.width});
    const int h = std::min({360, dst.height, surface_.height});
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
            dst.pixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(dst.width) + static_cast<std::size_t>(x)] =
                surface_.row(y)[x];
    return true;
}

void Scene::clearBackground() {
    Layer* bg = background_;
    if (!bg || !bg->hasSurface) return;
    std::fill(bg->bmp.pixels.begin(), bg->bmp.pixels.end(), std::uint16_t{0});
}

void Scene::frameRect(const Rect& r, std::uint32_t colorref) {
    Rect c = r;
    clipToScene(c);
    listB_.add(c);
    listA_.add(c);
    // FrameRect auf das ungeclippte Rechteck (GetDC der Szenenoberfläche, Clipping durch die Oberfläche)
    const std::uint16_t col = static_cast<std::uint16_t>(((colorref & 0xF8) << 8) | (((colorref >> 8) & 0xFC) << 3) |
                                                         ((colorref >> 16) & 0xF8) >> 3);
    const auto put = [&](int x, int y) {
        if (x >= 0 && y >= 0 && x < surface_.width && y < surface_.height) surface_.row(y)[x] = col;
    };
    if (r.right <= r.left || r.bottom <= r.top) return;
    for (int x = r.left; x < r.right; ++x) put(x, r.top), put(x, r.bottom - 1);
    for (int y = r.top; y < r.bottom; ++y) put(r.left, y), put(r.right - 1, y);
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
            Layer* L = slot;
            if (!L) break;
            if (L->active && static_cast<unsigned>(ftol(L->z)) == z && hits(L->area)) return L->area;
        }
    }
    for (auto& slot : layers_) {
        Layer* L = slot;
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
    if (!barShown_) clickWalk(x, y);  // +0x5BC == 0
    if (help_ && help_->isOpen()) {
        help_->close();
        return;
    }
    if (!clicksEnabled) {
        dbg(g_, "Klick %d,%d ignoriert: Eingabe gesperrt", x, y);
        return;
    }
    if (!clicksDuringFull && modal_ && modal_->active && modal_->running) {
        dbg(g_, "Klick %d,%d ignoriert: Vollbild-Ebene %s läuft", x, y, modal_->name.c_str());
        return;
    }
    if (globalClick && !editor_ && g_.clickEvent.enabled) {
        dbg(g_, "Klick %d,%d → globales Klick-Ereignis", x, y);
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
    if (g_.debug) {
        Actor* h = actors_[0];
        dbg(g_, "Klick %d,%d (Szene %d,%d) → %s%s | Held %s bei %d,%d", x, y, x, y - 60,
            lastHit_ ? lastHit_->name.c_str() : "kein Bereich",
            lastHit_ && !lastHit_->event->hasHandler() ? " (ohne Ereignis)" : "",
            h ? (h->active ? "sichtbar" : "unsichtbar") : "fehlt", h ? h->footX : 0, h ? h->footY : 0);
    }
    if (lastHit_) markEvent(g_, *lastHit_->event, 0x0B);
    if (bar_ && lastHit_ == &bar_->area()) {
        bar_->leave(false);
        return;
    }
    Actor* hero = actors_[0];
    if (lastHit_ && (!hero || lastHit_ != hero->area)) return;  // nicht der Bereich der Figur
    if (hero && hero->active) {
        hero->walkToClick(x, y - 60);
        return;
    }
    markEvent(g_, sceneEvent_, 0x16);
}

void Scene::clickWalk(int x, int y) {
    Actor* second = actors_[1];
    if (lastCreated_ == second) {
        if (second) second->walkToClick(x, y - 60);
    } else if (actors_[0]) {
        actors_[0]->walkToClick(x, y - 60);
    }
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

void Scene::disableAll() {
    disableInput();
    if (background_) background_->disable();
    for (auto& a : actors_)
        if (a) a->hide(clicksEnabled);
    if (modal_) modal_->disable();
    for (auto& l : layers_) {
        if (!l) break;
        l->disable();
    }
    for (auto& a : areas_) {
        if (!a) return;
        a->active = false;
    }
}

}  // namespace rah

namespace rah {

Layer* Scene::setBackground(Layer* bg) {
    Layer* old = background_;
    background_ = bg;
    listA_.addAll();  // CRectList_Add(+0x2F4, NULL): ganze Fläche
    if (background_) background_->enable();
    return old;
}

void Scene::setHero(Actor* a) {
    Actor* old = actors_[0];
    actors_[0] = a;
    if (path_ && a) a->setPathMap(path_);
    if (old && old->area) removeArea(old->area);
    if (a) {
        // Original: Name „Girlfriend“ nur wenn a ≠ +0x5E0 – nach der Zuweisung nie
        const std::string name = "Rodrigo";
        if (Area* ar = addArea(&name, nullptr, 0)) ar->attach(*a);
        a->running = true;  // +0x1A8 = 1
    }
}

bool Scene::insertLayer(Layer* l) {
    for (auto& slot : layers_) {
        if (slot) continue;
        slot = l;
        const int z = static_cast<int>(l->z);  // __ftol
        if (z >= 0 && z < 256) ++zCount_[static_cast<std::size_t>(z)];  // UNVERIFIED: Z außerhalb 0…255
        if (Area* a = addArea(&l->name, nullptr, 0)) a->attach(*l);
        return true;
    }
    return false;
}

bool Scene::removeLayer(Layer* l) {
    for (std::size_t i = 0; i < layers_.size(); ++i) {
        if (layers_[i] != l) continue;
        const int z = static_cast<int>(l->z);
        if (z >= 0 && z < 256) --zCount_[static_cast<std::size_t>(z)];
        for (std::size_t j = i; j + 1 < layers_.size(); ++j) layers_[j] = layers_[j + 1];
        layers_.back() = nullptr;
        if (Area* a = l->area) {
            removeArea(a);
            // 0x00436010 + Freigabe: der Bereich wird gelöscht
            if (!removedAreas_.empty() && removedAreas_.back().get() == a) removedAreas_.pop_back();
        }
        return true;
    }
    return false;
}

}  // namespace rah
