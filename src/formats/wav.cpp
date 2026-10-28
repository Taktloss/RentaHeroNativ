// SPDX-License-Identifier: GPL-3.0-or-later
#include "formats/wav.hpp"

#include "core/bytes.hpp"

namespace rah {

std::optional<Wave> parseWave(std::span<const std::uint8_t> f, std::string* error) {
    auto fail = [&](const char* m) -> std::optional<Wave> {
        if (error) *error = m;
        return std::nullopt;
    };
    if (f.size() < 12 || le32(f, 0) != 0x46464952u /*RIFF*/ || le32(f, 8) != 0x45564157u /*WAVE*/)
        return fail("kein RIFF/WAVE");
    Wave w;
    bool haveFmt = false, haveData = false;
    // mmioDescend sucht die Unterblöcke unabhängig von ihrer Reihenfolge
    for (std::size_t off = 12; off + 8 <= f.size();) {
        const std::uint32_t id = le32(f, off), size = le32(f, off + 4);
        const std::size_t body = off + 8;
        if (body + size > f.size() && id != 0x61746164u) return fail("Block abgeschnitten");
        if (id == 0x20746D66u /*fmt */ && !haveFmt) {
            if (size < 16) return fail("fmt zu kurz");
            w.formatTag = le16(f, body);
            w.channels = le16(f, body + 2);
            w.rate = static_cast<int>(le32(f, body + 4));
            w.avgBytesPerSec = le32(f, body + 8);
            w.blockAlign = le16(f, body + 12);
            w.bits = le16(f, body + 14);
            haveFmt = true;
        } else if (id == 0x61746164u /*data*/ && !haveData) {
            const std::size_t n = std::min<std::size_t>(size, f.size() - body);
            w.data.assign(f.begin() + static_cast<std::ptrdiff_t>(body), f.begin() + static_cast<std::ptrdiff_t>(body + n));
            haveData = true;
        }
        off = body + size + (size & 1);
    }
    if (!haveFmt || !haveData) return fail("fmt- oder data-Block fehlt");
    if (w.formatTag != 1 || (w.bits != 8 && w.bits != 16) || w.channels < 1 || w.channels > 2 || w.avgBytesPerSec == 0)
        return fail("nur PCM 8/16 Bit mono/stereo unterstützt");  // in den Daten nur PCM mono
    w.durationMs = static_cast<std::uint32_t>(static_cast<std::uint64_t>(w.data.size()) * 1000 / w.avgBytesPerSec);
    return w;
}

}  // namespace rah
