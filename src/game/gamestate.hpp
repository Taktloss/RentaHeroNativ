// SPDX-License-Identifier: GPL-3.0-or-later
// Zustandsblock CGameState (CGame+0x78, 0x4110 Bytes; docs/formats/gamestate.md)
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace rah {

class GameState {
public:
    // orig: Rent-A-Hero.exe 0x0044DA60 CGameState_ctor (alles 0, dann Reset)
    GameState() { reset(); }
    // orig: Rent-A-Hero.exe 0x0044DA90 CGameState_Reset – nur die Spielflags; der
    // Hinweis-Countdown 0x0000…0x3E7F bleibt (Original-Eigenheit)
    void reset();
    // int32 am Byte-Offset (wie CGame+0x78 → +Offset)
    std::int32_t& at(std::size_t offset) { return words_[offset / 4]; }

    static constexpr std::size_t kSize = 0x4110;
    static constexpr std::size_t kFlags = 0x3E80;

private:
    std::array<std::int32_t, kSize / 4> words_{};
};

}  // namespace rah
