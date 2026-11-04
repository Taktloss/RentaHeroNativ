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

int Mixer::create(std::shared_ptr<const Wave> wave, bool ctrl3d) {
    std::lock_guard lock(m_);
    const int id = next_++;
    Voice v;
    v.is3d = ctrl3d;
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

void Mixer::setPosition3D(int id, float x, float y, float z) {
    std::lock_guard lock(m_);
    if (auto it = voices_.find(id); it != voices_.end()) {
        it->second.x = x;
        it->second.y = y;
        it->second.z = z;
    }
}

void Mixer::setReverbMix(int id, float mix) {
    std::lock_guard lock(m_);
    if (auto it = voices_.find(id); it != voices_.end()) it->second.reverbMix = mix;
}

void Mixer::setReverb(int, float volume, float decaySec, float damping) {
    std::lock_guard lock(m_);
    reverbVolume_ = std::clamp(static_cast<double>(volume), 0.0, 1.0);
    reverbDecay_ = std::clamp(static_cast<double>(decaySec), 0.1, 20.0);
    reverbDamping_ = std::clamp(static_cast<double>(damping), 0.0, 2.0);
    configureReverb();
}

void Mixer::configureReverb() {
    // UNVERIFIED: Nachbildung des EAX-1.0-Halls (Hardware-Algorithmus unbekannt). Freeverb-Längen
    // auf 22 050 Hz halbiert; Nachhall −60 dB nach decaySec; Dämpfung 0…2 → Tiefpass im Rückweg.
    static constexpr std::size_t kComb[4] = {557, 593, 641, 677};
    static constexpr std::size_t kAll[2] = {278, 111};
    for (int ch = 0; ch < 2; ++ch) {
        const std::size_t spread = ch ? 11 : 0;
        for (std::size_t i = 0; i < 4; ++i) {
            Comb& c = combs_[static_cast<std::size_t>(ch)][i];
            const std::size_t len = kComb[i] + spread;
            if (c.buf.size() != len) c.buf.assign(len, 0.0), c.pos = 0;
            c.fb = std::pow(10.0, -3.0 * static_cast<double>(len) / (reverbDecay_ * kRate));
        }
        for (std::size_t i = 0; i < 2; ++i) {
            Allpass& a = allpasses_[static_cast<std::size_t>(ch)][i];
            const std::size_t len = kAll[i] + spread;
            if (a.buf.size() != len) a.buf.assign(len, 0.0), a.pos = 0;
        }
    }
    reverbReady_ = true;
}

void Mixer::mix(std::int16_t* out, int count) {
    std::vector<double> acc(static_cast<std::size_t>(count) * 2, 0.0);
    std::vector<double> send(static_cast<std::size_t>(count), 0.0);
    bool anySend = false;
    {
        std::lock_guard lock(m_);
        anySend = reverbReady_;  // Hall läuft auch ohne Eingang weiter (Nachhall)
        for (auto& [id, v] : voices_) {
            if (!v.playing) continue;
            const Wave& w = *v.wave;
            // 3D-Option: DirectSound3D-Vorgaben Mindestabstand 1, Abfall 1, Modus NORMAL, Listener im
            // Ursprung (Wine-Messung dsprobe, WINEPROBE.md) → Dämpfung 1/Abstand. UNVERIFIED: Panorama
            // der Hardware nach Azimut (sin = x / √(x² + z²)), Gegenkanal leiser
            double gl = 1.0, gr = 1.0, sendGain = 0.0;
            if (v.is3d) {
                const double d = std::sqrt(double{v.x} * v.x + double{v.y} * v.y + double{v.z} * v.z);
                const double att = d > 1.0 ? 1.0 / d : 1.0;
                const double h = std::sqrt(double{v.x} * v.x + double{v.z} * v.z);
                const double p = h > 0 ? v.x / h : 0.0;
                gl = att * std::min(1.0, std::sqrt(1.0 - p));
                gr = att * std::min(1.0, std::sqrt(1.0 + p));
                // Hallanteil: fest oder (−1) nach Abstand – UNVERIFIED
                sendGain = v.reverbMix >= 0 ? v.reverbMix : 1.0 - att;
            }
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
                    // Mono ohne 3D: beide Kanäle voll (Pan 0)
                    const double s = sampleAt(w, f0, ch) * (1 - t) + sampleAt(w, f1, ch) * t;
                    acc[static_cast<std::size_t>(i) * 2 + static_cast<std::size_t>(ch)] += s * v.gain * (ch ? gr : gl);
                    if (ch == 0 && sendGain > 0) send[static_cast<std::size_t>(i)] += s * v.gain * sendGain;
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
    if (anySend) {
        std::lock_guard lock(m_);
        const double damp = std::clamp(1.0 - reverbDamping_ / 2.0, 0.0, 1.0) * 0.4;
        for (std::size_t i = 0; i < send.size(); ++i) {
            const double in = send[i] * 0.015;  // Eingangspegel wie Freeverb
            for (std::size_t ch = 0; ch < 2; ++ch) {
                double o = 0;
                for (Comb& c : combs_[ch]) {
                    const double y = c.buf[c.pos];
                    c.store = y * (1 - damp) + c.store * damp;
                    c.buf[c.pos] = in + c.store * c.fb;
                    if (++c.pos == c.buf.size()) c.pos = 0;
                    o += y;
                }
                for (Allpass& a : allpasses_[ch]) {
                    const double b = a.buf[a.pos];
                    a.buf[a.pos] = o + b * 0.5;
                    if (++a.pos == a.buf.size()) a.pos = 0;
                    o = b - o;
                }
                acc[i * 2 + ch] += o * reverbVolume_ * 3.0;
            }
        }
    }
    for (std::size_t i = 0; i < acc.size(); ++i)
        out[i] = static_cast<std::int16_t>(std::clamp(std::lround(acc[i]), -32768L, 32767L));
}

}  // namespace rah
