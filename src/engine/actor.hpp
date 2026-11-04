// SPDX-License-Identifier: GPL-3.0-or-later
// Figur (CActor : CAnimLayer, 0x1B00 Bytes, vtable 0x0046B538): Fußpunkt, Richtungen,
// Laufen über die Wegsuche, Schrittgeräusche, sprechender Kopf (docs/ACTOR.md,
// docs/PATHFINDING.md, docs/formats/szn.md).
#pragma once

#include <array>
#include <memory>

#include "engine/animlayer.hpp"
#include "engine/audio.hpp"
#include "engine/pathfinder.hpp"

namespace rah {

class Actor final : public AnimLayer {
public:
    // orig: Rent-A-Hero.exe 0x004338A0 CActor_ctor
    explicit Actor(EngineGlobals& g);
    // orig: Rent-A-Hero.exe 0x00433C40
    ~Actor() override;

    // vtable +0x04: orig: Rent-A-Hero.exe 0x00433CB0 – Wegsucher, Schrittgeräusche, Ereignis
    // +0x16F0 freigeben
    void release();
    // orig: Rent-A-Hero.exe 0x00433B70 – Wiederverwendung beim Szenenwechsel (gleicher Typ)
    void reset();
    // orig: Rent-A-Hero.exe 0x00433E30 CActor_SetType (lädt beim ersten Mal den Kopf
    // „<Datei ohne Endung>Talk.a16“)
    void setType(int type);
    // orig: Rent-A-Hero.exe 0x00434550 – Schrittgeräusche links/rechts/alternativ
    void setStepSounds(AudioManager* mgr, const FoundFile* left, const FoundFile* right, const FoundFile* alt);
    // orig: Rent-A-Hero.exe 0x00434680 CActor_SetPathMap (baut das globale Raster neu)
    void setPathMap(const ZMap* map);

    // vtable +0x18: orig: Rent-A-Hero.exe 0x00435340 CActor_Update
    bool update(EngineGlobals& g) override;
    // vtable +0x14: orig: Rent-A-Hero.exe 0x00434A90 → CActor_SetPosition(x, y, 0)
    void setPos(int x, int y) override { setPosition(x, y, false); }
    // vtable +0x24: orig: Rent-A-Hero.exe 0x00435D10 – Laufziel löschen (Argument egal)
    void stop(int) override {
        targetX = 0;
        targetY = 0;
    }
    // orig: Rent-A-Hero.exe 0x00435AE0 CActor_Draw – beim Sprechen Kopfzeilen löschen, Kopf
    // darüber zeichnen
    bool drawActor(Surface565& s, int flag, const ZMap* zmap);

    // orig: Rent-A-Hero.exe 0x00434AE0 CActor_SetPosition(x, y, force) – Fußpunkt
    void setPosition(int x, int y, bool force);
    // orig: Rent-A-Hero.exe 0x00434AB0 CActor_SetPos(x, y, richtung)
    void setPosDir(int x, int y, int dir) {
        setPos(x, y);
        setDirection(dir);
    }
    // orig: Rent-A-Hero.exe 0x00434720 CActor_UpdateFoot / 0x004347D0 CActor_FootToLayer
    void updateFoot();
    void footToLayer(int& x, int& y);
    // orig: Rent-A-Hero.exe 0x00434850 CActor_FaceTowards
    void faceTowards(int x0, int y0, int x1, int y1);
    // orig: Rent-A-Hero.exe 0x00435B80 – Richtung sofort, Standbild
    void setDirection(int dir);
    // orig: Rent-A-Hero.exe 0x00435BA0 – Richtung über Drehung (ohne Ereignis)
    void turnTo(int dir);
    // orig: Rent-A-Hero.exe 0x00435BF0 CActor_Turn(richtung, ereignis)
    void turn(int dir, int handler);
    // orig: Rent-A-Hero.exe 0x00434D70 CActor_WalkTo / 0x00434D90 CActor_WalkTo2(x, y,
    // ereignis, richtung am Ziel)
    bool walkTo(int x, int y, int handler, int dir);
    // orig: Rent-A-Hero.exe 0x00434E80 CActor_WalkToClick
    bool walkToClick(int x, int y);
    // orig: Rent-A-Hero.exe 0x00434EA0 CActor_WalkCore(x, y, neu)
    bool walkCore(int x, int y, bool fresh, bool fromClick = false);
    // orig: Rent-A-Hero.exe 0x00435320 CActor_SetRun / 0x00435330 CActor_ClearRun (+0x58;
    // ClearRun setzt außerdem 0x0047D130 = 0, das nie ≠ 0 wird)
    void setRun() { alwaysUpdate = true; }
    void clearRun() { alwaysUpdate = false; }
    // orig: Rent-A-Hero.exe 0x00435D90 CActor_BlockRect / 0x00435DC0 CActor_UnblockRect
    void blockRect(int x1, int y1, int x2, int y2);
    void unblockRect(int x1, int y1, int x2, int y2);
    // orig: Rent-A-Hero.exe 0x00435F70 – Begleiter, der hinterherläuft
    void setFollower(Actor* a) { follower = a; }
    // orig: Rent-A-Hero.exe 0x00433DE0 CActor_Hide / 0x00433E10 CActor_Show (Eingabe der Szene)
    void hide(bool& sceneClicks);
    void show(bool& sceneClicks);
    // orig: Rent-A-Hero.exe 0x00435E00 – Kopf spricht (Lippensync oder Bereich in Schleife);
    // 0x00435DF0 = ohne Lippendaten
    void startTalk(const std::vector<std::uint16_t>* lip, int rate, AudioBuffer* audio);
    // orig: Rent-A-Hero.exe 0x00435F10 – Sprechen beenden
    void endTalk();

