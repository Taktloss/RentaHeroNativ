// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc15.hpp"

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
constexpr std::size_t kNight = 0x3E80;         // Wald bei Nacht
constexpr std::size_t kKanal3EFC = 0x3EFC;     // Loc07 (Kanal) – beim Einstieg zurückgesetzt
constexpr std::size_t kKanal3F00 = 0x3F00;
constexpr std::size_t kKanal3F04 = 0x3F04;     // beim Einstieg in den Kanal gesetzt
constexpr std::size_t kGliderHere37 = 0x3F24;  // Gleiter steht in Szene 37
constexpr std::size_t kBush38 = 0x3F38;        // Busch in Szene 38 (Weg nach 57) weg
constexpr std::size_t kPirate = 0x3F90;        // Pirat schaut heraus (aus Szene 53 mit Ankunft 572)
constexpr std::size_t kSay58 = 0x3F94;         // Kommentar beim ersten Weg nach 58
constexpr std::size_t kPirateSeen = 0x3F98;    // Pirat gesehen (liest Loc31)
constexpr std::size_t kGliderHere = 0x3F9C;    // Gleiter steht in Szene 57
constexpr std::size_t kSack = 0x3FA0;          // Geldsack liegt (Nacht; gesetzt in Loc12)
constexpr int kAmbient = 0x0B;
constexpr int kBobHandler = 0x0041B200;        // Funktionszeiger als Handler-Wert
}  // namespace

Loc15::Loc15(GameContext& ctx, int scene, int prevScene) : Game(ctx, 15, scene, prevScene) {
    g_.fnHandlers[kBobHandler] = {this, [this](Event&, int) { bobGlider(); }};
    getViewObjects();
    // Szenen-Varianten der Anfrage: 571 → Nacht, 570 → Tag, 572 → Pirat
    if (sceneNr_ == 0x23B) flag(kNight) = 1;
    else if (sceneNr_ == 0x23A) flag(kNight) = 0;
    else if (sceneNr_ == 0x23C) flag(kPirate) = 1;
    sceneNr_ = 0x39;
    ambientTimer_ = std::make_unique<Event>();
    unsigned mask;
    if (flag(kNight) == 0) {
        mask = 1;
        bird_ = loadSfx("Sfx_Bird");
        bird2_ = loadSfx("Sfx_Bird2");
        bird3_ = loadSfx("Sfx_Bird3");
        chirp_ = loadSfx("Sfx_Chirp");
        danger_ = loadSfx("Sfx_DangerStringsLow");
        passing_ = loadSfxVol("Sfx_Glider_PassingBy", 0x46);
        if (prevScene_ == 0x71) flag(kGliderHere) = 1;
    } else {
        flag(kGliderHere) = 0;
        mask = 2;
        owl_ = loadSfx("Sfx_Owl");
        thunder_ = loadSfx("Sfx_Thunder");
        thunder2_ = loadSfx("Sfx_Thunder2");
        if (prevScene_ == 0x71) prevScene_ = 0x3A;
    }
    if (prevScene_ == 0x35) mask |= 8;
    else flag(kPirate) = 0;
    if (flag(kGliderHere) != 0) mask |= 4;
    scene_.load("S57", mask);  // CSzene_Load(DAT_004738A0, maske)
    getViewObjects();
    const char* musicName;
    if (flag(kNight) == 0) {
        rodIn_ = obj("S57_RodRein_vd");
        rodOut_ = obj("S57_RodRaus_vd");
        if (hero_) hero_->setColor(-10, -34, -52);  // CLayer_SetColor 0x004473E0
        rodLooks_ = obj("S57_RodSchaut");
        gliderGone_ = obj("S57_GleiterWeg");
        rodDismount_ = obj("S57_RodAbsteig");
        glider_ = scene_.getObject("S57_Gleiter");
        gliderArrives_ = scene_.getObject("S57_GleiterHin");
        pirate_ = obj("S57_PiratGuck");
        musicName = "Loc10_ForestBeforeDarkness";
    } else {
        scene_.replaceBackground("S57_Back_nd");  // 0x0043FD90(…, 1)
        rodIn_ = obj("S57_RodRein_nd");
        rodOut_ = obj("S57_RodRaus_nd");
        pickUp_ = obj("Gen_RodPickUp");
        if (pickUp_) pickUp_->setColor(-122, -84, -24);
        sack_ = scene_.getObject("S57_Sack");
        if (hero_) hero_->setColor(-122, -84, -24);
        musicName = "Loc10_ForestInDarkness";
    }
    music_ = playMusic(musicName);
    kanal_ = scene_.getArea("S57_Kanal");
    to38_ = scene_.getArea("S57_To38");
    to58_ = scene_.getArea("S57_To58");
    gorge_ = scene_.getArea("S57_Schlucht");
    scratch_ = loadSfxVol("Sfx_Clothes_Scratched", 0x46);
    const auto cue = [](AnimLayer* L, int frame, AudioBuffer* b) {
        if (!L) return;
        AnimCue c;
        c.type = 1;
        c.frame = frame;
        c.buffer = b;
        L->addCue(c);  // CAnimLayer_AddCue 0x00442BB0
    };
    cue(rodIn_, 9, scratch_.get());
    cue(rodOut_, 3, scratch_.get());
    if (flag(kGliderHere) != 0 && hero_) hero_->blockRect(0x73, 0x13C, 0x118, 0x144);
    bob_ = 0;
    if (music_) {
        music_->setReverb(4, 0.208f, 0.478f, 0.0f);  // 0x3E54FDF4, 0x3EF4BC6A, 0
        music_->setReverbMix(0.1f);                  // 0x3DCCCCCD
    }
}

