// SPDX-License-Identifier: GPL-3.0-or-later
#include "video/iv5mask.hpp"

#include <algorithm>

namespace rah {

namespace {
// Bild-Größentabellen 0x10087C90 (Breite) / 0x10087CD0 (Höhe); 0 = ungültig
constexpr int kPicW[15] = {640, 320, 160, 704, 352, 352, 176, 240, 640, 704, 80, 88, 0, 0, 0};
constexpr int kPicH[15] = {480, 240, 120, 480, 240, 288, 144, 180, 240, 240, 60, 72, 0, 0, 0};
}  // namespace

// LE-Bitleser (LSB zuerst) wie in ir50_32.dll; außerhalb der Daten liest er Nullen
class BitReader {
public:
    BitReader(const std::uint8_t* d, std::size_t n, std::size_t bit = 0) : d_(d), n_(n), p_(bit) {}
    std::uint32_t peek(int bits) const {
        std::uint64_t v = 0;
        const std::size_t byte = p_ >> 3;
        for (int i = 0; i < 5; ++i)
            if (byte + static_cast<std::size_t>(i) < n_) v |= std::uint64_t{d_[byte + static_cast<std::size_t>(i)]} << (8 * i);
        return static_cast<std::uint32_t>((v >> (p_ & 7)) & ((std::uint64_t{1} << bits) - 1));
    }
    std::uint32_t get(int bits) {
        const std::uint32_t v = peek(bits);
        p_ += static_cast<std::size_t>(bits);
        return v;
    }
    void skip(std::size_t bits) { p_ += bits; }
    void align() { p_ = (p_ + 7) & ~std::size_t{7}; }
    std::size_t pos() const { return p_; }
    bool overrun() const { return p_ > n_ * 8; }

private:
    const std::uint8_t* d_;
    std::size_t n_;
    std::size_t p_;
};

bool Indeo5Mask::parseGop(BitReader& r) {
    const std::uint32_t g = r.get(8);  // ctx+0xBD
    std::size_t hdrEnd = 0;
    if (g & 1) hdrEnd = 16 + 8 * std::size_t{r.get(16)};  // vom Codec überlesen
    if (g & 0x20) return false;  // geschützt: kommt bei transparenten Videos nicht vor
    if (g & 0x40) return false;  // Kacheln: dito (formats/indeo5-transparenz.md, Prüfung)
    const std::uint32_t cfg = r.get(3);
    if (cfg > 2 && cfg != 6) return false;  // 0x100232D0: Fehler 2
    const int luma = static_cast<int>(cfg & 3) * 3 + 1, chroma = static_cast<int>(cfg >> 2) * 3 + 1;
    const std::uint32_t ps = r.get(4);
    if (ps == 15) {
        const std::uint32_t v = r.get(26);
        w_ = static_cast<int>(v >> 13);
        h_ = static_cast<int>(v & 0x1FFF);
    } else {
        w_ = kPicW[ps];
        h_ = kPicH[ps];
    }
    if (w_ <= 0 || h_ <= 0) return false;
    bands_ = luma + 2 * chroma;
    for (int i = 0; i < luma + chroma; ++i) {  // Bandbeschreibungen (U und V gemeinsam)
        if (r.get(6) & 0x38 && r.get(2) != 0) return false;  // Endmarke ≠ 0 → INT3
    }
    transparent_ = (g & 8) != 0;
    if (transparent_) {
        const std::uint32_t v = r.get(4);
        if (v & 7) return false;  // Fehler 0xE
        if (v & 8) r.skip(24);    // Füllfarbe ctx+0x150 (gemessen und im Bitstrom: Schwarz)
    }
    r.align();
    if (r.get(24) & 0x800000)
        while (r.get(16) & 0x8000) {
        }
    r.align();
    if (hdrEnd && r.pos() != hdrEnd) return false;
    haveGop_ = true;
    return true;
}

void Indeo5Mask::buildCodes() {
    const int rows = static_cast<int>(desc_.size());
    maxLen_ = 0;
    for (int row = 1; row <= rows; ++row) {
        const int pre = row == rows ? row - 1 : row;
        maxLen_ = std::max(maxLen_, pre + desc_[static_cast<std::size_t>(row - 1)]);
    }
    lut_.assign(std::size_t{1} << maxLen_, Code{});
    int sym = 0;
    for (int row = 1; row <= rows && sym <= 255; ++row) {
        const int pre = row == rows ? row - 1 : row;
        const std::uint32_t prefix = (1u << (row - 1)) - 1;  // row−1 Einsen, dann 0
        const int xb = desc_[static_cast<std::size_t>(row - 1)];
        for (int j = 0; j < (1 << xb) && sym <= 255; ++j, ++sym) {
            std::uint32_t rev = 0;  // Bitumkehr (Tabellen 0x100903F0)
            for (int k = 0; k < xb; ++k)
                if (j & (1 << k)) rev |= 1u << (xb - 1 - k);
            const std::uint32_t code = prefix | (rev << pre);
            const int len = pre + xb;
            for (std::uint32_t hi = 0; hi < (1u << (maxLen_ - len)); ++hi)
                lut_[code | (hi << len)] = Code{static_cast<std::uint8_t>(len), static_cast<std::uint8_t>(sym)};
        }
    }
}

bool Indeo5Mask::decodeRuns(const std::uint8_t* data, std::size_t size, int rows, int padW, bool delta) {
    BitReader r(data, size);
    std::uint8_t val = static_cast<std::uint8_t>(r.get(1));
    const std::size_t total = static_cast<std::size_t>(rows) * static_cast<std::size_t>(padW);
    std::vector<std::uint8_t> bits;
    bits.reserve(total);
    while (bits.size() < total) {
        const Code c = lut_[r.peek(maxLen_)];
        if (c.len == 0 || r.overrun()) return false;
        r.skip(c.len);
        if (c.sym == 0) {
            bits.insert(bits.end(), 255, val);  // 255 ohne Wechsel
        } else {
            bits.insert(bits.end(), c.sym, val);
            val ^= 1;
        }
    }
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < w_; ++x) {
            std::uint8_t b = bits[static_cast<std::size_t>(y) * static_cast<std::size_t>(padW) + static_cast<std::size_t>(x)];
            if (delta && y > 0) b ^= mask_[static_cast<std::size_t>(y - 1) * static_cast<std::size_t>(w_) + static_cast<std::size_t>(x)];
            mask_[static_cast<std::size_t>(y) * static_cast<std::size_t>(w_) + static_cast<std::size_t>(x)] = b;
        }
    }
    return true;
}

