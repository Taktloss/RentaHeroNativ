// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/ui.hpp"

#include <algorithm>
#include <cstdio>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "render/font.hpp"

namespace rah {
namespace {

// Angelehnt an das Spiel: dunkles Holz/Messing statt ImGui-Blau
void applyStyle(float scale) {
    ImGuiStyle& st = ImGui::GetStyle();
    ImGui::StyleColorsDark(&st);
    st.WindowRounding = 0;
    st.FrameRounding = 4;
    st.GrabRounding = 4;
    st.TabRounding = 4;
    st.FramePadding = ImVec2(10, 6);
    st.ItemSpacing = ImVec2(10, 8);
    st.WindowPadding = ImVec2(20, 16);
    ImVec4* c = st.Colors;
    const ImVec4 bg(0.10f, 0.08f, 0.06f, 1), panel(0.16f, 0.12f, 0.09f, 1), brass(0.78f, 0.58f, 0.25f, 1),
        brassHi(0.90f, 0.70f, 0.32f, 1), brassLo(0.55f, 0.40f, 0.17f, 1);
    c[ImGuiCol_WindowBg] = bg;
    c[ImGuiCol_ChildBg] = panel;
    c[ImGuiCol_PopupBg] = panel;
    c[ImGuiCol_Border] = ImVec4(0.35f, 0.27f, 0.16f, 1);
    c[ImGuiCol_Text] = ImVec4(0.95f, 0.91f, 0.82f, 1);
    c[ImGuiCol_TextDisabled] = ImVec4(0.58f, 0.52f, 0.44f, 1);
    c[ImGuiCol_FrameBg] = ImVec4(0.22f, 0.17f, 0.12f, 1);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.23f, 0.15f, 1);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.36f, 0.27f, 0.17f, 1);
    c[ImGuiCol_Button] = brassLo;
    c[ImGuiCol_ButtonHovered] = brass;
    c[ImGuiCol_ButtonActive] = brassHi;
    c[ImGuiCol_Header] = ImVec4(0.45f, 0.32f, 0.15f, 1);
    c[ImGuiCol_HeaderHovered] = brassLo;
    c[ImGuiCol_HeaderActive] = brass;
    c[ImGuiCol_Tab] = ImVec4(0.22f, 0.17f, 0.12f, 1);
    c[ImGuiCol_TabHovered] = brass;
    c[ImGuiCol_TabSelected] = brassLo;
    c[ImGuiCol_TabSelectedOverline] = brassHi;
    c[ImGuiCol_CheckMark] = brassHi;
    c[ImGuiCol_SliderGrab] = brass;
    c[ImGuiCol_SliderGrabActive] = brassHi;
    c[ImGuiCol_TitleBg] = panel;
    c[ImGuiCol_TitleBgActive] = panel;
    c[ImGuiCol_PlotHistogram] = brass;
    c[ImGuiCol_TableHeaderBg] = ImVec4(0.22f, 0.17f, 0.12f, 1);
    c[ImGuiCol_TableRowBgAlt] = ImVec4(1, 1, 1, 0.03f);
    st.ScaleAllSizes(scale);
}

}  // namespace

void uiCreate(SDL_Window* win) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;  // kein imgui.ini
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;
    // Fensterpunkte: Inhaltsmaßstab = Anzeigemaßstab / Pixeldichte (HiDPI rastert ImGui selbst)
    const float density = std::max(1.0f, SDL_GetWindowPixelDensity(win));
    const float scale = std::max(1.0f, SDL_GetWindowDisplayScale(win) / density);
    applyStyle(scale);
    ImGui::GetStyle().FontSizeBase = 17.0f * scale;
    if (const std::string f = Font::findSystemFont(false, false); !f.empty()) {
        ImFontConfig cfg;
        cfg.OversampleH = 2;
        io.Fonts->AddFontFromFileTTF(f.c_str(), 17.0f * scale, &cfg);
    } else {
        io.Fonts->AddFontDefault();
    }
}

void uiDestroy() { ImGui::DestroyContext(); }

std::string applyDisplay(Presenter& p, const DisplaySettings& d) {
    std::string err;
    if (!p.setDisplay(d, &err)) return err.empty() ? std::string("nicht verfügbar") : err;
    return {};
}

