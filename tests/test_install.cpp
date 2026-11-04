// SPDX-License-Identifier: GPL-3.0-or-later
#include <filesystem>
#include <cstdio>
#include <map>

#include "core/strings.hpp"
#include "test.hpp"
#include "vfs/dirfs.hpp"
#include "vfs/filesystem.hpp"
#include "vfs/install.hpp"

using namespace rah;

namespace {

class MemFs final : public FileSystem {
public:
    std::map<std::string, Bytes> files;
    bool exists(std::string_view p) const override { return files.count(toUpperAscii(std::string(p))) != 0; }
    std::optional<Bytes> read(std::string_view p) const override {
        auto it = files.find(toUpperAscii(std::string(p)));
        if (it == files.end()) return std::nullopt;
        return it->second;
    }
    std::vector<std::string> list(std::string_view) const override { return {}; }
};

std::filesystem::path tempDir(const char* name) {
    auto d = std::filesystem::temp_directory_path() / name;
    std::filesystem::remove_all(d);
    return d;
}

}  // namespace

TEST(install_subdir_strips_component_group) {
    CHECK(installSubdir("Ausf\xFChrbare Programmdateien") == "");
    CHECK(installSubdir("Ausf\xFChrbare Programmdateien\\Common") == "Common");
    CHECK(installSubdir("Common") == "Common");
    CHECK(installSubdir("") == "");
}

TEST(install_fails_without_cab_and_bad_cab) {
    MemFs fs;
    const auto d = tempDir("rah_install_test");
    std::string err;
    CHECK(!installFromCab(fs, d, &err) && !err.empty());
    fs.files["SETUP/DATA1.CAB"] = Bytes{'n', 'o', 'p', 'e'};
    CHECK(!installFromCab(fs, d, &err));
    CHECK(!std::filesystem::exists(d / ".cab"));  // Zwischendatei aufgeräumt
    CHECK(!installComplete(d));
    std::filesystem::remove_all(d);
}

// Mit Originaldaten (RAH_TEST_DATA = zusammengeführter CD-Ordner): echtes Entpacken
TEST(install_from_original_cab) {
    const auto root = rahtest::dataDir() / "RentAHero";
    SKIP_IF(rahtest::dataDir().empty() || !std::filesystem::exists(root / "SETUP"));
    DirFileSystem cd(root.string());
    const auto d = tempDir("rah_install_real");
    std::string err;
    int last = 0;
    const bool ok = installFromCab(cd, d, &err, [&](int i, int) { last = i; });
    if (!ok) std::fprintf(stderr, "Fehler: %s\n", err.c_str());
    CHECK(ok);
    CHECK(installComplete(d));
    CHECK(last == 130);
    CHECK(std::filesystem::file_size(d / "Rent-A-Hero.exe") == 716873);
    CHECK(std::filesystem::exists(d / "Common" / "003_PRI_05.wav"));
    std::filesystem::remove_all(d);
}
