// SPDX-License-Identifier: GPL-3.0-or-later
#include "formats/pe.hpp"

namespace rah {

namespace {
constexpr std::uint32_t kRtCursor = 1, kRtGroupCursor = 12, kSubdir = 0x80000000u;
}

bool PeResources::parse(Bytes file) {
    d_ = std::move(file);
    secs_.clear();
    auto ok = [&](std::size_t off, std::size_t n) { return off + n <= d_.size(); };
    if (!ok(0x40, 0) || le16(d_, 0) != 0x5A4D) return false;
    const std::size_t e = le32(d_, 0x3C);
    if (!ok(e, 24) || le32(d_, e) != 0x00004550) return false;
    const unsigned n = le16(d_, e + 6);
    const std::size_t opt = e + 24, optSize = le16(d_, e + 20);
    if (!ok(opt, 2)) return false;
    const std::size_t dd = opt + (le16(d_, opt) == 0x10B ? 96 : 112);
    if (!ok(dd + 2 * 8, 8)) return false;
    const std::uint32_t rva = le32(d_, dd + 2 * 8);
    for (unsigned i = 0; i < n; ++i) {
        const std::size_t s = opt + optSize + 40 * i;
        if (!ok(s, 40)) return false;
        const std::uint32_t vs = le32(d_, s + 8), va = le32(d_, s + 12), rs = le32(d_, s + 16), ra = le32(d_, s + 20);
        secs_.push_back({va, vs > rs ? vs : rs, ra});
    }
    const auto b = rva ? offset(rva) : std::nullopt;
    if (!b || !ok(*b, 16)) return false;
    base_ = *b;
    return true;
}

std::optional<std::size_t> PeResources::offset(std::uint32_t rva) const {
    for (const auto& s : secs_)
        if (rva >= s.va && rva - s.va < s.size) return std::size_t{s.raw} + (rva - s.va);
    return std::nullopt;
}

std::optional<std::uint32_t> PeResources::child(std::size_t dir, std::optional<std::uint32_t> id) const {
    if (dir + 16 > d_.size()) return std::nullopt;
    const unsigned count = le16(d_, dir + 12) + le16(d_, dir + 14);
    for (unsigned k = 0; k < count; ++k) {
        const std::size_t e = dir + 16 + 8 * k;
        if (e + 8 > d_.size()) return std::nullopt;
        if (!id || le32(d_, e) == *id) return le32(d_, e + 4);
    }
    return std::nullopt;
}

std::optional<std::span<const std::uint8_t>> PeResources::find(std::uint32_t type, std::uint32_t id) const {
    auto t = child(base_, type);
    if (!t || !(*t & kSubdir)) return std::nullopt;
    auto p = child(base_ + (*t & ~kSubdir), id);
    while (p && (*p & kSubdir)) p = child(base_ + (*p & ~kSubdir), std::nullopt);  // erste Sprache
    if (!p || base_ + *p + 8 > d_.size()) return std::nullopt;
    const auto off = offset(le32(d_, base_ + *p));
    const std::uint32_t size = le32(d_, base_ + *p + 4);
    if (!off || *off + size > d_.size()) return std::nullopt;
    return std::span<const std::uint8_t>(d_).subspan(*off, size);
}

std::optional<MonoCursor> loadCursor(const PeResources& pe, std::uint32_t group) {
    const auto g = pe.find(kRtGroupCursor, group);
    if (!g || g->size() < 6) return std::nullopt;
    const unsigned count = le16(*g, 4);
    for (unsigned k = 0; k < count; ++k) {
        if (6 + 14 * (k + 1) > g->size()) break;
        const auto c = pe.find(kRtCursor, le16(*g, 6 + 14 * k + 12));
        if (!c || c->size() < 4 + 40) continue;
        const std::uint32_t hdr = le32(*c, 4);
        const auto w = static_cast<int>(le32(*c, 8)), h2 = static_cast<int>(le32(*c, 12));
        if (le16(*c, 18) != 1 || w <= 0 || h2 <= 0 || h2 % 2) continue;
        const int h = h2 / 2;
        const std::size_t pal = 4 + hdr, stride = static_cast<std::size_t>((w + 31) / 32) * 4;
        const std::size_t xorOff = pal + 8, andOff = xorOff + stride * static_cast<std::size_t>(h);
        if (andOff + stride * static_cast<std::size_t>(h) > c->size()) continue;
        // Palette (BGRA): Index → Weiß, wenn hell
        bool white[2];
        for (int i = 0; i < 2; ++i) {
            const std::size_t q = pal + 4 * static_cast<std::size_t>(i);
            white[i] = (*c)[q] + (*c)[q + 1] + (*c)[q + 2] > 382;
        }
        MonoCursor m;
        m.width = w;
        m.height = h;
        m.hotX = le16(*c, 0);
        m.hotY = le16(*c, 2);
        m.pixels.resize(static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
        for (int y = 0; y < h; ++y) {
            const std::size_t row = static_cast<std::size_t>(h - 1 - y) * stride;  // von unten nach oben
            for (int x = 0; x < w; ++x) {
                const auto bit = [&](std::size_t base) {
                    return ((*c)[base + row + static_cast<std::size_t>(x >> 3)] >> (7 - (x & 7))) & 1;
                };
                const bool mask = bit(andOff), wh = white[bit(xorOff)];
                // AND 0: Farbe; AND 1: XOR-Weiß invertiert, XOR-Schwarz durchsichtig
                m.pixels[static_cast<std::size_t>(y * w + x)] =
                    mask ? (wh ? MonoCursor::Invert : MonoCursor::Transparent)
                         : (wh ? MonoCursor::White : MonoCursor::Black);
            }
        }
        return m;
    }
    return std::nullopt;
}

}  // namespace rah
