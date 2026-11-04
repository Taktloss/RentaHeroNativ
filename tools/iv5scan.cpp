// SPDX-License-Identifier: GPL-3.0-or-later
// Liest jedes Videobild aller Indeo-5-AVIs (FourCC IV50) mit Indeo5Mask und zählt die Ergebnisse
// (deckend / Maske / nicht nachgebildet). Beleg für die Abdeckung des Maskenlesers über alle
// Bilder (docs/VIDEO.md). Aufruf: rah_iv5scan <ordner> [<ordner> …]
// Vergleich mit dem Original-Codec: rah_iv5scan --compare <avi> <out.bin von iv5ref.exe>
// (alle Bilder; in.bin per tools/iv5ref/avi2bin.py <avi> in.bin 24 99999) – Maske 0 muss
// schwarz sein (Füllfarbe), außerdem Zählung schwarzer Pixel mit Maske 1.
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>

#include "core/bytes.hpp"
#include "video/iv5mask.hpp"

using namespace rah;

namespace {
std::uint32_t rd32(const std::uint8_t* p) {
    return p[0] | p[1] << 8 | p[2] << 16 | static_cast<std::uint32_t>(p[3]) << 24;
}
std::size_t find(const Bytes& d, const char* tag, std::size_t from = 0) {
    for (std::size_t i = from; i + 4 <= d.size(); ++i)
        if (std::memcmp(d.data() + i, tag, 4) == 0) return i;
    return std::string::npos;
}
}  // namespace

// Videobilder in idx1-Reihenfolge (= movi-Reihenfolge der Spieldaten)
template <class F>
bool forEachFrame(const Bytes& d, F&& f) {
    const std::size_t movi = find(d, "movi"), idx = find(d, "idx1", movi);
    if (movi == std::string::npos || idx == std::string::npos) return false;
    const std::uint32_t n = rd32(d.data() + idx + 4) / 16;
    for (std::uint32_t i = 0; i < n; ++i) {
        const std::uint8_t* ent = d.data() + idx + 8 + 16 * i;
        if (ent[2] != 'd' || (ent[3] != 'c' && ent[3] != 'b')) continue;
        std::size_t hdr = movi + rd32(ent + 8);
        if (hdr + 8 > d.size() || std::memcmp(d.data() + hdr, ent, 4) != 0) hdr = rd32(ent + 8);
        if (hdr + 8 > d.size() || std::memcmp(d.data() + hdr, ent, 4) != 0) return false;
        const std::size_t size = rd32(ent + 12);
        if (hdr + 8 + size > d.size()) return false;
        f(d.data() + hdr + 8, size);
    }
    return true;
}

int compare(const char* avi, const char* ref) {
    const auto d = readFile(avi);
    const auto r = readFile(ref);
    if (!d || !r) return 1;
    Indeo5Mask m;
    std::size_t pos = 0;
    long frames = 0, maskFrames = 0, bad = 0, black1 = 0, px1 = 0, px0 = 0;
    bool shortRef = false;
    forEachFrame(*d, [&](const std::uint8_t* p, std::size_t size) {
        const auto res = m.decode(p, size);
        const int w = m.width(), h = m.height();
        const std::size_t stride = (static_cast<std::size_t>(w) * 3 + 3) & ~std::size_t{3};
        const std::size_t fsz = stride * static_cast<std::size_t>(h);
        if (w <= 0 || pos + 4 + fsz > r->size()) {
            shortRef = true;
            return;
        }
        const std::uint8_t* px = r->data() + pos + 4;
        pos += 4 + fsz;
        ++frames;
        if (res != Indeo5Mask::Result::Mask) return;
        ++maskFrames;
        const auto& mk = m.mask();
        for (int y = 0; y < h; ++y) {
            const std::uint8_t* line = px + static_cast<std::size_t>(h - 1 - y) * stride;
            for (int x = 0; x < w; ++x) {
                const bool black = line[x * 3] == 0 && line[x * 3 + 1] == 0 && line[x * 3 + 2] == 0;
                if (mk[static_cast<std::size_t>(y) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x)] == 0) {
                    ++px0;
                    bad += !black;
                } else {
                    ++px1;
                    black1 += black;
                }
            }
        }
    });
    std::cout << std::filesystem::path(avi).filename().string() << ": " << frames << " Bilder (" << maskFrames
              << " mit Maske), Maske 0: " << px0 << " Pixel, davon nicht schwarz " << bad << "; Maske 1: " << px1
              << " Pixel, davon schwarz " << black1 << (shortRef ? " (Referenz kürzer!)" : "") << "\n";
    return bad == 0 && !shortRef ? 0 : 1;
}

