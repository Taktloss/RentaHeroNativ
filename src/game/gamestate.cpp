// SPDX-License-Identifier: GPL-3.0-or-later
#include "game/gamestate.hpp"

namespace rah {

namespace {
// Startwerte der 164 Spielflags ab 0x3E80 (Spalte „Start“ in formats/gamestate.md)
constexpr std::int32_t kStart[164] = {
     0,  0,  0,  0,  0,  1,  1,  0,  0,  1,  1,  1,  1,  1,  1,  1,
     1,  0,  0,  0,  0,  0,  1,  1,  1,  0,  0,  1,  1,  0,  1,  1,
     0,  1,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,
     0,  0,  0,  0,  1,  0,  0,  1,  0,  1,  1,  0,  1,  1,  1,  0,
     0,  0,  1,  1,  1,  1,  0,  0,  0,  1,  0,  1,  1,  1,  0,  0,
     0,  0,  0,  1,  0,  1,  1,  1,  0,  0,  1,  0,  0,  0,  0,  0,
     0,  1,  1,  1,  1,  0,  0, -1,  1,  0,  0,  0,  1,  1,  0,  0,
     1,  1,  0,  1,  1,  1,  1,  0,  1,  0,  0,  0,  0,  0,  0,  1,
     0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  0,  1,  1,  1,  0,  0,
     1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  1,  1,  0,  0,  1,  0,
     0,  1,  1,  1,
};
}  // namespace

void GameState::reset() {
    for (std::size_t i = 0; i < 164; ++i) words_[kFlags / 4 + i] = kStart[i];
}

}  // namespace rah
