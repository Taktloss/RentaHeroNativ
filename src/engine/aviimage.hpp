// SPDX-License-Identifier: GPL-3.0-or-later
// CImageAvi (vtable 0x0046B890): Videobild über DirectShow-Multimedia-Streaming ohne Uhr
// (docs/VIDEO.md). Der Dekoder ist austauschbar (FFmpeg im Spiel, Attrappe in Tests).
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "core/rect.hpp"
#include "render/surface.hpp"
#include "formats/bmp.hpp"

namespace rah {

// IAMMultiMediaStream + IDirectDrawMediaStream, auf das Nötige verkürzt (AMMSF_NOCLOCK)
class VideoDecoder {
public:
    virtual ~VideoDecoder() = default;
    virtual int width() const = 0;
    virtual int height() const = 0;
    virtual std::int64_t startTime() const = 0;  // GetTime nach dem Öffnen, 100 ns
    virtual std::int64_t duration() const = 0;   // GetDuration, 100 ns
    virtual void seek(std::int64_t t) = 0;       // IMultiMediaStream::Seek, 100 ns
    enum class Result { Ok, End, Error };
    // IDirectDrawStreamSample::Update: nächstes Bild als RGB565 (width × height, oben zuerst)
    virtual Result next(std::vector<std::uint16_t>& frame) = 0;
};

class AviImage {
public:
    // orig: Rent-A-Hero.exe 0x0043C7C0 CImageAvi_Load
    bool load(std::unique_ptr<VideoDecoder> dec);
    // orig: Rent-A-Hero.exe 0x0043CB10 CImageAvi_Draw(surface, rect) – Ziel ist ein Bild,
    // das Sample merkt es sich (gleiches Rechteck, anderes Ziel → schreibt weiter ins alte)
    bool draw(Image565& target, const Rect& rect) {
        return draw(Target{&target, target.width, target.height, target.pixels.data()}, rect);
    }
    // Primäroberfläche (Direktweg, +0x38C)
    bool draw(Surface565& target, const Rect& rect) {
        return draw(Target{&target, target.width, target.height, target.pixels.data()}, rect);
    }
    // orig: Rent-A-Hero.exe 0x0043CA80 CImageAvi_Next
    bool next();
    // orig: Rent-A-Hero.exe 0x0043C9E0 CImageAvi_Seek
    bool seek(int n);
    // orig: Rent-A-Hero.exe 0x0043C9A0 CImageAvi_Rewind
    void rewind();

    int width() const { return width_; }    // +0x338
    int height() const { return height_; }  // +0x33C
    int counter() const { return counter_; }
    int length() const { return length_; }
    bool hasSample() const { return sample_; }

private:
    // orig: Rent-A-Hero.exe 0x0043CD60 – Sample abschließen, Stream anhalten
    void endSample();
    // DirectDraw-Oberfläche als Ziel eines Samples
    struct Target {
        const void* id = nullptr;
        int width = 0, height = 0;
        std::uint16_t* pixels = nullptr;
    };
    bool draw(const Target& target, const Rect& rect);
    void createSample(const Target& target, const Rect& rect);

    std::unique_ptr<VideoDecoder> dec_;
    int width_ = 0, height_ = 0;
    int start_ = 0;            // +0x3C4
    int length_ = 0;           // +0x3C8
    int counter_ = 0;          // +0x3CC
    bool loaded_ = false;      // +0x3DC
    bool sample_ = false;      // +0x3E0
    bool want_ = false;        // +0x3E4
    Target sampleTarget_;  // Oberfläche des Samples +0x3F4
    Rect sampleRect_{};        // +0x464
    std::vector<std::uint16_t> frame_;
};

}  // namespace rah
