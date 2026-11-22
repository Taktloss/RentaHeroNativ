// SPDX-License-Identifier: GPL-3.0-or-later
// Durchspiel-Treiber (Port-Werkzeug, kein Original-Code): liest ein Skript mit Klicks auf
// benannte Bereiche, Gegenständen, Tasten und Erwartungen und spielt es im laufenden Spiel ab.
// Eingaben gehen wie echte Nachrichten über GameContext::pumpMessage; „bereit“ heißt: keine
// Wartestelle aktiv, kein Wechsel/Menü/Vollbildobjekt, Klicks erlaubt, Held steht – für eine
// Mindestzeit ohne Unterbrechung (docs/WALKTHROUGH.md).
#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace rah {

struct GameContext;
struct Area;
class Scene;

class Walkthrough {
public:
    enum class Status { Running, Done, Failed };

    bool load(const std::string& path, std::string* error);
    bool parse(const std::string& text, std::string* error);

    // Einmal je Game_Idle (jede Verschachtelungstiefe), nach dem Bildaufbau
    Status onIdle(GameContext& ctx);
    // Aus pumpMessage: liegt eine Eingabe an, wird sie ausgeführt → true (= eine Nachricht)
    bool takeInput(GameContext& ctx);

    // Spiel hat sich beendet: Erfolg nur, wenn der laufende Befehl `end` ist (danach nichts mehr)
    Status onQuit();

    const std::string& message() const { return message_; }
    Status status() const { return status_; }

    // Ausgabe der Schritte (Standard: stderr)
    std::function<void(const std::string&)> log;
    // Mausbewegung mit Zeigerwechsel wie im Spiel (main.cpp); fehlt → Scene::onMouseMove
    std::function<void(int x, int y)> mouseMove;
    // Bildschirmfoto des aktuellen Bilds (für `shot`)
    std::function<void(const std::string& path)> shot;

    std::uint32_t defaultStableMs = 1500;    // `wait` ohne Angabe
    std::uint32_t defaultTimeoutMs = 300000; // Spielzeit bis zum Abbruch einer Warte-Anweisung

private:
    struct Command {
        int line = 0;
        std::vector<std::string> args;
    };
    struct Input {
        enum Kind { None, Click, MouseMove, Key } kind = None;
        int x = 0, y = 0, vk = 0;
    };

    bool ready(GameContext& ctx) const { return notReady(ctx).empty(); }
    std::string notReady(GameContext& ctx) const;  // leer = bereit, sonst Grund
    bool runCommand(GameContext& ctx, const Command& c);  // true = fertig, nächster Befehl
    bool findPoint(Scene& s, Area* a, int& x, int& y) const;
    void fail(const Command& c, const std::string& why);
    void say(const std::string& s) const;

    std::vector<Command> cmds_;
    std::size_t pc_ = 0;
    Status status_ = Status::Running;
    std::string message_;
    std::vector<Input> inputs_;
    // Zustand des laufenden Befehls
    bool started_ = false;
    std::uint32_t t0_ = 0;          // Start des Befehls (Spielzeit)
    std::uint32_t readySince_ = 0;  // seit wann ununterbrochen bereit
    bool wasReady_ = false;
    int itemPhase_ = 0;
    // save/load: Zustand beim Speichern, nach dem Laden verglichen
    struct Snapshot {
        int location = -1, scene = -1;
        std::vector<int> items;               // als Menge (der Spielstand speichert keine Reihenfolge)
        std::vector<std::int32_t> flags;      // GameState ab kFlags
    };
    std::map<std::string, Snapshot> snaps_;
    std::map<std::string, bool> skipped_;  // Speichern hier nicht möglich (kein aktiver Held)
    Snapshot snapshot(GameContext& ctx) const;
    bool compare(const Command& c, const Snapshot& before, const Snapshot& after);
};

}  // namespace rah
