// SPDX-License-Identifier: GPL-3.0-or-later
#include <map>

#include "audio/mixer.hpp"
#include "core/strings.hpp"
#include "engine/audio.hpp"
#include "engine/globals.hpp"
#include "test.hpp"

using namespace rah;

namespace {

class MemFs final : public FileSystem {
public:
    std::map<std::string, Bytes> files;
    void put(const std::string& p, const Bytes& b) { files[toUpperAscii(p)] = b; }
    bool exists(std::string_view p) const override { return files.count(toUpperAscii(std::string(p))) != 0; }
    std::optional<Bytes> read(std::string_view p) const override {
        auto it = files.find(toUpperAscii(std::string(p)));
        if (it == files.end()) return std::nullopt;
        return it->second;
    }
    std::vector<std::string> list(std::string_view) const override { return {}; }
};

// PCM mono 8 Bit, 22 050 Hz, n Bytes mit Wert v
Bytes wav8(std::uint32_t n, std::uint8_t v = 200) {
    Bytes b;
    auto u32 = [&](std::uint32_t x) {
        for (int i = 0; i < 4; ++i) b.push_back(static_cast<std::uint8_t>(x >> (8 * i)));
    };
    auto u16 = [&](std::uint16_t x) {
        b.push_back(static_cast<std::uint8_t>(x)), b.push_back(static_cast<std::uint8_t>(x >> 8));
    };
    b.insert(b.end(), {'R', 'I', 'F', 'F'}), u32(36 + n), b.insert(b.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    u32(16), u16(1), u16(1), u32(22050), u32(22050), u16(1), u16(8);
    b.insert(b.end(), {'d', 'a', 't', 'a'}), u32(n);
    b.insert(b.end(), n, v);
    return b;
}

struct Rig {
    MemFs fs;
    EngineGlobals g;
    std::uint32_t clock = 1000;
    Mixer mixer;
    std::unique_ptr<AudioManager> mgr;
    Rig() {
        g.now = [this] { return clock; };
        mgr = std::make_unique<AudioManager>(g, &mixer);
        g.audio = mgr.get();
    }
    FoundFile put(const std::string& name, const Bytes& b) {
        fs.put(name, b);
        return FoundFile{&fs, name};
    }
    void advance(std::uint32_t ms) {
        for (std::uint32_t t = 0; t < ms; t += 10) {
            clock += 10;
            pollTimers(g);
        }
    }
};

}  // namespace

TEST(wav_parse_duration) {
    const Bytes b = wav8(22050 * 3 / 2);
    std::string err;
    auto w = parseWave(b, &err);
    REQUIRE(w);
    CHECK(w->rate == 22050 && w->bits == 8 && w->channels == 1 && w->durationMs == 1500);
}

TEST(audio_volume_formula_and_categories) {
    Rig r;
    auto f = r.put("A.WAV", wav8(100));
    AudioBuffer b;
    REQUIRE(b.open(&f, r.mgr.get(), kAudioSfx));
    CHECK(b.volumeDb == 0);
    b.setVolume(50);
    CHECK(b.volumeDb == -1666);  // 0.5 · 3333 − 3333 = −1666.5 → abgeschnitten
    b.setVolume(0);
    CHECK(b.volumeDb == -3333);
    b.setVolume(100);
    r.mgr->setSfxVolume(50);
    CHECK(b.volumeDb == -1666);
    r.mgr->setMusicVolume(0);  // andere Kategorie: keine Wirkung
    CHECK(b.volumeDb == -1666);
}

TEST(audio_fade_tick_steps_and_clamps) {
    Rig r;
    auto f = r.put("A.WAV", wav8(100));
    AudioBuffer b;
    REQUIRE(b.open(&f, r.mgr.get(), kAudioSfx));
    b.setVolume(0);
    b.fadeVolume(30, 5000);  // Schritt (30 − 0) / 50 = 0 → keine Blende (Original)
    CHECK(b.volumeStep == 0);
    b.fadeVolume(45, 1500);  // Schritt 3
    r.advance(1000);         // 10 Takte
    CHECK(b.volume == 30);
    r.advance(1000);
    CHECK(b.volume == 45 && b.volumeStep == 0);
    b.fadeFreq(50, 1000);  // (50 − 100) / 10 = −5
    r.advance(1000);
    CHECK(b.freq == 50 && b.freqStep == 0);
}

TEST(audio_buffer_end_event_after_duration) {
    Rig r;
    auto f = r.put("A.WAV", wav8(22050));  // 1000 ms
    AudioBuffer b;
    REQUIRE(b.open(&f, r.mgr.get(), kAudioSfx));
    Event e;
    e.setHandler(9);
    std::vector<int> fired;
    r.g.eventProc = [&](Event& ev, int t) { fired.push_back(ev.param * 100 + t); };
    b.setEndEvent(&e);
    CHECK(e.enabled);
    b.play();
    CHECK(b.playing && r.mixer.playing(1));
    r.advance(990);
    CHECK(!e.pending);
    r.advance(20);
    CHECK(e.pending && e.type == 0x1F && b.playing);  // +0x168 bleibt 1
    fireEvent(r.g, e, e.type);
    CHECK(fired == std::vector<int>{9 * 100 + 0x1F});
}

TEST(audio_stream_ends_on_refill_tick_and_repeats_with_growing_pause) {
    Rig r;
    auto f = r.put("M.WAV", wav8(22050 * 3));  // 3000 ms, Ring 2 s
    AudioStream m;
    REQUIRE(m.open(&f, r.mgr.get(), kAudioMusic));
    CHECK(m.repeatPause == 30 && m.ringSize == 44100);
    m.playLoop();  // vtable +0x10 → Wiederholen nach 30 s
    CHECK(m.playing && m.loop);
    // ohne Audio-Thread läuft der Abspielzeiger nicht: Datei per Mischer „abspielen“
    std::vector<std::int16_t> out(2 * 22050);
    for (int s = 0; s < 3; ++s) r.mixer.mix(out.data(), 22050);
    r.advance(2990);
    CHECK(m.playing);
    r.advance(260);  // nächster 250-ms-Takt nach 3000 ms
    CHECK(!m.playing && m.repeatPause == 40);
    r.advance(29000);
    CHECK(!m.playing);
    r.advance(1100);
    CHECK(m.playing);
}

TEST(audio_fade_out_delete_adopted) {
    Rig r;
    auto f = r.put("M.WAV", wav8(22050));
    bool gone = false;
    struct Probe final : AudioBuffer {
        bool* gone = nullptr;
        ~Probe() override { *gone = true; }
    };
    auto m = std::make_unique<Probe>();
    m->gone = &gone;
    REQUIRE(m->open(&f, r.mgr.get(), kAudioMusic));
    m->setVolume(80);
    m->deleteWhenSilent = true;  // 0x00438010(2)
    m->fadeVolume(0, 2000);      // Schritt −4
    AudioBuffer* raw = m.get();
    r.mgr->adopt(std::move(m));
    r.advance(1900);
    CHECK(!gone && raw->volume == 4);
    r.advance(200);
    CHECK(gone);
    r.mgr->setMusicVolume(50);  // ausgetragen: kein Zugriff mehr
}

TEST(audio_fade_out_quirk_low_volume_never_deleted) {
    Rig r;
    auto f = r.put("M.WAV", wav8(22050));
    auto m = std::make_unique<AudioStream>();
    REQUIRE(m->open(&f, r.mgr.get(), kAudioMusic));
    m->setVolume(15);
    m->deleteWhenSilent = true;
    m->fadeVolume(0, 2000);  // (0 − 15) / 20 = 0 → keine Blende, bleibt bestehen (Original)
    CHECK(m->volumeStep == 0);
}

TEST(audio_getpos_from_play_cursor) {
    Rig r;
    auto f = r.put("A.WAV", wav8(22050 * 2));
    AudioBuffer b;
    REQUIRE(b.open(&f, r.mgr.get(), kAudioSpeech));
    b.play();
    std::vector<std::int16_t> out(2 * 11025);
    r.mixer.mix(out.data(), 11025);  // 0,5 s gespielt
    CHECK(b.getPos() == 500);        // Bytes / (22050 · 0.001)
    AudioStream s;
    REQUIRE(s.open(&f, r.mgr.get(), kAudioSpeech));
    s.play();
    r.advance(300);
    CHECK(s.getPos() == 300);  // Stream: Zeit seit Play
}

TEST(audio_3d_option_pans_and_attenuates) {
    for (bool hw : {false, true}) {
        Rig r;
        r.mgr = std::make_unique<AudioManager>(r.g, &r.mixer, hw);
        r.g.audio = r.mgr.get();
        auto f = r.put("A.WAV", wav8(22050));
        AudioBuffer b;
        REQUIRE(b.open(&f, r.mgr.get(), kAudioSfx));
        b.setPos3D(0.4f, 0.0f, 0.0f);  // × 7 → x = 2.8: rechts, Abstand > 1
        b.play();
        std::vector<std::int16_t> out(2 * 64);
        r.mixer.mix(out.data(), 64);
        const int l = out[20], rr = out[21];
        if (!hw) CHECK(l == rr && rr > 15000);       // ohne 3D: mitte, voll
        else CHECK(l == 0 && rr > 0 && rr < 15000);  // ganz rechts, 1/2.8
    }
}

TEST(audio_3d_reverb_tail) {
    Rig r;
    r.mgr = std::make_unique<AudioManager>(r.g, &r.mixer, true);
    r.g.audio = r.mgr.get();
    auto f = r.put("A.WAV", wav8(2205));  // 0,1 s
    AudioBuffer b;
    REQUIRE(b.open(&f, r.mgr.get(), kAudioMusic));
    b.setReverb(15, 0.111f, 3.145f, 0.472f);  // EAX-Vorgabe FOREST
    b.setReverbMix(0.1f);
    b.play();
    std::vector<std::int16_t> out(2 * 22050);
    r.mixer.mix(out.data(), 22050);
    int tail = 0;
    for (std::size_t i = 2 * 4410; i < out.size(); ++i) tail = std::max(tail, std::abs(int{out[i]}));
    CHECK(tail > 0);  // Nachhall nach dem Ende der Daten
}
