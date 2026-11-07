# Automatischer Bau (GitHub Actions)

`.github/workflows/build.yml` läuft bei jedem Push auf `main`, bei Pull Requests, bei Tags `v*` und
von Hand („Run workflow“). **Originaldaten gibt es in der CI nicht.** Tests, die sie brauchen,
werden übersprungen (`RAH_TEST_DATA` fehlt).

| Job | Läuft auf | Inhalt |
|---|---|---|
| `appimage` | ubuntu-24.04 | `tools/appimage/build.sh` im Docker-Container (Ubuntu 22.04), mit Tests; Artefakte `rentahero-linux-appimage` und `rentahero-linux-tar` |
| `windows` | ubuntu-24.04 | llvm-mingw 20261006 (SHA-256 geprüft), `tools/windows/build.sh`; Artefakt `rentahero-win64` (ZIP) |
| `windows-test` | windows-2025 | `rah_tests.exe` mit den DLLs auf echtem Windows; dazu ein Startmenü-Bild (`RAH_LAUNCHER_SHOT`), das scheitern darf, weil der Runner keine GPU hat |
| `release` | ubuntu-24.04 | nur bei Tags `v*`: **Release** mit AppImage, Linux-tar.gz und Windows-ZIP; Tags mit `-` (z. B. `v1.0-rc1`) als Vorabversion, sonst „Latest“ |

- Die Quellen und gebauten Abhängigkeiten (`build-*/src`, `build-*/deps`) liegen im Actions-Cache.
  Der Schlüssel ist der Hash von `tools/deps/sources.sh`, der Bauskripte und des Containerfile bzw. der
  Toolchain-Datei. Ändert sich eines davon, wird alles neu gebaut.
- Die Actions sind auf Commit-SHAs festgelegt, die Version steht als Kommentar dahinter
  (checkout v7.0.1, cache v6.1.0, upload-artifact v7.0.2, download-artifact v8.0.2).
- Release: `git tag v0.1.0 && git push origin v0.1.0`. Das Release erscheint nach dem
  Bau unter „Releases“ (im privaten Repository nur für Berechtigte sichtbar).
- Das Repository ist privat, deshalb zählen die Minuten gegen das Kontingent des Kontos.
  Windows-Runner kosten doppelt.
