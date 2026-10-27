// SPDX-License-Identifier: GPL-3.0-or-later
#include "test.hpp"

namespace rahtest {

std::vector<Case>& registry() {
    static std::vector<Case> r;
    return r;
}
int failures = 0;

}  // namespace rahtest

int main() {
    int skipped = 0;
    for (const auto& c : rahtest::registry()) {
        const int before = rahtest::failures;
        try {
            c.fn();
        } catch (const rahtest::Skip&) {
            ++skipped;
            std::cout << "SKIP " << c.name << " (RAH_TEST_DATA nicht gesetzt)\n";
            continue;
        }
        std::cout << (rahtest::failures == before ? "ok   " : "FAIL ") << c.name << "\n";
    }
    std::cout << rahtest::registry().size() << " Tests, " << rahtest::failures << " Fehler, " << skipped
              << " übersprungen\n";
    return rahtest::failures == 0 ? 0 : 1;
}
