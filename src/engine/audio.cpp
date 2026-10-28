// SPDX-License-Identifier: GPL-3.0-or-later
#include "engine/audio.hpp"

#include <algorithm>
#include <cstdio>

#include "audio/mixer.hpp"
#include "engine/globals.hpp"

namespace rah {

namespace {
int clampInt(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
}  // namespace

std::shared_ptr<const Wave> loadWave(const FoundFile& file) {
    const auto data = file.read();
    if (!data) return nullptr;
    auto w = parseWave(*data, nullptr);
    if (!w) return nullptr;
    return std::make_shared<const Wave>(std::move(*w));
}

// ---- CAudio ---------------------------------------------------------------

AudioManager::AudioManager(EngineGlobals& g, Mixer* mixer, bool hw3d) : g_(g), mixer_(mixer), hw3d_(hw3d && mixer) {
    tick_.start(g.now(), 100, true);  // FUN_0044D7B0(timer, 100, 100, this, CAudio_FadeTick)
    // EAX-1.0-Vorgabe der Karte bis zum ersten Setzen: GENERIC (0.5, 1.493 s, 0.5) – UNVERIFIED
    if (hw3d_) mixer_->setReverb(0, 0.5f, 1.493f, 0.5f);
}

AudioManager::~AudioManager() {
    adopted_.clear();
    // Original löscht alle noch eingetragenen Puffer; im Port gehören sie ihren Besitzern
}

void AudioManager::apply(int kind) {
    for (AudioBuffer* b : slots_)
        if (b && b->kind == kind) b->applyVolume();
}

void AudioManager::setSfxVolume(int v) {
    sfx_ = clampInt(v, 0, 100);
    apply(kAudioSfx);
}

void AudioManager::setMusicVolume(int v) {
    music_ = clampInt(v, 0, 100);
    apply(kAudioMusic);
}

void AudioManager::setSpeechVolume(int v) {
    speech_ = clampInt(v, 0, 100);
    apply(kAudioSpeech);
}

void AudioManager::add(AudioBuffer* b) {
    for (auto& s : slots_) {
        if (!s) {
            s = b;
            return;
        }
    }
    // mehr als 100 Puffer: wird still nicht eingetragen (keine Blenden)
}

void AudioManager::remove(AudioBuffer* b) {
    for (auto& s : slots_) {
        if (s == b) {
            s = nullptr;
            return;
        }
    }
}

void AudioManager::fadeTick() {
    for (std::size_t i = 0; i < slots_.size(); ++i) {
        AudioBuffer* b = slots_[i];
        if (!b) continue;
        if (b->freqStep != 0) {
            b->freq += b->freqStep;
            if ((b->freqStep < 0 && b->freq <= b->freqTarget) || (b->freqStep > 0 && b->freqTarget < b->freq)) {
                b->freqStep = 0;
                b->freq = b->freqTarget;
            }
            b->setFreq(b->freq);
        }
        if (b->volumeStep != 0) {
            b->volume += b->volumeStep;
            if ((b->volumeStep < 0 && b->volume <= b->volumeTarget) ||
                (b->volumeStep > 0 && b->volumeTarget <= b->volume)) {
                b->volumeStep = 0;
                b->volume = b->volumeTarget;
            }
            b->applyVolume();
            if (b->deleteWhenSilent && b->volume == 0 && b->volumeTarget == 0) {
                // (***this)(1): Puffer löscht sich selbst
                auto it = std::find_if(adopted_.begin(), adopted_.end(), [b](const auto& p) { return p.get() == b; });
                if (it != adopted_.end()) adopted_.erase(it);
                // UNVERIFIED: nicht übernommene Puffer mit +0x12C kommen im Spielcode nicht vor
            }
        }
    }
}

void AudioManager::adopt(std::unique_ptr<AudioBuffer> b) {
    if (b) adopted_.push_back(std::move(b));
}

void AudioManager::poll() {
    const std::uint32_t now = g_.now();
    while (tick_.fire(now)) fadeTick();
    // Kopie: Timer-Rückrufe können Puffer austragen
    const auto snapshot = slots_;
    for (AudioBuffer* b : snapshot) {
        if (b && std::find(slots_.begin(), slots_.end(), b) != slots_.end()) b->pollTimers(now);
    }
}

// ---- CAudioBuffer ---------------------------------------------------------

std::uint32_t AudioBuffer::now() const { return mgr_ ? mgr_->globals().now() : 0; }

AudioBuffer::~AudioBuffer() {
    AudioBuffer::stop();  // 0x00437270 direkt (nicht virtuell)
    endTimer_.stop();
    if (mgr_) {
        mgr_->remove(this);
        if (voice_ && mgr_->mixer()) mgr_->mixer()->destroy(voice_);
    }
}

bool AudioBuffer::open(const FoundFile* file, AudioManager* mgr, int k) {
    if (file) path = file->path;
    mgr_ = mgr;
    if (mgr) mgr->add(this);
    if (!file || !mgr || !mgr->mixer()) return false;
    wave_ = loadWave(*file);
    if (!wave_) return false;
    dataSize = static_cast<std::uint32_t>(wave_->data.size());  // +0x16C
    durationMs = wave_->durationMs;                             // +0x174
    // Flags 0xE2, mit +0x74 zusätzlich DSBCAPS_CTRL3D (DirectSound3D verlangt mono)
    has3D_ = mgr->hw3d() && wave_->channels == 1;
    voice_ = mgr->mixer()->create(wave_, has3D_);
    prepare();  // vtable +0x1C; danach Daten in den Puffer (0x00436FE0)
    baseFreq = wave_->rate;  // GetFormat: nSamplesPerSec
    avgBytesPerSec = wave_->avgBytesPerSec;
    freq = 100;
    volume = 100;
    kind = k;
    applyVolume();
    return true;
}

int AudioBuffer::getPos() {
    if (voice_ && mgr_ && mgr_->mixer()) {
        const double bytes = static_cast<double>(mgr_->mixer()->position(voice_));
        return static_cast<int>(bytes / (static_cast<double>(avgBytesPerSec) * 0.001));
    }
    return static_cast<int>(now() - startTime);
}

void AudioBuffer::prepare() {
    if (prepared) return;
    if (voice_ && mgr_) mgr_->mixer()->setPosition(voice_, 0);
    prepared = true;
}

void AudioBuffer::play() {
    if (!voice_) return;
    if (playing) stop();
    if (!prepared) prepare();
    if (endEvent) endTimer_.start(now(), durationMs, false);  // CTimer_Start(Dauer, …, einmalig)
    mgr_->mixer()->play(voice_, false);
    startTime = now();
    playing = true;
    prepared = false;
    loop = false;
}

void AudioBuffer::playLoop() {
    if (!voice_) return;
    if (playing) stop();
    if (!prepared) prepare();
    mgr_->mixer()->play(voice_, true);
    playing = true;
    prepared = false;
    loop = true;
}

void AudioBuffer::markEnd() {
    if (!endEvent) return;
    endEvent->pending = true;
    endEvent->type = 0x1F;
    endEvent->param = 0x1F;
    if (endEvent->immediate && mgr_) fireEvent(mgr_->globals(), *endEvent, 0x1F);
}

void AudioBuffer::stop() {
    if (voice_ && mgr_ && mgr_->mixer()) mgr_->mixer()->stop(voice_);
    playing = false;
    markEnd();
    endTimer_.stop();
}

void AudioBuffer::pollTimers(std::uint32_t t) {
    if (endTimer_.fire(t)) markEnd();  // 0x00437600: nur markieren, +0x168 bleibt 1
}

void AudioBuffer::setVolume(int v) {
    volume = clampInt(v, 0, 100);
    volumeStep = 0;
    applyVolume();
}

void AudioBuffer::fadeVolume(int v, int ms) {
    volumeTarget = v;
    const int ticks = ms / 100;
    if (ticks == 0) {
        // Original: Division durch 0 (Absturz). Port: Meldung, Ziel sofort im nächsten Takt
        std::fprintf(stderr, "Ton: Blendzeit %d ms < 100 (Original stürzt ab)\n", ms);
        volumeStep = v - volume;
        return;
    }
    volumeStep = (v - volume) / ticks;
}

void AudioBuffer::applyVolume() {
    // x87 (Win32-Vorgabe: 53 Bit Genauigkeit): vol · 0.01f [· Kategorie · 0.01f] · 3333.0f
    // − 3333.0f, _ftol schneidet ab
    constexpr double k01 = static_cast<double>(0.01f);  // 0x0046B740
    double f = volume * k01;
    if (mgr_) {
        if (kind == kAudioSfx) f = f * mgr_->sfxVolume() * k01;
        else if (kind == kAudioSpeech) f = f * mgr_->speechVolume() * k01;
        else if (kind == kAudioMusic) f = f * mgr_->musicVolume() * k01;
    }
    const int db = static_cast<int>(f * 3333.0 - 3333.0);  // 0x0046B73C
    volumeDb = clampInt(db, -3333, 0);  // 0x004373B0
    if (voice_ && mgr_ && mgr_->mixer()) mgr_->mixer()->setVolume(voice_, volumeDb);
}

void AudioBuffer::setFreq(int p) {
    freq = clampInt(p, 1, 200);
    applyFreq(baseFreq * freq / 100);
}

void AudioBuffer::fadeFreq(int p, int ms) {
    freqTarget = p;
    const int ticks = ms / 100;
    if (ticks == 0) {
        std::fprintf(stderr, "Ton: Blendzeit %d ms < 100 (Original stürzt ab)\n", ms);
        freqStep = p - freq;
        return;
    }
    freqStep = (p - freq) / ticks;
}

void AudioBuffer::applyFreq(int hz) {
    hz = clampInt(hz, 100, 100000);
    if (voice_ && mgr_ && mgr_->mixer()) mgr_->mixer()->setFrequency(voice_, hz);
}

void AudioBuffer::setPos3D(float x, float y, float z) {
    // × 7.0 (0x0046B744) → IDirectSound3DBuffer::SetPosition; nur mit Hardware-3D (+0x74)
    pos3D[0] = x * 7.0f;
    pos3D[1] = y * 7.0f;
    pos3D[2] = z * 7.0f;
    if (has3D_ && voice_) mgr_->mixer()->setPosition3D(voice_, pos3D[0], pos3D[1], pos3D[2]);  // 0x00437530
}

void AudioBuffer::setReverb(int environment, float vol, float decay, float damping) {
    if (has3D_ && voice_) mgr_->mixer()->setReverb(environment, vol, decay, damping);
}

void AudioBuffer::setReverbMix(float mix) {
    if (has3D_ && voice_) mgr_->mixer()->setReverbMix(voice_, mix);
}

void AudioBuffer::setEndEvent(Event* e) {
    if (!e) return;
    endEvent = e;
    e->enabled = true;
}

void AudioBuffer::fadeIn(int v, int ms) {
    if (playing) return;
    setVolume(0);
    fadeVolume(v, ms);
    playLoop();
}

// ---- Stream ---------------------------------------------------------------

AudioStream::~AudioStream() {
    refillTimer_.stop();
    repeatTimer_.stop();
}

bool AudioStream::open(const FoundFile* file, AudioManager* mgr, int k) {
    if (file) path = file->path;
    mgr_ = mgr;
    repeatPause = 30;
    if (mgr) mgr->add(this);
    if (!file || !mgr || !mgr->mixer()) return false;
    wave_ = loadWave(*file);
    if (!wave_) return false;
    // Ring = nAvgBytesPerSec · 2000 / 1000, höchstens die Datengröße
    ringSize = std::min<std::uint32_t>(wave_->avgBytesPerSec * static_cast<std::uint32_t>(bufferMs) / 1000,
                                       static_cast<std::uint32_t>(wave_->data.size()));
    dataSize = static_cast<std::uint32_t>(wave_->data.size());
    durationMs = wave_->durationMs;
    has3D_ = mgr->hw3d() && wave_->channels == 1;  // Flags 0xE0 bzw. 0xF0
    voice_ = mgr->mixer()->create(wave_, has3D_);
    baseFreq = wave_->rate;
    avgBytesPerSec = wave_->avgBytesPerSec;
    freq = 100;
    volume = 100;
    prepare();
    kind = k;
    applyVolume();
    return true;
}

void AudioStream::prepare() {
    if (prepared) return;
    if (voice_ && mgr_) mgr_->mixer()->setPosition(voice_, 0);
    readPos_ = std::min(ringSize, dataSize);  // Ring ab Dateianfang gefüllt
    playBase_ = 0;
    prepared = true;
}

void AudioStream::play() {
    if (!voice_) return;
    if (playing) stop();
    if (!prepared) prepare();
    // Ring läuft in Schleife; nach dem Dateiende folgt Stille – im Port: Daten einmal, dann still
    mgr_->mixer()->play(voice_, false);
    playing = true;
    prepared = false;
    playStart_ = startTime = now();
    refillTimer_.start(now(), static_cast<std::uint32_t>(refillMs), true);
}

void AudioStream::playRepeat(int pauseSec) {
    loop = true;
    repeatPause = pauseSec;
    if (!playing) play();
}

void AudioStream::stop() {
    if (voice_ && mgr_ && mgr_->mixer()) mgr_->mixer()->stop(voice_);
    playing = false;
    refillTimer_.stop();
    markEnd();
}

void AudioStream::refill() {
    if (!wave_ || !voice_) return;
    const std::uint32_t remaining = dataSize - readPos_;
    const std::uint32_t elapsed = now() - playStart_;  // +0x1A0
    if (elapsed >= durationMs && remaining == 0) {
        stop();  // vtable +0x14
    } else {
        // Ring bis zum Abspielzeiger nachfüllen: Lesestand = gespielt + Ringgröße
        const std::uint32_t played = mgr_->mixer()->position(voice_) - playBase_;
        readPos_ = std::min(dataSize, std::max(readPos_, played + ringSize));
    }
    if (!playing && loop) {
        if (repeatPause == 0) {
            play();
        } else {
            repeatTimer_.start(now(), static_cast<std::uint32_t>(repeatPause) * 1000, false);
            if (repeatPause < 300) repeatPause += 10;
        }
    }
}

void AudioStream::repeatNow() {
    repeatTimer_.stop();
    if (!playing) play();
}

void AudioStream::pollTimers(std::uint32_t t) {
    if (refillTimer_.fire(t)) refill();
    if (repeatTimer_.fire(t)) repeatNow();
}

}  // namespace rah
