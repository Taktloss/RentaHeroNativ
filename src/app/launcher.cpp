// SPDX-License-Identifier: GPL-3.0-or-later
#include "app/launcher.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "game/savegame.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_stdlib.h"
#include "app/ui.hpp"
#include "render/font.hpp"
#include "vfs/vfs.hpp"
#ifdef RAH_HAVE_UNSHIELD
#include "vfs/install.hpp"
#endif

namespace rah {

namespace {

// Ergebnisse der SDL-Dateidialoge (Rückruf ggf. aus einem anderen Thread)
struct DialogResult {
    enum Target { AddSource, Common, SaveDir } target;
    std::vector<std::string> paths;
};
struct DialogQueue {
    std::mutex m;
    std::vector<DialogResult> results;
    std::string error;
};
struct DialogCall {
    DialogQueue* q;
    DialogResult::Target target;
};

void SDLCALL onDialog(void* user, const char* const* files, int) {
    std::unique_ptr<DialogCall> call(static_cast<DialogCall*>(user));
    std::lock_guard lock(call->q->m);
    if (!files) {
        call->q->error = std::string("Dateiauswahl nicht verfügbar: ") + SDL_GetError();
        return;
    }
    DialogResult r{call->target, {}};
    for (; *files; ++files) r.paths.emplace_back(*files);
    if (!r.paths.empty()) call->q->results.push_back(std::move(r));
}

std::string fileTime(const std::filesystem::path& p) {
    std::error_code ec;
    const auto t = std::filesystem::last_write_time(p, ec);
    if (ec) return {};
    // ohne clock_cast (fehlt in libc++): Abstand zu jetzt auf die Systemuhr übertragen
    const auto sys = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        t - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
    const std::time_t tt = std::chrono::system_clock::to_time_t(sys);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof buf, "%d.%m.%Y %H:%M", &tm);
    return buf;
}

struct SaveEntry {
    std::string path, name, time;
    bool used = false;
};

std::string withSlash(std::string d) {
    if (!d.empty() && d.back() != '/' && d.back() != '\\') d += '/';
    return d;
}

// Spielstände wie die Liste im Spielmenü (0x004446C0): SaveGame00…08, dazu SaveGameExit
std::vector<SaveEntry> scanSaves(const std::string& dir, SaveEntry& exitSave) {
    SaveGlobals g;
    g.dir_ = withSlash(dir);
    std::vector<SaveEntry> v;
    auto entry = [](const std::string& path) {
        SaveEntry e;
        e.path = path;
        if (std::filesystem::exists(path)) {
            e.used = true;
            if (auto h = readSaveHeader(path)) e.name = cp1252ToUtf8(saveName(h->name));
            e.time = fileTime(path);
        }
        return e;
    };
    for (int i = 0; i < 9; ++i) v.push_back(entry(slotPath(g, i)));
    exitSave = entry(g.dir_ + "SaveGameExit.rah");
    return v;
}

const ImVec4 kOk(0.55f, 0.85f, 0.45f, 1), kBad(0.95f, 0.45f, 0.35f, 1), kWarn(0.95f, 0.78f, 0.35f, 1);

class Launcher {
public:
    Launcher(SDL_Window* w, Presenter& p, LauncherSettings& s, const LauncherEnv& e) : win_(w), p_(p), s_(s), env_(e) {}

    // Ein Bild; liefert eine Wahl, sobald gestartet/beendet wird
    std::optional<LaunchChoice> frame();
    void requestQuit() { quit_ = true; }
    void forceTab(int t) { forceTab_ = t; }
    // Prüfmodus RAH_LAUNCHER_AUTO: new | continue | load:<Platz 0…8>
    void autoStart(const std::string& a) { auto_ = a; }
    void runAuto(const std::string& a) {
        recheck();
        rescanSaves();
        if (!dataOk()) {
            std::fprintf(stderr, "Startmenü: Datenquelle ungültig\n");
            quit_ = true;
        } else if (a == "new") start(LaunchChoice::NewGame);
        else if (a == "continue") start(LaunchChoice::Continue, exitSave_.path);
        else if (a.rfind("load:", 0) == 0) start(LaunchChoice::Load, saves_.at(std::stoul(a.substr(5))).path);
        else quit_ = true;
    }

private:
    void recheck() {
        check_ = checkDataSources(s_.sources);
        checked_ = true;
#ifdef RAH_HAVE_UNSHIELD
        installed_ = installComplete(env_.installDir);
#endif
    }
    std::string saveDir() const { return s_.saveDir.empty() ? env_.defaultSaveDir : s_.saveDir; }
    bool haveCommon() const { return !s_.common.empty(); }
    bool dataOk() const { return checked_ && !s_.sources.empty() && check_.ok(haveCommon()) && (haveCommon() || env_.canExtract); }
    void rescanSaves() { saves_ = scanSaves(saveDir(), exitSave_); }
    void openFolder(DialogResult::Target t, const std::string& start);
    void openImage();
    void pollDialogs();
    void start(LaunchChoice::Kind k, std::string path = {});
    std::optional<LaunchChoice> finishStart();

