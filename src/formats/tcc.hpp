// SPDX-License-Identifier: GPL-3.0-or-later
// TCC-Bilder und A16-Bildfolgen (RLE-Ebenen R, G, B und optional Alpha).
// Format und Original-Eigenheiten: docs/formats/tcc.md.
#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "core/bytes.hpp"

namespace rah {

struct TccHeader {
    std::uint32_t flags = 0;               // Bit 0: 4-Bit-Ebenen, Bit 4: Deltabild
    std::array<std::uint32_t, 4> sizes{};  // RLE-Bytes R, G, B, Alpha (0 = kein Alpha)
    std::uint32_t width = 0, height = 0;
    static constexpr std::size_t kSize = 28;
};

// Ebenen eines dekodierten Bilds. Bei A16 sind sie zugleich der Zustand für Deltabilder.
struct TccPlanes {
    std::uint32_t width = 0, height = 0;
    std::array<std::vector<std::uint8_t>, 3> rgb;  // je width*height (+16 Reserve wie im Original)
    std::vector<std::uint8_t> alpha;               // leer = kein Alpha
    bool hasAlpha() const { return !alpha.empty(); }
};

// orig: Rent-A-Hero.exe 0x004380B0 – RLE-Ebene dekodieren. Liefert false bei
// fehlerhaften Daten (das Original prüft keine Grenzen, docs/formats/tcc.md).
bool tccRleDecode(std::span<const std::uint8_t> src, std::span<std::uint8_t> dst);

// orig: Rent-A-Hero.exe 0x00438800 – 4-Bit-Werte in-place expandieren; das Paar (0, 1)
// wird wie im Original nie expandiert.
void tccExpand4(std::span<std::uint8_t> plane, std::size_t n);

bool tccReadHeader(std::span<const std::uint8_t> data, std::size_t off, TccHeader& h);

// Farbaddition je Kanal (SZN RedAdd/GreenAdd/BlueAdd): lut[i] = clamp(i + wert).
// orig: Rent-A-Hero.exe 0x004384F0 CImageTcc_SetColor / 0x004385D0 (Anwendung in-place)
struct TccColorAdd {
    int r = 0, g = 0, b = 0;
    bool active() const { return r != 0 || g != 0 || b != 0; }
    void apply(TccPlanes& p) const;
};

// Einzelbild (.TCC).
class TccImage {
public:
    bool load(std::span<const std::uint8_t> data, std::string* error);
    const TccPlanes& planes() const { return planes_; }
    TccPlanes& planes() { return planes_; }

private:
    TccPlanes planes_;
};

// Bildfolge (.A16), höchstens 300 Bilder.
class A16Anim {
public:
    static constexpr int kMaxFrames = 300;

    bool load(Bytes data, std::string* error);
    int frameCount() const { return static_cast<int>(offsets_.size()); }
    int current() const { return current_; }
    bool isKeyFrame(int i) const { return keys_[static_cast<std::size_t>(i)]; }

    // orig: Rent-A-Hero.exe 0x0043BD20 – nächstes Bild; erster Aufruf nach Laden/Rewind
    // behält Bild 0. false am Ende.
    bool next();
    // orig: Rent-A-Hero.exe 0x0043BCE0
    void rewind();
    // orig: Rent-A-Hero.exe 0x0043BDC0
    bool seek(int n);

    void setColorAdd(const TccColorAdd& c) { color_ = c; }
    const TccPlanes& planes() const { return planes_; }

private:
    bool decode(int i);

    Bytes data_;
    std::vector<std::size_t> offsets_;
    std::vector<bool> keys_;
    TccPlanes planes_;
    TccColorAdd color_;
    int current_ = 0;
    bool fresh_ = true;  // nächster next() bleibt auf dem aktuellen Bild
};

}  // namespace rah
