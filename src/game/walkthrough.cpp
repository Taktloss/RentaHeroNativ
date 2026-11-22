// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/walkthrough.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>

#include "engine/actor.hpp"
#include "engine/area.hpp"
#include "engine/inventory.hpp"
#include "engine/scene.hpp"
#include "game/game.hpp"
#include "game/gamestate.hpp"
#include "game/savegame.hpp"

namespace rah {

namespace {

int toInt(const std::string& s) {
    return static_cast<int>(std::strtol(s.c_str(), nullptr, 0));
}

constexpr int kBarY = 450;  // Bildschirm-y in der Inventarleiste (Szenen-y 390)

}  // namespace

bool Walkthrough::load(const std::string& path, std::string* error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        if (error) *error = "nicht lesbar: " + path;
        return false;
    }
    std::stringstream ss;
    ss << in.rdbuf();
    return parse(ss.str(), error);
}

bool Walkthrough::parse(const std::string& text, std::string* error) {
    static const char* const kKnown[] = {"wait",  "sleep", "click", "clickxy", "move", "item",
                                         "key",   "expect", "shot", "note", "areas", "end", "save", "load"};
    cmds_.clear();
    std::istringstream in(text);
    std::string line;
    int n = 0;
    while (std::getline(in, line)) {
        ++n;
        if (const auto h = line.find('#'); h != std::string::npos) line.resize(h);
        std::istringstream ls(line);
        Command c;
        c.line = n;
        for (std::string w; ls >> w;) c.args.push_back(w);
        if (c.args.empty()) continue;
        bool ok = false;
        for (const char* k : kKnown) ok = ok || c.args[0] == k;
        if (!ok) {
            if (error) *error = "Zeile " + std::to_string(n) + ": unbekannter Befehl " + c.args[0];
            return false;
        }
        cmds_.push_back(std::move(c));
    }
    pc_ = 0;
    status_ = Status::Running;
    return true;
}

void Walkthrough::say(const std::string& s) const {
    if (log) log(s);
    else std::fprintf(stderr, "%s\n", s.c_str());
}

void Walkthrough::fail(const Command& c, const std::string& why) {
    status_ = Status::Failed;
    message_ = "Zeile " + std::to_string(c.line) + " (" + c.args[0] + "): " + why;
    say("[walk] FEHLER " + message_);
}

std::string Walkthrough::notReady(GameContext& ctx) const {
    const EngineGlobals& g = ctx.g;
    if (g.idleDepth != 1) return "Wartestelle (Tiefe " + std::to_string(g.idleDepth) + ")";
    if (g.gameMenuOpen) return "Spielmenü offen";
    if (ctx.request.pending) return "Schauplatzwechsel";
    if (!ctx.current) return "kein Schauplatz";
    Scene& s = ctx.scene;
    if (!s.clicksEnabled) return "Klicks gesperrt";
    // globales Klick-Ereignis 0x00481EE8 aktiv: ein Klick würde nur Sprache/Sequenz beenden
    // (CSzene_OnClick Schritt 5, speechEnd) – das wäre Überspringen, nicht Spielen
    if (s.globalClick && g.clickEvent.enabled) return "Sprache/Sequenz (Klick überspringt)";
    if (AnimLayer* m = s.modal(); m && m->active && m->running) return "Vollbildobjekt läuft";
    if (const Actor* h = ctx.current->hero()) {
        if (h->targetX != 0 || h->targetY != 0) return "Held läuft";
        if (h->turning) return "Held dreht sich";
        if (h->talking) return "Held spricht";
        if (!h->walkDone) return "Laufauftrag offen";
    }
    if (!inputs_.empty()) return "Eingabe ausstehend";
    return {};
}

bool Walkthrough::findPoint(Scene& s, Area* a, int& x, int& y) const {
    // erst die Mitte, dann ein Raster über das Rechteck – getroffen wird genau wie beim Klick
    const Rect r = a->rect;
    const int cx = (r.left + r.right) / 2, cy = (r.top + r.bottom) / 2;
    if (s.hitTest(cx, cy) == a) {
        x = cx;
        y = cy;
        return true;
    }
    for (int yy = r.top; yy < r.bottom; yy += 2) {
        for (int xx = r.left; xx < r.right; xx += 2) {
            if (s.hitTest(xx, yy) == a) {
                x = xx;
                y = yy;
                return true;
            }
        }
    }
    return false;
}

