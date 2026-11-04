// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc28.hpp"

#include <bit>
#include <cstdlib>

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
float f32(std::uint32_t bits) { return std::bit_cast<float>(bits); }
constexpr std::size_t kRanamaRing = 0x3E8C;  // Szene 31 setzt 1: Ranama trägt danach den Ring (Loc06 S27 Ladebit 1, Hinweis 06_027_A)
constexpr std::size_t kTalk35 = 0x40C4;     // Gesprächsstufe Kapitän/Fremder (1…11)
constexpr std::size_t kFirstBush = 0x40C8;  // erster Besuch: 031_ROD_03
constexpr std::size_t kFirst32 = 0x40CC;    // erster Besuch Gasse: 032_ROD_01
constexpr std::size_t kParoleTalk = 0x40D0; // Wache fragt nach der Parole (einmal)
constexpr std::size_t kCart = 0x40D4;       // Wagen: 1 Kommentar, 2 kramen (Gegenstand 25), 3 leer
constexpr std::size_t kDungeon = 0x40D8;    // Kerker-Kommentare 1→2→3
constexpr std::size_t kGuard32 = 0x40DC;    // Gespräch mit der Wache in 32 (1…3)
constexpr std::size_t kStranger = 0x40E0;   // Fremder hat die Parole verraten
constexpr std::size_t kDisguised = 0x40E4;  // im Busch als Pirat verkleidet
constexpr std::size_t kBushTalk = 0x40E8;   // Busch-Stufe ohne Gegenstände (1…3)
constexpr std::size_t kOutside = 0x40F0;    // Rodrigo steht als Pirat vor dem Busch
constexpr int kScene31 = 0x1F, kScene32 = 0x20, kScene35 = 0x23;
}  // namespace

Loc28::Loc28(GameContext& ctx, int scene, int prevScene) : Game(ctx, 28, scene, prevScene) {
    getViewObjects();
    const char* musicName = nullptr;
    switch (sceneNr_) {
    case 0x1E:
    case 0x1F:
    case 0x24: {
        sceneNr_ = kScene31;
        scene_.load("S31", 0);  // CSzene_Load(DAT_004759F0, 0)
        getViewObjects();
        enterCity_ = loadAnim("S31_EnterCity");
        lid_ = obj("S31_Deckel");
        freArrives_ = obj("S31_FreArrives");
        freEnters_ = obj("S31_FreEntersCity");
        freParole_ = obj("S31_FreParole");
        freTalk_ = obj("S31_FreTalk");
        guard31_ = obj("S31_Wache");
        jasClimbsOut_ = obj("S31_JasClimbsOut");
        pirGlider_ = obj("S31_PirGlider");
        rodBushDownUp_ = obj("S31_RodBuschRunterRauf");
        rodBushTalk_ = obj("S31_RodBushTalk");
        rodClimbsOut_ = obj("S31_RodClimbsOut");
        rodEntersBush_ = obj("S31_RodEntersBush");
        rodGlider_ = obj("S31_RodGliderAnim");
        rodLeavesBush_ = obj("S31_RodLeavesBush");
        pirBushDownUp_ = obj("S31_RodPirBushRunterRauf");
        pirBushTalk_ = obj("S31_RodPirBushTalk");
        pirEntersBush_ = obj("S31_RodPirEntersBush");
        pirLeavesBush_ = obj("S31_RodPirLeavesBush");
        pirParole_ = obj("S31_RodPirParole");
        const auto cue = [](AnimLayer* a, int frame, AudioBuffer* b) {
            if (!a) return;
            AnimCue c;
            c.type = 1;
            c.frame = frame;
            c.buffer = b;
            a->addCue(c);  // CAnimLayer_AddCue 0x00442BB0
        };
        winding_ = loadSfx("Sfx_Winding");
        cue(lid_, 0, winding_.get());
        glider_ = loadSfx("Sfx_Glider_PassingBy");
        glider3_ = loadSfx("Sfx_Glider_PassingBy3");
        if (glider_) glider_->setFreq(0x3C);  // CAudioBuffer_SetFreq
        cue(pirGlider_, 0, glider3_.get());
        cue(rodGlider_, 1, glider_.get());
        stepL_ = loadSfx("Gen_StepLeft");
        stepR_ = loadSfx("Gen_StepRight");
        cue(rodClimbsOut_, 0x23, stepL_.get());
        cue(rodClimbsOut_, 0x27, stepR_.get());
        cue(rodClimbsOut_, 0x2F, stepL_.get());
        cue(jasClimbsOut_, 0x1F, stepL_.get());
        cue(jasClimbsOut_, 0x24, stepR_.get());
        cue(jasClimbsOut_, 0x30, stepL_.get());
        to30_ = scene_.getArea("S31_To30_113");
        to32_ = scene_.getArea("S31_To32");
        wall_ = scene_.getArea("S31_Wall");
        ambientTimer_ = std::make_unique<Event>();
        bird_ = loadSfx("Sfx_Bird");
        bird2_ = loadSfx("Sfx_Bird2");
        bird3_ = loadSfx("Sfx_Bird3");
        chirp_ = loadSfx("Sfx_Chirp");
        musicName = "Loc28_Endavin";
        break;
    }
    case kScene32: {
        scene_.load("S32", 0);  // CSzene_Load(DAT_0047583C, 0)
        getViewObjects();
        rodRummages_ = obj("S32_RodKramtAnim");
        guard32_ = obj("S32_Wache");
        static constexpr const char* kAreas[] = {"S32_To31",       "S32_Wagen",        "S32_FensterLinks",
                                                 "S32_FensterRechts", "S32_Kneipe",    "S32_Treppen",
                                                 "S32_SchuttLinks", "S32_SchuttRechts", "S32_FassLinks",
                                                 "S32_FassRechts",  "S32_Kerker"};
        for (std::size_t i = 0; i < 11; ++i) areas32_[i] = scene_.getArea(kAreas[i]);
        musicName = "Loc28_Endavin";
        break;
    }
    case kScene35:
        scene_.load("S35", 0);  // CSzene_Load(DAT_00475770, 0)
        getViewObjects();
        scene_.clearFlag510();  // CSzene_ClearFlag510 0x0044B190
        stranger_ = obj("S35_Fremder");
        captain_ = obj("S35_Captain");
        window_ = scene_.getObject("S35_Window");
        to34_ = scene_.getArea("S35_To34");
        to34b_ = scene_.getArea("S35_To34_2");
        talkTimer_ = std::make_unique<Event>();
        musicName = "Loc30_PrisionInEndavin";
        break;
    default:
        break;  // Original: kein PlayMusic, Hall auf die vorhandene Musik (+0x08)
    }
    if (musicName) music_ = playMusic(musicName);
    if (music_) {
        music_->setReverb(0x10, f32(0x3DE353F8), f32(0x40311687), f32(0x3E656042));
        music_->setReverbMix(f32(0x3DCCCCCD));  // 0x004375B0
    }
}

