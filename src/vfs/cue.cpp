// SPDX-License-Identifier: GPL-3.0-or-later
#include "vfs/cue.hpp"

#include <sstream>

#include "core/bytes.hpp"
#include "core/strings.hpp"

namespace rah {

namespace {

bool modeLayout(const std::string& mode, unsigned& size, unsigned& offset) {
    const std::string m = toUpperAscii(mode);
    if (m == "MODE1/2048") { size = 2048; offset = 0; return true; }
    if (m == "MODE1/2352") { size = 2352; offset = 16; return true; }
    // MODE2/2352: Daten in Form 1 hinter 12 Sync + 4 Kopf + 8 Subheader (docs/DATAPATHS.md)
    if (m == "MODE2/2352") { size = 2352; offset = 24; return true; }
    if (m == "MODE2/2336") { size = 2336; offset = 8; return true; }
    return false;
}

}  // namespace

std::optional<CueTrack> parseCue(const std::string& text, const std::filesystem::path& baseDir) {
    std::istringstream in(text);
    std::string line;
    std::filesystem::path currentFile;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream ls(line);
        std::string keyword;
        ls >> keyword;
        keyword = toUpperAscii(keyword);
        if (keyword == "FILE") {
            const auto q1 = line.find('"');
            const auto q2 = line.rfind('"');
            std::string name;
            if (q1 != std::string::npos && q2 > q1) {
                name = line.substr(q1 + 1, q2 - q1 - 1);
            } else {
                ls >> name;
            }
            currentFile = baseDir / name;
        } else if (keyword == "TRACK") {
            std::string number, mode;
            ls >> number >> mode;
            CueTrack t;
            if (!currentFile.empty() && modeLayout(mode, t.sectorSize, t.dataOffset)) {
                t.binFile = currentFile;
                t.mode = mode;
                return t;
            }
        }
    }
    return std::nullopt;
}

std::optional<CueTrack> parseCueFile(const std::filesystem::path& cuePath) {
    auto data = readFile(cuePath);
    if (!data) return std::nullopt;
    return parseCue(std::string(data->begin(), data->end()), cuePath.parent_path());
}

}  // namespace rah
