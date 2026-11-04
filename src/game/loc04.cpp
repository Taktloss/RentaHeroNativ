// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc04.hpp"

#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
constexpr std::size_t kRamil = 0x3E84;      // 1 = Ramil kommt, 2 = Ramil war da
constexpr std::size_t kRamilTold = 0x3E88;  // Ramil hat seine Geschichte erzählt
constexpr std::size_t kVisits = 0x3ED0;     // Besuche im Büro (Start zählt +1)
constexpr std::size_t kSackInBox = 0x3ED4;  // Geldsack liegt in der Kiste (Gegenstand 7)
constexpr int kUseHandler = 0x0040CF50;     // Inventar: CGame_DescribeItem(typ)
constexpr int kClickEvent = 30;             // Klick ins Leere: aufstehen
}  // namespace

Loc04::Loc04(GameContext& ctx, int scene, int prevScene) : Game(ctx, 4, scene, prevScene) {
    g_.fnHandlers[kUseHandler] = {this, [this](Event&, int t) { describeItem(t); }};
    getViewObjects();
    sceneNr_ = 6;
    // Sprungziele mit vorgegebenem Zustand
    switch (prevScene_) {
    case 1001:
        flag(kSackInBox) = 0;
        flag(kRamil) = 0;
        flag(kVisits) = 0;
        break;
    case 1002:
        flag(kSackInBox) = 0;
        flag(kRamil) = 0;
        flag(kVisits) = 1;
        break;
    case 1003:
        flag(kSackInBox) = 1;
        flag(kVisits) = 2;
        break;
    case 1004:
        flag(kRamil) = 1;
        flag(kSackInBox) = 0;
        flag(kVisits) = 3;
        break;
    default: break;
    }
    unsigned mask = 0;
    if (flag(kRamil) == 1 || flag(kVisits) == 0) {
        scene_.load("006_rodtalk", 0xFFFF);
        getViewObjects();
        rodTalk_ = obj("RodTalk");
        bgRodTalk_ = scene_.background();
    }
    if (flag(kRamil) == 1) {
        scene_.setBackground(nullptr);
        scene_.load("006_ramtalk", 0xFFFF);
        getViewObjects();
        ramTalk_ = obj("RamTalk");
        rodSitAct1_ = obj("RodSitAct1");
        rodSitAct2_ = obj("RodSitAct2");
        bgRamTalk_ = scene_.background();
        depthRam_ = scene_.depthMap();
        doorRamil_ = scene_.getArea("006_DoorRamil");
        newsRamil_ = scene_.getArea("006_NewsRamil");
        barrelRamil_ = scene_.getArea("006_FassRamil");
        if (rodSitAct1_ && rodSitAct1_->area) rodSitAct1_->area->active = false;
        if (rodSitAct2_ && rodSitAct2_->area) rodSitAct2_->area->active = false;
        ramilIn_ = loadAnim("RamilRein");
        ramilOut_ = loadAnim("RamilRaus");
        ramilMoney_ = loadAnim("RamilGeld");
        mask = 3;
    }
    if (flag(kSackInBox) != 0) mask |= 2;
    scene_.setBackground(nullptr);
    scene_.setDepth(nullptr);  // 0x0044A140(0)
    scene_.load("006", mask);
    getViewObjects();
    bgMain_ = scene_.background();
    depthMain_ = scene_.depthMap();
    if (mask & 2) rodTake_ = obj("RodTake");
    rodDeskTalk_ = obj("RodDeskTalk");
    if (rodDeskTalk_ && rodDeskTalk_->area) rodDeskTalk_->area->active = false;
    rodSitLayer_ = obj("RodSetzenLayer");
    if (rodSitLayer_ && rodSitLayer_->area) rodSitLayer_->area->active = false;
    desktop_ = scene_.getArea("006_Desktop");
    boar_ = scene_.getArea("006_Eber");
    box_ = scene_.getArea("006_Kiste");
    elephant_ = scene_.getArea("006_Elefant");
    cabinet_ = scene_.getArea("006_Kasten");
    door_ = scene_.getArea("006_Door");
    barrel_ = scene_.getArea("006_Fass");
    news_ = scene_.getArea("006_News");
    drawer2_ = scene_.getArea("006_Lade2");
    drawer3_ = scene_.getArea("006_Lade3");
    drawer4_ = scene_.getArea("006_Lade4");
    rodIn_ = loadAnim("RodRein");
    rodOut_ = loadAnim("RodRaus");
    rodSit_ = loadAnim("RodSetzen");
    rummage_ = loadSfxVol("Sfx_Kramen", 50);
    moneyBag_ = loadSfxVol("Sfx_MoneyBag", 70);
    doorCreak_ = loadSfxVol("Sfx_Door_CreakShort", 50);
    tock_ = loadSfxVol("Sfx_Tock", 70);
    stepLeft_ = loadSfxVol("Gen_StepLeft", 75);
    stepRight_ = loadSfxVol("Gen_StepRight", 75);
    tockSoft_ = loadSfx("Sfx_TockSoft");
    music_ = playMusic("Loc04_RodrigosOffice");
    if (music_) {
        music_->setReverb(4, 0.208f, 0.478f, 0.0f);  // 0x3E54FDF4, 0x3EF4BC6A, 0
        music_->setReverbMix(0.1f);                  // 0x3DCCCCCD
    }
}