void Loc28::destroyLocal() {
    ambientTimer_.reset();
    talkTimer_.reset();
    enterCity_.reset();
    for (auto* b : {&glider_, &glider3_, &bird_, &bird2_, &bird3_, &chirp_, &winding_, &stepL_, &stepR_}) b->reset();
}

bool Loc28::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo / WalkTo2
    return wait();
}

bool Loc28::playEnd(AnimLayer* a, unsigned mode) {
    const int cb = beginWait();
    if (a) a->playEndEvent(mode, cb);
    return wait();
}

bool Loc28::playRange(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

bool Loc28::talk(const char* key, AnimLayer* speaker, int from, int to) {
    const int cb = beginWait();
    speak("Loc28", key, speaker, 0x0C, from, to, cb);  // CGame_Speak
    return wait();
}

bool Loc28::hasAllItems() {
    InventoryBar* bar = scene_.inventory();
    return bar && bar->has(1) && bar->has(3) && bar->has(4) && bar->has(0x0E);
}

void Loc28::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    switch (sceneNr_) {
    case kScene31:
        start31();
        return;
    case kScene32:
        start32();
        return;
    case kScene35:
        setEventProc(&Loc28::onEvent35);
        for (Layer* l : {static_cast<Layer*>(stranger_), static_cast<Layer*>(captain_), window_})
            if (l) l->enable();
        if (to34_) to34_->setEvent(0x1F);
        if (to34b_) to34b_->setEvent(0x20);
        setTimerHandler(*talkTimer_, 1000, 0, 0x21);
        scene_.enableInput();
        return;
    default:
        return;
    }
}

