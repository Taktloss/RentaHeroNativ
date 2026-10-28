// SPDX-License-Identifier: GPL-3.0-or-later
#include <map>
#include <vector>

#include "core/strings.hpp"
#include "engine/help.hpp"
#include "engine/scene.hpp"
#include "test.hpp"

using namespace rah;

namespace {

class MemFs final : public FileSystem {
public:
    std::map<std::string, Bytes> files;
    void put(const std::string& p, Bytes b) { files[toUpperAscii(p)] = std::move(b); }
    bool exists(std::string_view p) const override { return files.count(toUpperAscii(std::string(p))) != 0; }
    std::optional<Bytes> read(std::string_view p) const override {
        auto it = files.find(toUpperAscii(std::string(p)));
        if (it == files.end()) return std::nullopt;
        return it->second;
    }
    std::vector<std::string> list(std::string_view) const override { return {}; }
};

void put32(Bytes& b, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) b.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
}

// einfarbiges TCC w×h (w ≤ 255), optional mit Alpha
Bytes tcc(int w, int h, std::uint8_t r, int alpha) {
    auto plane = [&](std::uint8_t v) {
        Bytes p;
        for (int y = 0; y < h; ++y) {
            p.push_back(1);
            p.push_back(static_cast<std::uint8_t>(w));
            p.push_back(v);
        }
        return p;
    };
    Bytes f;
    put32(f, 0);
    const auto n = static_cast<std::uint32_t>(3 * h);
    put32(f, n);
    put32(f, n);
    put32(f, n);
    put32(f, alpha >= 0 ? n : 0);
    put32(f, static_cast<std::uint32_t>(w));
    put32(f, static_cast<std::uint32_t>(h));
    for (std::uint8_t v : {r, std::uint8_t{0}, std::uint8_t{0}}) {
        const Bytes p = plane(v);
        f.insert(f.end(), p.begin(), p.end());
    }
    if (alpha >= 0) {
        const Bytes p = plane(static_cast<std::uint8_t>(alpha));
        f.insert(f.end(), p.begin(), p.end());
    }
    return f;
}

struct Rig {
    MemFs common;
    DataPaths paths{nullptr, &common};
    EngineGlobals g;
    Surface565 surf{640, 420}, screen{640, 480};
    std::unique_ptr<Scene> scene;
    HelpOverlay help{g};
    std::vector<std::string> texts;
    std::vector<Rect> outputs;
    int hints = 0;

    Rig() {
        common.put("HelpIcon.tcc", tcc(60, 60, 0xF8, -1));
        common.put("Helpmask.tcc", tcc(10, 10, 0x80, 255));
        common.put("Helpmaskr.tcc", tcc(10, 10, 0x40, 255));
        std::string hlp = "Hallo\r\nWelt";
        for (char& c : hlp) c = static_cast<char>(static_cast<unsigned char>(c) ^ 0x80);
        common.put("01_046_A.hlp", Bytes(hlp.begin(), hlp.end()));
        g.screen = &screen;
        g.drawHelpText = [this](Surface565&, const Rect&, const std::string& t) { texts.push_back(t); };
        g.outputSceneRect = [this](const Rect& r) { outputs.push_back(r); };
        scene = std::make_unique<Scene>(paths, g, surf);
        help.setup(scene.get(), [this] {
            ++hints;
            help.show("01_046_A");
        }, true);
        const std::string icon = "HelpIcon.tcc";
        help.load(&icon);
        scene->setHelp(&help);
    }
};

}  // namespace

TEST(help_decode) {
    std::string a = "\xC8\xE1";  // 'H' ^ 0x80, 'a' ^ 0x80
    hlpDecode(a);
    CHECK(a == "Ha");
    std::string b = "Ha";
    hlpDecode(b);
    CHECK(b == "Ha");
}

TEST(help_icon_on_off_on_screen) {
    Rig r;
    REQUIRE(r.help.icon() != nullptr);
    CHECK(r.help.icon()->name == "HelpIcon");
    r.help.iconOn();
    CHECK(r.help.iconShown() && r.screen.row(30)[600] == pack565(0xF8, 0, 0));
    r.help.iconOff();
    CHECK(!r.help.iconShown() && r.screen.row(30)[600] == 0);
}

TEST(help_click_opens_and_next_click_closes) {
    Rig r;
    r.scene->onClick(580, 10);  // x < 581: kein Hinweis
    CHECK(r.hints == 0);
    r.scene->onClick(581, 10);  // auch ohne sichtbares Symbol
    CHECK(r.hints == 1 && r.help.isOpen());
    REQUIRE(!r.texts.empty());
    CHECK(r.texts.back() == "Hallo\r\nWelt");
    CHECK(r.help.textRect() == (Rect{20, 20, 300, 340}));  // ohne Figur links
    CHECK(r.surf.row(5)[5] == pack565(0x80, 0, 0));        // Maske in der Szenenoberfläche
    CHECK(!r.outputs.empty() && r.outputs.back() == (Rect{0, 0, 640, 420}));
    CHECK(r.scene->onMouseMove(300, 200) == 0);             // bei offener Hilfe nichts
    r.scene->onClick(300, 200);
    CHECK(!r.help.isOpen() && r.hints == 1);
}