Loc04::~Loc04() {
    shutdown();
    const auto it = g_.fnHandlers.find(kUseHandler);
    if (it != g_.fnHandlers.end() && it->second.owner == this) g_.fnHandlers.erase(it);
}

void Loc04::destroyLocal() {
    // Hintergründe und Tiefenkarten (+0x11C…+0x12C): Port – gehören der Szene bis reset()
    rodIn_.reset();
    rodOut_.reset();
    rodSit_.reset();
    ramilIn_.reset();
    ramilOut_.reset();
    ramilMoney_.reset();
    rummage_.reset();
    moneyBag_.reset();
    doorCreak_.reset();
    tock_.reset();
    stepLeft_.reset();
    stepRight_.reset();
    tockSoft_.reset();
}

bool Loc04::playWait(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

void Loc04::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    ++flag(kVisits);
    setEventProc(&Loc04::onEvent);
    if (auto* bar = scene_.inventory()) bar->setUseHandler(kUseHandler);  // 0x0043FA60
    const std::pair<Area*, int> events[] = {{desktop_, 1}, {boar_, 2},    {box_, 3},      {elephant_, 4},
                                            {cabinet_, 5}, {door_, 6},    {barrel_, 11},  {news_, 12},
                                            {drawer2_, 13}, {drawer3_, 14}, {drawer4_, 15}};
    for (const auto& [a, ev] : events)
        if (a) a->setEvent(ev);  // Port-Schutz, Original ohne Prüfung; alle Namen in den Daten vorhanden (tools/name_check.py)
    for (const auto& [a, ev] : {std::pair<Area*, int>{doorRamil_, 7}, {newsRamil_, 8}, {barrelRamil_, 9}}) {
        if (!a) continue;
        a->setEvent(ev);
        a->active = false;
    }
    if (ramTalk_ && ramTalk_->area) {
        ramTalk_->area->setEvent(10);
        ramTalk_->area->active = false;
    }
    showView(bgMain_, 0);
    if (restored_ != 0) {
        if (hero_) hero_->show(scene_.clicksEnabled);
        scene_.enableInput();
        return;
    }
    // Eintreten: Tür, dann der Held als freie Ebene bis an seinen Platz
    playSfx(doorCreak_.get());
    scene_.setModal(rodIn_.get());
    playWait(rodIn_.get(), 11, 0, 17);
    playSfx(tock_.get());
    playWait(rodIn_.get(), 10, 18, -1);
    scene_.disableInput();
    if (hero_) {
        scene_.setHero(nullptr);
        scene_.insertLayer(hero_);
        hero_->setPosZ(442.0, 109.0, 5.0, 115.0);
        playWait(hero_, 11, 99, 101);
        hero_->setFrameEvent(18, 51);  // Schrittgeräusche über Bildereignisse 16…19
        hero_->playRangeNoEvent(13, 48, 59);
        const int cb = beginWait();
        hero_->moveTo(223.0, 122.0, 0.0, 72.0, 30, cb);
        wait();
        scene_.removeLayer(hero_);
        scene_.setHero(hero_);
        hero_->show(scene_.clicksEnabled);
        hero_->setPosDir(300, 350, 3);
    }
    scene_.enableInput();
    if (flag(kRamil) == 1) {
        sitDown(2);
        scene_.disableInput();
        scene_.setModal(ramilIn_.get());
        playWait(ramilIn_.get(), 11, 0, 5);
        playSfx(doorCreak_.get());
        playWait(ramilIn_.get(), 11, 6, 16);
        playSfx(doorCreak_.get());
        playWait(ramilIn_.get(), 11, 17, 23);
        playSfx(tock_.get());
        playWait(ramilIn_.get(), 10, 24, -1);
        showView(bgMain_, 1);
        if (second_) {
            second_->setPosDir(576, 356, 2);
            second_->show(scene_.clicksEnabled);
        }
        ramilShown_ = 1;
        speak("Loc04", "006_ROD_02", rodDeskTalk_, 0x0C, 0, 9, 0);
        const int cb = beginWait();
        if (second_) second_->walkTo(130, 292, cb, 5);
        wait();
        ramLine("006_RAM_01");
        scene_.enableInput();
        ramStage_ = 1;
        scene_.enableInput();
        return;
    }
    if (flag(kVisits) == 1) {
        sitDown(0);
        const int cb = beginWait();
        speak("Loc04", "006_ROD_01", rodTalk_, 0x0C, 0, 10, cb);
        wait();
        delay(1000);
        showView(bgMain_, 1);
        scene_.setClickEvent(kClickEvent);
        scene_.enableInput();
        return;
    }
    if (hero_) hero_->walkToClick(300, 350);
    scene_.enableInput();
}