void Loc28::strangerSequence() {
    playEnd(freArrives_, 10);
    if (freArrives_) freArrives_->disable();
    if (freTalk_) freTalk_->enable();
    talk("031_PW1_01", guard31_, 0x1E, 0x2B);
    talk("031_FRE_01", freTalk_);
    talk("031_PW1_02b", guard31_, 0x1E, 0x2B);
    if (freTalk_) freTalk_->disable();
    playEnd(freParole_, 10);
    if (freParole_) freParole_->disable();
    if (freEnters_) freEnters_->enable();
    talk("031_PW1_02", guard31_, 0x1E, 0x2B);
    playEnd(freEnters_, 10);
    if (freEnters_) freEnters_->disable();
    talk("031_ROD_02", rodBushTalk_);
    flag(kStranger) = 1;
}

void Loc28::start31() {
    setEventProc(&Loc28::onEvent31);
    flag(kRanamaRing) = 1;
    if (lid_) lid_->enable();
    if (guard31_) guard31_->enable();
    InventoryBar* bar = scene_.inventory();
    if (restored_ == 0) {
        if (prevScene_ == kScene32) {
            // aus der Stadt zurück
            if (hero_) hero_->show(scene_.clicksEnabled);
            scene_.disableInput();
            if (hero_) hero_->setPosDir(0x104, 0xF4, 2);
            walk(200, 0xE9, 2);  // CActor_WalkTo2
            speak("Loc28", "031_PW1_13", guard31_, 0x0C, 0x1E, 0x2B, 0);
            walk(0x5A, 0x106, 6);
            flag(kOutside) = 1;
            scene_.enableInput();
        } else if (prevScene_ == 0x22) {
            // Flucht aus dem Kerker mit Jasper, Abflug mit dem Gleiter
            scene_.disableInput();
            delay(1000);
            for (const char* k : {"031_ROD_17", "031_JAS_17", "031_ROD_18", "031_JAS_18", "031_ROD_19"}) {
                const int cb = beginWait();
                speak("Loc28", k, cb);  // CGame_SayNoActor
                wait();
            }
            playEnd(lid_, 11);
            if (lid_) lid_->setPosZ(500.0, 232.0, 159.0, 100.0);  // 0x004474C0
            if (rodClimbsOut_) rodClimbsOut_->start(10);
            delay(0x514);
            playEnd(jasClimbsOut_, 10);
            {
                const int cb = beginWait();
                speak("Loc28", "031_PW2_20", cb);
                wait();
            }
            playRange(guard31_, 11, 0x5B, 100);
            {
                const int cb = beginWait();
                sayActor("Loc28", "031_PW1_20", guard31_, cb);
                wait();
            }
            playRange(guard31_, 11, 0x69, 0x75);
            playEnd(rodGlider_, 10);
            if (guard31_) guard31_->playRange(11, 0, 0x76, -1);
            playEnd(pirGlider_, 10);
            changeScene(0x42, 10);
        } else {
            // Rodrigo versteckt sich im Busch
            playEnd(rodEntersBush_, 10);
            if (rodBushTalk_) rodBushTalk_->enable();
            if (bar && bar->has(1) && flag(kStranger) == 0 && flag(kFirstBush) == 0 && random(1, 3) == 3) {
                strangerSequence();
            } else if (flag(kFirstBush) != 0) {
                talk("031_ROD_03", rodBushTalk_);
                flag(kFirstBush) = 0;
            }
            if (hasAllItems() && flag(kStranger) == 0) strangerSequence();
        }
    } else if (hero_) {
        hero_->show(scene_.clicksEnabled);
    }
    if (to30_) to30_->setEvent(1);
    if (to32_) to32_->setEvent(2);
    if (wall_) wall_->setEvent(3);
    if (guard31_ && guard31_->area) guard31_->area->setEvent(4);
    if (hero_) hero_->blockRect(0xD7, 0xE8, 0xE6, 0xF4);
    if (bar) {
        if (bar->has(1)) bar->setItemEvent(1, 5);
        if (bar->has(4)) bar->setItemEvent(4, 6);
        if (bar->has(3)) bar->setItemEvent(3, 8);
        if (bar->has(0x0E)) bar->setItemEvent(0x0E, 7);
    }
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(5, 15) * 1000), 0, 1000);
    scene_.enableInput();
}

