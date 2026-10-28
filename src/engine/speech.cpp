// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/speech.hpp"

#include <cstdio>

#include "engine/inventory.hpp"
#include "formats/ini.hpp"

namespace rah {

Speech::Speech(EngineGlobals& g) : g_(g) {
    g.speechRunning = false;
    g.clickEvent.enabled = false;
    g.clickEvent.pending = false;
    EngineGlobals* gp = &g;
    g.clickEvent.setHandler([gp](Event&, int) { speechEnd(*gp); });  // FUN_0043F240(…, 0x00446560)
    g.speechDone.enabled = false;
    g.speechDone.pending = false;
    g.subtitleRect = {40, 424, 600, 475};
}

bool Speech::load(const DataPaths& paths, const std::optional<FoundFile>& ger, std::string_view name, bool) {
    text_ = "NULL";
    if (ger) {
        if (const auto data = ger->read()) text_ = IniFile(*data).getString("SPEECH", name, "NULL", 0x400);
    }
    if (text_ == "NULL") {
        char buf[0x400];
        std::snprintf(buf, sizeof buf, "Sorry, but Speech [%.*s] not found...", static_cast<int>(name.size()),
                      name.data());
        text_ = buf;
    }
    // <name>.WAV (und Lippensync) – Audiopuffer folgt mit dem Audio-Port
    if (g_.audioTrace && paths.findFile(name, "WAV")) g_.audioTrace("Sprache " + std::string(name));
    return true;
}

void Speech::play(int doneHandler) {
    // Figuren-Animation (0x00481EA0) folgt mit CActor. Ohne Audiopuffer: Dauer nach Textlänge.
    std::uint32_t ms = static_cast<std::uint32_t>(static_cast<int>(text_.size()) / 20 * 1000);
    if (ms < 1000) ms = 1000;
    setEventTimer(g_, g_.clickEvent, ms, 0);
    g_.speechRunning = true;
    g_.speechDone.setHandler(doneHandler);
    g_.clickEvent.enabled = true;
    g_.speechDone.enabled = true;
    if (g_.subtitles) {
        rendererClearStatusText(g_);
        rendererSetStatusText(g_, g_.subtitleRect, text_);
    }
}

void speechEnd(EngineGlobals& g) {
    g.clickEvent.enabled = false;
    g.clickEvent.pending = false;
    rendererClearStatusText(g);
    g.speechRunning = false;
    // Audiopuffer abmelden/stoppen und Figur anhalten folgen mit Audio-Port und CActor
    g.speechDone.mark(g, 0x1F);
    fireEvent(g, g.speechDone, g.speechDone.type);
}

void speechStop(EngineGlobals& g) {
    if (g.speechRunning) speechEnd(g);  // 0x004460C0
    g.clickEvent.enabled = false;
    g.clickEvent.pending = false;
    g.speechDone.enabled = false;
    g.speechDone.pending = false;
}

}  // namespace rah
