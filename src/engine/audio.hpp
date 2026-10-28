// SPDX-License-Identifier: GPL-3.0-or-later
// Ton: Audio-Manager CAudio, Puffer CAudioBuffer, Stream (Musik/Sprache) – docs/AUDIO.md.
// DirectSound wird durch den Mischer (audio/mixer) ersetzt; ohne Mischer bleibt alles stumm
// wie im Original ohne DirectSound (+0x04 = 0: Open scheitert, Aufrufe wirken nicht).
#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "engine/event.hpp"
#include "engine/paths.hpp"
#include "formats/wav.hpp"

namespace rah {

class Mixer;
struct EngineGlobals;
class AudioBuffer;

// Kategorie +0x04 (bestimmt die Gesamtlautstärke)
enum AudioKind { kAudioSfx = 2, kAudioSpeech = 3, kAudioMusic = 4 };

// Port-Ersatz für einen CTimer (timeSetEvent), geprüft in pollTimers (TIMING.md)
struct AudioTimer {
    bool running = false, periodic = false;
    std::uint32_t due = 0, period = 0;
    void start(std::uint32_t now, std::uint32_t ms, bool rep) {
        running = true;
        periodic = rep;
        period = ms;
        due = now + ms;
    }
    void stop() { running = false; }
    bool fire(std::uint32_t now) {  // true = fällig (einmalig → aus)
        if (!running || static_cast<std::int32_t>(now - due) < 0) return false;
        if (periodic) due += period;
        else running = false;
        return true;
    }
};

class AudioManager {
public:
    static constexpr int kSlots = 100;
    // orig: Rent-A-Hero.exe 0x00436530 CAudio_ctor (Blend-Takt 100 ms) + 0x00436670 (DirectSound)
    AudioManager(EngineGlobals& g, Mixer* mixer);
    ~AudioManager();
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // orig: Rent-A-Hero.exe 0x004369D0 / 0x00436A20 / 0x00436A70 (0…100, wirkt sofort)
    void setSfxVolume(int v);
    void setMusicVolume(int v);
    void setSpeechVolume(int v);
    int sfxVolume() const { return sfx_; }
    int musicVolume() const { return music_; }
    int speechVolume() const { return speech_; }
    Mixer* mixer() const { return mixer_; }
    EngineGlobals& globals() { return g_; }

    // orig: Rent-A-Hero.exe 0x00436AC0 CAudio_AddBuffer / 0x00436AF0 CAudio_RemoveBuffer
    void add(AudioBuffer* b);
    void remove(AudioBuffer* b);
    // orig: Rent-A-Hero.exe 0x00436B20 CAudio_FadeTick
    void fadeTick();
    // Port: fällige Timer (Blend-Takt, Ende-/Nachlade-/Wiederholungs-Timer der Puffer)
    void poll();
    // Port: Besitz eines Puffers mit „bei Stille löschen“ (+0x12C) übernehmen – FadeTick löscht ihn
    void adopt(std::unique_ptr<AudioBuffer> b);

private:
    void apply(int kind);
    EngineGlobals& g_;
    Mixer* mixer_;                                // +0x04 IDirectSound (nullptr = keins)
    int sfx_ = 100, music_ = 100, speech_ = 100;  // +0x68, +0x6C, +0x70 (rah.ini, Vorgabe 100)
    std::array<AudioBuffer*, kSlots> slots_{};    // +0x84
    AudioTimer tick_;                             // +0x214
    std::vector<std::unique_ptr<AudioBuffer>> adopted_;
};

class AudioBuffer {
public:
    // orig: Rent-A-Hero.exe 0x00436C30 CAudioBuffer_ctor
    AudioBuffer() = default;
    // orig: Rent-A-Hero.exe 0x00436D00 CAudioBuffer_dtor (stoppt – Ende-Ereignis!, austragen)
    virtual ~AudioBuffer();
    AudioBuffer(const AudioBuffer&) = delete;
    AudioBuffer& operator=(const AudioBuffer&) = delete;

    // vtable +0x04 – orig: Rent-A-Hero.exe 0x00436DC0 (ganze Datei in einen statischen Puffer)
    virtual bool open(const FoundFile* file, AudioManager* mgr, int kind);
    // vtable +0x08 – orig: Rent-A-Hero.exe 0x00437160 (mit Ende-Ereignis: Timer nach Dauer)
    virtual void play();
    // vtable +0x0C – orig: Rent-A-Hero.exe 0x00436DB0 (→ +0x10)
    virtual void playRepeat(int) { playLoop(); }
    // vtable +0x10 – orig: Rent-A-Hero.exe 0x00437100 (Schleife)
    virtual void playLoop();
    // vtable +0x14 – orig: Rent-A-Hero.exe 0x00437270 (Ende-Ereignis Typ 0x1F markieren)
    virtual void stop();
    // vtable +0x1C – orig: Rent-A-Hero.exe 0x004370D0 (auf Anfang, einmal je Play)
    virtual void prepare();