int main(int argc, char** argv) {
    if (argc == 4 && std::strcmp(argv[1], "--compare") == 0) return compare(argv[2], argv[3]);
    if (argc < 2) {
        std::cerr << "Aufruf: rah_iv5scan <ordner> […]\n";
        return 2;
    }
    int files = 0, masked = 0;
    long total[3] = {0, 0, 0}, sizeMismatch = 0;
    for (int a = 1; a < argc; ++a) {
        for (const auto& e : std::filesystem::recursive_directory_iterator(argv[a])) {
            std::string ext = e.path().extension().string();
            for (auto& c : ext) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            if (ext != ".AVI") continue;
            const auto d = readFile(e.path());
            if (!d) continue;
            const std::size_t strf = find(*d, "strf"), movi = find(*d, "movi"), idx = find(*d, "idx1", movi);
            if (strf == std::string::npos || movi == std::string::npos || idx == std::string::npos) continue;
            const std::uint8_t* bih = d->data() + strf + 8;
            if (std::memcmp(bih + 16, "IV50", 4) != 0) continue;
            const int w = static_cast<int>(rd32(bih + 4)), h = static_cast<int>(rd32(bih + 8));
            ++files;
            Indeo5Mask m;
            long cnt[3] = {0, 0, 0};
            const std::uint32_t n = rd32(d->data() + idx + 4) / 16;
            for (std::uint32_t i = 0; i < n; ++i) {
                const std::uint8_t* ent = d->data() + idx + 8 + 16 * i;
                if (ent[2] != 'd' || (ent[3] != 'c' && ent[3] != 'b')) continue;
                // idx1-Versatz relativ zu „movi“ (üblich) oder absolut – am Chunk-Kopf prüfen
                std::size_t hdr = movi + rd32(ent + 8);
                if (hdr + 8 > d->size() || std::memcmp(d->data() + hdr, ent, 4) != 0) hdr = rd32(ent + 8);
                if (hdr + 8 > d->size() || std::memcmp(d->data() + hdr, ent, 4) != 0) {
                    std::cerr << e.path().filename().string() << ": Index passt nicht\n";
                    break;
                }
                const std::size_t off = hdr + 8, size = rd32(ent + 12);
                if (off + size > d->size()) break;
                const auto r = m.decode(d->data() + off, size);
                ++cnt[static_cast<int>(r)];
                if (r == Indeo5Mask::Result::Mask && (m.width() != w || m.height() != h)) ++sizeMismatch;
            }
            for (int k = 0; k < 3; ++k) total[k] += cnt[k];
            if (cnt[1] || cnt[2]) {
                ++masked;
                std::cout << e.path().filename().string() << ": deckend " << cnt[0] << ", Maske " << cnt[1]
                          << ", nicht nachgebildet " << cnt[2] << "\n";
            }
        }
    }
    std::cout << files << " Indeo-5-Videos, " << masked << " mit Maske/Sonderfall; Bilder: deckend " << total[0]
              << ", Maske " << total[1] << ", nicht nachgebildet " << total[2] << ", Maskengröße falsch "
              << sizeMismatch << "\n";
    return total[2] == 0 && sizeMismatch == 0 ? 0 : 1;
}