bool Walkthrough::takeInput(GameContext& ctx) {
    if (inputs_.empty()) return false;
    const Input in = inputs_.front();
    inputs_.erase(inputs_.begin());
    Scene& cs = ctx.curScene();
    switch (in.kind) {
    case Input::MouseMove:
        if (mouseMove) mouseMove(in.x, in.y);
        else cs.onMouseMove(in.x, in.y);
        break;
    case Input::Click: {
        // wie WndProc: WM_LBUTTONDOWN → CActor_ClearRun (0x00435330), OnClick
        if (Actor* h = cs.actor(0)) h->clearRun();
        cs.onClick(in.x, in.y);
        break;
    }
    case Input::Key: cs.onKeyDown(in.vk); break;
    case Input::None: break;
    }
    return true;
}

Walkthrough::Snapshot Walkthrough::snapshot(GameContext& ctx) const {
    Snapshot sn;
    if (ctx.current) {
        sn.location = ctx.current->location();
        sn.scene = ctx.current->sceneNumber();
    }
    if (const InventoryBar* bar = ctx.scene.inventory()) {
        for (int i = 0; i < 100; ++i)
            if (bar->has(i)) sn.items.push_back(i);
    }
    for (std::size_t o = GameState::kFlags; o < GameState::kSize; o += 4) sn.flags.push_back(ctx.state.at(o));
    return sn;
}

bool Walkthrough::compare(const Command& c, const Snapshot& a, const Snapshot& b) {
    if (a.location != b.location || a.scene != b.scene) {
        fail(c, "nach dem Laden Szene " + std::to_string(b.location) + "/" + std::to_string(b.scene) +
                    " statt " + std::to_string(a.location) + "/" + std::to_string(a.scene));
        return false;
    }
    if (a.items != b.items) {
        std::string x, y;
        for (int i : a.items) x += " " + std::to_string(i);
        for (int i : b.items) y += " " + std::to_string(i);
        fail(c, "Gegenstände nach dem Laden:" + y + " statt" + x);
        return false;
    }
    // Flags: Abweichungen nur melden – der Schauplatz-Konstruktor läuft beim Laden erneut und darf
    // Flags wie beim Betreten setzen (Original-Verhalten, z. B. Besuchszähler)
    std::string diff;
    int n = 0;
    for (std::size_t i = 0; i < a.flags.size(); ++i) {
        if (a.flags[i] == b.flags[i]) continue;
        if (++n <= 8) {
            char buf[48];
            std::snprintf(buf, sizeof buf, " 0x%04zX:%d→%d", GameState::kFlags + i * 4, a.flags[i], b.flags[i]);
            diff += buf;
        }
    }
    if (n > 0) say("[walk]   Flags nach dem Laden geändert (" + std::to_string(n) + "):" + diff);
    return true;
}

Walkthrough::Status Walkthrough::onQuit() {
    if (status_ != Status::Running) return status_;
    if (pc_ < cmds_.size() && cmds_[pc_].args[0] == "end" && pc_ + 1 == cmds_.size()) {
        status_ = Status::Done;
        say("[walk] Spiel beendet (erwartet) – fertig");
        return status_;
    }
    if (pc_ < cmds_.size()) fail(cmds_[pc_], "Spiel hat sich vorzeitig beendet");
    else status_ = Status::Done;
    return status_;
}

Walkthrough::Status Walkthrough::onIdle(GameContext& ctx) {
    if (status_ != Status::Running) return status_;
    if (pc_ >= cmds_.size()) {
        status_ = Status::Done;
        say("[walk] fertig");
        return status_;
    }
    const std::uint32_t now = ctx.g.now();
    const Command& c = cmds_[pc_];
    if (!started_) {
        started_ = true;
        t0_ = now;
        wasReady_ = false;
        itemPhase_ = 0;
    }
    // Warte-Befehle brauchen keine Bereitschaft; alle Eingaben warten auf „bereit“ (kurz)
    if (runCommand(ctx, c)) {
        ++pc_;
        started_ = false;
    }
    return status_;
}

