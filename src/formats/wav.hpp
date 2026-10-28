// SPDX-License-Identifier: GPL-3.0-or-later
// WAV (RIFF/WAVE, docs/formats/media.md). In den Daten nur PCM mono 22 050 Hz, 8/16 Bit.
#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace rah {

struct Wave {
    int formatTag = 0, channels = 0, rate = 0, bits = 0, blockAlign = 0;
    std::uint32_t avgBytesPerSec = 0;
    std::vector<std::uint8_t> data;
    // +0x48: Dauer in ms = Größe · 1000 / nAvgBytesPerSec (64 Bit, abgerundet)
    std::uint32_t durationMs = 0;
};

// orig: Rent-A-Hero.exe 0x0044DF00 CWave_Open (mmio: RIFF/WAVE, "fmt ", "data")
std::optional<Wave> parseWave(std::span<const std::uint8_t> file, std::string* error);

}  // namespace rah
