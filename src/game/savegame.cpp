// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/savegame.hpp"

#include <cstdio>
#include <cstring>
#include <fstream>

#include "engine/actor.hpp"
#include "engine/inventory.hpp"
#include "game/game.hpp"

namespace rah {

namespace {

// ReadFile-Verhalten: am Dateiende TRUE mit 0 Bytes – das Ziel bleibt dann unverändert
struct Reader {
    std::ifstream in;
    std::size_t read(void* dst, std::size_t n) {
        in.read(static_cast<char*>(dst), static_cast<std::streamsize>(n));
        const auto got = static_cast<std::size_t>(in.gcount());
        if (in.eof()) in.clear(std::ios::eofbit);
        return got;
    }
    bool ok() const { return !in.bad(); }
    // Teilweise gelesene Werte überschreiben nur die gelesenen Bytes (Little-Endian wie x86)
    bool i32(std::int32_t& v) {
        unsigned char b[4];
        const std::size_t got = read(b, 4);
        unsigned char cur[4];
        std::memcpy(cur, &v, 4);
        for (std::size_t i = 0; i < got; ++i) cur[i] = b[i];
        std::memcpy(&v, cur, 4);
        return ok();
    }
};

void put32(std::ofstream& o, std::int32_t v) {
    unsigned char b[4] = {static_cast<unsigned char>(v), static_cast<unsigned char>(v >> 8),
                          static_cast<unsigned char>(v >> 16), static_cast<unsigned char>(v >> 24)};
    o.write(reinterpret_cast<const char*>(b), 4);
}

// Schauplatz löschen (vtable +0 mit 1) bzw. ohne Spiel CSzene_Clear
void dropCurrent(GameContext& ctx) {
    if (!ctx.current) {
        ctx.scene.reset();  // 0x004487A0 CSzene_Clear
        return;
    }
    ctx.current->shutdown();
    ctx.retired.push_back(std::move(ctx.current));
}

}  // namespace

void copySaveName(std::array<char, 30>& buf, std::string_view s) {
    // strcpy ohne Längenprüfung; Namen sind höchstens 29 Zeichen (GetDlgItemTextA cchMax 30)
    const std::size_t n = std::min(s.size(), buf.size() - 1);
    std::memcpy(buf.data(), s.data(), n);
    buf[n] = '\0';
}

std::string saveName(const std::array<char, 30>& buf) {
    std::size_t n = 0;
    while (n < buf.size() && buf[n] != '\0') ++n;
    return std::string(buf.data(), n);
}

std::string slotPath(const SaveGlobals& s, int slot) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "SaveGame%02d.rah", slot);
    return s.dir_ + buf;
}

std::optional<SaveHeader> readSaveHeader(const std::string& path) {
    Reader r{std::ifstream(path, std::ios::binary)};
    if (!r.in) return std::nullopt;
    SaveHeader h;
    unsigned char b[4];
    h.versionRead = r.read(b, 4) == 4;
    if (h.versionRead) std::memcpy(&h.version, b, 4);
    h.nameBytes = r.read(h.name.data(), h.name.size());
    return h;
}

