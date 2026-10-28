// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/speech.hpp"

#include <cstdio>

#include "engine/audio.hpp"
#include "engine/inventory.hpp"
#include "formats/ini.hpp"

namespace rah {

Speech::Speech(EngineGlobals& g) : g_(g) {
    g.speechRunning = false;
    g.speechBuffer = nullptr;  // 0x00481EB0 = 0
    g.clickEvent.enabled = false;
    g.clickEvent.pending = false;
    EngineGlobals* gp = &g;
    g.clickEvent.setHandler([gp](Event&, int) { speechEnd(*gp); });  // FUN_0043F240(…, 0x00446560)
    g.speechDone.enabled = false;
    g.speechDone.pending = false;
    g.subtitleRect = {40, 424, 600, 475};
}

bool Speech::load(const DataPaths& paths, const std::optional<FoundFile>& ger, std::string_view name, bool stream) {
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
    // <name>.WAV vorhanden → Puffer (stream: Streamklasse), Open(pfad, g_Audio, 3).
    // Port: ohne Audio-Manager (kein Tonausgabegerät) kein Puffer → Timer-Weg.
    const auto wav = paths.findFile(name, "WAV");
    if (wav && g_.audio) {
        AudioBuffer* b = stream ? new AudioStream : new AudioBuffer;
        b->open(&*wav, g_.audio, kAudioSpeech);
        g_.speechBuffer = b;
    }
    // Lippensync-Datei (nur ohne Stream) folgt mit den Figuren
    return true;
}

void Speech::play(int doneHandler) {
    if (AudioBuffer* b = g_.speechBuffer) {
        b->setEndEvent(&g_.clickEvent);  // 0x004375E0: Ende → g_ClickEvent
        // 3D-Position aus der Figur folgt mit CActor
        b->play();
        g_.speechRunning = true;
    }
    // Figuren-Animation (0x00481EA0) folgt mit CActor
    if (!g_.speechBuffer) {
        // ohne Audiopuffer: Dauer nach Textlänge
        std::uint32_t ms = static_cast<std::uint32_t>(static_cast<int>(text_.size()) / 20 * 1000);
        if (ms < 1000) ms = 1000;
        setEventTimer(g_, g_.clickEvent, ms, 0);
        g_.speechRunning = true;
    }
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
    if (g.speechBuffer) g.speechBuffer->stop();  // 0x004375E0(0) wirkt nicht; vtable +0x14
    // Figur anhalten bzw. Bild setzen folgt mit CActor
    g.speechDone.mark(g, 0x1F);
    fireEvent(g, g.speechDone, g.speechDone.type);
}

void speechStop(EngineGlobals& g) {
    if (g.speechRunning) speechEnd(g);  // 0x004460C0
    if (g.speechBuffer) g.speechBuffer->stop();
    g.clickEvent.enabled = false;
    g.clickEvent.pending = false;
    g.speechDone.enabled = false;
    g.speechDone.pending = false;
    if (g.speechBuffer) {
        g.speechBuffer->stop();
        delete g.speechBuffer;  // Original lässt den Zeiger stehen; der nächste CSpeech_ctor setzt 0
        g.speechBuffer = nullptr;
    }
    // Lippensync-Daten freigeben folgt mit den Figuren
}

}  // namespace rah