bool Walkthrough::runCommand(GameContext& ctx, const Command& c) {
    const std::uint32_t now = ctx.g.now();
    const auto& a = c.args;
    auto arg = [&](std::size_t i) -> std::string { return i < a.size() ? a[i] : std::string(); };
    auto where = [&] {
        char b[64];
        std::snprintf(b, sizeof b, "[walk %7.1fs] %d/%d L%d:", now / 1000.0,
                      ctx.current ? ctx.current->location() : -1, ctx.current ? ctx.current->sceneNumber() : -1,
                      c.line);
        return std::string(b);
    };
    auto full = [&] {
        std::string s;
        for (const auto& w : a) s += " " + w;
        return s;
    };
    // Bereitschaft über eine Mindestzeit
    auto stableFor = [&](std::uint32_t ms) {
        if (!ready(ctx)) {
            wasReady_ = false;
            return false;
        }
        if (!wasReady_) {
            wasReady_ = true;
            readySince_ = now;
        }
        return now - readySince_ >= ms;
    };
    auto timedOut = [&](std::uint32_t limit) {
        if (now - t0_ <= limit) return false;
        fail(c, "Zeitüberschreitung (" + std::to_string(limit) + " ms Spielzeit), zuletzt: " +
                    (ready(ctx) ? std::string("bereit") : notReady(ctx)));
        return true;
    };

    const std::string& op = a[0];
    if (op == "note") {
        say(where() + full());
        return true;
    }
    if (op == "save" || op == "load") {
        // über den Spielstand-Auftrag wie das Spielmenü (g_SaveRequest 2/1, Save_Process 0x004045C0)
        SaveGlobals& sv = ctx.save;
        const std::string name = arg(1).empty() ? std::string("walk") : arg(1);
        if (itemPhase_ == 0) {
            if (!stableFor(200)) {
                timedOut(defaultTimeoutMs);
                return false;
            }
            if (op == "save") {
                // wie Save_Write 0x00404930 / GameMenu_Open (canSave): nur mit aktivem Helden
                const Actor* h = ctx.current ? ctx.current->hero() : nullptr;
                if (!h || !h->active) {
                    skipped_[name] = true;
                    say(where() + full() + " übersprungen (kein aktiver Held, Speichern wie im Original gesperrt)");
                    return true;
                }
                skipped_.erase(name);
            }
            if (op == "load" && skipped_.count(name)) {
                say(where() + full() + " übersprungen");
                return true;
            }
            if (op == "load" && snaps_.find(name) == snaps_.end()) {
                fail(c, "kein Spielstand " + name + " gespeichert");
                return false;
            }
            if (op == "save") snaps_[name] = snapshot(ctx);
            sv.path = sv.dir_ + "Walk_" + name + ".rah";
            copySaveName(sv.name, name);
            sv.request = op == "save" ? 2 : 1;
            itemPhase_ = 1;
            wasReady_ = false;
            return false;
        }
        if (sv.request != 0) return false;  // noch nicht verarbeitet
        if (op == "save") {
            std::ifstream f(sv.path, std::ios::binary);
            if (!f) {
                fail(c, "Spielstand nicht geschrieben: " + sv.path);
                return false;
            }
            say(where() + full() + " ok");
            return true;
        }
        if (!stableFor(defaultStableMs)) {
            timedOut(defaultTimeoutMs);
            return false;
        }
        if (!compare(c, snaps_[name], snapshot(ctx))) return false;
        say(where() + full() + " ok");
        return true;
    }
    if (op == "end") {
        // wartet, bis sich das Spiel selbst beendet (onQuit); läuft nie von allein weiter
        timedOut(defaultTimeoutMs * 4);
        return false;
    }
    if (op == "sleep") {
        return now - t0_ >= static_cast<std::uint32_t>(toInt(arg(1)));
    }
    if (op == "wait") {
        // wait [ruhig-ms] [max-ms]
        const auto st = a.size() > 1 ? static_cast<std::uint32_t>(toInt(arg(1))) : defaultStableMs;
        const auto to = a.size() > 2 ? static_cast<std::uint32_t>(toInt(arg(2))) : defaultTimeoutMs;
        if (stableFor(st)) return true;
        timedOut(to);
        return false;
    }
    if (op == "areas") {
        // anklickbare Bereiche der Szene (aktiv und an mindestens einem Punkt treffbar)
        if (!stableFor(200)) {
            timedOut(defaultTimeoutMs);
            return false;
        }
        Scene& s = ctx.curScene();
        std::string list;
        for (int i = 0; i < Scene::kMaxAreas && s.area(i); ++i) {
            Area* ar = s.area(i);
            int x = 0, y = 0;
            if (ar->active && findPoint(s, ar, x, y)) list += " " + ar->name;
        }
        const InventoryBar* bar = ctx.scene.inventory();
        std::string items;
        for (int i = 0; bar && i < bar->count(); ++i) items += " " + std::to_string(bar->entry(i).id);
        say(where() + " Bereiche:" + list + (items.empty() ? "" : " | Gegenstände:" + items));
        return true;
    }
    if (op == "shot") {
        if (shot) shot(arg(1));
        say(where() + full());
        return true;
    }
    if (op == "expect") {
        const std::string what = arg(1);
        bool ok = true;
        std::string got;
        if (what == "scene") {
            const int loc = toInt(arg(2)), sc = toInt(arg(3));
            ok = ctx.current && ctx.current->location() == loc && ctx.current->sceneNumber() == sc;
            got = ctx.current ? std::to_string(ctx.current->location()) + "/" +
                                    std::to_string(ctx.current->sceneNumber())
                              : "keine";
        } else if (what == "item" || what == "noitem") {
            const InventoryBar* bar = ctx.scene.inventory();
            const bool has = bar && bar->has(toInt(arg(2)));
            ok = (what == "item") == has;
            got = has ? "vorhanden" : "fehlt";
        } else if (what == "flag") {
            const std::int32_t v = ctx.state.at(static_cast<std::size_t>(toInt(arg(2))));
            ok = v == toInt(arg(3));
            got = std::to_string(v);
        } else if (what == "area") {
            Area* ar = ctx.curScene().getArea(arg(2));
            ok = ar && ar->active;
            got = ar ? (ar->active ? "aktiv" : "inaktiv") : "fehlt";
        } else {
            fail(c, "unbekannte Erwartung " + what);
            return false;
        }
        if (!ok) {
            fail(c, "erwartet" + full().substr(7) + ", ist " + got);
            return false;
        }
        say(where() + full() + " ok");
        return true;
    }

    // Eingaben: erst bereit (kurz), dann genau einmal einreihen
    if (!stableFor(op == "key" ? 0 : 200)) {
        timedOut(defaultTimeoutMs);
        return false;
    }
    if (op == "click" || op == "move") {
        Scene& s = ctx.curScene();
        Area* ar = s.getArea(arg(1));
        if (!ar) {
            fail(c, "Bereich " + arg(1) + " gibt es in dieser Szene nicht");
            return false;
        }
        if (!ar->active) {
            fail(c, "Bereich " + arg(1) + " ist nicht aktiv");
            return false;
        }
        int x = 0, y = 0;
        if (!findPoint(s, ar, x, y)) {
            fail(c, "Bereich " + arg(1) + " ist nirgends anklickbar (verdeckt/transparent)");
            return false;
        }
        inputs_.push_back({Input::MouseMove, x, y + 60, 0});
        if (op == "click") inputs_.push_back({Input::Click, x, y + 60, 0});
        say(where() + full() + " → (" + std::to_string(x) + "," + std::to_string(y + 60) + ")");
        return true;
    }
    if (op == "clickxy") {
        const int x = toInt(arg(1)), y = toInt(arg(2));
        inputs_.push_back({Input::MouseMove, x, y, 0});
        inputs_.push_back({Input::Click, x, y, 0});
        say(where() + full());
        return true;
    }
    if (op == "key") {
        inputs_.push_back({Input::Key, 0, 0, toInt(arg(1))});
        say(where() + full());
        return true;
    }
    if (op == "item") {
        // Platz suchen, bis er sichtbar ist am Rand blättern (je Mausbewegung ein Platz,
        // Handler 0x00441FC0), dann darauf zeigen und klicken
        const InventoryBar* bar = ctx.scene.inventory();
        const int id = toInt(arg(1));
        int k = -1;
        if (bar) {
            for (int i = 0; i < bar->count(); ++i) {
                if (bar->entry(i).id == id) k = i;
            }
        }
        if (k < 0) {
            fail(c, "Gegenstand " + arg(1) + " nicht in der Leiste");
            return false;
        }
        const int first = bar->first();
        if (k < first) {
            inputs_.push_back({Input::MouseMove, 2, kBarY, 0});
            return false;  // nach dem Blättern erneut prüfen
        }
        if (k >= first + 10) {
            inputs_.push_back({Input::MouseMove, 637, kBarY, 0});
            return false;
        }
        const int x = (k - first) * 64 + 32;
        inputs_.push_back({Input::MouseMove, x, kBarY, 0});
        inputs_.push_back({Input::Click, x, kBarY, 0});
        say(where() + full() + " → Platz " + std::to_string(k));
        return true;
    }
    fail(c, "nicht umgesetzt");
    return false;
}

}  // namespace rah