bool displayControls(DisplaySettings& d, bool filtersAvailable, bool* fullscreen) {
    bool changed = false;
    const float w = ImGui::GetFontSize() * 16;
    const FilterInfo* crt = findFilter(crtFilters(), d.crt);
    const bool crtOn = crt && crt->preset;
    if (!filtersAvailable)
        ImGui::TextColored(ImVec4(0.95f, 0.78f, 0.35f, 1), "Ohne OpenGL 3.3 nur „Original“ und „Weich“.");
    ImGui::BeginDisabled(crtOn);
    ImGui::SetNextItemWidth(w);
    const FilterInfo* cur = findFilter(scalerFilters(), d.filter);
    if (ImGui::BeginCombo("Filter", cur ? cur->name : "?")) {
        for (const auto& f : scalerFilters()) {
            const bool ok = filtersAvailable || !f.preset;
            ImGui::BeginDisabled(!ok);
            if (ImGui::Selectable(f.name, cur == &f)) {
                d.filter = f.id;
                changed = true;
            }
            ImGui::EndDisabled();
            if (f.hint[0]) ImGui::SetItemTooltip("%s", f.hint);
        }
        ImGui::EndCombo();
    }
    ImGui::EndDisabled();
    if (crtOn) {
        ImGui::SameLine();
        ImGui::TextDisabled("(CRT skaliert selbst)");
    }
    ImGui::BeginDisabled(!filtersAvailable);
    ImGui::SetNextItemWidth(w);
    if (ImGui::BeginCombo("CRT-Röhre", crt ? crt->name : "Aus")) {
        for (const auto& f : crtFilters()) {
            if (ImGui::Selectable(f.name, crt == &f)) {
                d.crt = f.id;
                changed = true;
            }
            if (f.hint[0]) ImGui::SetItemTooltip("%s", f.hint);
        }
        ImGui::EndCombo();
    }
    changed |= ImGui::Checkbox("Entbänderung (16-Bit-Farbstufen glätten)", &d.deband);
    ImGui::EndDisabled();
    if (ImGui::RadioButton("Ganzzahlig skalieren", d.integerScale)) {
        changed |= !d.integerScale;
        d.integerScale = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Fenster füllen (4:3)", !d.integerScale)) {
        changed |= d.integerScale;
        d.integerScale = false;
    }
    if (fullscreen) changed |= ImGui::Checkbox("Vollbild", fullscreen);
    return changed;
}

// ---------------------------------------------------------------------------------------------

void GameOverlay::toast(std::string text) {
    toast_ = std::move(text);
    toastUntil_ = SDL_GetTicks() + 2500;
}

void GameOverlay::changed(const std::string& label) {
    const std::string err = applyDisplay(p_, s_.display);
    toast(err.empty() ? label : label + " – " + err);
    saveLauncherSettings(ini_, s_);
}

bool GameOverlay::handleEvent(const SDL_Event& ev) {
    // Tab gedrückt halten: Hotspots (HOTSPOTS.md). Das Original wertet VK_TAB nicht aus
    // (0x0044CE50/0x0044CF00), das Spiel bekommt die Taste deshalb nicht
    if ((ev.type == SDL_EVENT_KEY_DOWN || ev.type == SDL_EVENT_KEY_UP) && ev.key.key == SDLK_TAB && !open_ &&
        s_.hotspotKey) {
        hotspots_ = ev.type == SDL_EVENT_KEY_DOWN;
        return true;
    }
    if (ev.type == SDL_EVENT_WINDOW_FOCUS_LOST) hotspots_ = false;
    if (ev.type == SDL_EVENT_KEY_DOWN && (ev.key.mod & SDL_KMOD_ALT) && !(ev.key.mod & SDL_KMOD_CTRL)) {
        const SDL_Keycode k = ev.key.key;
        if (k == SDLK_F || k == SDLK_C || k == SDLK_D || k == SDLK_I || k == SDLK_O || k == SDLK_RETURN || k == SDLK_KP_ENTER) {
            if (ev.key.repeat) return true;
            DisplaySettings& d = s_.display;
            if (k == SDLK_F) {
                const int dir = (ev.key.mod & SDL_KMOD_SHIFT) ? -1 : 1;
                if (!p_.supportsFilters()) d.filter = d.filter == "smooth" ? "original" : "smooth";
                else d.filter = nextFilterId(scalerFilters(), d.filter, dir);
                if (findFilter(crtFilters(), d.crt) && d.crt != "off") d.crt = "off";
                changed("Filter: " + displayLabel(d));
            } else if (k == SDLK_C) {
                d.crt = nextFilterId(crtFilters(), d.crt, 1);
                changed(d.crt == "off" ? "CRT aus – " + displayLabel(d) : displayLabel(d));
            } else if (k == SDLK_D) {
                d.deband = !d.deband;
                changed(std::string("Entbänderung ") + (d.deband ? "an" : "aus"));
            } else if (k == SDLK_I) {
                d.integerScale = !d.integerScale;
                changed(d.integerScale ? "Ganzzahlig skalieren" : "Fenster füllen (4:3)");
            } else if (k == SDLK_O) {
                open_ = !open_;
            } else {
                s_.fullscreen = !s_.fullscreen;
                SDL_SetWindowFullscreen(win_, s_.fullscreen);
                saveLauncherSettings(ini_, s_);
            }
            return true;
        }
    }
    if (!open_) return false;
    ImGui_ImplSDL3_ProcessEvent(&ev);
    if (ev.type == SDL_EVENT_KEY_DOWN && ev.key.key == SDLK_ESCAPE) {
        open_ = false;
        return true;
    }
    const ImGuiIO& io = ImGui::GetIO();
    switch (ev.type) {
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_MOTION:
    case SDL_EVENT_MOUSE_WHEEL: return io.WantCaptureMouse;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
    case SDL_EVENT_TEXT_INPUT: return io.WantCaptureKeyboard;
    default: return false;
    }
}

