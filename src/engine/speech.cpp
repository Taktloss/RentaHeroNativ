// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/speech.hpp"

#include <cstdio>

#include "engine/animlayer.hpp"
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
    g.speaker = nullptr;
    g.speakerActor = false;
    g.lipSyncLoaded = false;
    g.lipSyncData.clear();
    g.lipSyncRate = 20;
}

void speechSetSpeaker(EngineGlobals& g, AnimLayer* layer, int mode, int from, int to) {
    g.speaker = layer;
    g.speakerFrom = from;
    g.speakerTo = to;
    g.speakerMode = mode;
    g.speakerActor = mode == -1;
}

bool parseLipSync(const Bytes& file, std::vector<std::uint16_t>& out, int& rate) {
    std::vector<std::uint8_t> buf(file.size() + 100, 0xFF);
    const auto u16 = [&](std::size_t i) {
        return i * 2 + 1 < file.size() ? static_cast<std::uint16_t>(file[i * 2] | (file[i * 2 + 1] << 8)) : 0xFFFF;
    };
    std::size_t from = 0;
    if (file.size() >= 2 && u16(0) == 0xFFFF) {  // Kopf (ReadFile 10 Bytes)
        rate = u16(2);
        from = u16(1);
    } else {
        rate = 20;
    }
    if (from > file.size()) return false;  // ReadFile mit negativer Länge scheitert
    std::copy(file.begin() + static_cast<std::ptrdiff_t>(from), file.end(), buf.begin());
    out.resize(buf.size() / 2);
    for (std::size_t i = 0; i < out.size(); ++i) out[i] = static_cast<std::uint16_t>(buf[i * 2] | (buf[i * 2 + 1] << 8));
    return true;
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
    if (g_.speechBuffer && !stream) {
        if (const auto acs = paths.findFile(name, "ACS")) {
            g_.lipSyncLoaded = false;  // alte Daten freigeben
            g_.lipSyncData.clear();
            if (const auto data = acs->read()) {
                int rate = 20;
                if (parseLipSync(*data, g_.lipSyncData, rate)) {
                    g_.lipSyncRate = rate;
                    g_.lipSyncLoaded = true;
                }
            }
        }
    }
    return true;
}

void Speech::play(int doneHandler) {
    if (AudioBuffer* b = g_.speechBuffer) {
        b->setEndEvent(&g_.clickEvent);  // 0x004375E0: Ende → g_ClickEvent
        if (g_.speaker) {
            // X-Mitte des Sprechers 0…639 → −0.2…0.2 (0x00447CB0), z = 0.1
            const Rect r = g_.speaker->getRect();
            const float cx = static_cast<float>((r.right - r.left) / 2 + r.left);
            const float x = (cx - 0.0f) / (639.0f - 0.0f) * (0.2f - -0.2f) + -0.2f;
            b->setPos3D(x, 0.0f, 0.1f);
        }
        b->play();
        g_.speechRunning = true;
    }
    if (g_.speaker) {
        if (!g_.speakerActor) {
            if (!g_.lipSyncLoaded || !g_.speechBuffer)
                g_.speaker->playRangeNoEvent(0x0D, g_.speakerFrom, g_.speakerTo);
            else
                g_.speaker->setLipSync(&g_.lipSyncData, g_.lipSyncRate, g_.speechBuffer, 0);
        }
        // Figur-Variante (0x00435DF0 / 0x00435E00) folgt mit CActor
    }
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
    if (g.speaker && !g.speakerActor) {
        g.speaker->stop(0);  // vtable +0x24(0)
        if (g.speakerMode == 10) g.speaker->disable();
        else if (g.speakerMode == 11) {
            if (g.speakerTo != -1) g.speaker->showFrame(g.speakerTo);
        } else if (g.speakerMode == 12) {
            g.speaker->showFrame(g.speakerFrom != -1 ? g.speakerFrom : 0);
        }
    }
    // Figur-Variante (0x00435F10) folgt mit CActor
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
    // Figur-Variante: läuft und Figur → 0x00435F10 (folgt mit CActor)
    g.lipSyncLoaded = false;
    g.lipSyncData.clear();
}

}  // namespace rah
