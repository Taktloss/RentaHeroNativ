// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/help.hpp"

#include "engine/scene.hpp"

namespace rah {

void hlpDecode(std::string& text) {
    if (text.empty() || static_cast<unsigned char>(text[0]) < 0x80) return;
    for (char& c : text) c = static_cast<char>(static_cast<unsigned char>(c) ^ 0x80);
}

HelpOverlay::HelpOverlay(EngineGlobals& g) : g_(g) {
    // Schrift +0x100 (Arial 15, normal) liefert die Plattform über g.drawHelpText
    useRight();
}

HelpOverlay::~HelpOverlay() = default;

void HelpOverlay::setup(Scene* scene, std::function<void()> showHint, bool newRenderer) {
    if (scene) scene_ = scene;
    if (showHint) showHint_ = std::move(showHint);
    if (newRenderer || !icon_) {
        load(nullptr);
        redrawIcon();
    }
}

std::unique_ptr<Layer> HelpOverlay::loadLayer(const std::optional<FoundFile>& file, int x, int y) {
    if (!file) return nullptr;  // CSzene_FindPath ohne Treffer → Laden scheitert; HelpIcon/HelpMask/
                                // HelpMaskR.TCC liegen in Common (Pflicht), tritt im Spiel nicht auf
    auto L = std::make_unique<Layer>();
    if (!L->load(*file, 0, nullptr)) return nullptr;  // Farbschlüssel Renderer+0x04 = Schwarz
    L->z = 1.0;
    L->renderToSurface();   // 0x00446C90
    L->releaseImage(false); // 0x00446DE0(0)
    L->setPos(x, y);        // Slot 5
    L->enable();
    // 0x00447F00: Name = Dateiname ohne Pfad und Endung
    const auto slash = L->path.find_last_of("/\\:");
    L->name = slash == std::string::npos ? L->path : L->path.substr(slash + 1);
    if (const auto dot = L->name.rfind('.'); dot != std::string::npos) L->name.resize(dot);
    return L;
}

void HelpOverlay::load(const std::string* name) {
    if (!scene_) return;
    if (name) iconFile_ = scene_->paths().findPath(*name);
    else if (!iconFile_) return;  // kein gespeicherter Pfad
    icon_ = loadLayer(iconFile_, 0x244, 0);
    if (!icon_) return;
    maskLeft_ = loadLayer(scene_->paths().findPath("Helpmask.tcc"), 0, 0);
    if (!maskLeft_) return;
    maskRight_ = loadLayer(scene_->paths().findPath("Helpmaskr.tcc"), 0x140, 0);
}

void HelpOverlay::drawIcon() {
    if (!icon_) {
        load(nullptr);
        if (!icon_) return;
    }
    // SetPos(580 + Client-x, Client-y): der Port hat keinen Fensterversatz
    icon_->setPos(0x244, 0);
    if (g_.screen) icon_->draw(*g_.screen, 0, nullptr);  // 0x00447150 auf Renderer+0x14
    iconShown_ = true;
}

void HelpOverlay::clearIcon() {
    // Renderer-Slot 1 (0x0043EBC0): Farbfüllung mit dem Farbschlüssel (Schwarz)
    if (g_.screen) g_.screen->fill(0x244, 0, 0x280, 0x3C, 0);
    iconShown_ = false;
}

void HelpOverlay::iconOn() {
    if (!iconShown_) drawIcon();
}

void HelpOverlay::iconOff() {
    if (iconShown_) clearIcon();
}

void HelpOverlay::redrawIcon() {
    if (iconShown_) drawIcon();
}

void HelpOverlay::useLeft() {
    textRect_ = {20, 20, 300, 340};
    mask_ = maskLeft_.get();
    if (mask_) mask_->setPos(0, 0);  // Original prüft nicht; Masken liegen in Common – Port-Schutz
}

void HelpOverlay::useRight() {
    textRect_ = {340, 20, 620, 340};
    mask_ = maskRight_.get();
    if (mask_) mask_->setPos(0x140, 0);
}

void HelpOverlay::show(std::string_view name) {
    if (!scene_) return;
    const auto file = scene_->paths().findFile(name, "hlp");
    if (!file) return;
    const auto data = file->read();
    if (!data) return;
    // Datei + 100 Nullbytes; DrawTextA liest bis zum ersten Nullbyte
    std::string text(data->begin(), data->end());
    hlpDecode(text);
    if (const auto nul = text.find('\0'); nul != std::string::npos) text.resize(nul);
    // Figur CSzene+0x5E0 fehlt oder Fuß-x (+0x15D0) > 319 → links, sonst rechts (0x00441040)
    const Actor* hero = scene_->actor(0);
    if (!hero || hero->footX > 319) useLeft();
    else useRight();
    Surface565& s = scene_->surface();  // Renderer+0x18
    if (mask_) mask_->draw(s, 1, nullptr);
    if (g_.drawHelpText) g_.drawHelpText(s, textRect_, text);
    if (g_.outputSceneRect) g_.outputSceneRect(Rect{0, 0, 640, 420});  // 0x0043E1D0
    open_ = true;
}

void HelpOverlay::close() {
    open_ = false;
    if (scene_) scene_->invalidateAll();
}

void HelpOverlay::onIconClick() {
    if (showHint_) showHint_();
}

void HelpOverlay::release() {
    icon_.reset();
    maskLeft_.reset();
    maskRight_.reset();
    mask_ = nullptr;  // Original lässt +0x14 stehen (zeigt dann ins Leere)
}

}  // namespace rah