void GameOverlay::drawHotspots(const std::vector<HotspotView>& hs) {
    // Bild → Fenster: windowToFrame ist in beiden Darstellungswegen linear
    float ax = 0, ay = 0, bx = 0, by = 0;
    p_.windowToFrame(0, 0, &ax, &ay);
    p_.windowToFrame(100, 100, &bx, &by);
    if (bx == ax || by == ay) return;
    const float kx = 100.0f / (bx - ax), ky = 100.0f / (by - ay);
    const auto win = [&](float fx, float fy) { return ImVec2((fx - ax) * kx, (fy - ay) * ky); };
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    const ImU32 fill = IM_COL32(255, 196, 64, 80), edge = IM_COL32(255, 210, 110, 200);
    for (const HotspotView& h : hs) {
        float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;
        for (const SDL_FRect& c : h.cells) {
            dl->AddRectFilled(win(c.x, c.y), win(c.x + c.w, c.y + c.h), fill);
            x0 = std::min(x0, c.x), y0 = std::min(y0, c.y);
            x1 = std::max(x1, c.x + c.w), y1 = std::max(y1, c.y + c.h);
        }
        if (h.cells.empty()) continue;
        const ImVec2 a = win(x0, y0), b = win(x1, y1);
        dl->AddRect(a, b, edge, 3.0f, 1.5f);
        if (h.label.empty()) continue;
        const ImVec2 ts = ImGui::CalcTextSize(h.label.c_str());
        const float pad = 4.0f;
        ImVec2 p((a.x + b.x - ts.x) * 0.5f, (a.y + b.y - ts.y) * 0.5f);
        dl->AddRectFilled(ImVec2(p.x - pad, p.y - pad * 0.5f), ImVec2(p.x + ts.x + pad, p.y + ts.y + pad * 0.5f),
                          IM_COL32(25, 18, 8, 215), 4.0f);
        dl->AddText(p, IM_COL32(255, 232, 170, 255), h.label.c_str());
    }
}

void GameOverlay::present(SDL_Surface** grab) {
    const bool showToast = !toast_.empty() && SDL_GetTicks() < toastUntil_;
    if (!showToast) toast_.clear();
    const bool showHot = (forceHotspots_ || (hotspots_ && s_.hotspotKey)) && hotspotSrc_;
    if (!open_ && !showToast && !showHot) {
        p_.present(false, grab);
        return;
    }
    p_.imguiNewFrame();
    ImGui::NewFrame();
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    if (showHot) drawHotspots(hotspotSrc_());
    if (showToast) {
        ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + 12, vp->WorkPos.y + 12));
        ImGui::SetNextWindowBgAlpha(0.75f);
        ImGui::Begin("##toast", nullptr,
                     ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
                         ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoSavedSettings);
        ImGui::TextUnformatted(toast_.c_str());
        ImGui::End();
    }
    if (open_) {
        ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowBgAlpha(0.92f);
        bool keep = true;
        if (ImGui::Begin("Bild (Alt+O)", &keep, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse)) {
            bool fs = s_.fullscreen;
            if (displayControls(s_.display, p_.supportsFilters(), &fs)) {
                if (fs != s_.fullscreen) {
                    s_.fullscreen = fs;
                    SDL_SetWindowFullscreen(win_, fs);
                }
                const std::string err = applyDisplay(p_, s_.display);
                if (!err.empty()) toast(displayLabel(s_.display) + " – " + err);
                saveLauncherSettings(ini_, s_);
            }
            ImGui::Separator();
            ImGui::TextDisabled("Alt+F Filter (Umschalt: zurück) · Alt+C CRT · Alt+D Entbänderung\n"
                                "Alt+I Skalierung · Alt+Enter Vollbild · Esc/Alt+O schließen");
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 34);
            ImGui::TextDisabled("%s", p_.info().c_str());
            ImGui::PopTextWrapPos();
        }
        ImGui::End();
        if (!keep) open_ = false;
    }
    ImGui::Render();
    p_.present(true, grab);
}

}  // namespace rah