void Loc04::showView(Layer* bg, int desk) {
    if (bg && scene_.background() != bg) {
        scene_.disableAll();
        scene_.setBackground(bg);  // +0x60 folgt
        if (bg == bgMain_) {
            scene_.setDepth(depthMain_);
            for (Area* a : {desktop_, boar_, box_, elephant_, cabinet_, door_, barrel_, news_, drawer2_, drawer3_, drawer4_})
                if (a) a->active = true;
            if (ramilShown_ && second_) second_->show(scene_.clicksEnabled);
        } else if (bg == bgRamTalk_) {
            scene_.setDepth(depthRam_);
            if (desk != 0) {
                for (Area* a : {doorRamil_, newsRamil_, barrelRamil_})
                    if (a) a->active = true;
                if (ramTalk_) {
                    ramTalk_->enable();
                    if (ramTalk_->area) ramTalk_->area->active = true;
                }
                if (rodSitAct2_) {
                    rodSitAct2_->enable();
                    rodSitAct2_->seekFrame(0);
                }
            }
        } else if (bg == bgRodTalk_) {
            scene_.setDepth(nullptr);
            if (rodTalk_) {
                rodTalk_->enable();
                rodTalk_->seekFrame(0);
            }
        }
    }
    if (Layer* cur = scene_.background()) cur->enable();
    atDesk_ = desk;
    if (scene_.background() == bgMain_) {
        if (desk == 0) {
            if (rodDeskTalk_) rodDeskTalk_->disable();
            if (hero_) hero_->show(scene_.clicksEnabled);
        } else {
            if (hero_) hero_->hide(scene_.clicksEnabled);
            scene_.enableInput();
            if (rodDeskTalk_) rodDeskTalk_->enable();
        }
    }
    scene_.invalidateAll();
    scene_.enableInput();
}

void Loc04::sitDown(int mode) {
    if (mode != 1) scene_.disableInput();
    if (mode == 0) {
        const int cb = beginWait();
        if (hero_) hero_->walkTo(171, 259, cb, 6);
        if (!wait()) return;
        playAnimWait(rodSit_.get());
        showView(bgRodTalk_, 1);
    } else {
        const int cb = beginWait();
        if (hero_) hero_->walkTo(171, 247, cb, 6);
        if (!wait()) return;
        scene_.disableInput();
        if (hero_) hero_->hide(scene_.clicksEnabled);
        if (rodSitLayer_) rodSitLayer_->enable();
        if (mode == 2) {
            playWait(rodSitLayer_, 10, 0, 5);
            if (rodSitLayer_) rodSitLayer_->disable();
            showView(bgRamTalk_, 0);
            if (rodSitAct1_) rodSitAct1_->enable();
            const int c = beginWait();
            if (rodSitAct1_) rodSitAct1_->playEndEvent(11, c);
            wait();
            if (rodSitAct1_) rodSitAct1_->disable();
        } else {
            const int c = beginWait();
            if (rodSitLayer_) rodSitLayer_->playEndEvent(10, c);
            wait();
            if (rodSitLayer_) rodSitLayer_->disable();
            showView(bgMain_, 1);
            scene_.setClickEvent(kClickEvent);
        }
    }
    atDesk_ = 1;
    scene_.enableInput();
}

void Loc04::standUp() {
    atDesk_ = 0;
    scene_.disableInput();
    if (scene_.background() != bgMain_) showView(bgMain_, 1);
    if (rodDeskTalk_) rodDeskTalk_->disable();
    if (rodSitLayer_) rodSitLayer_->enable();
    playWait(rodSitLayer_, 10, 20, 0);
    if (rodSitLayer_) rodSitLayer_->disable();
    if (hero_) hero_->setPosDir(173, 247, 6);
    showView(bgMain_, 0);
    scene_.enableInput();
    scene_.setClickEvent(0);
}

