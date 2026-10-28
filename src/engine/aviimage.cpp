// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/aviimage.hpp"

#include <algorithm>

namespace rah {

bool AviImage::load(std::unique_ptr<VideoDecoder> dec) {
    if (!dec) return false;
    dec_ = std::move(dec);
    start_ = counter_ = static_cast<int>(dec_->startTime() / 1000000);  // 100-ns-Einheiten → 100 ms
    length_ = static_cast<int>(dec_->duration() / 1000000);
    width_ = dec_->width();
    height_ = dec_->height();
    // 640×180 meldet 360 und verdoppelt Zeilen (+0x3D8) – kommt in den Daten nicht vor, nicht portiert
    if (width_ == 640 && height_ == 180) height_ = 360;
    loaded_ = true;
    want_ = true;  // nach SetState(RUN)
    return true;
}

void AviImage::createSample(Image565& target, const Rect& rect) {
    sampleTarget_ = &target;
    sampleRect_ = rect;
    sample_ = true;  // CreateSample + SetState(RUN)
}

bool AviImage::draw(Image565& target, const Rect& rect) {
    ++counter_;
    if (!loaded_) return false;
    if (!sample_) {
        if (!want_) return true;
        createSample(target, rect);
    }
    if (!(rect == sampleRect_)) {
        // SetRect, wenn das Ziel die Oberfläche des Samples ist, sonst neues Sample
        if (sampleTarget_ == &target) sampleRect_ = rect;
        else createSample(target, rect);
    }
    switch (dec_->next(frame_)) {
    case VideoDecoder::Result::Ok: break;
    case VideoDecoder::Result::End: counter_ = length_ + 1; return true;  // MS_S_ENDOFSTREAM
    case VideoDecoder::Result::Error: return false;
    }
    // Sample schreibt das Bild in sein Rechteck der Oberfläche.
    // UNVERIFIED: Rechteck abweichender Größe (DirectDraw würde strecken) – hier abgeschnitten.
    Image565& t = *sampleTarget_;
    const int w = std::min(dec_->width(), sampleRect_.width()), h = std::min(dec_->height(), sampleRect_.height());
    for (int y = 0; y < h; ++y) {
        const int ty = sampleRect_.top + y;
        if (ty < 0 || ty >= t.height) continue;
        for (int x = 0; x < w; ++x) {
            const int tx = sampleRect_.left + x;
            if (tx < 0 || tx >= t.width) continue;
            t.pixels[static_cast<std::size_t>(ty) * static_cast<std::size_t>(t.width) + static_cast<std::size_t>(tx)] =
                frame_[static_cast<std::size_t>(y) * static_cast<std::size_t>(dec_->width()) + static_cast<std::size_t>(x)];
        }
    }
    return true;
}

void AviImage::endSample() {
    if (loaded_ && sample_) {
        // CompletionStatus(COMPSTAT_WAIT), SetState(STOP), Sample freigeben
        sample_ = false;
        sampleTarget_ = nullptr;
    }
    want_ = false;
}

bool AviImage::next() {
    if (counter_ < length_) {
        want_ = true;
        return true;
    }
    if (sample_) endSample();
    return false;
}

bool AviImage::seek(int n) {
    want_ = true;
    if (n != counter_) {
        if (n <= length_) counter_ = n;
        // Produkt in 32 Bit (läuft ab n > 2147 über), dann vorzeichenerweitert
        const auto t = static_cast<std::int32_t>(static_cast<std::uint32_t>(counter_) * 1000000u);
        if (dec_) dec_->seek(t);
    }
    return true;
}

void AviImage::rewind() {
    counter_ = start_;
    if (dec_) dec_->seek(0);
    want_ = true;
    next();
}

}  // namespace rah
