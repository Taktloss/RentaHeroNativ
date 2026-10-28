// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/loc06.hpp"

#include "engine/area.hpp"
#include "engine/inventory.hpp"

namespace rah {

namespace {
constexpr std::size_t kRing = 0x3E8C;      // Ranama trägt den Ring (06_027_A)
constexpr std::size_t kVisit = 0x3EE8;     // Besuche in Szene 26 (1 = erster: Ranama schwebt)
constexpr std::size_t kRingTalk = 0x3EEC;  // Gesprächsstufe Ring (Start 1)
constexpr std::size_t kTalk = 0x3EF0;      // Gesprächsstufe (Start 1)
constexpr int kBobHandler = 0x0040F4F0;
constexpr int kUseHandler = 0x0040F4E0;    // Inventar: CGame_DescribeItem(typ)
constexpr int kItem = 14;
}  // namespace

Loc06::Loc06(GameContext& ctx, int scene, int prevScene) : Game(ctx, 6, scene, prevScene) {
    g_.fnHandlers[kBobHandler] = {this, [this](Event&, int) { ranamaBob(); }};
    g_.fnHandlers[kUseHandler] = {this, [this](Event&, int t) { describeItem(t); }};
    getViewObjects();
    timer_ = std::make_unique<Event>();
    // Einstiege mit vorgegebenem Zustand (Szenennummern über 1000: Prüf-/Sprungziele)
    switch (prevScene_) {
    case 1001: flag(kVisit) = 0; break;
    case 1002: flag(kVisit) = 1; break;
    case 1003: flag(kVisit) = 2; break;
    case 2001:
        flag(kVisit) = 2;
        flag(kRing) = 1;
        break;
    case 2003:
        flag(kVisit) = 2;
        flag(kRingTalk) = 3;
        flag(kRing) = 1;
        break;
    default: break;
    }
    if (sceneNr_ == 27) flag(kVisit) = 2;
    if (sceneNr_ == 27) {
        scene_.load("027", flag(kRing) != 0 ? 1u : 0u);
        getViewObjects();
        if (flag(kRing) != 0) {
            earGrab_ = loadAnim("S27_OhrGreif");
            ranLooks_ = dynamic_cast<AnimLayer*>(scene_.getObject("S27_RanSchaut"));
            ranLooksRing_ = dynamic_cast<AnimLayer*>(scene_.getObject("S27_RanSchautRing"));
        }
        rodTalk_ = dynamic_cast<AnimLayer*>(scene_.getObject("S27_RodTalk"));
        ranTalk27_ = dynamic_cast<AnimLayer*>(scene_.getObject("S27_RanTalk"));
        ranTalkRing_ = dynamic_cast<AnimLayer*>(scene_.getObject("S27_RanTalkRing"));
        exit27_ = scene_.getArea("S27_Exit");
    } else {
        sceneNr_ = 26;
        flag(kVisit) += 1;
        scene_.load("026", flag(kVisit) != 1 ? 2u : 1u);
        getViewObjects();
        if (flag(kVisit) == 1) {
            ranFalls_ = dynamic_cast<AnimLayer*>(scene_.getObject("S26_RanFaellt"));
            ranTalk_ = dynamic_cast<AnimLayer*>(scene_.getObject("S26_RanTalk"));
        } else {
            ranBroom_ = dynamic_cast<AnimLayer*>(scene_.getObject("S26_RanBesen"));
            ranama_ = scene_.getArea("S26_Ranama");
        }
        exit_ = scene_.getArea("S26_Exit");
        bottle_ = scene_.getArea("S26_Bottle");
        telescope_ = scene_.getArea("S26_Telescope");
        backArea_ = scene_.getArea("S26_Background");
    }
    boing_ = loadSfxVol("Sfx_Boing2", 70);
    knock_ = loadSfx("Sfx_Knock_Wet");
    gong_ = loadSfx("Sfx_Gong");
    ratsch_ = loadSfx("Sfx_Ratsch");
    tock_ = loadSfxVol("Sfx_Tock", 50);
    music_ = playMusic("Loc06_Ranama");
    if (music_) {
        music_->setReverb(6, 0.403f, 4.279f, 0.5f);  // 0x3ECE5604, 0x4088ED91, 0x3F000000
        music_->setReverbMix(0.1f);                   // 0x3DCCCCCD
    }
}

Loc06::~Loc06() {
    shutdown();
    for (int id : {kBobHandler, kUseHandler}) {
        const auto it = g_.fnHandlers.find(id);
        if (it != g_.fnHandlers.end() && it->second.owner == this) g_.fnHandlers.erase(it);
    }
}

void Loc06::destroyLocal() {
    timer_.reset();
    earGrab_.reset();
    boing_.reset();
    knock_.reset();
    gong_.reset();
    ratsch_.reset();
    tock_.reset();
}

bool Loc06::playWait(AnimLayer* a, unsigned mode, int from, int to) {
    const int cb = beginWait();
    if (a) a->playRange(mode, cb, from, to);
    return wait();
}

void Loc06::rodLine(std::string_view key) {
    if (sceneNr_ == 26) {
        heroSay(key);  // CGame_HeroSayWait
        return;
    }
    const int cb = beginWait();
    sayActor("Loc06", key, rodTalk_, cb);
    wait();
}

void Loc06::ranLine(std::string_view key) {
    const int cb = beginWait();
    sayActor("Loc06", key, sceneNr_ == 26 ? ranTalk_ : ranTalk27_, cb);
    wait();
}

void Loc06::start() {
    if (music_) music_->fadeIn(80, 2000);  // CMusic_FadeIn 0x00437FC0
    setEventProc(&Loc06::onEvent);
    if (auto* bar = scene_.inventory()) bar->setUseHandler(kUseHandler);  // 0x0043FA60
    scene_.disableAll();
    if (auto* bg = scene_.background()) bg->enable();
    const auto areaOf = [](AnimLayer* l) { return l ? l->area : nullptr; };  // Ebene +0x1C0
    if (sceneNr_ == 27) {
        if (exit27_) exit27_->setEvent(10);
        if (ranTalkRing_) ranTalkRing_->enable();
        if (Area* a = areaOf(ranTalkRing_)) a->setEvent(12);
        if (rodTalk_) rodTalk_->enable();
        if (ranTalk27_) ranTalk27_->enable();
        if (Area* a = areaOf(ranTalk27_)) a->setEvent(13);
        if (flag(kRing) != 0) {
            if (Area* a = areaOf(ranLooksRing_)) {
                a->setEvent(11);
                a->active = false;
            }
            if (Area* a = areaOf(ranLooks_)) a->setEvent(13);
        }
        scene_.enableInput();
        return;
    }
    if (exit_) exit_->setEvent(1);
    if (bottle_) bottle_->setEvent(2);
    if (telescope_) telescope_->setEvent(3);
    if (backArea_) backArea_->setEvent(4);
    if (restored_ == 0) {
        // Original: CActor_SetPos mit nur zwei PUSHs; die Richtung (6) kommt von einem gemeinsamen
        // PUSH vor der Verzweigung (0x0040EC79)
        if (prevScene_ == 27) {
            if (hero_) hero_->setPosDir(319, 352, 6);
            if (scene_.inventory() && scene_.inventory()->has(kItem)) leave();
        } else {
            if (hero_) hero_->setPosDir(12, 320, 6);
        }
    }
    if (hero_) hero_->show(scene_.clicksEnabled);
    const int visit = flag(kVisit);
    if (visit == 1) {
        if (ranTalk_) ranTalk_->enable();
        if (Area* a = areaOf(ranTalk_)) a->setEvent(5);
        ranamaBob();
        scene_.disableInput();
        int cb = beginWait();
        if (hero_) hero_->walkTo(149, 342, cb, 6);
        wait();
        rodLine("026_ROD_01");
        ranLine("026_RAN_01");
        cb = beginWait();
        if (hero_) hero_->walkTo(319, 352, cb, 6);
        wait();
        rodLine("026_ROD_02");
        ranLine("026_RAN_02");
        rodLine("026_ROD_03");
        scene_.enableInput();
        return;
    }
    if (visit == 2) {
        if (ranBroom_) ranBroom_->enable();
        if (ranama_) ranama_->setEvent(5);
        scene_.disableInput();
        if (hero_) hero_->walkTo(319, 352, 0, 6);
        playWait(ranBroom_, 11, 0, 6);
        if (boing_) boing_->play();
        playWait(ranBroom_, 11, 7, 17);
        if (boing_) boing_->play();
        playWait(ranBroom_, 11, 18, 44);
        if (tock_) tock_->play();
        playWait(ranBroom_, 11, 45, 49);
        if (tock_) tock_->play();
        playWait(ranBroom_, 11, 50, 52);
        const int cb = beginWait();
        speak("Loc06", "026_RAN_03", ranBroom_, 0x0B, 46, 52, cb);
        wait();
        rodLine("026_ROD_04");
        scene_.enableInput();
        return;
    }
    if (ranama_) ranama_->setEvent(5);
    if (ranBroom_) {
        ranBroom_->enable();
        ranBroom_->showFrame(52);
    }
    scene_.disableInput();
    if (restored_ == 0 && prevScene_ != 27) {
        const int cb = beginWait();
        if (hero_) hero_->walkTo(319, 352, cb, 6);
        wait();
        scene_.enableInput();
        return;
    }
    scene_.enableInput();
}

void Loc06::onEvent(Event& e, int) {
    switch (e.param) {
    case 1:
        timer_->enabled = false;  // +0x00/+0x04 = 0
        timer_->pending = false;
        leave();
        return;
    case 2:
        walkAndComment(98, 355, 1, bottleCount_ == 0 ? "026_ROD_06" : "026_ROD_07");
        ++bottleCount_;
        return;
    case 3: walkAndComment(319, 352, 5, "026_ROD_08"); return;
    case 4: walkAndComment(259, 344, 4, "026_ROD_09"); return;
    case 5: {
        if (flag(kVisit) == 1) {
            faceAndSay("026_ROD_05");
            return;
        }
        const int cb = beginWait();
        if (hero_) hero_->walkTo(319, 352, cb, 6);
        wait();
        changeScene(27, 6);
        return;
    }
    case 11:
        scene_.disableInput();
        timer_->enabled = false;
        timer_->pending = false;
        scene_.setModal(earGrab_.get());
        playWait(earGrab_.get(), 11, 0, 5);
        if (ratsch_) ratsch_->play();
        playWait(earGrab_.get(), 10, 6, -1);
        if (auto* bar = scene_.inventory()) bar->add(kItem);
        [[fallthrough]];
    case 10: changeScene(26, 6); return;
    case 12: break;
    case 13:
        if (ringPending_ != 0) return;
        if (flag(kTalk) == 1) {
            rodLine("027_ROD_01");
            ranLine("027_RAN_01");
            rodLine("027_ROD_02");
            ranLine("027_RAN_02");
            flag(kTalk) = 2;
            return;
        }
        rodLine("027_ROD_03");
        ranLine("027_RAN_03");
        rodLine("027_ROD_04");
        ranLine("027_RAN_04");
        rodLine("027_ROD_05");
        flag(kTalk) = 2;
        return;
    case 20: {
        // Zeit für den Griff zum Ohr abgelaufen
        scene_.disableInput();
        ringPending_ = 0;
        if (ranLooksRing_) ranLooksRing_->disable();
        playWait(ranLooks_, 11, 12, 22);
        if (ranLooks_) ranLooks_->disable();
        if (ranTalk27_) ranTalk27_->enable();
        if (ranTalkRing_) ranTalkRing_->enable();
        if (exit27_) exit27_->active = true;
        if (ranTalk27_ && ranTalk27_->area) ranTalk27_->area->active = true;
        if (ranTalkRing_ && ranTalkRing_->area) ranTalkRing_->area->active = true;
        if (exit27_) exit27_->active = true;
        ranLine("027_RAN_08");
        flag(kRingTalk) = 3;
        scene_.enableInput();
        return;
    }
    default: return;
    }
    // Ereignis 12: Ring
    if (flag(kRing) == 0) {
        rodLine("027_ROD_06");
        ranLine("027_RAN_05");
        return;
    }
    const int stage = flag(kRingTalk);
    if (stage == 1) {
        rodLine("027_ROD_07");
        ranLine("027_RAN_06");
        flag(kRingTalk) = 2;
        return;
    }
    if (stage == 2) {
        rodLine("027_ROD_08");
        ranLine("027_RAN_07");
        flag(kRingTalk) = 3;
        return;
    }
    scene_.disableInput();
    rodLine("027_ROD_09");
    if (ranTalk27_) ranTalk27_->disable();
    if (ranTalkRing_) ranTalkRing_->disable();
    if (ranLooks_) ranLooks_->enable();
    playWait(ranLooks_, 11, 0, 11);
    if (ranLooksRing_) ranLooksRing_->enable();
    if (ranLooksRing_ && ranLooksRing_->area) ranLooksRing_->area->active = true;
    if (ranLooks_ && ranLooks_->area) ranLooks_->area->active = true;
    if (exit27_) exit27_->active = false;
    scene_.enableInput();
    setTimerHandler(*timer_, 2000, 0, 20);
    ringPending_ = 1;
}

void Loc06::ranamaBob() {
    double y = 124.0;  // 0x405F0000
    switch (bob_) {
    case 0:
        bob_ = 1;
        y = 111.0;  // 0x405BC000
        break;
    case 1: bob_ = 0; break;
    case 2: bob_ = 3; break;
    case 3:
        if (ranTalk_) ranTalk_->disable();
        if (ranFalls_) ranFalls_->enable();
        playWait(ranFalls_, 11, 0, 23);
        if (knock_) knock_->play();
        if (gong_) gong_->play();
        playWait(ranFalls_, 11, 24, -1);
        delay(1000);
        changeScene(7, 1);
        return;
    default: return;
    }
    if (ranTalk_) ranTalk_->moveTo(0.0, y, 0.0, 0.0, 13, kBobHandler);
}

void Loc06::leave() {
    scene_.disableInput();
    auto* bar = scene_.inventory();
    if (bar && bar->has(kItem)) {
        speak("Loc06", "027_RAN_09", ranBroom_, 0x0D, 46, 52, 0);
        if (hero_) hero_->fps = 20;  // +0x54
    }
    int cb = beginWait();
    if (hero_) hero_->walkTo(12, 320, cb, 2);
    wait();
    if (hero_) {
        hero_->fps = 10;
        hero_->hide(scene_.clicksEnabled);
    }
    if (bar && bar->has(kItem)) {
        cb = beginWait();
        speak("Loc06", "027_RAN_10", ranBroom_, 0x0D, 46, 52, cb);
        wait();
    }
    if (flag(kVisit) == 1) {
        bob_ = 3 - (bob_ != 0 ? 1 : 0);
        return;
    }
    changeScene(7, 1);
}

}  // namespace rah