    const ZMap* pathMap = nullptr;           // +0x15C8
    int dir = 0;                             // +0x15CC
    int footX = 0, footY = 0;                // +0x15D0, +0x15D4
    std::unique_ptr<AudioBuffer> sfxLeft, sfxRight, sfxAlt;  // +0x15D8, +0x15DC, +0x15E0
    std::array<int, 10> tintR{}, tintG{}, tintB{}, tintX{}, tintY{}, tintRange{};  // +0x15E4…
    bool tintZones = false;                  // +0x16D4
    int zpos = 0;                            // +0x16D8
    double zScale = 0;                       // +0x16E0
    int zoomVal = 0;                         // +0x16E8
    int perCor = 100;                        // +0x16EC
    std::unique_ptr<Event> oldWalkEvent;     // +0x16F0 (abgebrochenes Laufereignis)
    int footOffX = 0, footOffY = 0;          // +0x16F4, +0x16F8
    int footDX = 0, footDY = 0;              // +0x16FC, +0x1700
    int stepXDefault = 0, stepYDefault = 0;  // +0x1704, +0x1708
    int unk170C = 0, unk1710 = 0, unk1714 = 0, unk1718 = 0;  // gesetzt, Leser unbekannt
    int headHeight = 0;                      // +0x171C
    std::array<int, 96> stepX{}, stepY{};    // +0x1720, +0x18A0
    std::array<int, 8> firstFrame{}, lastFrame{}, standFrame{};  // +0x1A20, +0x1A40, +0x1A60
    int sfxFrameLeft = 10, sfxFrameRight = 4, unk1A88 = 12;      // +0x1A80, +0x1A84, +0x1A88
    int shownDir = -1;                       // +0x1A8C
    std::array<int, 11> dirOfCode{};         // +0x1A90 Richtung je Code (FaceTowards)
    int walkMode = 0;                        // +0x1AD0
    int dirAtTarget = -1;                    // +0x1AD4
    bool turning = false;                    // +0x1AD8
    bool talking = false;                    // +0x1ADC
    int targetX = 0, targetY = 0;            // +0x1AE0, +0x1AE4 (0, 0 = kein Ziel)
    bool walkDone = true;                    // +0x1AE8
    int type = 0;                            // +0x1AEC
    std::unique_ptr<PathFinder> pathFinder;  // +0x1AF0
    std::unique_ptr<AnimLayer> head;         // +0x1AF4 („Talk.a16“)
    Actor* follower = nullptr;               // +0x1AF8
    bool savedClicks = false;                // +0x1AFC
    std::unique_ptr<Event> walkEvent;        // +0x1C8 (Typ 0x29 fertig, 0x2A abgebrochen)

private:
    void fireWalkEvent(int type);
    void turnImpl(int dir, bool withEvent);
    const FileSystem* fs_ = nullptr;  // Quelle der Figurendatei (für „Talk.a16“)

public:
    // CLayer_Load (Slot 2/3) mit Merken der Quelle
    bool load(const FoundFile& file, std::uint16_t colorKey, std::string* error) override;
};

}  // namespace rah
