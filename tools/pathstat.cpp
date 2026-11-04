// SPDX-License-Identifier: GPL-3.0-or-later
// Aufruf: rah_pathstat <cd-ordner> <suchen je karte> (docs/PATHFINDING.md „Messung“)
// Misst Wegsucher-Knoten und Wegpunkte über alle *_PATH.ZBF mit Zufallspaaren.
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <map>
#include "core/bytes.hpp"
#include "engine/pathfinder.hpp"
#include "formats/zbf.hpp"
using namespace rah;
int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Aufruf: rah_pathstat <cd-ordner> <suchen je karte>\n";
        return 2;
    }
    int maxNodes = 0, maxWp = 0, maps = 0, runs = 0, found = 0;
    std::string worstN, worstW;
    std::map<std::string, std::pair<int,int>> over;  // Karte -> (>=20 Wegpunkte, Suchen)
    std::mt19937 rng(1999);
    for (const auto& e : std::filesystem::recursive_directory_iterator(argv[1])) {
        std::string n = e.path().filename().string();
        for (auto& c : n) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (n.size() < 9 || n.substr(n.size() - 9) != "_PATH.ZBF") continue;
        ZMap m;
        std::string err;
        if (!loadZMap(*readFile(e.path()), m, &err)) continue;
        ++maps;
        PathGrid grid;
        PathFinder p(grid, m);
        std::vector<std::pair<int, int>> freeCells;
        for (int x = 4; x < 636; x += 4)
            for (int y = 4; y < 356; y += 4)
                if (pathCellFree(grid, x, y)) freeCells.push_back({x, y});
        if (freeCells.size() < 2) continue;
        std::uniform_int_distribution<std::size_t> d(0, freeCells.size() - 1);
        for (int i = 0; i < std::stoi(argv[2]); ++i) {
            auto a = freeCells[d(rng)], b = freeCells[d(rng)];
            ++runs;
            if (!p.find(a.first, a.second, b.first, b.second)) continue;
            ++found;
            if (p.nodesUsed() > maxNodes) { maxNodes = p.nodesUsed(); worstN = n; }
            int x, y, w = 0;
            while (p.next(x, y)) ++w;
            if (w > maxWp) { maxWp = w; worstW = n; }
            over[n].second++;
            if (w >= 20) over[n].first++;
        }
    }
    for (auto& [k, v] : over) if (v.first) std::cout << k << ": " << v.first << "/" << v.second << "\n";
    std::cout << maps << " Karten, " << runs << " Suchen (" << found << " gefunden); max Knoten "
              << maxNodes << " (" << worstN << "), max Wegpunkte " << maxWp << " (" << worstW << ")\n";
}
