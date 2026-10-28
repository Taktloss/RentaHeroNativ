// SPDX-License-Identifier: GPL-3.0-or-later
#include <filesystem>

#include "core/strings.hpp"
#include "test.hpp"
#include "video/ffvideo.hpp"

using namespace rah;

// Alle AVIs der Daten: öffnen, Länge (100-ms-Einheiten) = Bildanzahl, alle Bilder bis zum
// Ende, Seek liefert dasselbe Bild wie fortlaufendes Dekodieren.
TEST(ffvideo_original_avis) {
    const auto root = rahtest::dataDir() / "RentAHero";
    SKIP_IF(rahtest::dataDir().empty() || !std::filesystem::exists(root));
    int files = 0, bad = 0;
    for (const auto& e : std::filesystem::recursive_directory_iterator(root)) {
        if (!e.is_regular_file() || !equalsNoCase(e.path().extension().string(), ".avi")) continue;
        ++files;
        auto data = readFile(e.path());
        REQUIRE(data);
        std::string err;
        auto v = openFfmpegVideo(*data, &err);
        if (!v) {
            ++bad;
            std::cerr << "     " << e.path().filename().string() << ": " << err << "\n";
            continue;
        }
        const auto len = v->duration() / 1000000;
        std::vector<std::uint16_t> f, third;
        int n = 0;
        VideoDecoder::Result r;
        while ((r = v->next(f)) == VideoDecoder::Result::Ok) {
            if (n == 2) third = f;
            ++n;
        }
        bool ok = r == VideoDecoder::Result::End && n == len && f.size() == static_cast<std::size_t>(v->width() * v->height());
        if (len > 3) {
            v->seek(2000000);
            ok = ok && v->next(f) == VideoDecoder::Result::Ok && f == third;
        }
        if (!ok) {
            ++bad;
            std::cerr << "     " << e.path().filename().string() << ": " << n << " Bilder, Länge " << len << "\n";
        }
    }
    std::cerr << "     " << files << " AVIs, " << bad << " fehlerhaft\n";
    CHECK(files > 100 && bad == 0);
}
