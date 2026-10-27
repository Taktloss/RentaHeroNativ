// SPDX-License-Identifier: GPL-3.0-or-later
// Kleines Testgerüst ohne externe Abhängigkeiten.
#pragma once

#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace rahtest {

struct Case {
    const char* name;
    std::function<void()> fn;
};
std::vector<Case>& registry();
extern int failures;

struct Register {
    Register(const char* name, std::function<void()> fn) { registry().push_back({name, std::move(fn)}); }
};

// Ordner mit den Originaldaten (CD-Images, zusammengeführter CD-Ordner); leer = Datentests überspringen.
inline std::filesystem::path dataDir() {
    const char* d = std::getenv("RAH_TEST_DATA");
    return d ? std::filesystem::path(d) : std::filesystem::path();
}

struct Skip {};

}  // namespace rahtest

#define RAH_CAT2(a, b) a##b
#define RAH_CAT(a, b) RAH_CAT2(a, b)
#define TEST(name)                                                              \
    static void RAH_CAT(test_, name)();                                         \
    static rahtest::Register RAH_CAT(reg_, name)(#name, RAH_CAT(test_, name));  \
    static void RAH_CAT(test_, name)()
#define CHECK(cond)                                                                       \
    do {                                                                                  \
        if (!(cond)) {                                                                    \
            ++rahtest::failures;                                                          \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK(" #cond ") fehlgeschlagen\n"; \
        }                                                                                 \
    } while (0)
#define REQUIRE(cond)                                                                       \
    do {                                                                                    \
        if (!(cond)) {                                                                      \
            ++rahtest::failures;                                                            \
            std::cerr << __FILE__ << ":" << __LINE__ << ": REQUIRE(" #cond ") fehlgeschlagen\n"; \
            return;                                                                         \
        }                                                                                   \
    } while (0)
#define SKIP_IF(cond) \
    do {              \
        if (cond) throw rahtest::Skip{}; \
    } while (0)