Loc15::~Loc15() {
    shutdown();
    const auto it = g_.fnHandlers.find(kBobHandler);
    if (it != g_.fnHandlers.end() && it->second.owner == this) g_.fnHandlers.erase(it);
}

void Loc15::destroyLocal() {
    // Reihenfolge wie 0x0041A7A0
    ambientTimer_.reset();
    for (auto* b : {&bird_, &bird2_, &bird3_, &chirp_, &owl_, &thunder_, &thunder2_, &danger_, &passing_, &scratch_})
        b->reset();
}

bool Loc15::walk(int x, int y, int dir) {
    const int cb = beginWait();
    if (hero_) hero_->walkTo(x, y, cb, dir);  // CActor_WalkTo
    return wait();
}

bool Loc15::playEnd(AnimLayer* a, unsigned mode) {
    const int cb = beginWait();
    if (a) a->playEndEvent(mode, cb);
    return wait();
}

bool Loc15::playRange(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

bool Loc15::moveWait(Layer* l, double x, double y, int steps) {
    const int cb = beginWait();
    if (l) l->moveTo(x, y, 0.0, 0.0, steps, cb);  // CLayer_MoveTo 0x004475B0
    return wait();
}

void Loc15::bobGlider() {
    double y;
    if (bob_ == 0) {
        bob_ = 1;
        y = 207.0;  // 0x4069E000
    } else if (bob_ == 1) {
        bob_ = 0;
        y = 209.0;  // 0x406A2000
    } else {
        return;
    }
    if (glider_) glider_->moveTo(0.0, y, 0.0, 0.0, 2, kBobHandler);
}

void Loc15::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    setEventProc(&Loc15::onEvent);
    setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(5, 15) * 1000), 0, kAmbient);
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();  // +0x60
    if (kanal_) kanal_->setEvent(1);
    if (to38_) to38_->setEvent(2);
    if (to58_) to58_->setEvent(3);
    if (gorge_) gorge_->setEvent(4);
    if (flag(kSack) != 0 && sack_) {
        sack_->enable();
        if (sack_->area) sack_->area->setEvent(6);
    }
    if (flag(kGliderHere) != 0) {
        if (glider_ && glider_->area) glider_->area->setEvent(5);
        if (glider_) glider_->enable();
        bobGlider();
    }
    bool show = true;
    if (prevScene_ == 0x26) {
        if (restored_ == 0) {
            if (hero_) {
                hero_->setPosDir(0xD4, 0xB2, 0);
                hero_->walkTo(0x111, 0x14B, 0, 0);  // CActor_WalkTo2
            }
            show = false;
        }
    } else if (prevScene_ == 0x35) {
        if (restored_ == 0) {
            // Rodrigo klettert aus dem Kanal
            playEnd(rodOut_, 10);
            if (hero_) {
                hero_->setPosDir(0x28, 0x142, 6);
                hero_->show(scene_.clicksEnabled);
                hero_->walkTo(100, 0x142, 0, 6);
            }
            show = false;
        }
    } else if (prevScene_ == 0x71) {
        if (restored_ == 0) {
            // Gleiter kommt angeflogen und landet
            if (glider_) glider_->disable();
            if (gliderArrives_) {
                gliderArrives_->enable();
                gliderArrives_->fps = 0x14;           // +0x54
                gliderArrives_->setPos(0x249, 0x80);  // vtable +0x14
            }
            if (passing_) {
                passing_->play();
                passing_->fadeFreq(0x19, 2000);
            }
            moveWait(gliderArrives_, 227.0, 198.0, 0x32);  // 0x406C6000, 0x4068C000
            moveWait(gliderArrives_, 127.0, 208.0, 0x28);  // 0x405FC000, 0x406A0000
            delay(500);
            if (gliderArrives_) gliderArrives_->disable();
            playEnd(rodDismount_, 10);
            if (hero_) hero_->setPosDir(0xCD, 0x136, 0);
        }
        if (hero_) hero_->show(scene_.clicksEnabled);
        if (glider_) {
            glider_->enable();
            if (glider_->area) glider_->area->active = true;
        }
        show = false;
    } else if (restored_ == 0) {
        if (hero_) {
            hero_->setPosDir(0x23A, 300, 2);
            hero_->walkTo(0x1C5, 0x13F, 0, 1);
        }
        show = false;
    }
    if (show && hero_) hero_->show(scene_.clicksEnabled);
    if (flag(kPirate) == 0) {
        if (gorge_) gorge_->active = false;  // +0xB8 = 0
        scene_.enableInput();
        return;
    }
    // Pirat schaut kurz aus der Schlucht
    flag(kPirate) = 0;
    flag(kPirateSeen) = 1;
    if (pirate_) pirate_->enable();
    if (danger_) danger_->play();
    moveWait(pirate_, -10.0, 0.0, 10);  // 0xC0240000
    playEnd(pirate_, 11);
    delay(300);
    moveWait(pirate_, -30.0, 0.0, 5);  // 0xC03E0000
    if (pirate_) pirate_->disable();
    scene_.enableInput();
}