    void tabPlay();
    void tabData();
    void tabOptions();
    void extractPopup();

    SDL_Window* win_;
    Presenter& p_;
    LauncherSettings& s_;
    const LauncherEnv& env_;
    DataCheck check_;
    bool checked_ = false, installed_ = false, quit_ = false;
    std::vector<SaveEntry> saves_;
    SaveEntry exitSave_;
    int selected_ = -1;
    std::string pathInput_, commonInput_, saveDirInput_;
    DialogQueue dialogs_;
    std::string message_;
    bool firstFrame_ = true, gotoData_ = false;
    int forceTab_ = -1;  // Prüfmodus RAH_LAUNCHER_SHOT
    std::string auto_;   // Prüfmodus RAH_LAUNCHER_AUTO
    // Start, ggf. nach dem Entpacken
    std::optional<LaunchChoice> pending_;
    std::thread worker_;
    std::atomic<int> progI_{0}, progN_{0};
    std::atomic<bool> workDone_{false};
    bool working_ = false;
    std::string workErr_;
    bool workOk_ = false;
};

void Launcher::openFolder(DialogResult::Target t, const std::string& startDir) {
    auto* call = new DialogCall{&dialogs_, t};
    SDL_ShowOpenFolderDialog(onDialog, call, win_, startDir.empty() ? nullptr : startDir.c_str(), false);
}

void Launcher::openImage() {
    static const SDL_DialogFileFilter filters[] = {
        {"CD-Abbild (*.cue, *.iso)", "cue;iso;CUE;ISO"},
        {"Alle Dateien", "*"},
    };
    auto* call = new DialogCall{&dialogs_, DialogResult::AddSource};
    SDL_ShowOpenFileDialog(onDialog, call, win_, filters, 2, nullptr, true);
}

void Launcher::pollDialogs() {
    std::vector<DialogResult> rs;
    {
        std::lock_guard lock(dialogs_.m);
        rs.swap(dialogs_.results);
        if (!dialogs_.error.empty()) {
            message_ = dialogs_.error + " – Pfad bitte eintippen.";
            dialogs_.error.clear();
        }
    }
    for (auto& r : rs) {
        switch (r.target) {
        case DialogResult::AddSource:
            for (auto& p : r.paths) s_.sources.push_back(p);
            recheck();
            break;
        case DialogResult::Common:
            s_.common = commonInput_ = r.paths.front();
            recheck();
            break;
        case DialogResult::SaveDir:
            s_.saveDir = saveDirInput_ = r.paths.front();
            rescanSaves();
            break;
        }
    }
}

void Launcher::start(LaunchChoice::Kind k, std::string path) {
    LaunchChoice c;
    c.kind = k;
    c.savePath = std::move(path);
    c.common = s_.common;
    saveLauncherSettings(env_.iniFile, s_);
    pending_ = c;
#ifdef RAH_HAVE_UNSHIELD
    if (!haveCommon() && !installComplete(env_.installDir)) {
        // DATA1.CAB entpacken (wie main.cpp ohne --common), im Hintergrund mit Fortschritt
        working_ = true;
        workDone_ = false;
        progI_ = 0;
        progN_ = 0;
        worker_ = std::thread([this, sources = s_.sources] {
            Vfs cd;
            std::string err;
            bool ok = true;
            for (const auto& p : sources)
                if (!cd.mountPath(p, &err)) ok = false;
            if (ok) ok = installFromCab(cd, env_.installDir, &err, [this](int i, int n) {
                progI_ = i;
                progN_ = n;
            });
            workOk_ = ok;
            workErr_ = err;
            workDone_ = true;
        });
        ImGui::OpenPopup("Entpacken");
    }
#endif
}

std::optional<LaunchChoice> Launcher::finishStart() {
    if (!pending_ || working_) return std::nullopt;
    LaunchChoice c = *pending_;
    pending_.reset();
    if (c.common.empty()) c.common = (env_.installDir / "Common").string();
    return c;
}

void Launcher::extractPopup() {
    const ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    if (!ImGui::BeginPopupModal("Entpacken", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;
    ImGui::TextUnformatted("Common-Dateien und Rent-A-Hero.exe werden aus SETUP/DATA1.CAB entpackt:");
    ImGui::TextDisabled("%s", env_.installDir.string().c_str());
    const int n = progN_, i = progI_;
    char label[32];
    std::snprintf(label, sizeof label, "%d / %d", i, n);
    ImGui::ProgressBar(n > 0 ? float(i) / float(n) : 0.0f, ImVec2(ImGui::GetFontSize() * 28, 0), label);
    if (workDone_) {
        if (worker_.joinable()) worker_.join();
        working_ = false;
        if (!workOk_) {
            message_ = "Entpacken fehlgeschlagen: " + workErr_ +
                       " – alternativ unter „Datenquelle“ einen vorhandenen Common-Ordner angeben.";
            pending_.reset();
        }
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

void Launcher::tabPlay() {
    if (!dataOk()) {
        ImGui::TextColored(kWarn, "Zuerst unter „Datenquelle“ die Spiel-CDs angeben.");
        if (ImGui::Button("Zur Datenquelle")) gotoData_ = true;
        ImGui::Spacing();
    }
    ImGui::BeginDisabled(!dataOk());
    const float bw = ImGui::GetFontSize() * 14;
    const ImVec2 big(bw, ImGui::GetFontSize() * 2.4f);

    ImGui::BeginDisabled(!exitSave_.used);
    if (ImGui::Button("Fortsetzen", big)) start(LaunchChoice::Continue, exitSave_.path);
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() * 0.3f));
    if (exitSave_.used) ImGui::Text("Automatisch beim Beenden gespeichert\n%s", exitSave_.time.c_str());
    else ImGui::TextDisabled("Kein automatischer Spielstand\n(entsteht beim Beenden über das Spielmenü)");
    ImGui::EndGroup();

    if (ImGui::Button("Neues Spiel", big)) start(LaunchChoice::NewGame);
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() * 0.5f));
    ImGui::Checkbox("Intro überspringen", &s_.skipIntro);
    ImGui::EndGroup();
    ImGui::EndDisabled();

    ImGui::Spacing();
    ImGui::SeparatorText("Spielstände");
    ImGui::TextDisabled("Ordner: %s", saveDir().c_str());
    const float tableH = ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing() * 1.2f;
    if (ImGui::BeginTable("saves", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerH | ImGuiTableFlags_ScrollY,
                          ImVec2(0, std::max(tableH, ImGui::GetFontSize() * 6)))) {
        ImGui::TableSetupColumn("Name");
        ImGui::TableSetupColumn("Gespeichert", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 9);
        ImGui::TableHeadersRow();
        for (int i = 0; i < static_cast<int>(saves_.size()); ++i) {
            const SaveEntry& e = saves_[i];
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::BeginDisabled(!e.used);
            ImGui::PushID(i);
            const char* label = e.used ? (e.name.empty() ? "(ohne Namen)" : e.name.c_str()) : "– leer –";
            if (ImGui::Selectable(label, selected_ == i, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
                selected_ = i;
                if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && dataOk()) start(LaunchChoice::Load, e.path);
            }
            ImGui::PopID();
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(e.time.c_str());
            ImGui::EndDisabled();
        }
        ImGui::EndTable();
    }
    const bool canLoad = dataOk() && selected_ >= 0 && selected_ < static_cast<int>(saves_.size()) && saves_[selected_].used;
    ImGui::BeginDisabled(!canLoad);
    if (ImGui::Button("Laden")) start(LaunchChoice::Load, saves_[selected_].path);
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Aktualisieren")) rescanSaves();
    ImGui::SameLine(ImGui::GetContentRegionAvail().x + ImGui::GetCursorPosX() - ImGui::CalcTextSize("Beenden").x -
                    ImGui::GetStyle().FramePadding.x * 2);
    if (ImGui::Button("Beenden")) quit_ = true;
}

void Launcher::tabData() {
    ImGui::TextWrapped("Die beiden Spiel-CDs als Ordner (eingelegte CD, kopierter Inhalt), als .iso oder als "
                       "BIN/CUE-Abbild (.cue wählen). Ein Ordner mit dem zusammengeführten Inhalt beider CDs "
                       "reicht ebenfalls.");
    ImGui::Spacing();
    int remove = -1;
    if (ImGui::BeginTable("sources", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter)) {
        ImGui::TableSetupColumn("Pfad");
        ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, ImGui::GetFontSize() * 6);
        for (int i = 0; i < static_cast<int>(s_.sources.size()); ++i) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("CD-Quelle %d: %s", i + 1, s_.sources[i].c_str());
            ImGui::TableNextColumn();
            ImGui::PushID(i);
            if (ImGui::SmallButton("Entfernen")) remove = i;
            ImGui::PopID();
        }
        if (s_.sources.empty()) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::TextDisabled("Noch keine Datenquelle angegeben.");
        }
        ImGui::EndTable();
    }
    if (remove >= 0) {
        s_.sources.erase(s_.sources.begin() + remove);
        recheck();
    }
    if (ImGui::Button("Ordner hinzufügen …")) openFolder(DialogResult::AddSource, {});
    ImGui::SameLine();
    if (ImGui::Button("Abbild hinzufügen … (.cue/.iso)")) openImage();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFontSize() * 8);
    const bool enter = ImGui::InputTextWithHint("##path", "oder Pfad eintippen, z. B. D:\\ oder /run/media/…", &pathInput_,
                                                ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    if ((ImGui::Button("Hinzufügen") || enter) && !pathInput_.empty()) {
        s_.sources.push_back(pathInput_);
        pathInput_.clear();
        recheck();
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Prüfung");
    if (!checked_) recheck();
    for (const auto& e : check_.errors) ImGui::TextColored(kBad, "Nicht lesbar: %s", e.c_str());
    if (!s_.sources.empty()) {
        ImGui::TextColored(check_.missing.empty() ? kOk : kBad, "Spielordner gefunden: %d von %d", check_.found,
                           check_.expected);
        if (!check_.missing.empty()) {
            std::string m;
            for (const auto& d : check_.missing) m += (m.empty() ? "" : ", ") + d;
            ImGui::TextWrapped("Fehlt: %s", m.c_str());
            ImGui::TextDisabled("Fehlen viele Schauplätze, ist vermutlich nur eine der beiden CDs angegeben.");
        }
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Common-Dateien (Rodrigo, Musik, Inventar)");
    int mode = haveCommon() ? 1 : 0;
    ImGui::BeginDisabled(!env_.canExtract);
    if (ImGui::RadioButton("Aus der CD entpacken (SETUP/DATA1.CAB)", mode == 0)) {
        s_.common.clear();
        recheck();
    }
    ImGui::EndDisabled();
    if (!env_.canExtract) {
        ImGui::SameLine();
        ImGui::TextDisabled("(ohne libunshield gebaut)");
    }
    if (mode == 0) {
        ImGui::Indent();
        if (!s_.sources.empty())
            ImGui::TextColored(check_.hasCab ? kOk : kBad, check_.hasCab ? "SETUP/DATA1.CAB gefunden" : "SETUP/DATA1.CAB fehlt (CD 1)");
        ImGui::TextDisabled("Ziel: %s – %s", env_.installDir.string().c_str(),
                            installed_ ? "bereits entpackt" : "wird beim ersten Start entpackt");
        ImGui::Unindent();
    }
    if (ImGui::RadioButton("Vorhandenen Common-Ordner verwenden (installiertes Original)", mode == 1) && mode == 0) {
        if (commonInput_.empty()) openFolder(DialogResult::Common, {});
        else s_.common = commonInput_;
    }
    if (mode == 1 || !commonInput_.empty()) {
        ImGui::Indent();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFontSize() * 9);
        if (ImGui::InputText("##common", &commonInput_, ImGuiInputTextFlags_EnterReturnsTrue) && !commonInput_.empty())
            s_.common = commonInput_;
        ImGui::SameLine();
        if (ImGui::Button("Durchsuchen …##common")) openFolder(DialogResult::Common, s_.common);
        if (haveCommon())
            ImGui::TextColored(std::filesystem::is_directory(s_.common) ? kOk : kBad, "%s",
                               std::filesystem::is_directory(s_.common) ? "Ordner vorhanden" : "Ordner nicht gefunden");
        ImGui::Unindent();
    }
}

void Launcher::tabOptions() {
    ImGui::SeparatorText("Ton");
    ImGui::Checkbox("Stumm (ohne Tonausgabe)", &s_.mute);
    ImGui::BeginDisabled(s_.mute);
    const float w = ImGui::GetFontSize() * 18;
    ImGui::SetNextItemWidth(w);
    ImGui::SliderInt("Musik", &s_.musicVol, 0, 100, "%d %%");
    ImGui::SetNextItemWidth(w);
    ImGui::SliderInt("Effekte", &s_.sfxVol, 0, 100, "%d %%");
    ImGui::SetNextItemWidth(w);
    ImGui::SliderInt("Sprache", &s_.speechVol, 0, 100, "%d %%");
    ImGui::Checkbox("3D-Ton wie mit Hardware-3D und EAX 1.0", &s_.audio3d);
    ImGui::SetItemTooltip("Das Original nutzt 3D nur mit freien Hardware-3D-Puffern der Soundkarte;\n"
                          "aus = wie ein heutiges System ohne Hardware-3D (AUDIO.md).");
    ImGui::EndDisabled();

    ImGui::SeparatorText("Spiel");
    ImGui::Checkbox("Untertitel", &s_.subtitles);
    ImGui::Checkbox("Intro überspringen (bei „Neues Spiel“)", &s_.skipIntro);
    ImGui::SetItemTooltip("Port-Option: startet direkt in Schauplatz 1, Szene 7 (Ende des Intros).\n"
                          "Das Original zeigt das Intro immer.");
    ImGui::Checkbox("Hotspots anzeigen, solange Tab gedrückt ist", &s_.hotspotKey);
    ImGui::SetItemTooltip("Port-Hilfe: markiert alle anklickbaren Stellen der Szene mit ihrem Namen.\n"
                          "Das Original kennt das nicht; Tab ist dort ohne Funktion.");
    ImGui::Checkbox("Korrektur: Rodrigo springt nicht an unerreichbare Stellen", &s_.fixWalkJump);
    ImGui::SetItemTooltip("Port-Korrektur: Findet der Wegsucher zum Klickpunkt keinen Weg, setzt das\n"
                          "Original die Figur direkt dorthin (z. B. hinter Büsche am Bildrand).\n"
                          "An: Rodrigo bleibt stehen. Aus = wie im Original.");
    ImGui::Checkbox("Beim Schließen des Fensters automatisch speichern", &s_.saveOnClose);
    ImGui::SetItemTooltip("Port-Option: wie „Beenden“ im Spielmenü (SaveGameExit.rah).");

    ImGui::SeparatorText("Bild");
    const bool wasFull = s_.fullscreen;
    if (displayControls(s_.display, p_.supportsFilters(), &s_.fullscreen)) {
        if (wasFull != s_.fullscreen) SDL_SetWindowFullscreen(win_, s_.fullscreen);
        const std::string err = applyDisplay(p_, s_.display);
        if (!err.empty()) message_ = displayLabel(s_.display) + ": " + err;
    }
    ImGui::TextDisabled("Im Spiel: Alt+F Filter, Alt+C CRT, Alt+D Entbänderung, Alt+I Skalierung,\n"
                        "Alt+Enter Vollbild, Alt+O Bild-Einstellungen.");
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 40);
    ImGui::TextDisabled("Ausgabe: %s", p_.info().c_str());
    ImGui::PopTextWrapPos();

    ImGui::SeparatorText("Spielstand-Ordner");
    bool custom = !s_.saveDir.empty();
    if (ImGui::RadioButton("Benutzer-Datenordner (Standard)", !custom)) {
        s_.saveDir.clear();
        rescanSaves();
    }
    ImGui::SameLine();
    ImGui::TextDisabled("%s", env_.defaultSaveDir.c_str());
    if (ImGui::RadioButton("Eigener Ordner", custom) && !custom) {
        if (saveDirInput_.empty()) openFolder(DialogResult::SaveDir, {});
        else {
            s_.saveDir = saveDirInput_;
            rescanSaves();
        }
    }
    if (custom || !saveDirInput_.empty()) {
        ImGui::Indent();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetFontSize() * 9);
        if (ImGui::InputText("##savedir", &saveDirInput_, ImGuiInputTextFlags_EnterReturnsTrue) && !saveDirInput_.empty()) {
            s_.saveDir = saveDirInput_;
            rescanSaves();
        }
        ImGui::SameLine();
        if (ImGui::Button("Durchsuchen …##save")) openFolder(DialogResult::SaveDir, s_.saveDir);
        ImGui::Unindent();
    }
    ImGui::TextDisabled("Der Ordner des installierten Originals (Common) kann auch gewählt werden.");
}

