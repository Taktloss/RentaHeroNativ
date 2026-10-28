// SPDX-License-Identifier: GPL-3.0-or-later
#include "audio/mixer.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace rah {

namespace {

// ein Abtastwert als −32768…32767 (8 Bit vorzeichenlos, 16 Bit vorzeichenbehaftet)
double sampleAt(const Wave& w, std::size_t frame, int ch) {
    const int c = std::min(ch, w.channels - 1);
    const std::size_t off = frame * static_cast<std::size_t>(w.blockAlign) + static_cast<std::size_t>(c) * (w.bits / 8);
    if (w.bits == 8) return (static_cast<int>(w.data[off]) - 128) * 256.0;
    return static_cast<std::int16_t>(w.data[off] | (w.data[off + 1] << 8));
}

std::size_t frames(const Wave& w) { return w.blockAlign ? w.data.size() / static_cast<std::size_t>(w.blockAlign) : 0; }

}  // namespace

int Mixer::create(std::shared_ptr<const Wave> wave) {
    std::lock_guard lock(m_);
    const int id = next_++;
    Voice v;
    v.freq = wave->rate;
    v.wave = std::move(wave);
    voices_.emplace(id, std::move(v));
    return id;
}

void Mixer::destroy(int id) {
    std::lock_guard lock(m_);
    voices_.erase(id);
}

void Mixer::play(int id, bool loop) {
    std::lock_guard lock(m_);
    if (auto it = voices_.find(id); it != voices_.end()) {
        it->second.playing = true;
        it->second.loop = loop;
    }
}

void Mixer::stop(int id) {
    std::lock_guard lock(m_);
    if (auto it = voices_.find(id); it != voices_.end()) it->second.playing = false;
}

void Mixer::setPosition(int id, std::uint32_t bytePos) {
    std::lock_guard lock(m_);
    if (auto it = voices_.find(id); it != voices_.end() && it->second.wave->blockAlign)
        it->second.pos = static_cast<double>(bytePos / static_cast<std::uint32_t>(it->second.wave->blockAlign));
}

std::uint32_t Mixer::position(int id) {
    std::lock_guard lock(m_);
    auto it = voices_.find(id);
    if (it == voices_.end()) return 0;
    return static_cast<std::uint32_t>(it->second.pos) * static_cast<std::uint32_t>(it->second.wave->blockAlign);
}

bool Mixer::playing(int id) {
    std::lock_guard lock(m_);
    auto it = voices_.find(id);
    return it != voices_.end() && it->second.playing;
}

void Mixer::setVolume(int id, int hundredthsDb) {
    std::lock_guard lock(m_);
    if (auto it = voices_.find(id); it != voices_.end())
        it->second.gain = hundredthsDb <= -10000 ? 0.0 : std::pow(10.0, hundredthsDb / 2000.0);
}

void Mixer::setFrequency(int id, int hz) {
    std::lock_guard lock(m_);
    if (auto it = voices_.find(id); it != voices_.end()) it->second.freq = hz;
}

void Mixer::mix(std::int16_t* out, int count) {
    std::vector<double> acc(static_cast<std::size_t>(count) * 2, 0.0);
    {
        std::lock_guard lock(m_);
        for (auto& [id, v] : voices_) {
            if (!v.playing) continue;
            const Wave& w = *v.wave;
            const std::size_t n = frames(w);
            if (n == 0) {
                v.playing = false;
                continue;
            }
            const double step = static_cast<double>(v.freq) / kRate;
            for (int i = 0; i < count && v.playing; ++i) {
                // UNVERIFIED: Umrechnung der DirectSound-Software-Mischung – hier linear interpoliert
                const auto f0 = static_cast<std::size_t>(v.pos);
                const double t = v.pos - static_cast<double>(f0);
                std::size_t f1 = f0 + 1;
                if (f1 >= n) f1 = v.loop ? 0 : f0;
                for (int ch = 0; ch < 2; ++ch) {
                    // Mono ohne Panorama: beide Kanäle voll (Pan 0)
                    const double s = sampleAt(w, f0, ch) * (1 - t) + sampleAt(w, f1, ch) * t;
                    acc[static_cast<std::size_t>(i) * 2 + static_cast<std::size_t>(ch)] += s * v.gain;
                }
                v.pos += step;
                if (v.pos >= static_cast<double>(n)) {
                    if (v.loop) v.pos -= static_cast<double>(n);
                    else {
                        v.pos = static_cast<double>(n);
                        v.playing = false;  // statischer Puffer ohne Schleife hält am Ende an
                    }
                }
            }
        }
    }
    for (std::size_t i = 0; i < acc.size(); ++i)
        out[i] = static_cast<std::int16_t>(std::clamp(std::lround(acc[i]), -32768L, 32767L));
}

}  // namespace rah