bool saveRead(GameContext& ctx) {
    SaveGlobals& s = ctx.save;
    Reader r{std::ifstream(s.path, std::ios::binary)};
    if (!r.in) return false;  // CreateFileA(OPEN_EXISTING) gescheitert
    LocationRequest& q = ctx.request;
    if (!r.i32(s.version)) return false;
    if (s.version != 1) return false;
    r.read(s.name.data(), s.name.size());
    // Kopf direkt in die Wechsel-Globals (0x0047D748, 0x0047D744, 0x0047D73C, 0x0047D740)
    if (!r.i32(q.prevLocation) || !r.i32(q.prevScene) || !r.i32(q.nextLocation) || !r.i32(q.nextScene)) return false;
    if (!r.i32(s.x) || !r.i32(s.y) || !r.i32(s.dir)) return false;
    InventoryBar* bar = ctx.scene.inventory();
    if (bar) bar->clear();  // 0x004418D0
    for (int i = 0; i < 100; ++i) {
        std::int32_t has = 0;
        if (!r.i32(has)) return false;  // teilweise geladener Zustand bleibt (Original)
        if (has != 0 && bar) bar->add(i);  // 0x00441960
    }
    std::int32_t size = 0;
    if (!r.i32(size)) return false;
    // Port: Lesegröße auf den Zustandsblock begrenzt (Original liest ungeprüft – Überlauf)
    std::size_t n = size < 0 ? 0 : static_cast<std::size_t>(size);
    if (n > GameState::kSize) n = GameState::kSize;
    std::array<unsigned char, GameState::kSize> block{};
    for (std::size_t i = 0; i < GameState::kSize; i += 4) std::memcpy(&block[i], &ctx.state.at(i), 4);
    r.read(block.data(), n);
    if (!r.ok()) return false;
    for (std::size_t i = 0; i < GameState::kSize; i += 4) std::memcpy(&ctx.state.at(i), &block[i], 4);
    s.restorePos = true;  // 0x0047D764 = 1
    return true;
}

bool saveWrite(GameContext& ctx) {
    Game* game = ctx.current.get();
    if (!game || !game->hero() || !game->hero()->active) return false;
    SaveGlobals& s = ctx.save;
    std::ofstream o(s.path, std::ios::binary | std::ios::trunc);  // CREATE_ALWAYS
    if (!o) return false;
    s.version = 1;
    const Actor* h = game->hero();
    s.x = h->footX;
    s.y = h->footY;
    s.dir = h->dir;
    const LocationRequest& q = ctx.request;
    put32(o, s.version);
    o.write(s.name.data(), static_cast<std::streamsize>(s.name.size()));
    put32(o, q.prevLocation);
    put32(o, q.prevScene);
    put32(o, q.nextLocation);
    put32(o, q.nextScene);
    put32(o, s.x);
    put32(o, s.y);
    put32(o, s.dir);
    const InventoryBar* bar = ctx.scene.inventory();
    for (int i = 0; i < 100; ++i) put32(o, bar && bar->has(i) ? 1 : 0);
    put32(o, static_cast<std::int32_t>(GameState::kSize));
    // state[CSzene+0x5D4] = CGame+0x10 (Hinweis-Countdown der aktuellen Szene)
    const int sc = ctx.scene.sceneNumber();
    if (sc >= 0 && sc < 4000) ctx.state.at(static_cast<std::size_t>(sc) * 4) = game->hintCountdown();
    for (std::size_t i = 0; i < GameState::kSize; i += 4) put32(o, ctx.state.at(i));
    return static_cast<bool>(o);
}

void saveProcess(GameContext& ctx) {
    if (ctx.g.gameMenuOpen) return;
    SaveGlobals& s = ctx.save;
    switch (s.request) {
    case 1:
        s.request = 0;
        dropCurrent(ctx);
        if (saveRead(ctx)) switchLocation(ctx);
        return;
    case 2:
        s.request = 0;
        if (ctx.current) saveWrite(ctx);
        return;
    case 3:
        s.request = 0;
        dropCurrent(ctx);
        ctx.state.reset();                                    // 0x0044DA90
        if (auto* bar = ctx.scene.inventory()) bar->clear();  // 0x004418D0
        ctx.request.nextLocation = 40;
        ctx.request.nextScene = 114;
        ctx.request.prevScene = 0;
        switchLocation(ctx);
        return;
    case 4:
        s.request = 0;
        if (ctx.current && s.canSave) {
            s.path = s.dir_ + "SaveGameExit.rah";
            copySaveName(s.name, "AutoSaveGame");
            saveWrite(ctx);
        }
        if (ctx.postQuit) ctx.postQuit();
        return;
    case 5:
        s.request = 0;
        if (ctx.postQuit) ctx.postQuit();
        return;
    default:
        return;
    }
}

}  // namespace rah
