// SPDX-License-Identifier: GPL-3.0-or-later
#include "video/ffvideo.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <cstring>
#include <map>

#include "render/surface.hpp"
#include "video/iv5mask.hpp"

namespace rah {

namespace {

constexpr AVRational k100ns{1, 10000000};

class FfmpegVideo final : public VideoDecoder {
public:
    ~FfmpegVideo() override {
        sws_freeContext(sws_);
        av_frame_free(&frame_);
        av_packet_free(&pkt_);
        avcodec_free_context(&codec_);
        if (fmt_) avformat_close_input(&fmt_);
        if (io_) {
            av_freep(&io_->buffer);
            avio_context_free(&io_);
        }
    }

    bool open(Bytes data, std::string* error) {
        auto fail = [&](const char* m) {
            if (error) *error = m;
            return false;
        };
        data_ = std::move(data);
        constexpr int kBuf = 64 * 1024;
        auto* buf = static_cast<unsigned char*>(av_malloc(kBuf));
        io_ = avio_alloc_context(buf, kBuf, 0, this, &readCb, nullptr, &seekCb);
        if (!io_) return fail("avio_alloc_context");
        fmt_ = avformat_alloc_context();
        fmt_->pb = io_;
        if (avformat_open_input(&fmt_, nullptr, nullptr, nullptr) < 0) return fail("kein lesbares AVI");
        if (avformat_find_stream_info(fmt_, nullptr) < 0) return fail("keine Stream-Infos");
        stream_ = av_find_best_stream(fmt_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (stream_ < 0) return fail("kein Videostream");
        AVStream* st = fmt_->streams[stream_];
        const AVCodec* c = avcodec_find_decoder(st->codecpar->codec_id);
        if (!c) return fail("Codec fehlt in FFmpeg");
        codec_ = avcodec_alloc_context3(c);
        if (avcodec_parameters_to_context(codec_, st->codecpar) < 0 || avcodec_open2(codec_, c, nullptr) < 0)
            return fail("Codec nicht geöffnet");
        indeo5_ = st->codecpar->codec_id == AV_CODEC_ID_INDEO5;
        width_ = st->codecpar->width;
        height_ = st->codecpar->height;
        tb_ = st->time_base;
        // Dauer = Länge des AVI-Streams (dwLength · dwScale / dwRate)
        const std::int64_t len = st->duration > 0 ? st->duration : st->nb_frames;
        duration_ = av_rescale_q(len, tb_, k100ns);
        framePts_ = st->avg_frame_rate.num > 0 ? av_rescale_q(1, av_inv_q(st->avg_frame_rate), tb_) : 1;
        if (framePts_ <= 0) framePts_ = 1;
        frame_ = av_frame_alloc();
        pkt_ = av_packet_alloc();
        cur_.resize(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) * 3);
        lookRgb_ = cur_;
        return true;
    }

    int width() const override { return width_; }
    int height() const override { return height_; }
    std::int64_t startTime() const override { return 0; }
    std::int64_t duration() const override { return duration_; }

    void seek(std::int64_t t) override {
        // UNVERIFIED: negative Zeiten (32-Bit-Überlauf in CImageAvi_Seek) – hier auf 0 begrenzt
        const std::int64_t target = av_rescale_q(std::max<std::int64_t>(t, 0), k100ns, tb_);
        av_seek_frame(fmt_, stream_, target, AVSEEK_FLAG_BACKWARD);
        avcodec_flush_buffers(codec_);
        masks_.clear();
        eof_ = drained_ = false;
        look_ = haveCur_ = false;
        nextPts_ = target;
    }

    Result next(std::vector<std::uint16_t>& out) override {
        if (av_rescale_q(nextPts_, tb_, k100ns) >= duration_) return Result::End;  // MS_S_ENDOFSTREAM
        // Ein Update je Bildzeit: alle Bilder mit pts ≤ Bildzeit verbrauchen, das letzte zeigen.
        // Lücken (leere Chunks) wiederholen das vorige Bild. UNVERIFIED: DirectShow bei leeren Chunks.
        for (;;) {
            if (!look_) {
                const int r = decodeOne();
                if (r < 0) return Result::Error;
                if (r == 0) break;
                look_ = true;
            }
            if (lookPts_ > nextPts_ && haveCur_) break;
            std::swap(cur_, lookRgb_);
            haveCur_ = true;
            look_ = false;
            if (lookPts_ >= nextPts_) break;
        }
        if (!haveCur_) return Result::End;
        out.resize(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_));
        for (std::size_t i = 0; i < out.size(); ++i) out[i] = pack565(cur_[3 * i], cur_[3 * i + 1], cur_[3 * i + 2]);
        nextPts_ += framePts_;
        return Result::Ok;
    }

private:
    // nächstes dekodiertes Bild nach lookRgb_/lookPts_; 1 = Bild, 0 = Ende, −1 = Fehler
    int decodeOne() {
        for (;;) {
            int r = avcodec_receive_frame(codec_, frame_);
            if (r == 0) {
                lookPts_ = frame_->best_effort_timestamp != AV_NOPTS_VALUE ? frame_->best_effort_timestamp : nextPts_;
                convert(lookRgb_);
                applyMask(frame_->pts, lookRgb_);
                return 1;
            }
            if (r == AVERROR_EOF) return 0;
            if (r != AVERROR(EAGAIN)) return -1;
            if (eof_) {
                if (drained_) return 0;
                avcodec_send_packet(codec_, nullptr);
                drained_ = true;
                continue;
            }
            r = av_read_frame(fmt_, pkt_);
            if (r < 0) {
                eof_ = true;
                continue;
            }
            if (pkt_->stream_index == stream_) {
                readMask();
                avcodec_send_packet(codec_, pkt_);
            }
            av_packet_unref(pkt_);
        }
    }