void Loc28::start32() {
    setEventProc(&Loc28::onEvent32);
    if (guard32_) guard32_->enable();
    for (int i = 0; i < 11; ++i)
        if (areas32_[i]) areas32_[i]->setEvent(0x0B + i);
    if (guard32_ && guard32_->area) guard32_->area->setEvent(0x16);
    if (InventoryBar* bar = scene_.inventory()) {
        bar->setItemEvent(1, 0x17);
        bar->setItemEvent(4, 0x18);
        bar->setItemEvent(3, 0x1A);
        bar->setItemEvent(0x0E, 0x19);
    }
    if (hero_) hero_->show(scene_.clicksEnabled);
    if (restored_ == 0 && hero_) {
        if (prevScene_ == kScene31)
            hero_->setPosDir(0x69, 0x167, 6);
        else if (prevScene_ == 0x21)
            hero_->setPosDir(0xF7, 0x112, 6);
    }
    if (flag(kFirst32) != 0) {
        flag(kFirst32) = 0;
        const int cb = beginWait();
        speak("Loc28", "032_ROD_01", hero_, 0x0C, 0, 8, cb);
        if (!wait()) return;
    }
    scene_.enableInput();
}

void Loc28::toggleDisguise() {
    scene_.disableInput();
    if (flag(kDisguised) == 0) {
        flag(kDisguised) = 1;
        talk("031_ROD_22", rodBushTalk_);
        if (rodBushTalk_) rodBushTalk_->disable();
        playEnd(rodBushDownUp_, 10);
        playRange(pirBushDownUp_, 10, 10, 0);
        if (pirBushTalk_) pirBushTalk_->enable();
    } else {
        flag(kDisguised) = 0;
        talk("031_ROD_04", pirBushTalk_);
        if (pirBushTalk_) pirBushTalk_->disable();
        playEnd(pirBushDownUp_, 10);
        playRange(rodBushDownUp_, 10, 10, 0);
        if (rodBushTalk_) rodBushTalk_->enable();
    }
    scene_.enableInput();
}

