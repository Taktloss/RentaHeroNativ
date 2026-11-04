// SPDX-License-Identifier: GPL-3.0-or-later
// Plattformanpassungen des Programmstarts (Port, kein Original-Gegenstück). docs/WINDOWS.md
#pragma once

namespace rah {

// Windows: an die Konsole des Aufrufers anhängen (GUI-Programm aus cmd/PowerShell gestartet);
// ohne Konsole stdout/stderr in eine Logdatei im Benutzer-Datenordner. Sonst nichts.
void platformInit();
// Windows ohne Konsole: bei rc ≠ 0 den Logtext in einem Meldungsfenster zeigen.
void platformExit(int rc);

}  // namespace rah