void Loc15::onEvent(Event& e, int) {
    switch (e.param) {
    case 1:
        // in den Kanal
        if (!walk(0x2C, 0x143, 2)) return;
        scene_.disableInput();
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playEnd(rodIn_, 10);
        bob_ = 2;
        if (flag(kGliderHere) != 0 || flag(kGliderHere37) != 0 || flag(kNight) != 0) {
            flag(kKanal3F04) = 1;
            flag(kKanal3EFC) = 0;
            flag(kKanal3F00) = 0;
        }
        scene_.enableInput();
        changeScene(0x35, 7);
        return;
    case 2:
        if (!walk(0xD4, 0xB2, 4)) return;
        flag(kBush38) = 1;
        changeScene(0x26, 10);
        return;
    case 3:
        if (flag(kSay58) != 0) {
            flag(kSay58) = 0;
            heroSay("057_ROD_02");
        }
        if (!walk(0x23A, 300, 6)) return;
        changeScene(0x3A, 0x10);
        return;
    case 4:
        // in die Schlucht schauen
        if (!walk(0x6D, 0x132, 6)) return;
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playEnd(rodLooks_, 10);
        if (hero_) hero_->show(scene_.clicksEnabled);
        heroSay("057_ROD_01");
        return;
    case 5:
        // mit dem Gleiter abfliegen
        if (!walk(0xCD, 0x136, 0)) return;
        scene_.disableInput();
        bob_ = 2;
        flag(kGliderHere) = 0;
        if (glider_) glider_->disable();
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playRange(gliderGone_, 11, 0, 0x30);
        if (passing_) {
            passing_->setFreq(0x19);
            passing_->play();
            passing_->fadeFreq(100, 2000);
        }
        playRange(gliderGone_, 10, 0x31, -1);
        changeScene(0x71, 0x1F);
        return;
    case 6:
        // Geldsack aufheben (Nacht)
        if (!walk(0x14, 0x164, 4)) return;
        scene_.disableInput();
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playRange(pickUp_, 11, 0, 7);
        if (sack_) sack_->disable();
        if (auto* bar = scene_.inventory()) bar->add(7);  // Inventory_Add(+0x74, 7)
        flag(kSack) = 0;
        playRange(pickUp_, 10, 8, -1);
        if (hero_) hero_->show(scene_.clicksEnabled);
        scene_.enableInput();
        heroSay("057_ROD_03");
        return;
    case kAmbient: {
        AudioBuffer* b = nullptr;
        if (flag(kNight) != 0) {
            switch (random(1, 4)) {
            case 1:
            case 2: b = owl_.get(); break;
            case 3: b = thunder_.get(); break;
            case 4: b = thunder2_.get(); break;
            default: break;
            }
        } else {
            std::unique_ptr<AudioBuffer>* day[] = {&bird_, &bird2_, &bird3_, &chirp_};
            const int k = random(1, 4);
            if (k >= 1 && k <= 4) {
                b = day[k - 1]->get();
                const int vol = random(0x32, 100);
                if (b) b->setVolume(vol);
            }
        }
        if (b) playSfxRandom3D(*b);
        setTimerHandler(*ambientTimer_, static_cast<std::uint32_t>(random(5, 10) * 1000), 0, kAmbient);
        return;
    }
    default:
        return;
    }
}

}  // namespace rah
