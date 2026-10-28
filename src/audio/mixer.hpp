// SPDX-License-Identifier: GPL-3.0-or-later
// Software-Mischer als Ersatz für DirectSound-Sekundärpuffer (docs/AUDIO.md „Port“).
// Ausgabe im Format des Primärpuffers 0x00436750: 22 050 Hz, 16 Bit, Stereo.
// Thread-sicher: Steuerung im Hauptthread, mix() im Audio-Thread der Plattform.
#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <mutex>

#include "formats/wav.hpp"

namespace rah {

class Mixer {
public:
    static constexpr int kRate = 22050;

    int create(std::shared_ptr<const Wave> wave);  // Puffer anlegen, Kennung > 0
    void destroy(int id);
    // IDirectSoundBuffer::Play (ab aktueller Position) / Stop (Position bleibt)
    void play(int id, bool loop);
    void stop(int id);
    void setPosition(int id, std::uint32_t bytePos);  // SetCurrentPosition
    std::uint32_t position(int id);                    // Abspielzeiger in Bytes
    bool playing(int id);
    void setVolume(int id, int hundredthsDb);  // SetVolume: −10000…0 (1/100 dB)
    void setFrequency(int id, int hz);         // SetFrequency
    // count Stereo-Bildpaare (L, R) mischen; mit Sättigung
    void mix(std::int16_t* out, int count);

private:
    struct Voice {
        std::shared_ptr<const Wave> wave;
        double pos = 0;  // in Abtastwerten
        bool playing = false, loop = false;
        double gain = 1.0;
        int freq = 0;
    };
    std::mutex m_;
    std::map<int, Voice> voices_;
    int next_ = 1;
};

}  // namespace rah
