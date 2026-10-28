// SPDX-License-Identifier: GPL-3.0-or-later
#include <map>

#include "core/strings.hpp"
#include "engine/animlayer.hpp"
#include "engine/aviimage.hpp"
#include "engine/scene.hpp"
#include "test.hpp"

using namespace rah;

namespace {

// Attrappe: n Bilder 4×2 bei 10 fps, Pixelwert = 0x0841 · (Bild + 1)
class FakeVideo final : public VideoDecoder {
public:
    explicit FakeVideo(int n, std::vector<std::int64_t>* seeks = nullptr) : n_(n), seeks_(seeks) {}
    int width() const override { return 4; }
    int height() const override { return 2; }
    std::int64_t startTime() const override { return 0; }
    std::int64_t duration() const override { return std::int64_t{n_} * 1000000; }
    void seek(std::int64_t t) override {
        pos_ = static_cast<int>(t / 1000000);
        if (seeks_) seeks_->push_back(t);
    }
    Result next(std::vector<std::uint16_t>& f) override {
        if (pos_ >= n_) return Result::End;
        f.assign(8, value(pos_++));
        return Result::Ok;
    }
    static std::uint16_t value(int frame) { return static_cast<std::uint16_t>(0x0841 * (frame + 1)); }

private:
    int n_, pos_ = 0;
    std::vector<std::int64_t>* seeks_;
};

class MemFs final : public FileSystem {
public:
    std::map<std::string, Bytes> files;
    void put(const std::string& p, std::string_view s) { files[toUpperAscii(p)] = Bytes(s.begin(), s.end()); }
    bool exists(std::string_view p) const override { return files.count(toUpperAscii(std::string(p))) != 0; }
    std::optional<Bytes> read(std::string_view p) const override {
        auto it = files.find(toUpperAscii(std::string(p)));
        if (it == files.end()) return std::nullopt;
        return it->second;
    }
    std::vector<std::string> list(std::string_view) const override { return {}; }
};

Image565 blank() { return Image565{4, 2, std::vector<std::uint16_t>(8, 0)}; }

}  // namespace

TEST(avi_image_counter_sample_and_end) {
    std::vector<std::int64_t> seeks;
    AviImage a;
    REQUIRE(a.load(std::make_unique<FakeVideo>(5, &seeks)));
    CHECK(a.length() == 5 && a.counter() == 0 && a.width() == 4);
    Image565 t = blank();
    const Rect r{0, 0, 4, 2};
    for (int i = 0; i < 5; ++i) {
        CHECK(a.next());
        CHECK(a.draw(t, r));
        CHECK(t.pixels[7] == FakeVideo::value(i));
    }
    CHECK(a.counter() == 5);
    CHECK(!a.next());  // Länge erreicht → Sample beendet
    CHECK(!a.hasSample());
    CHECK(a.draw(t, r));  // ohne Sample und Anforderung: nur der Zähler läuft
    CHECK(a.counter() == 6 && t.pixels[0] == FakeVideo::value(4));
    // Seek auf den aktuellen Zählerstand rührt den Stream nicht an
    a.seek(6);
    CHECK(seeks.empty());
    a.seek(2);
    REQUIRE(seeks.size() == 1);
    CHECK(seeks[0] == 2000000 && a.counter() == 2);
    CHECK(a.draw(t, r));  // neues Sample, Bild 2
    CHECK(t.pixels[0] == FakeVideo::value(2) && a.counter() == 3);
    // Ende des Streams beim Update: Zähler = Länge + 1, Ziel unverändert
    a.draw(t, r);
    a.draw(t, r);
    CHECK(a.draw(t, r));
    CHECK(a.counter() == 6 && t.pixels[0] == FakeVideo::value(4));
    // Seek über die Länge: Zähler bleibt, Stream wird trotzdem auf ihn gesetzt
    a.seek(9);
    CHECK(seeks.back() == 6000000 && a.counter() == 6);
    a.rewind();
    CHECK(a.counter() == 0 && seeks.back() == 0);
}

TEST(avi_image_same_rect_other_target_keeps_old) {
    AviImage a;
    REQUIRE(a.load(std::make_unique<FakeVideo>(5)));
    Image565 t1 = blank(), t2 = blank();
    a.draw(t1, {0, 0, 4, 2});
    a.next();
    a.draw(t2, {0, 0, 4, 2});  // Sample hängt an t1
    CHECK(t1.pixels[0] == FakeVideo::value(1) && t2.pixels[0] == 0);
    a.next();
    a.draw(t2, {1, 0, 4, 2});  // Rechteck geändert, andere Oberfläche → neues Sample
    CHECK(t2.pixels[1] == FakeVideo::value(2) && t2.pixels[0] == 0);
}

TEST(avi_layer_in_scene_loops_all_frames) {
    MemFs fs;
    DataPaths paths{&fs, nullptr};
    EngineGlobals g;
    std::uint32_t clock = 1000;
    g.now = [&] { return clock; };
    int opened = 0;
    g.openVideo = [&](const FoundFile&, std::string*) -> std::unique_ptr<VideoDecoder> {
        ++opened;
        return std::make_unique<FakeVideo>(5);
    };
    Surface565 surf{640, 420};
    Scene scene{paths, g, surf};
    // BMP-Hintergrund 640×360 grau (24 Bit, unten zuerst)
    std::string bmp = "BM";
    auto p32 = [&](std::uint32_t v) {
        for (int i = 0; i < 4; ++i) bmp.push_back(static_cast<char>(v >> (8 * i)));
    };
    p32(54 + 640 * 3 * 360), p32(0), p32(54), p32(40), p32(640), p32(360);
    bmp += std::string("\x01\x00\x18\x00", 4);
    for (int i = 0; i < 6; ++i) p32(0);
    bmp += std::string(640 * 3 * 360, '\x40');
    fs.put("GAME/MENU/BG.bmp", bmp);
    fs.put("GAME/MENU/V.avi", "x");
    fs.put("GAME/MENU/TV.szn",
           "[GENERAL]\r\nBackground1=Bg\r\nLayer1=Vid\r\n[Bg]\r\nFilename=BG.bmp\r\n"
           "[Vid]\r\nFilename=V.avi\r\nX=10\r\nY=20\r\nZ=10\r\nfps=10\r\n");
    REQUIRE(scene.load("TV", 0));
    CHECK(opened == 1 && scene.unsupportedLayers() == 0);
    auto* L = dynamic_cast<AnimLayer*>(scene.layer(0));
    REQUIRE(L && L->avi);
    CHECK(L->alpha && !L->hasSurface);  // bis zum ersten Zeichnen
    scene.background()->enable();
    L->enable();
    L->start(13);
    std::vector<std::uint16_t> seen;
    for (int i = 0; i < 40; ++i) {
        scene.update(true);
        scene.compose(false);
        scene.clearOutput();
        scene.dispatchEvents();
        const std::uint16_t px = surf.row(20)[10];
        if (seen.empty() || seen.back() != px) seen.push_back(px);
        clock += 50;
    }
    CHECK(!L->alpha && L->hasSurface);
    REQUIRE(seen.size() >= 11);
    for (std::size_t k = 0; k < 11; ++k) CHECK(seen[k] == FakeVideo::value(static_cast<int>(k % 5)));
}
