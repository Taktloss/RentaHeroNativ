// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc31.hpp"

#include "engine/area.hpp"

namespace rah {

namespace {
constexpr int kAmbientEvent = 200;
constexpr std::size_t kTo57Open = 0x3F98;  // Ziel Szene 57 frei: Loc15 setzt es, wenn der Pirat gesehen wurde (0x0041AC0E)
constexpr std::size_t kTo30Open = 0x3E88;  // Ziel Szene 31/Loc28 frei: Loc04 setzt es nach Ramils Geschichte (0x0040CE3C)
}  // namespace

Loc31::Loc31(GameContext& ctx, int scene, int prevScene) : Game(ctx, 31, 113, prevScene) {
    (void)scene;  // Original setzt +0x2C fest auf 0x71
    getViewObjects();
    scene_.load("S113", 0xFFFF);  // CSzene_LoadAll(DAT_00476120)
    getViewObjects();
    to30_ = loadAnim("S113_To30");
    to37_ = loadAnim("S113_To37");
    to57_ = loadAnim("S113_To57");
    to8_ = loadAnim("S113_To8");
    // UNVERIFIED: das Original prüft weder auf fehlende noch auf statische Ebenen
    backLoop_ = dynamic_cast<AnimLayer*>(scene_.getObject("S113_BackLoop"));
    areaTo8_ = scene_.getArea("S113_To8");
    areaTo37_ = scene_.getArea("S113_To37");
    areaTo57_ = scene_.getArea("S113_To57");
    areaTo30_ = scene_.getArea("S113_To30");
    seagull_ = loadSfx("Sfx_Seagull");
    wind_ = loadSfx("Sfx_Wind");
    ocean_ = loadSfxVol("Sfx_Ocean", 40);
    passing_ = loadSfx("Sfx_Glider_PassingBy");
    door_ = loadSfx("Sfx_Door_Opened");
    ambientTimer_ = std::make_unique<Event>();
    music_ = playMusic("Loc01_Smashville");
    if (music_) {
        music_->setReverb(0x10, 0.111f, 2.767f, 0.224f);  // 0x3DE353F8, 0x40311687, 0x3E656042
        music_->setReverbMix(0.1f);                       // 0x3DCCCCCD
    }
}

void Loc31::destroyLocal() {
    ambientTimer_.reset();  // 0x0043F160 (austragen) + Löschen
    to30_.reset();
    to37_.reset();
    to57_.reset();
    to8_.reset();
    seagull_.reset();
    wind_.reset();
    ocean_.reset();
    passing_.reset();
    door_.reset();
}

void Loc31::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    setEventProc(&Loc31::onEvent);
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    if (backLoop_) backLoop_->start(13);
    if (ocean_) ocean_->playLoop();  // vtable +0x10
    if (areaTo8_) areaTo8_->setEvent(1);
    if (areaTo37_) areaTo37_->setEvent(2);
    if (ctx_.state.at(kTo57Open) != 0 && areaTo57_) areaTo57_->setEvent(3);
    if (ctx_.state.at(kTo30Open) != 0 && areaTo30_) areaTo30_->setEvent(4);
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(3, 5) * 1000), 0, kAmbientEvent);
    scene_.enableInput();
}

void Loc31::takeOff(AnimLayer* flight) {
    scene_.disableInput();
    if (backLoop_) {
        backLoop_->stop(11);  // vtable +0x24: Endmodus
        const int cb = beginWait();
        backLoop_->setEndEvent(cb);  // 0x004434A0
        wait();
    }
    if (passing_) {
        passing_->setFreq(25);
        passing_->play();
        passing_->fadeFreq(100, 2000);
    }
    scene_.setModal(flight);
    const int cb = beginWait();
    if (flight) flight->playEndEvent(11, cb);
    wait();
}

void Loc31::onEvent(Event& e, int) {
    // Original ohne Prüfung des Wartergebnisses: Szenenwechsel auch nach Abbruch
    const auto fly = [this](AnimLayer* flight, int scene, int location) {
        if (door_) door_->play();
        takeOff(flight);
        changeScene(scene, location);
    };
    switch (e.param) {
    case 1: fly(to8_.get(), 8, 1); return;
    case 2: fly(to37_.get(), 37, 10); return;
    case 3: fly(to57_.get(), 57, 15); return;
    case 4: fly(to30_.get(), 31, 28); return;
    case kAmbientEvent: {
        const int r = random(1, 2);
        if (r == 1 && seagull_) {
            seagull_->setVolume(random(50, 100));
            playSfxRandom3D(*seagull_);
        } else if (r == 2 && wind_) {
            playSfxRandom3D(*wind_);
        }
        setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(3, 5) * 1000), 0, kAmbientEvent);
        return;
    }
    default: return;
    }
}

}  // namespace rah
