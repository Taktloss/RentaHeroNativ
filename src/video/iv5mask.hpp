// SPDX-License-Identifier: GPL-3.0-or-later
// Transparenzmaske von Indeo-5-Bildern (IV50), die FFmpeg nicht auswertet
// (docs/formats/indeo5-transparenz.md). Nachgebildet nach Intels ir50_32.dll; abgedeckt
// sind die Varianten der Spieldaten (eine Kachel, Bandlängen im Bild).
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace rah {

class Indeo5Mask {
public:
    enum class Result { Opaque, Mask, Unsupported };
    // Ein Bild (AVI-Chunk) lesen. Mask: mask() = Breite × Höhe, 0 = transparent, 1 = sichtbar.
    // Opaque: Video ohne Transparenz. Unsupported: Variante nicht nachgebildet.
    Result decode(const std::uint8_t* data, std::size_t size);
    const std::vector<std::uint8_t>& mask() const { return mask_; }
    int width() const { return w_; }
    int height() const { return h_; }

private:
    // orig: ir50_32.dll 0x100232D0 (GOP-Kopf)
    bool parseGop(class BitReader& r);
    // orig: ir50_32.dll 0x100016F0 – Huffman-Tabelle aus Deskriptor
    void buildCodes();
    // orig: ir50_32.dll 0x1004F3C0 – Lauflängen einer Kachel
    bool decodeRuns(const std::uint8_t* data, std::size_t size, int rows, int padW, bool delta);

    int w_ = 0, h_ = 0, bands_ = 0;
    bool haveGop_ = false, transparent_ = false;
    std::vector<std::uint8_t> desc_;  // ctx+0x1D110: xbits je Zeile
    struct Code {
        std::uint8_t len = 0;
        std::uint8_t sym = 0;
    };
    std::vector<Code> lut_;  // Nachschlagen über maxLen_ Bits (LSB zuerst)
    int maxLen_ = 0;
    std::vector<std::uint8_t> mask_;
};

}  // namespace rah