    // Indeo 5: Transparenzmaske, die FFmpeg überspringt (formats/indeo5-transparenz.md). Der
    // Original-Codec füllt transparente Pixel mit Schwarz (= Farbschlüssel der Ebenen).
    void readMask() {
        if (!indeo5_ || pkt_->size <= 0) return;
        const auto r = iv5_.decode(pkt_->data, static_cast<std::size_t>(pkt_->size));
        if (r == Indeo5Mask::Result::Mask && iv5_.width() == width_ && iv5_.height() == height_)
            masks_[pkt_->pts] = iv5_.mask();
        // UNVERIFIED: nicht nachgebildete Varianten (Kacheln, Schutz) bleiben deckend
    }
    void applyMask(std::int64_t pts, std::vector<std::uint8_t>& rgb) {
        if (masks_.empty()) return;
        const auto it = masks_.find(pts);
        if (it != masks_.end()) {
            const auto& m = it->second;
            for (std::size_t i = 0; i < m.size(); ++i)
                if (!m[i]) rgb[3 * i] = rgb[3 * i + 1] = rgb[3 * i + 2] = 0;
        }
        masks_.erase(masks_.begin(), masks_.upper_bound(pts));
    }

    void convert(std::vector<std::uint8_t>& rgb) {
        sws_ = sws_getCachedContext(sws_, width_, height_, static_cast<AVPixelFormat>(frame_->format), width_, height_,
                                    AV_PIX_FMT_RGB24, SWS_POINT, nullptr, nullptr, nullptr);
        if (!sws_) return;
        std::uint8_t* dst[1] = {rgb.data()};
        const int stride[1] = {width_ * 3};
        sws_scale(sws_, frame_->data, frame_->linesize, 0, height_, dst, stride);
    }

    static int readCb(void* op, std::uint8_t* buf, int size) {
        auto* self = static_cast<FfmpegVideo*>(op);
        const std::size_t left = self->data_.size() - self->pos_;
        if (left == 0) return AVERROR_EOF;
        const std::size_t n = std::min(left, static_cast<std::size_t>(size));
        std::memcpy(buf, self->data_.data() + self->pos_, n);
        self->pos_ += n;
        return static_cast<int>(n);
    }
    static std::int64_t seekCb(void* op, std::int64_t off, int whence) {
        auto* self = static_cast<FfmpegVideo*>(op);
        const auto size = static_cast<std::int64_t>(self->data_.size());
        if (whence & AVSEEK_SIZE) return size;
        std::int64_t p = off;
        if ((whence & 3) == SEEK_CUR) p += static_cast<std::int64_t>(self->pos_);
        else if ((whence & 3) == SEEK_END) p += size;
        if (p < 0 || p > size) return -1;
        self->pos_ = static_cast<std::size_t>(p);
        return p;
    }

    Bytes data_;
    std::size_t pos_ = 0;
    AVIOContext* io_ = nullptr;
    AVFormatContext* fmt_ = nullptr;
    AVCodecContext* codec_ = nullptr;
    SwsContext* sws_ = nullptr;
    AVFrame* frame_ = nullptr;
    AVPacket* pkt_ = nullptr;
    int stream_ = -1;
    int width_ = 0, height_ = 0;
    AVRational tb_{1, 10};
    std::int64_t duration_ = 0;   // 100 ns
    std::int64_t framePts_ = 1;   // pts je Bild
    std::int64_t nextPts_ = 0;    // pts des nächsten auszugebenden Bilds
    std::int64_t lookPts_ = 0;    // pts des vorausgelesenen Bilds
    bool look_ = false, haveCur_ = false, eof_ = false, drained_ = false;
    std::vector<std::uint8_t> cur_, lookRgb_;  // RGB24: aktuelles und vorausgelesenes Bild
    bool indeo5_ = false;
    Indeo5Mask iv5_;
    std::map<std::int64_t, std::vector<std::uint8_t>> masks_;  // pts → Maske
};

}  // namespace

std::unique_ptr<VideoDecoder> openFfmpegVideo(Bytes data, std::string* error) {
    auto v = std::make_unique<FfmpegVideo>();
    if (!v->open(std::move(data), error)) return nullptr;
    return v;
}

}  // namespace rah
