// SPDX-License-Identifier: GPL-3.0-or-later
// Sprachausgabe und Untertitel (CSpeech, docs/SPEECH.md). Der Zustand ist im Original global
// (eine Sprache gleichzeitig) und liegt hier in EngineGlobals; das Objekt hält nur den Text.
#pragma once

#include <string>
#include <string_view>

#include "engine/globals.hpp"
#include "engine/paths.hpp"

namespace rah {

class Speech {
public:
    // orig: Rent-A-Hero.exe 0x00445FB0 CSpeech_ctor – setzt die globalen Ereignisse zurück
    explicit Speech(EngineGlobals& g);
    // orig: Rent-A-Hero.exe 0x004460F0 CSpeech_Load(ger, name, stream). Ohne Audio-Port wird
    // kein Audiopuffer angelegt (SPEECH.md „Port“). Liefert wie das Original immer true.
    bool load(const DataPaths& paths, const std::optional<FoundFile>& ger, std::string_view name, bool stream);
    // orig: Rent-A-Hero.exe 0x004463B0 CSpeech_Play(ereignis)
    void play(int doneHandler);
    const std::string& text() const { return text_; }

private:
    EngineGlobals& g_;
    std::string text_;  // 0x400 Bytes
};

// orig: Rent-A-Hero.exe 0x00446560 – Ende (Handler von g_ClickEvent): Untertitel weg,
// Ereignis „Sprache fertig“ mit Typ 0x1F sofort auslösen
void speechEnd(EngineGlobals& g);
// orig: Rent-A-Hero.exe 0x00446030 – laufende Sprache beenden (löst das Ende aus), Ereignisse aus
void speechStop(EngineGlobals& g);

}  // namespace rah
