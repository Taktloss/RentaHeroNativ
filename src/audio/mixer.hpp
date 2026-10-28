// SPDX-License-Identifier: GPL-3.0-or-later
// Software-Mischer als Ersatz für DirectSound-Sekundärpuffer (docs/AUDIO.md „Port“).
// Ausgabe im Format des Primärpuffers 0x00436750: 22 050 Hz, 16 Bit, Stereo.
// Thread-sicher: Steuerung im Hauptthread, mix() im Audio-Thread der Plattform.
#pragma once

#include <cstdint>
#include <array>
#include <map>
#include <vector>
#include <memory>
#include <mutex>

#include "formats/wav.hpp"

namespace rah {

class Mixer {
public:
    static constexpr int kRate = 22050;

    // Puffer anlegen, Kennung > 0; ctrl3d = DSBCAPS_CTRL3D (nur mit 3D-Option, AUDIO.md „3D“)
    int create(std::shared_ptr<const Wave> wave, bool ctrl3d = false);
    void destroy(int id);
    // IDirectSoundBuffer::Play (ab aktueller Position) / Stop (Position bleibt)
    void play(int id, bool loop);
    void stop(int id);
    void setPosition(int id, std::uint32_t bytePos);  // SetCurrentPosition
    std::uint32_t position(int id);                    // Abspielzeiger in Bytes
    bool playing(int id);
    void setVolume(int id, int hundredthsDb);  // SetVolume: −10000…0 (1/100 dB)
    void setFrequency(int id, int hz);         // SetFrequency
    // IDirectSound3DBuffer::SetPosition (Listener im Ursprung, Blick +z, Standardwerte)
    void setPosition3D(int id, float x, float y, float z);
    // EAX 1.0: Hall der Umgebung (DSPROPSETID_EAX_ReverbProperties, DSPROPERTY_EAX_ALL) und
    // Hallanteil eines Puffers (DSPROPSETID_EAXBUFFER_ReverbProperties, REVERBMIX; −1 = nach Abstand)
    void setReverb(int environment, float volume, float decaySec, float damping);
    void setReverbMix(int id, float mix);
    // count Stereo-Bildpaare (L, R) mischen; mit Sättigung
    void mix(std::int16_t* out, int count);

private:
    struct Voice {
        std::shared_ptr<const Wave> wave;
        double pos = 0;  // in Abtastwerten
        bool playing = false, loop = false;
        double gain = 1.0;
        int freq = 0;
        bool is3d = false;
        float x = 0, y = 0, z = 0;
        float reverbMix = -1.0f;  // EAX_REVERBMIX_USEDISTANCE
    };
    // Hall: 4 Kammfilter + 2 Allpässe je Kanal (Schroeder), Parameter aus EAX 1.0
    struct Comb {
        std::vector<double> buf;
        std::size_t pos = 0;
        double fb = 0, store = 0;
    };
    struct Allpass {
        std::vector<double> buf;
        std::size_t pos = 0;
    };
    void configureReverb();
    double reverbVolume_ = 0.5, reverbDecay_ = 1.493, reverbDamping_ = 0.5;  // EAX-Vorgabe GENERIC
    std::array<std::array<Comb, 4>, 2> combs_;
    std::array<std::array<Allpass, 2>, 2> allpasses_;
    bool reverbReady_ = false;
    std::mutex m_;
    std::map<int, Voice> voices_;
    int next_ = 1;
};

}  // namespace rah
