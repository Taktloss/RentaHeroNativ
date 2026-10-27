// SPDX-License-Identifier: GPL-3.0-or-later
#include "formats/tcc.hpp"

#include <algorithm>

namespace rah {

namespace {

constexpr std::size_t kSlack = 0x10;  // Pufferreserve wie im Original

}  // namespace

bool tccRleDecode(std::span<const std::uint8_t> src, std::span<std::uint8_t> dst) {
    std::size_t pos = 0, out = 0;
    while (pos < src.size()) {
        if (pos + 2 > src.size()) return false;
        const std::uint8_t op = src[pos];
        const std::size_t n = src[pos + 1];
        if (out + n > dst.size()) return false;
        if (op & 1) {  // n × Wert
            if (pos + 3 > src.size()) return false;
            std::fill_n(dst.begin() + static_cast<std::ptrdiff_t>(out), n, src[pos + 2]);
            pos += 3;
        } else if (op & 2) {  // n Zielbytes überspringen
            pos += 2;
        } else {  // n Bytes kopieren
            if (pos + 2 + n > src.size()) return false;
            std::copy_n(src.begin() + static_cast<std::ptrdiff_t>(pos + 2), n,
                        dst.begin() + static_cast<std::ptrdiff_t>(out));
            pos += 2 + n;
        }
        out += n;
    }
    return true;
}

void tccExpand4(std::span<std::uint8_t> plane, std::size_t n) {
    std::size_t i3 = n;
    while (i3 >= 2 && i3 - 2 > 0) {
        const std::size_t i2 = i3 - 2;
        const std::uint8_t b = plane[i2 / 2];
        plane[i2] = static_cast<std::uint8_t>(b & 0xF0);
        plane[i3 - 1] = static_cast<std::uint8_t>(b << 4);
        i3 = i2;
    }
}

bool tccReadHeader(std::span<const std::uint8_t> data, std::size_t off, TccHeader& h) {
    if (off + TccHeader::kSize > data.size()) return false;
    h.flags = le32(data, off);
    for (std::size_t i = 0; i < 4; ++i) h.sizes[i] = le32(data, off + 4 + 4 * i);
    h.width = le32(data, off + 20);
    h.height = le32(data, off + 24);
    return true;
}

void TccColorAdd::apply(TccPlanes& p) const {
    const int add[3] = {r, g, b};
    const std::size_t n = std::size_t{p.width} * p.height;
    for (std::size_t c = 0; c < 3; ++c) {
        if (add[c] == 0) continue;
        std::array<std::uint8_t, 256> lut{};
        for (int i = 0; i < 256; ++i) lut[static_cast<std::size_t>(i)] =
            static_cast<std::uint8_t>(std::clamp(i + add[c], 0, 255));
        for (std::size_t i = 0; i < n; ++i) p.rgb[c][i] = lut[p.rgb[c][i]];
    }
}

namespace {

// Ein Bild ab `off` in die (ggf. schon belegten) Ebenen dekodieren.
// orig: Rent-A-Hero.exe 0x0043B740 CImageTcc_Decode
bool decodeInto(std::span<const std::uint8_t> data, std::size_t off, TccPlanes& p, bool first,
                std::size_t* next, std::string* error) {
    auto fail = [&](const char* why) {
        if (error) *error = why;
        return false;
    };
    TccHeader h;
    if (!tccReadHeader(data, off, h)) return fail("TCC-Kopf abgeschnitten");
    // Plausibilität vor dem Anlegen der Puffer (z. B. GAME/LOC06/S26_RING.TCC: 32 Bytes Rest)
    const std::size_t body = std::size_t{h.sizes[0]} + h.sizes[1] + h.sizes[2] + h.sizes[3];
    if (h.width == 0 || h.height == 0 || h.width > 4096 || h.height > 4096 ||
        off + TccHeader::kSize + body > data.size()) {
        return fail("TCC-Kopf unplausibel");
    }
    const std::size_t n = std::size_t{h.width} * h.height;
    if (first) {
        p.width = h.width;
        p.height = h.height;
        for (auto& plane : p.rgb) plane.assign(n + kSlack, 0);
        // Alpha-Puffer nur, wenn das erste Bild Alpha hat (orig: 0x0043B740)
        p.alpha.clear();
        if (h.sizes[3] != 0) p.alpha.assign(n + kSlack, 0);
    } else if (h.width != p.width || h.height != p.height) {
        return fail("A16: Bildgröße wechselt");
    }
    std::size_t pos = off + TccHeader::kSize;
    for (std::size_t c = 0; c < 3; ++c) {
        if (pos + h.sizes[c] > data.size()) return fail("TCC-Ebene abgeschnitten");
        if (!tccRleDecode(data.subspan(pos, h.sizes[c]), p.rgb[c])) return fail("TCC-RLE fehlerhaft");
        if (h.flags & 1) tccExpand4(p.rgb[c], n);
        pos += h.sizes[c];
    }
    if (h.sizes[3] != 0) {
        if (p.alpha.empty()) return fail("A16: Alpha erst in einem Folgebild");
        if (pos + h.sizes[3] > data.size()) return fail("Alpha-Ebene abgeschnitten");
        if (!tccRleDecode(data.subspan(pos, h.sizes[3]), p.alpha)) return fail("Alpha-RLE fehlerhaft");
        pos += h.sizes[3];
    }
    if (next) *next = pos;
    return true;
}

}  // namespace

bool TccImage::load(std::span<const std::uint8_t> data, std::string* error) {
    return decodeInto(data, 0, planes_, true, nullptr, error);
}

// orig: Rent-A-Hero.exe 0x0043BB60 CImageA16_Load
bool A16Anim::load(Bytes data, std::string* error) {
    data_ = std::move(data);
    offsets_.clear();
    keys_.clear();
    std::size_t off = 0;
    TccHeader h;
    while (offsets_.size() < kMaxFrames && tccReadHeader(data_, off, h)) {
        const std::size_t len = TccHeader::kSize + std::size_t{h.sizes[0]} + h.sizes[1] + h.sizes[2] + h.sizes[3];
        if (off + len > data_.size()) break;
        offsets_.push_back(off);
        keys_.push_back((h.flags & 0x10) == 0);
        off += len;
    }
    if (offsets_.empty()) {
        if (error) *error = "A16 ohne Bilder";
        return false;
    }
    current_ = 0;
    fresh_ = true;
    if (!decodeInto(data_, offsets_[0], planes_, true, nullptr, error)) return false;
    color_.apply(planes_);
    return true;
}

bool A16Anim::decode(int i) {
    if (!decodeInto(data_, offsets_[static_cast<std::size_t>(i)], planes_, false, nullptr, nullptr)) return false;
    // LUT in-place auf den persistenten Zustand → Tönung kumuliert (docs/formats/tcc.md)
    color_.apply(planes_);
    current_ = i;
    return true;
}

bool A16Anim::next() {
    if (fresh_) {
        fresh_ = false;
        return true;
    }
    if (current_ + 1 >= frameCount()) return false;
    return decode(current_ + 1);
}

void A16Anim::rewind() {
    decode(0);
    fresh_ = true;
}

bool A16Anim::seek(int n) {
    if (n < 1) {
        rewind();
        return true;
    }
    // UNVERIFIED: Verhalten für n >= Bildzahl (Original prüft nur n > 299)
    if (n > kMaxFrames - 1 || n >= frameCount()) return false;
    int start = n;
    while (start > 0 && !keys_[static_cast<std::size_t>(start)]) --start;
    if (current_ >= start && current_ < n) start = current_ + 1;  // vom aktuellen Bild aus näher
    else if (current_ == n) return true;
    for (int i = start; i <= n; ++i) {
        if (!decode(i)) return false;
    }
    fresh_ = false;
    return true;
}

}  // namespace rah