Indeo5Mask::Result Indeo5Mask::decode(const std::uint8_t* data, std::size_t size) {
    BitReader r(data, size);
    if (r.get(5) != 0x1F) return Result::Unsupported;
    const std::uint32_t type = r.get(3);
    r.get(8);
    if (type > 4) return Result::Unsupported;
    if (type == 0 && !parseGop(r)) {
        haveGop_ = false;
        return Result::Unsupported;
    }
    if (!haveGop_) return Result::Unsupported;
    if (!transparent_) return Result::Opaque;
    // Bildkopf 0x100232D0 (ctx+0xBE)
    const std::uint32_t flags = r.get(8);
    if (flags & 1) r.skip(24);
    if (flags & 0x10) r.skip(16);
    if (flags & 0x20) {
        while (std::uint32_t n = r.get(8)) r.skip(8 * std::size_t{n});
    }
    if ((flags & 0x40) && r.get(3) == 7) r.skip(4 * std::size_t{r.get(4)});
    r.skip(3);
    r.align();
    if (!(flags & 0x80)) return Result::Unsupported;  // ohne Bandlängen nur über die Koeffizienten
    std::size_t b = r.pos() >> 3;
    for (int i = 0; i < bands_; ++i) {  // 0x1001E020: leer → 1 Byte, sonst 24-Bit-Länge
        if (b + 4 > size) return Result::Unsupported;
        b += (data[b] & 1) ? 1 : (std::size_t{data[b + 1]} | std::size_t{data[b + 2]} << 8 | std::size_t{data[b + 3]} << 16);
    }
    if (b >= size) return Result::Unsupported;
    // Maske 0x1004F840
    BitReader m(data + b, size - b);
    if (flags & 1) m.skip(24);  // Länge der Maskendaten
    m.skip(64 * std::size_t{m.get(3)});  // Rechtecke (Begrenzungsrahmen)
    const bool delta = m.get(1) != 0;
    if (m.get(1)) {
        desc_.resize(m.get(4));
        for (auto& x : desc_) x = static_cast<std::uint8_t>(m.get(4));
        if (desc_.empty()) return Result::Unsupported;
        for (auto x : desc_)
            if (x > 8) return Result::Unsupported;
        buildCodes();
    }
    m.align();
    const std::size_t t = b + (m.pos() >> 3);
    if (t >= size) return Result::Unsupported;
    mask_.assign(static_cast<std::size_t>(w_) * static_cast<std::size_t>(h_), 0);
    // Eine Kachel = ganzes Bild (sichtbar, ctx+0xEC = 1)
    BitReader k(data + t, size - t);
    const bool filled = k.get(1) != 0;
    const bool sized = k.get(1) != 0;
    if (filled) {
        std::fill(mask_.begin(), mask_.end(), static_cast<std::uint8_t>(sized));  // Bit 1 = Füllwert
        return Result::Mask;
    }
    std::size_t start = t + 1;
    if (sized) {
        std::uint32_t n = k.get(8);
        if (n == 0xFF) n = k.get(24);
        start = t + ((k.pos() + 7) >> 3);
    }
    if (desc_.empty() || start >= size) return Result::Unsupported;
    const int padW = (w_ + 31) & ~31;
    return decodeRuns(data + start, size - start, h_, padW, delta) ? Result::Mask : Result::Unsupported;
}

}  // namespace rah
