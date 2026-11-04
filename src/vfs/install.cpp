// SPDX-License-Identifier: GPL-3.0-or-later
#include "vfs/install.hpp"

#include <libunshield.h>

#include <fstream>
#include <system_error>

#include "core/strings.hpp"
#include "vfs/filesystem.hpp"

namespace rah {

namespace fs = std::filesystem;

bool installComplete(const fs::path& dest) {
    std::error_code ec;
    return fs::exists(dest / kInstallDone, ec) && fs::is_directory(dest / "Common", ec);
}

std::string installSubdir(const std::string& cabDir) {
    // unshield_directory_name liefert je nach Kabinett „Common“ oder „<Gruppe>\\Common“:
    // maßgeblich ist die letzte Komponente; alles andere liegt im Hauptordner (Rent-A-Hero.exe)
    std::string d = cabDir;
    for (char& c : d)
        if (c == '\\') c = '/';
    const auto slash = d.find_last_of('/');
    const std::string last = slash == std::string::npos ? d : d.substr(slash + 1);
    return toUpperAscii(last) == "COMMON" ? "Common" : std::string();
}

namespace {

struct UnshieldCloser {
    void operator()(Unshield* u) const { unshield_close(u); }
};

}  // namespace

bool installFromCab(const FileSystem& cd, const fs::path& dest, std::string* err,
                    const std::function<void(int, int)>& progress) {
    const auto fail = [err](std::string m) {
        if (err) *err = std::move(m);
        return false;
    };
    std::error_code ec;
    fs::create_directories(dest, ec);
    if (ec) return fail("Ordner " + dest.string() + ": " + ec.message());
    fs::remove(dest / kInstallDone, ec);

    // Datenquelle kann ein Image sein: Kabinett als echte Datei bereitstellen
    const auto data = cd.read("SETUP/DATA1.CAB");
    if (!data) return fail("SETUP/DATA1.CAB nicht gefunden (CD1 angeben)");
    // libunshield leitet aus dem Namen die Teilarchive ab (DATA1.cab, DATA1.hdr): Name beibehalten
    const fs::path tmpDir = dest / ".cab";
    fs::create_directories(tmpDir, ec);
    const fs::path tmp = tmpDir / "DATA1.CAB";
    {
        std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
        out.write(reinterpret_cast<const char*>(data->data()), static_cast<std::streamsize>(data->size()));
        if (!out) return fail("Schreiben von " + tmp.string() + " fehlgeschlagen");
    }
    const auto cleanup = [&] { fs::remove_all(tmpDir, ec); };

    unshield_set_log_level(0);  // UNSHIELD_LOG_LEVEL_LOWEST: keine Meldungen auf stderr
    std::unique_ptr<Unshield, UnshieldCloser> u(unshield_open(tmp.string().c_str()));
    if (!u) {
        cleanup();
        return fail("DATA1.CAB lässt sich nicht öffnen (libunshield)");
    }
    const int n = unshield_file_count(u.get());
    int saved = 0;
    for (int i = 0; i < n; ++i) {
        if (!unshield_file_is_valid(u.get(), i)) continue;
        const char* name = unshield_file_name(u.get(), i);
        const char* dir = unshield_directory_name(u.get(), unshield_file_directory(u.get(), i));
        if (!name) continue;
        const fs::path sub = installSubdir(dir ? dir : "");
        fs::create_directories(dest / sub, ec);
        const fs::path target = dest / sub / name;
        // unshield_file_save prüft die MD5-Summe des Kabinetts
        if (!unshield_file_save(u.get(), i, target.string().c_str())) {
            cleanup();
            return fail(std::string("Entpacken von ") + name + " fehlgeschlagen");
        }
        ++saved;
        if (progress) progress(saved, n);
    }
    u.reset();
    cleanup();
    if (!fs::exists(dest / "Rent-A-Hero.exe", ec) || !fs::is_directory(dest / "Common", ec))
        return fail("DATA1.CAB enthielt nicht Rent-A-Hero.exe und Common/");
    std::ofstream(dest / kInstallDone) << saved << '\n';
    return true;
}

}  // namespace rah