void Loc04::deskComment(std::string_view key) {
    if (atDesk_ == 0) {
        walkAndComment(270, 288, 3, key);
        return;
    }
    speak("Loc04", key, rodDeskTalk_, 0x0C, 0, 5, 0);
}

void Loc04::comment3(int x, int y, int dir, std::string_view key) {
    if (atDesk_ == 0) {
        walkAndComment(x, y, dir, key);
        return;
    }
    speak("Loc04", key, rodDeskTalk_, 0x0C, 0, 9, 0);
}

void Loc04::rodLine(std::string_view key, int view) {
    if (view == 1) {
        showView(bgMain_, atDesk_);
        const int cb = beginWait();
        speak("Loc04", key, rodDeskTalk_, 0x0C, 0, 9, cb);
        wait();
        return;
    }
    if (view == 2) {
        showView(bgRamTalk_, atDesk_);
        const int cb = beginWait();
        sayActor("Loc04", key, rodSitAct2_, cb);
        wait();
        return;
    }
    showView(bgRodTalk_, atDesk_);
    const int cb = beginWait();
    sayActor("Loc04", key, rodTalk_, cb);
    wait();
}

void Loc04::ramLine(std::string_view key) {
    showView(bgRamTalk_, atDesk_);
    const int cb = beginWait();
    sayActor("Loc04", key, ramTalk_, cb);
    wait();
}

