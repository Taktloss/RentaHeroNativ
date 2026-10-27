// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/scene.hpp"

#include <cmath>

#include "core/strings.hpp"

namespace rah {

namespace {

inline int ftol(double v) {
    return static_cast<int>(std::trunc(v));
}

}  // namespace

Scene::Scene(const DataPaths& paths, EngineGlobals& globals, Surface565& target, std::uint16_t colorKey, int yOffset)
    : paths_(paths), g_(globals), surface_(target), colorKey_(colorKey), yOffset_(yOffset) {
    windowStart_ = g_.now();  // orig: 0x004485FD in CSzene_ctor
}

namespace {

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

    // Vollbildobjekt +0x5DC (verdrängt alle übrigen Updates) folgt mit dessen Port.
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
    // Inventarleiste +0x5B4 (FUN_00441CE0 bei +0x408 oder Stufe 1) folgt.
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
    // Bereiche (CSzene_AddArea) und Figuren folgen mit CArea/CActor.
    loaded_ = true;
    g_.sceneChanged = true;
    return true;
}

bool Scene::createBackground(const std::optional<FoundFile>& file) {
    background_.reset();
    if (!file) return false;
    auto bg = std::make_unique<Layer>();
    std::string err;
    if (!bg->load(*file, colorKey_, &err)) return false;  // AVI-Hintergrund: CAnimLayer, noch offen
    bg->z = 255.0;
    bg->setPos(0, yOffset_);
    background_ = std::move(bg);
    loaded_ = true;
    g_.sceneChanged = true;
    Rect r = background_->getRect();
    clipToScene(r);
    listA_.add(r);
    background_->name = file->path;  // 0x00447F00 (Name aus Pfad), danach vom Lader überschrieben
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
        if (animated && file && layerKindFromName(file->path) != LayerKind::A16) ++unsupported_;  // AVI folgt
        return nullptr;
    }
    L->setPos(0, yOffset_);
    lastLayer_ = slot;
    g_.sceneChanged = true;
    // Ebenen werden nicht aktiviert (Bedingung „Dateiname == NULL“, 0x0044AC48); das tun die
    // Schauplätze. Automatischer Bereich mit dem Ebenennamen folgt mit CArea.
    layers_[static_cast<std::size_t>(slot)] = std::move(L);
    return layers_[static_cast<std::size_t>(slot)].get();
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

void Scene::invalidateAll() {
    listB_.addAll();
    listA_.addAll();
    g_.sceneChanged = true;
}

bool Scene::compose(bool force) {
    if (!loaded_) return false;
    if (!g_.sceneChanged && !force) return false;
    // Vollbildobjekt +0x5DC (Schritt 2) und Figuren (Schritte 4, 8, 11) folgen mit ihren Klassen.
    if (!background_) return false;
    // Schritt 5: Bereichsname unter der Maus folgt mit CArea.

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
    // 12. Bereichsname/Editor-Rahmen folgen; Slot +0x10 der letzten Ebene ist ein Debug-Text.
    g_.sceneChanged = false;
    return true;
}

}  // namespace rah