std::optional<LaunchChoice> Launcher::frame() {
    if (firstFrame_) {
        commonInput_ = s_.common;
        saveDirInput_ = s_.saveDir;
        recheck();
        rescanSaves();
    }
    pollDialogs();
    if (!auto_.empty()) {
        const std::string a = std::move(auto_);
        auto_.clear();
        runAuto(a);
    }
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(vp->WorkPos);
    ImGui::SetNextWindowSize(vp->WorkSize);
    ImGui::Begin("Rent-A-Hero", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus);
    ImGui::PushFont(nullptr, ImGui::GetStyle().FontSizeBase * 1.9f);
    ImGui::TextColored(ImVec4(0.90f, 0.70f, 0.32f, 1), "Rent-A-Hero");
    ImGui::PopFont();
    ImGui::SameLine();
    ImGui::TextDisabled("  Source Port – benötigt die Original-CDs (neo Software, 1998/99)");
    ImGui::Spacing();

    if (ImGui::BeginTabBar("tabs")) {
        const bool toData = (firstFrame_ && !dataOk()) || gotoData_ || forceTab_ == 1;
        const bool toPlay = (firstFrame_ && dataOk() && forceTab_ < 0) || forceTab_ == 0;
        gotoData_ = false;
        if (ImGui::BeginTabItem("Spielen", nullptr, toPlay ? ImGuiTabItemFlags_SetSelected : 0)) {
            ImGui::Spacing();
            tabPlay();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Datenquelle", nullptr, toData ? ImGuiTabItemFlags_SetSelected : 0)) {
            ImGui::Spacing();
            tabData();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Optionen", nullptr, forceTab_ == 2 ? ImGuiTabItemFlags_SetSelected : 0)) {
            ImGui::Spacing();
            tabOptions();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    if (working_ && !ImGui::IsPopupOpen("Entpacken")) ImGui::OpenPopup("Entpacken");
    extractPopup();
    if (!message_.empty()) {
        ImGui::OpenPopup("Hinweis");
        ImGui::SetNextWindowPos(vp->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(vp->WorkSize.x * 0.8f, FLT_MAX));
        if (ImGui::BeginPopupModal("Hinweis", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::PushTextWrapPos(vp->WorkSize.x * 0.75f);
            ImGui::TextUnformatted(message_.c_str());
            ImGui::PopTextWrapPos();
            if (ImGui::Button("OK")) {
                message_.clear();
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    }
    ImGui::End();
    firstFrame_ = false;

    if (quit_) {
        if (worker_.joinable()) worker_.join();
        saveLauncherSettings(env_.iniFile, s_);
        return LaunchChoice{};
    }
    return finishStart();
}

}  // namespace

LaunchChoice runLauncher(SDL_Window* win, Presenter& p, LauncherSettings& s, const LauncherEnv& env) {
    SDL_StartTextInput(win);
    p.clearFrame();
    Launcher l(win, p, s, env);
    std::optional<LaunchChoice> result;
    // Prüfmodus: jeden Reiter einmal als <präfix>-<n>.bmp speichern, dann beenden
    const char* shot = SDL_getenv("RAH_LAUNCHER_SHOT");
    int shotFrame = 0;
    while (!result) {
        SDL_Event ev;
        if (shot) {
            l.forceTab(shotFrame / 4);
            while (SDL_PollEvent(&ev)) ImGui_ImplSDL3_ProcessEvent(&ev);
        } else if (SDL_WaitEventTimeout(&ev, 100)) {
            // Leerlauf: auf Ereignisse warten (mit Zeitlimit für Fortschritt und Dateidialoge)
            do {
                ImGui_ImplSDL3_ProcessEvent(&ev);
                if (ev.type == SDL_EVENT_QUIT || ev.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) l.requestQuit();
            } while (SDL_PollEvent(&ev));
        }
        p.imguiNewFrame();
        ImGui::NewFrame();
        result = l.frame();
        ImGui::Render();
        SDL_Surface* grab = nullptr;
        const bool doShot = shot && shotFrame % 4 == 3;
        p.present(true, doShot ? &grab : nullptr);
        if (doShot) {
            if (grab) {
                const std::string f = std::string(shot) + "-" + std::to_string(shotFrame / 4) + ".bmp";
                SDL_SaveBMP(grab, f.c_str());
                SDL_DestroySurface(grab);
            }
            if (shotFrame / 4 == 2) {
                if (const char* a = SDL_getenv("RAH_LAUNCHER_AUTO")) {
                    l.autoStart(a);
                    shot = nullptr;
                } else {
                    l.requestQuit();
                }
            }
        }
        ++shotFrame;
    }
    SDL_StopTextInput(win);
    return *result;
}

}  // namespace rah