void Loc04::onEvent(Event& e, int) {
    const int id = e.param;
    const char* key = nullptr;  // Fälle mit break: comment3(413, 297, 5, key)
    switch (id) {
    case 1: {
        if (flag(kSackInBox) == 0) {
            const char* k = "006_ROD_04";
            if (desktopToggle_ == 0) {
                k = "006_ROD_03";
            } else if (atDesk_ == 0) {
                sitDown(1);
                if (atDesk_ == 0) return;
            }
            deskComment(k);
            desktopToggle_ = desktopToggle_ == 0 ? 1 : 0;
            return;
        }
        if (atDesk_ != 0) standUp();
        scene_.disableInput();
        int cb = beginWait();
        if (hero_) hero_->walkTo(131, 246, cb, 6);
        if (!wait()) {
            scene_.enableInput();
            return;
        }
        if (hero_) hero_->hide(scene_.clicksEnabled);
        playWait(rodTake_, 11, 0, 9);
        playSfx(rummage_.get());
        playWait(rodTake_, 10, 10, -1);
        if (hero_) hero_->show(scene_.clicksEnabled);
        cb = beginWait();
        if (hero_) hero_->turn(0, cb);
        wait();
        scene_.enableInput();
        sayActor("Loc04", "006_ROD_05", hero_, 0);
        flag(kSackInBox) = 0;
        if (auto* bar = scene_.inventory()) bar->add(7);
        return;
    }
    case 2:
        comment3(336, 256, 4, boarToggle_ == 0 ? "006_ROD_06" : "006_ROD_07");
        boarToggle_ = boarToggle_ == 0 ? 1 : 0;
        return;
    case 3: comment3(359, 260, 5, "006_ROD_10"); return;
    case 4:
        comment3(150, 344, 3, elephantToggle_ == 0 ? "006_ROD_08" : "006_ROD_09");
        elephantToggle_ = elephantToggle_ == 0 ? 1 : 0;
        return;
    case 5:
        if (atDesk_ != 0) standUp();
        key = "006_ROD_11";
        break;
    case 6: {
        if (atDesk_ != 0) standUp();
        int cb = beginWait();
        if (hero_) hero_->walkTo(300, 350, cb, 7);
        if (!wait()) return;
        scene_.disableInput();
        if (hero_) {
            scene_.setHero(nullptr);
            scene_.insertLayer(hero_);
            hero_->setPosZ(223.0, 122.0, 18.0, 72.0);
            hero_->playRangeNoEvent(13, 0, 11);
            hero_->setFrameEvent(16, 3);
            cb = beginWait();
            hero_->moveTo(465.0, 100.0, 0.0, 125.0, 30, cb);
            wait();
            scene_.removeLayer(hero_);
            scene_.setHero(hero_);
        }
        scene_.setModal(rodOut_.get());
        playWait(rodOut_.get(), 11, 0, 6);
        playSfx(doorCreak_.get());
        playWait(rodOut_.get(), 11, 7, 18);
        playSfx(tock_.get());
        playWait(rodOut_.get(), 11, 19, -1);
        scene_.enableInput();
        changeScene(7, 1);
        return;
    }
    case 7:
        scene_.disableInput();
        if (rodSitAct2_) rodSitAct2_->disable();
        if (rodSitAct1_) {
            rodSitAct1_->enable();
            rodSitAct1_->playRangeNoEvent(11, 19, 5);
        }
        ramLine("006_RAM_02");
        playWait(rodSitAct1_, 11, 5, 19);
        if (rodSitAct1_) rodSitAct1_->disable();
        if (rodSitAct2_) rodSitAct2_->enable();
        rodLine("006_ROD_12", 2);
        scene_.enableInput();
        return;
    case 8:
    case 9:
        scene_.disableInput();
        rodLine(id == 8 ? "006_ROD_27" : "006_ROD_26", 2);
        scene_.enableInput();
        return;
    case 10: {
        const char* ram = nullptr;
        switch (ramStage_) {
        case 2:
            rodLine("006_ROD_16", 0);
            ram = "006_RAM_06";
            break;
        case 3:
            rodLine("006_ROD_17", 0);
            ramLine("006_RAM_07");
            rodLine("006_ROD_18", 2);
            showView(bgRamTalk_, atDesk_);
            ++ramStage_;
            return;
        case 4:
            rodLine("006_ROD_19", 0);
            ram = "006_RAM_08";
            break;
        case 5:
            rodLine("006_ROD_20", 0);
            ram = "006_RAM_09";
            break;
        case 6:
            // Ramil bezahlt: Geldsack in die Kiste
            rodLine("006_ROD_21", 1);
            speak("Loc04", "006_RAM_10", 0);  // CGame_SayNoActor
            scene_.setModal(ramilMoney_.get());
            playWait(ramilMoney_.get(), 11, 0, 25);
            playSfx(tockSoft_.get());
            playWait(ramilMoney_.get(), 11, 26, 46);
            playSfx(moneyBag_.get());
            playWait(ramilMoney_.get(), 11, 47, 59);
            playSfx(tock_.get());
            playWait(ramilMoney_.get(), 10, 60, -1);
            flag(kSackInBox) = 1;
            showView(bgRamTalk_, atDesk_);
            ++ramStage_;
            return;
        case 7:
            rodLine("006_ROD_22", 0);
            ram = "006_RAM_11";
            break;
        case 8:
            rodLine("006_ROD_23", 2);
            ram = "006_RAM_12";
            break;
        case 9: {
            // Ramil geht
            rodLine("006_ROD_24", 1);
            scene_.disableInput();
            const int cb = beginWait();
            if (second_) second_->walkTo(576, 356, cb, 6);
            wait();
            if (second_) second_->hide(scene_.clicksEnabled);
            ramilShown_ = 0;
            scene_.setModal(ramilOut_.get());
            playWait(ramilOut_.get(), 11, 0, 5);
            playSfx(doorCreak_.get());
            playWait(ramilOut_.get(), 10, 6, -1);
            playSfx(tock_.get());
            showView(bgMain_, atDesk_);
            ramStage_ = -1;
            flag(kRamil) = 2;
            rodLine("006_ROD_25", 1);
            scene_.setClickEvent(kClickEvent);
            scene_.enableInput();
            ++ramStage_;
            return;
        }
        default:
            rodLine("006_ROD_13", 0);
            ramLine("006_RAM_03");
            rodLine("006_ROD_14", 2);
            ramLine("006_RAM_04");
            flag(kRamilTold) = 1;
            rodLine("006_ROD_15", 2);
            ram = "006_RAM_05";
            break;
        }
        ramLine(ram);
        ++ramStage_;
        return;
    }
    case 11: deskComment("006_ROD_26"); return;
    case 12: deskComment("006_ROD_27"); return;
    case 13:
    case 14:
    case 15:
        if (atDesk_ != 0) standUp();
        key = id == 13 ? "006_ROD_28" : id == 14 ? "006_ROD_29" : "006_ROD_30";
        break;
    // Schrittgeräusche des Helden als freie Ebene: Bildereignis → Klang, nächstes Bildereignis
    case 16:
        playSfx(stepLeft_.get());
        if (hero_) hero_->setFrameEvent(17, 9);
        return;
    case 17:
        playSfx(stepRight_.get());
        if (hero_) hero_->setFrameEvent(16, 3);
        return;
    case 18:
        playSfx(stepLeft_.get());
        if (hero_) hero_->setFrameEvent(19, 57);
        return;
    case 19:
        playSfx(stepRight_.get());
        if (hero_) hero_->setFrameEvent(18, 51);
        return;
    case kClickEvent:
        if (atDesk_ != 0) standUp();
        return;
    default: return;
    }
    comment3(413, 297, 5, key);
}

}  // namespace rah
