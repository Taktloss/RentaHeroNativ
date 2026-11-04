// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/actor.hpp"

#include <ctime>

#include "core/ftol.hpp"
#include "core/strings.hpp"

namespace rah {

namespace {

// orig: Rent-A-Hero.exe 0x00447CB0 – lineare Abbildung a…b → c…d
float mapRange(float v, float a, float b, float c, float d) {
    return (v - a) / (b - a) * (d - c) + c;
}

}  // namespace

Actor::Actor(EngineGlobals& g) : AnimLayer(g) {
    running = true;  // +0x1A8
    loop = true;     // +0x1AC
    z = 0.0;         // +0x20
    // +0x1A90: Richtung je Code (1 rechts, 2 links, 4 unten, 8 oben)
    dirOfCode = {0, 6, 2, 0, 0, 7, 1, 0, 4, 5, 3};
    firstFrame = {0x0C, 0x18, 0x24, 0x30, 0x3C, 0x48, 0x54, 0x00};
    lastFrame = {0x17, 0x23, 0x2F, 0x3B, 0x47, 0x53, 0x5F, 0x0B};
    standFrame = {0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x61};
    g.srand(static_cast<std::uint32_t>(std::time(nullptr)));  // srand(time(NULL))
    type = 0;
    setType(0);  // Pfad noch leer → kein Kopf
    perCor = 100;
    walkDone = true;
    head.reset();  // +0x1AF4 = 0 nach SetType
    headHeight = 0;  // +0x171C
}

Actor::~Actor() {
    release();
    head.reset();
}

bool Actor::load(const FoundFile& file, std::uint16_t colorKey, std::string* error) {
    fs_ = file.fs;
    return AnimLayer::load(file, colorKey, error);
}

void Actor::release() {
    pathFinder.reset();
    sfxLeft.reset();
    sfxRight.reset();
    sfxAlt.reset();
    oldWalkEvent.reset();
}

void Actor::reset() {
    release();  // vtable +0x04
    zScale = 0;
    z = 0;
    pathMap = nullptr;
    walkMode = 0;
    zoomVal = 0;
    zpos = 0;
    targetY = 0;
    targetX = 0;
    shownDir = -1;
    dir = 0;
    perCor = 100;
    walkDone = true;
    frame = 0;  // +0x2C0; +0x2C4 = 0 (Feld im Port nicht abgebildet, Bedeutung offen)
    imageSeek(standFrame[0]);  // Bild-vtable +0x1C
    AnimLayer::update(g_);
    talking = false;
    turning = false;
    setColor(0, 0, 0);
    tintR.fill(0);
    tintG.fill(0);
    tintB.fill(0);
    tintX.fill(0);
    tintY.fill(0);
    tintRange.fill(0);
    tintZones = false;
}

void Actor::setType(int t) {
    const auto fill = [&](int sx, int sy) {
        stepXDefault = sx;
        stepYDefault = sy;
        stepX.fill(sx);
        stepY.fill(sy);
    };
    // Bilder 36…47 und 84…95 (je 12): Typ 1 volle, sonst halbe Schrittweite
    static constexpr int kX1[12] = {18, 16, 18, 10, 8, 20, 20, 22, 22, 14, 6, 20};
    static constexpr int kX0[12] = {9, 8, 9, 5, 4, 10, 10, 11, 11, 7, 3, 10};
    static constexpr int kY[12] = {0, 0, 1, 1, 0, 0, 0, 0, 1, 2, 0, 0};
    const auto special = [&](const int* xs) {
        for (int i = 0; i < 12; ++i) {
            stepX[static_cast<std::size_t>(36 + i)] = stepX[static_cast<std::size_t>(84 + i)] = xs[i];
            stepY[static_cast<std::size_t>(36 + i)] = stepY[static_cast<std::size_t>(84 + i)] = kY[i];
        }
    };
    switch (t) {
    case 1:
        footOffX = 0x6C, footOffY = 0x13E;
        fill(15, 5);
        unk170C = 12, unk1710 = 12;
        special(kX1);
        unk1714 = 0, unk1718 = 0;
        headHeight = 100;
        break;
    default:
        footOffX = 0x37, footOffY = 0xA2;
        fill(9, 3);
        unk170C = 7, unk1710 = 3;
        special(kX0);
        unk1714 = 0, unk1718 = 0;
        headHeight = 50;
        break;
    case 10:
        footOffX = 0x37, footOffY = 0x96;
        fill(9, 3);
        unk170C = 5, unk1710 = 5;
        standFrame = {22, 34, 46, 52, 70, 82, 94, 4};
        break;
    case 11:
        footOffX = 0x4E, footOffY = 0xC6;
        fill(13, 4);
        unk170C = 8, unk1710 = 8;
        standFrame = {13, 25, 37, 49, 61, 73, 85, 1};
        break;
    case 12:
        footOffX = 0x2A, footOffY = 0x7C;
        fill(9, 3);
        unk170C = 5, unk1710 = 5;
        standFrame = {13, 25, 37, 49, 61, 73, 85, 1};
        break;
    }
    type = t;
    if (!head) {
        // Pfad ohne die letzten 4 Zeichen + "Talk.a16"
        std::string p = path.size() >= 4 ? path.substr(0, path.size() - 4) : std::string();
        p += "Talk.a16";
        auto h = std::make_unique<AnimLayer>(g_);
        std::string err;
        // Original: bei Fehlschlag gelöscht, Zeiger bleibt aber stehen (späteres Sprechen
        // greift ins Leere). Port: kein Kopf (Abweichung nur bei fehlender Datei).
        if (fs_ && h->AnimLayer::load(FoundFile{fs_, p}, 0, &err)) {
            h->renderToSurface();  // 0x00446C90
            h->name = fileStem(h->path);
            head = std::move(h);
        }
    }
}

void Actor::setStepSounds(AudioManager* mgr, const FoundFile* left, const FoundFile* right, const FoundFile* alt) {
    sfxLeft = std::make_unique<AudioBuffer>();
    sfxLeft->open(left, mgr, kAudioSfx);
    sfxRight = std::make_unique<AudioBuffer>();
    sfxRight->open(right, mgr, kAudioSfx);
    sfxAlt = std::make_unique<AudioBuffer>();
    sfxAlt->open(alt, mgr, kAudioSfx);
}

void Actor::setPathMap(const ZMap* map) {
    pathMap = map;
    pathFinder.reset();
    if (map) pathFinder = std::make_unique<PathFinder>(g_.pathGrid, *map);
    targetY = 0;
    targetX = 0;
}

void Actor::updateFoot() {
    const int zi = ftol(zoom);
    footDX = (footOffX * zi) / 100;
    footDY = (footOffY * zi) / 100;
    footX = ftol(x) + footDX;
    footY = ftol(y) + footDY;
}

void Actor::footToLayer(int& fx, int& fy) {
    const int zi = ftol(zoom);
    footDX = (footOffX * zi) / 100;
    footDY = (footOffY * zi) / 100;
    fx -= footDX;
    fy -= footDY;
}

void Actor::setPosition(int px, int py, bool force) {
    if (pathMap && zpos != 0) {
        const int cx = px >= 640 ? 639 : (px < 0 ? 0 : px);
        const int cy = py >= 360 ? 359 : (py < 0 ? 0 : py);
        const int v = pathMap->at(cx, cy);
        if (v == 0) {
            if (!force) return;
        } else {
            z = v;
            double f;
            if (pathMap->sMax == 0.0f) f = static_cast<double>(zpos) / v;  // +0x114 == 0.0
            else f = zScale / ((v - pathMap->vMin) * pathMap->k + pathMap->sMin);
            zoom = zoomVal * f;
            updateZoomRect();
        }
    }
    int lx = px, ly = py;
    footToLayer(lx, ly);
    Layer::setPos(lx, ly);  // 0x00446E70
    updateFoot();
    if (!tintZones) return;
    int r = 0, gr = 0, b = 0;
    if (colorR != 0 || colorG != 0 || colorB != 0) {
        r = colorR;
        gr = colorG;
        b = colorB;
    }
    for (std::size_t i = 0; i < 10; ++i) {
        const int range = tintRange[i];
        if (range == 0) continue;
        const int ax = iabs(footX - tintX[i]);
        if (ax > range) continue;
        const int ay = iabs(footY - tintY[i]);
        if (ay > range) continue;
        int w = ((range - ay) * 100) / range - 100 + ((range - ax) * 100) / range;
        if (w < 0) w = 0;
        r += (tintR[i] * w) / 100;
        gr += (tintG[i] * w) / 100;
        b += (tintB[i] * w) / 100;
    }
    const TccColorAdd& c = imageColor();
    if (c.r != r || c.g != gr || c.b != b) setImageColorNoDecode(r, gr, b);  // Bild +0x28(r, g, b, 0)
}

void Actor::faceTowards(int x0, int y0, int x1, int y1) {
    int dx = x1 - x0, dy = y1 - y0;
    int ax = iabs(dx);
    const int ay = iabs(dy);
    if (ax == 0 && ay == 0) ax = 1;
    const int a = (ay * 90) / (ay + ax);
    if (a < 23 || a > 67) {
        if (ay < ax) dy = 0;
        else dx = 0;
    }
    int code = dx > 0 ? 1 : (dx < 0 ? 2 : 0);
    if (dy > 0) code |= 4;
    else if (dy < 0) code |= 8;
    if (code == 0) return;
    walkMode = dx == 0 ? 2 : (dy == 0 ? 1 : 3);
    dir = dirOfCode[static_cast<std::size_t>(code)];
}

void Actor::setDirection(int d) {
    dir = d;
    // 0x00435B8A: Original indiziert +0x1A60 ungeprüft. Alle Richtungen in Daten und Code liegen in
    // 0…7 (tools/data_scan.py); nur ein manipulierter Spielstand käme darüber – Port maskiert.
    showFrame(standFrame[static_cast<std::size_t>(d & 7)]);
}

void Actor::turnTo(int d) {
    if (dir == d) {
        showFrame(standFrame[static_cast<std::size_t>(d & 7)]);
        return;
    }
    shownDir = dir;
    dir = d;
    turning = true;
    dirAtTarget = -1;
}

void Actor::fireWalkEvent(int t) {
    if (!walkEvent) return;
    walkEvent->mark(g_, t);  // +4 = 1, Typ/Param = t, Sofort-Flag → CEvent_Fire
    walkDone = true;
}

void Actor::turnImpl(int d, bool withEvent) {
    if (dir == d) {
        showFrame(standFrame[static_cast<std::size_t>(d & 7)]);
        if (withEvent) fireWalkEvent(0x29);
        return;
    }
    shownDir = dir;
    dir = d;
    turning = true;
    dirAtTarget = -1;
}

void Actor::turn(int d, int handler) {
    if (handler != 0) {
        walkDone = false;
        if (!walkEvent) walkEvent = std::make_unique<Event>();
        walkEvent->setHandler(handler);
        walkEvent->enabled = true;
    }
    turnImpl(d, handler != 0);
}

bool Actor::walkTo(int px, int py, int handler, int d) {
    if (handler != 0) walkDone = false;  // 0x00434D90
    // 0x00434DC0 CActor_SetWalk
    if (handler != 0) {
        if (!walkEvent) walkEvent = std::make_unique<Event>();
        walkEvent->setHandler(handler);
        walkEvent->enabled = true;
    }
    if (d != -1) dirAtTarget = d;
    return walkCore(px, py, handler != 0);
}

bool Actor::walkToClick(int px, int py) {
    dirAtTarget = -1;
    return walkCore(px, py, false);
}

bool Actor::walkCore(int px, int py, bool fresh) {
    enable();
    if (!pathMap) return false;
    if (footX == px && footY == py) {
        if (dirAtTarget != -1 && dirAtTarget != dir) {
            if (!walkEvent) {
                turnImpl(dirAtTarget, false);
            } else {
                // CActor_Turn(richtung, Handler des Ereignisses)
                walkDone = false;
                walkEvent->pending = false;
                walkEvent->enabled = true;
                turnImpl(dirAtTarget, walkEvent->hasHandler());
            }
            clearRun();
            return true;
        }
        fireWalkEvent(0x29);
        clearRun();
        return true;
    }
    if (!fresh && walkEvent) {
        if (!walkDone) {
            // laufende Aktion abbrechen: Typ 0x2A, Ereignis wandert nach +0x16F0
            walkEvent->mark(g_, 0x2A);
            oldWalkEvent = std::move(walkEvent);
            walkDone = true;
        } else {
            walkEvent->enabled = false;
            walkEvent->pending = false;
            walkEvent.reset();
        }
    }
    if (zpos == 0) {
        targetX = px;
        targetY = py;
        return true;
    }
    // Ziel einrasten: unten, oben, rechts, links (vorzeichenlose Vergleiche)
    const PathGrid& grid = g_.pathGrid;
    unsigned ty = static_cast<unsigned>(py), tx = static_cast<unsigned>(px);
    while (!pathCellFree(grid, px, static_cast<int>(ty))) {
        if (0x163 < ++ty) {
            ty = static_cast<unsigned>(py);
            break;
        }
    }
    while (!pathCellFree(grid, px, static_cast<int>(ty))) {
        if (--ty < 5) {
            ty = static_cast<unsigned>(py);
            break;
        }
    }
    while (!pathCellFree(grid, static_cast<int>(tx), static_cast<int>(ty))) {
        if (0x27B < ++tx) {
            tx = static_cast<unsigned>(px);
            break;
        }
    }
    while (!pathCellFree(grid, static_cast<int>(tx), static_cast<int>(ty))) {
        if (--tx < 5) {
            tx = static_cast<unsigned>(px);
            break;
        }
    }
    const int sx = static_cast<int>(tx), sy = static_cast<int>(ty);
    // UNVERIFIED: Pixel außerhalb der Karte liest das Original aus fremdem Speicher
    const bool onMap = sx >= 0 && sx < ZMap::kWidth && sy >= 0 && sy < ZMap::kHeight;
    if (!onMap || pathMap->at(sx, sy) == 0) {
        fireWalkEvent(0x29);
        clearRun();
        return false;
    }
    const auto jump = [&] {
        targetY = 0;
        targetX = 0;
        setPosition(sx, sy, true);  // kein Weg: Figur springt ans Ziel
        fireWalkEvent(0x29);
        clearRun();
        return true;
    };
    // Original ruft über Zeiger 0; jede SZN mit Figur hat eine vorhandene Wegkarte (wBuf, geprüft per
    // Datenscan) – reiner Port-Schutz
    if (!pathFinder) return jump();
    if (targetX == 0 && targetY == 0) {
        targetX = sx;
        targetY = sy;
        if (!pathFinder->find(footX, footY, sx, sy)) return jump();
    } else {
        const int ox = targetX, oy = targetY;
        targetX = sx;
        targetY = sy;
        if (!pathFinder->find(footX, footY, sx, sy)) {
            if (!pathFinder->find(ox, oy, targetX, targetY)) return jump();
            setPosition(ox, oy, true);
        }
    }
    pathFinder->next(targetX, targetY);  // Startpunkt verwerfen
    if (!pathFinder->next(targetX, targetY)) {
        targetX = 0;
        targetY = 0;
        return true;
    }
    const int before = dir;
    faceTowards(footX, footY, targetX, targetY);
    if (before != dir) {
        shownDir = before;
        turning = true;
    }
    return true;
}

bool Actor::update(EngineGlobals& g) {
    bool arrived = false, newLeg = false;
    if (talking) {
        head->update(g);  // vtable +0x18
        dirty = true;
        return true;
    }
    bool walk = !turning;
    if (turning) {
        if (shownDir == dir) {
            turning = false;
            if (targetX == 0 && targetY == 0) arrived = true;
        } else {
            // kürzere Drehrichtung, ein Schritt je Update
            const bool cw = shownDir < dir ? dir - shownDir < 5 : shownDir - dir > 3;
            shownDir += cw ? 1 : -1;
            if (shownDir >= 8) shownDir = 0;
            else if (shownDir < 0) shownDir = 7;
            const auto s = static_cast<std::size_t>(shownDir);
            if (lastFrame[s] - firstFrame[s] <= frame) frame = 0;
            if (frame < 0) frame = 0;
            imageSeek(firstFrame[s] + frame);
        }
        walk = !turning;
    }
    if (walk) {
        if (footX == targetX && footY == targetY && (targetX != 0 || targetY != 0)) {
            if (!pathFinder || !pathFinder->next(targetX, targetY)) {  // !pathFinder: Port-Schutz (s. o.)
                targetX = 0;
                targetY = 0;
                arrived = true;
            } else {
                faceTowards(footX, footY, targetX, targetY);
                newLeg = true;
            }
        }
        if (targetY != 0 || targetX != 0) {
            const auto d = static_cast<std::size_t>(dir & 7);
            // Bildindex der Schritttabelle (0x004354DA)
            int n = firstFrame[d];
            if (frame >= 0) {
                n = firstFrame[d] + 1;
                if (frame <= 13) {
                    n += frame;
                    if (frame + 1 > lastFrame[d] - firstFrame[d]) n -= 12;
                }
            }
            // Feste Tabellen (Z. 26/27): n liegt immer in firstFrame[d] … lastFrame[d] ⊂ 0…95 (frame ≤ 10:
            // ≤ last; 11…13: −12 → ≤ first+2; sonst first/first+1) – die Begrenzung greift nie
            const auto ni = static_cast<std::size_t>(n < 0 ? 0 : (n > 95 ? 95 : n));
            const double sx = stepX[ni] * zoom * 0.01;
            const double sy = perCor != 100 ? static_cast<double>(stepY[ni] * perCor) * 0.01 * zoom * 0.01
                                            : stepY[ni] * zoom * 0.01;
            int mode = walkMode;
            if (mode == 3) mode = 2;  // Diagonale: gleiche Werte wie senkrecht (0x004355A3)
            const int ix = ftol(sx + 0.5), iy = ftol(sy + 0.5);
            const int dx = targetX - footX, dy = targetY - footY;
            double vx, vy;
            if (mode == 1) {
                // Hauptachse x, auf Restweg begrenzt; y anteilig
                vx = ix;
                if (dx < 0) vx = ix * -1.0;
                if (iabs(dx) < iabs(ftol(vx))) vx = dx;
                const double k = vx != 0.0 ? dx / vx : static_cast<double>(dx);
                vy = iabs(dy) / k + 0.5;
                if (dy < 0) vy *= -1.0;
                if (iabs(dy) < iabs(ftol(vy))) vy = dy;
            } else {
                vy = iy;
                if (dy < 0) vy = iy * -1.0;
                if (iabs(dy) < iabs(ftol(vy))) vy = dy;
                const double k = vy != 0.0 ? dy / vy : static_cast<double>(dy);
                vx = iabs(dx) / k + 0.5;
                if (dx < 0) vx *= -1.0;
                if (iabs(dx) < iabs(ftol(vx))) vx = dx;
            }
            setPosition(ftol(vx) + footX, ftol(vy) + footY, true);
            if (dir != shownDir) {
                const auto s = static_cast<std::size_t>(dir & 7);
                if (lastFrame[s] - firstFrame[s] <= frame || frame < 0) frame = 0;
                imageSeek(firstFrame[s] + frame);
                shownDir = dir;
            }
        }
        if (arrived) {
            if (dirAtTarget == -1 || dir == dirAtTarget) {
                imageSeek(standFrame[static_cast<std::size_t>(dir & 7)]);
                frame = -1;
                shownDir = -1;
                fireWalkEvent(0x29);
                clearRun();
            } else {
                dir = dirAtTarget;
                turning = true;
                dirAtTarget = -1;
            }
        }
    }
    if (turning || targetX != 0 || arrived) {
        const float px = mapRange(static_cast<float>(footX), 0.0f, 639.0f, -1.0f, 1.0f);
        const float pz = mapRange(static_cast<float>(z), 0.0f, 255.0f, 0.1f, 0.7f);
        if (!AnimLayer::update(g)) return false;
        AudioBuffer* b = nullptr;
        if (frame == sfxFrameLeft && sfxLeft) {
            b = sfxLeft.get();
        } else if (frame == sfxFrameRight && sfxRight) {
            b = g.rand() % 10 == 5 ? sfxAlt.get() : sfxRight.get();
        }
        if (b) {
            b->setPos3D(px, 0.0f, pz);
            b->play();  // vtable +0x08
        }
    }
    const auto d = static_cast<std::size_t>(dir & 7);
    if (!turning && lastFrame[d] - firstFrame[d] <= frame) {
        frame = -1;
        imageSeek(firstFrame[d]);
    }
    if (!follower || (!arrived && !newLeg)) return true;
    // Begleiter: 20 Pixel hinter die Figur (vorzeichenlose Grenzen wie im Original)
    unsigned fx = static_cast<unsigned>(footX), fy = static_cast<unsigned>(footY);
    const auto right = [&] { fx += 20; if (0x27F < fx) fx = 0x27F; };
    const auto left = [&] { fx -= 20; if (fx < 1) fx = 1; };
    const auto down = [&] { fy += 20; if (0x167 < fy) fy = 0x167; };
    const auto up = [&] { fy -= 20; if (fy < 1) fy = 1; };
    switch (dir) {
    case 2: right(); break;
    case 3:
    case 4: right(); down(); break;
    case 5: left(); down(); break;
    case 6: left(); break;
    case 7: left(); up(); break;
    default: right(); up(); break;
    }
    if (!follower->walkTo(static_cast<int>(fx), static_cast<int>(fy), 0, dir))
        follower->walkTo(footX, footY, 0, dir);
    return true;
}

bool Actor::drawActor(Surface565& s, int flag, const ZMap* zmap) {
    if (!talking || !head) return draw(s, flag, zmap);  // CLayer_DrawDepth
    anim.clearAlphaRows(headHeight);  // CImage_ClearRows(Bild, Kopfhöhe)
    if (!draw(s, flag, zmap)) return false;
    return head->draw(s, flag, zmap);
}

void Actor::blockRect(int x1, int y1, int x2, int y2) {
    if (pathFinder) pathBlockRect(g_.pathGrid, x1, y1, x2, y2);
}

void Actor::unblockRect(int x1, int y1, int x2, int y2) {
    if (pathFinder) pathUnblockRect(g_.pathGrid, x1, y1, x2, y2);
}

void Actor::hide(bool& sceneClicks) {
    savedClicks = sceneClicks;  // +0x1AFC = CSzene+0x514
    disable();
    sceneClicks = false;  // CSzene_DisableInput
}

void Actor::show(bool& sceneClicks) {
    enable();
    sceneClicks = savedClicks;
}

void Actor::startTalk(const std::vector<std::uint16_t>* lip, int rate, AudioBuffer* audio) {
    if (!head) return;
    const auto d = static_cast<std::size_t>(dir & 7);
    turning = false;
    talking = true;
    showFrame(standFrame[d]);
    head->z = z;
    head->x = x;
    head->y = y;
    head->fps = fps;
    head->setZoom(ftol(zoom));
    const TccColorAdd& c = imageColor();
    head->setColor(c.r, c.g, c.b);  // CLayer_SetColor(Kopf, Bild-R, -G, -B)
    if (lip && audio) head->setLipSync(lip, rate, audio, firstFrame[d]);
    else head->playRangeNoEvent(0x0D, firstFrame[d], lastFrame[d]);
}

void Actor::endTalk() {
    if (!head) return;  // Original: Zeiger ohne Prüfung
    head->stop(0);  // vtable +0x24(0)
    head->disable();
    talking = false;
    fps = head->fps;
    showFrame(0);
    showFrame(standFrame[static_cast<std::size_t>(dir & 7)]);
    dirty = true;
}

}  // namespace rah