void Loc28::gateOrBush() {
    if (flag(kOutside) != 0) {
        // als Pirat durchs Stadttor
        if (hero_) hero_->blockRect(0xD7, 0xE8, 0xE6, 0xF4);
        if (!walk(0xF4, 0xF5, 5)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        scene_.setModal(enterCity_.get());
        playEnd(enterCity_.get(), 11);
        changeScene(kScene32, 0x1C);
        return;
    }
    InventoryBar* bar = scene_.inventory();
    if (!hasAllItems()) {
        if (bar && (bar->has(1) || bar->has(3) || bar->has(4) || bar->has(0x0E))) {
            talk("031_ROD_09", rodBushTalk_);
            return;
        }
        if (flag(kDisguised) != 0) return;
        switch (flag(kBushTalk)) {
        case 1:
            talk("031_ROD_05", rodBushTalk_);
            flag(kBushTalk) = 2;
            return;
        case 2:
            flag(kBushTalk) = 3;
            scene_.disableInput();
            talk("031_ROD_06", rodBushTalk_);
            speak("Loc28", "031_PW2_06", 0);  // CGame_SayNoActor ohne Warten
            delay(2000);
            playRange(guard31_, 11, 0x32, 0x3C);
            talk("031_PW1_06", guard31_, 0x3C, 0x46);
            {
                const int cb = beginWait();
                speak("Loc28", "031_PW2_07", cb);
                wait();
            }
            if (guard31_) guard31_->playRange(11, 0, 0x50, 0x5A);
            talk("031_ROD_07", rodBushTalk_);
            scene_.enableInput();
            return;
        case 3:
            talk("031_ROD_08", rodBushTalk_);
            return;
        default:
            return;
        }
    }
    // alle vier Teile der Verkleidung: als Pirat aus dem Busch
    scene_.disableInput();
    if (flag(kDisguised) == 0) {
        flag(kDisguised) = 1;
        if (rodBushTalk_) rodBushTalk_->disable();
        playEnd(rodBushDownUp_, 10);
        playRange(pirBushDownUp_, 10, 10, 0);
    } else if (pirBushTalk_) {
        pirBushTalk_->disable();
    }
    playEnd(pirLeavesBush_, 10);
    delay(2000);
    flag(kOutside) = 1;
    if (hero_) {
        hero_->show(scene_.clicksEnabled);
        hero_->setPosDir(4, 0x108, 6);
    }
    walk(0x5A, 0x106, 6);  // CActor_WalkTo2
    if (flag(kParoleTalk) != 0) {
        flag(kParoleTalk) = 0;
        talk("031_PW1_11", guard31_);
        talk("031_ROD_11", hero_);
        talk("031_PW1_12b", guard31_);
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playEnd(pirParole_, 10);
        if (hero_) hero_->show(scene_.clicksEnabled);
        talk("031_PW1_12", guard31_);
    }
    scene_.enableInput();
}

void Loc28::onEvent31(Event& e, int) {
    const auto itemOrDisguise = [this](const char* key, bool waitAfter) {
        if (flag(kOutside) != 0) {
            faceAndSay("GEN_ROD_01");
            return;
        }
        if (hasAllItems()) {
            toggleDisguise();
            if (waitAfter) wait();  // Original: CGame_Wait ohne BeginWait (Platz frei → sofort)
            return;
        }
        talk(key, rodBushTalk_);
    };
    switch (e.param) {
    case 1:  // → 113 (Gleiter) bzw. zurück in den Busch
        if (flag(kOutside) == 0) {
            if (flag(kDisguised) != 0) {
                talk("031_ROD_04", pirBushTalk_);
                if (pirBushTalk_) pirBushTalk_->disable();
                playRange(pirBushDownUp_, 10, 0, 10);
                playRange(rodBushDownUp_, 10, 10, 0);
                if (rodBushTalk_) rodBushTalk_->enable();
                flag(kDisguised) = 0;
            }
            if (rodBushTalk_) rodBushTalk_->disable();
            playEnd(rodLeavesBush_, 11);
            changeScene(0x71, 0x1F);  // Original: ohne Prüfung des Wartens
            return;
        }
        if (!walk(4, 0xA0, 2)) return;
        flag(kOutside) = 0;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        delay(2000);
        playEnd(pirEntersBush_, 10);
        if (pirBushTalk_) pirBushTalk_->enable();
        scene_.enableInput();
        return;
    case 2:
        gateOrBush();
        return;
    case 3:
        talk("031_ROD_10", rodBushTalk_);
        return;
    case 4:  // Wache
        if (flag(kOutside) == 0) {
            gateOrBush();
            return;
        }
        if (!walk(0xC6, 0x88, 6)) return;
        switch (random(1, 3)) {
        case 1:
            talk("031_ROD_14", hero_);
            talk("031_PW1_14", guard31_);
            return;
        case 2:
            talk("031_ROD_15", hero_);
            talk("031_PW1_15", guard31_);
            return;
        case 3:
            talk("031_ROD_16", hero_);
            talk("031_PW1_16", guard31_);
            return;
        default:
            return;
        }
    case 5: itemOrDisguise("031_ROD_25", true); return;
    case 6: itemOrDisguise("031_ROD_26", true); return;
    case 8: itemOrDisguise("031_ROD_26b", true); return;
    case 7:
        if (flag(kOutside) != 0) {
            faceAndSay("GEN_ROD_01");
            return;
        }
        if (hasAllItems()) {
            toggleDisguise();
            return;
        }
        {
            const int r = random(1, 2);
            if (r == 1) talk("031_ROD_23", rodBushTalk_);
            else if (r == 2) talk("031_ROD_24", rodBushTalk_);
        }
        return;
    case 1000: {
        const int r = random(1, 4);
        AudioBuffer* birds[] = {bird_.get(), bird2_.get(), bird3_.get(), chirp_.get()};
        if (r >= 1 && r <= 4) {
            const int v = random(0x32, 100);
            if (AudioBuffer* b = birds[r - 1]) {
                b->setVolume(v);  // CAudioBuffer_SetVolume
                playSfxRandom3D(*b);
            }
        }
        setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(2, 10) * 1000), 0, 1000);
        return;
    }
    default:
        return;
    }
}

