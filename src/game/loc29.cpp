// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc29.hpp"

#include <bit>

#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
float f32(std::uint32_t bits) { return std::bit_cast<float>(bits); }
constexpr std::size_t kJugServed = 0x40F4;  // Krug steht da (nach Gesprächsstufe 2)
constexpr std::size_t kTalk = 0x40F8;       // Gesprächsstufe mit den Piraten (1…3)
}  // namespace

Loc29::Loc29(GameContext& ctx, int scene, int prevScene) : Game(ctx, 29, scene, prevScene) {
    sceneNr_ = 0x21;
    getViewObjects();
    drinking_ = loadSfxVol("Sfx_Drinking", 0x32);
    knock_ = loadSfx("Sfx_Knock_Wet");
    tock_ = loadSfx("Sfx_Tock");
    piano_ = loadSfx("Sfx_PianoStroke");
    scene_.load("S33", 0xFFFF);  // CSzene_LoadAll(DAT_00475E08)
    getViewObjects();
    enter_ = loadAnim("S33_Enter");
    drinkLook_ = loadAnim("S33_RodKipptAndLook");
    pirate1_ = obj("S33_Pirate1");
    jug_ = scene_.getObject("S33_Krug1");
    pirate2_ = obj("S33_Pirate2");
    pirate3_ = obj("S33_Pirate3");
    rodLeaves_ = obj("S33_RodGeht");
    rodrigo_ = obj("S33_Rodrigo");
    to32_ = scene_.getArea("S33_To32");
    music_ = playMusic("Loc29_PubInEndavin");
    if (music_) {
        music_->setReverb(4, f32(0x3E54FDF4), f32(0x3EF4BC6A), 0.0f);
        music_->setReverbMix(f32(0x3DCCCCCD));  // 0x004375B0
    }
}

void Loc29::destroyLocal() {
    enter_.reset();
    drinkLook_.reset();
    for (auto* b : {&drinking_, &knock_, &tock_, &piano_}) b->reset();
}

bool Loc29::talk(const char* key, AnimLayer* speaker, int from, int to) {
    const int cb = beginWait();
    speak("Loc29", key, speaker, 0x0C, from, to, cb);  // CGame_Speak
    return wait();
}

bool Loc29::playRange(AnimLayer* a, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(11, cb, from, to);
    return wait();
}

void Loc29::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    setEventProc(&Loc29::onEvent);
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    if (restored_ == 0 && flag(kJugServed) == 0) playAnimWait(enter_.get());
    for (AnimLayer* a : {pirate1_, pirate2_, pirate3_, rodrigo_})
        if (a) a->enable();
    if (flag(kJugServed) != 0) {
        if (jug_) {
            jug_->enable();
            if (jug_->area) jug_->area->setEvent(3);
        }
        if (pirate3_) pirate3_->playRange(11, 0, 0, -1);
    }
    if (flag(kTalk) == 2 && pirate3_) pirate3_->playRange(11, 0, 0, -1);
    scene_.enableInput();
    if (pirate2_) pirate2_->playRange(11, 0, 0, 10);
    for (AnimLayer* a : {pirate1_, pirate2_, pirate3_})
        if (a && a->area) a->area->setEvent(2);
    if (InventoryBar* bar = scene_.inventory()) {
        for (int id : {1, 0x0E, 4, 3}) bar->setItemEvent(id, 4);
    }
    if (to32_) to32_->setEvent(1);
    scene_.enableInput();
}

void Loc29::onEvent(Event& e, int) {
    switch (e.param) {
    case 1:  // hinaus → 28/32
        scene_.disableInput();
        if (rodrigo_) rodrigo_->disable();
        if (rodLeaves_) rodLeaves_->enable();
        {
            const int cb = beginWait();
            if (rodLeaves_) rodLeaves_->playRange(10, cb, 0, -1);
            wait();
        }
        changeScene(0x20, 0x1C);
        return;
    case 2:  // Piraten ansprechen
        switch (flag(kTalk)) {
        case 1:
            talk("033_ROD_01", rodrigo_, 0, 0x0F);
            talk("033_PI2_01", pirate1_, 0x13, 0x27);
            if (pirate1_) pirate1_->playRange(11, 0, 8, 0x13);
            talk("033_PI3_01", pirate2_, 0x55, 0x68);
            flag(kTalk) = 2;
            if (pirate3_) pirate3_->playRange(11, 0, 0, -1);
            return;
        case 2:
            scene_.disableInput();
            talk("033_ROD_02", rodrigo_, 0, 0x0F);
            talk("033_PI2_02", pirate1_, 0, 8);
            talk("033_PI3_02", pirate2_, 0x55, 0x68);
            talk("033_PI3_03", pirate2_, 10, 0x16);
            playRange(pirate2_, 0x2C, 0x49);
            if (tock_) tock_->play();  // vtable +0x08
            if (pirate1_) pirate1_->playRange(11, 0, 8, 0x13);
            playRange(pirate2_, 0x4A, 0x54);
            if (jug_) jug_->enable();
            if (pirate2_) pirate2_->showFrame(0x2C);
            if (jug_ && jug_->area) jug_->area->setEvent(3);
            flag(kJugServed) = 1;
            flag(kTalk) = 3;
            scene_.enableInput();
            return;
        case 3:
            talk("033_ROD_04", rodrigo_, 0, 0x0F);
            talk("033_PI2_04", pirate1_, 0, 8);
            return;
        default:
            return;
        }
    case 3: {  // Krug austrinken: Rodrigo fällt um, aufwachen im Kerker → 30/34
        scene_.disableInput();
        for (Layer* l : {static_cast<Layer*>(pirate1_), static_cast<Layer*>(pirate2_), static_cast<Layer*>(pirate3_),
                         static_cast<Layer*>(rodrigo_), jug_})
            if (l) l->disable();
        if (to32_) to32_->active = false;  // +0xB8
        AnimLayer* a = drinkLook_.get();
        scene_.setModal(a);
        playRange(a, 0, 9);
        if (drinking_) drinking_->play();
        playRange(a, 10, 0x26);
        if (piano_) piano_->play();
        playRange(a, 0x27, 0x41);
        if (knock_) knock_->play();
        playRange(a, 0x42, 0x4C);
        speak("Loc29", "033_PI2_05", 0);  // CGame_SayNoActor
        delay(4000);
        playRange(a, 0x4D, 0x89);
        speak("Loc29", "033_PI3_05", 0);
        playRange(a, 0x8A, -1);
        changeScene(0x22, 0x1E);
        return;
    }
    case 4:
        speak("Loc29", "GEN_ROD_01", 0);  // CGame_SayNoActor
        return;
    default:
        return;
    }
}

}  // namespace rah