    // orig: Rent-A-Hero.exe 0x004372E0 / 0x00437310 / 0x00437430 / 0x00437480 / 0x004374F0
    void setVolume(int v);
    void fadeVolume(int v, int ms);
    void setFreq(int p);
    void fadeFreq(int p, int ms);
    void setPos3D(float x, float y, float z);
    // orig: Rent-A-Hero.exe 0x004375E0 – Ende-Ereignis (+0x130) setzen und aktivieren (nullptr: nichts)
    void setEndEvent(Event* e);
    // orig: Rent-A-Hero.exe 0x00437FD0 (nur wenn still: Lautstärke 0, Blende, vtable +0x10)
    void fadeIn(int v, int ms);
    // orig: Rent-A-Hero.exe 0x00437560 / 0x004375B0: Eigenschaften (+0x158, nur mit 3D-Hardware)
    void setProperty(int, float, float, float) {}
    void setProperty2(int) {}
    // orig: Rent-A-Hero.exe 0x00437350 – Lautstärke anwenden (1/100 dB, −3333…0)
    void applyVolume();

    // Port: fällige eigene Timer
    virtual void pollTimers(std::uint32_t) ;

    int kind = 0;                 // +0x04
    std::string path;             // +0x08
    int volume = 100;             // +0x10C
    int volumeDb = 0;             // +0x110
    int freq = 100;               // +0x114 (%)
    int baseFreq = 0;             // +0x118 (Hz)
    int freqTarget = 100, freqStep = 0;  // +0x11C, +0x120
    int volumeTarget = 100, volumeStep = 0;  // +0x124, +0x128
    bool deleteWhenSilent = false;  // +0x12C
    Event* endEvent = nullptr;    // +0x130
    std::uint32_t startTime = 0;  // +0x15C
    bool prepared = false;        // +0x164
    bool playing = false;         // +0x168
    std::uint32_t dataSize = 0;   // +0x16C
    std::uint32_t durationMs = 0; // +0x174
    bool loop = false;            // +0x178
    float pos3D[3]{};             // nur gespeichert: ohne 3D-Hardware kein IDirectSound3DBuffer

protected:
    void applyFreq(int hz);       // orig: Rent-A-Hero.exe 0x004374C0 (100…100 000 Hz)
    void markEnd();               // Ende-Ereignis: pending, Typ/Param 0x1F, Sofort-Flag → auslösen
    std::uint32_t now() const;
    AudioManager* mgr_ = nullptr; // +0x134
    int voice_ = 0;               // +0x13C IDirectSoundBuffer
    std::shared_ptr<const Wave> wave_;
    AudioTimer endTimer_;         // +0x184
};

// Stream-Puffer (Musik, CGame_Say): 2 s Ring, Nachladen alle 250 ms, Ende nach Zeit
class AudioStream final : public AudioBuffer {
public:
    // orig: Rent-A-Hero.exe 0x004376C0 Ctor / 0x00437750 Dtor
    AudioStream() = default;
    ~AudioStream() override;
    // orig: Rent-A-Hero.exe 0x004377F0
    bool open(const FoundFile* file, AudioManager* mgr, int kind) override;
    // orig: Rent-A-Hero.exe 0x00437E50
    void play() override;
    // orig: Rent-A-Hero.exe 0x00437E20 – Wiederholen nach pause Sekunden
    void playRepeat(int pauseSec) override;
    // orig: Rent-A-Hero.exe 0x00437E10 → +0x0C(30)
    void playLoop() override { playRepeat(30); }
    // orig: Rent-A-Hero.exe 0x00437F50
    void stop() override;
    // orig: Rent-A-Hero.exe 0x00437DC0 (Ring ab Dateianfang füllen)
    void prepare() override;
    void pollTimers(std::uint32_t t) override;

    int repeatPause = 0;         // +0x180 s (Open: 30; nach jeder Wiederholung +10 bis 300)
    std::uint32_t ringSize = 0;  // +0x16C
    int bufferMs = 2000;         // +0x194
    int refillMs = 250;          // +0x198

private:
    // orig: Rent-A-Hero.exe 0x00437B80 (Nachladen, Ende erkennen, Wiederholung planen)
    void refill();
    // orig: Rent-A-Hero.exe 0x00437D70 (Wiederholungs-Timer)
    void repeatNow();
    std::uint32_t readPos_ = 0;  // Lesestand der Datei (CWave +0x58)
    std::uint32_t playBase_ = 0; // Port: Abspielstand beim Start (Bytes)
    std::uint32_t playStart_ = 0;// +0x19C
    AudioTimer refillTimer_;     // +0x188
    AudioTimer repeatTimer_;     // +0x18C
};

// Laden einer WAV-Datei für open() (Port: Datei einmal lesen; nullptr bei Fehler)
std::shared_ptr<const Wave> loadWave(const FoundFile& file);

}  // namespace rah