void Loc28::onEvent32(Event& e, int) {
    switch (e.param) {
    case 0x0B:
        if (walk(0x69, 0x167, 2)) changeScene(kScene31, 0x1C);
        return;
    case 0x0C: walkAndComment(0x89, 0x137, 4, "032_ROD_13"); return;
    case 0x0D: walkAndComment(0xB3, 0x120, 4, "032_ROD_14"); return;
    case 0x0E: walkAndComment(0x130, 0x118, 4, "032_ROD_15"); return;
    case 0x0F:  // Kneipe → 29/33
        if (walk(0x100, 0x124, 4)) changeScene(0x21, 0x1D);
        return;
    case 0x10: walkAndComment(0x169, 0x80, 6, "032_ROD_16"); return;
    case 0x11: {
        const int r = random(1, 2);
        if (r == 1) walkAndComment(0x17E, 0xA9, 4, "032_ROD_02");
        else if (r == 2) walkAndComment(0x17E, 0xA9, 4, "032_ROD_03");
        return;
    }
    case 0x12: {  // Schutt rechts / Wagen: Gegenstand 25
        const int s = flag(kCart);
        const bool there = hero_ && std::abs(hero_->footX - 0x251) <= 9 && std::abs(hero_->footY - 0xCB) <= 9;
        if (s == 1 || (s == 2 && !there)) {
            walkAndComment(0x251, 0xCB, 6, "032_ROD_04");
            flag(kCart) = 2;
            return;
        }
        if (s == 2) {
            scene_.disableInput();
            flag(kCart) = 3;
            if (!walk(0x251, 0xCB, 6)) return;
            if (hero_) hero_->hide(scene_.clicksEnabled);
            playEnd(rodRummages_, 11);
            if (InventoryBar* bar = scene_.inventory()) bar->add(0x19);
            if (rodRummages_) rodRummages_->disable();
            if (hero_) hero_->show(scene_.clicksEnabled);
            walkAndComment(0x251, 0xCB, 6, "032_ROD_05");
            scene_.enableInput();
            return;
        }
        if (s == 3) walkAndComment(0x251, 0xCB, 6, "032_ROD_06");
        return;
    }
    case 0x13: walkAndComment(0x1E2, 0xEC, 4, "032_ROD_17"); return;
    case 0x14: walkAndComment(0x21B, 0xE0, 4, "032_ROD_18"); return;
    case 0x15: {  // Kerker
        const char* key = nullptr;
        switch (flag(kDungeon)) {
        case 1: flag(kDungeon) = 2; key = "032_ROD_07"; break;
        case 2: flag(kDungeon) = 3; key = "032_ROD_08"; break;
        case 3: key = "032_ROD_09"; break;
        default: return;
        }
        walkAndComment(0x173, 0xD3, 3, key);
        return;
    }
    case 0x16: {  // Wache
        if (!walk(0xE2, 0x14F, 1)) return;
        const auto pair = [this](const char* rod, const char* pw) {
            {
                const int cb = beginWait();
                sayActor("Loc28", rod, hero_, cb);
                wait();
            }
            const int cb = beginWait();
            sayActor("Loc28", pw, guard32_, cb);
            wait();
        };
        switch (flag(kGuard32)) {
        case 1: pair("032_ROD_10", "032_PW2_10"); flag(kGuard32) = 2; return;
        case 2: pair("032_ROD_11", "032_PW2_11"); flag(kGuard32) = 3; return;
        case 3: pair("032_ROD_12", "032_PW2_12"); return;
        default: return;
        }
    }
    case 0x17:
    case 0x18:
    case 0x19:
    case 0x1A:
        faceAndSay("GEN_ROD_01");
        return;
    default:
        return;
    }
}

void Loc28::onEvent35(Event& e, int) {
    switch (e.param) {
    case 0x1F:
    case 0x20:
        changeScene(0x22, 0x1E);
        return;
    case 0x21: {
        // Kapitän und Fremder unterhalten sich; jede Zeile meldet sich mit 0x22 zurück
        static constexpr const char* kLines[] = {"035_CAP_01", "035_FRE_01", "035_CAP_02", "035_FRE_02",
                                                 "035_CAP_03", "035_FRE_03", "035_CAP_04", "035_FRE_04",
                                                 "035_CAP_05", "035_FRE_05"};
        const int s = flag(kTalk35);
        if (s >= 1 && s <= 10) {
            AnimLayer* who = (s & 1) ? captain_ : stranger_;
            speak("Loc28", kLines[s - 1], who, 0x0C, 0, 8, 0x22);
            flag(kTalk35) = s + 1;
            return;
        }
        if (s != 0x0B) return;
        scene_.disableInput();
        talk("035_CAP_06", captain_);
        playRange(captain_, 11, 0x1F, 0x28);
        if (captain_) captain_->playRange(11, 0, 0x29, 0x30);
        talk("035_FRE_06", stranger_);
        playRange(captain_, 11, 0x31, 0x39);
        talk("035_FRE_07", stranger_);
        playRange(captain_, 11, 0x3A, -1);
        flag(kTalk35) = 1;
        scene_.enableInput();
        if (talkTimer_) setTimerHandler(*talkTimer_, 1000, 0, 0x21);
        return;
    }
    case 0x22:
        if (talkTimer_) setTimerHandler(*talkTimer_, 200, 0, 0x21);
        return;
    default:
        return;
    }
}

}  // namespace rah
