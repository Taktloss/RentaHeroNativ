# Arbeitsumgebung

Host: Nobara Linux (Fedora 44-basiert), Kernel 7.2, x86_64.

## Originaldaten bereitstellen

1. CD-Images (`Rent a Hero CD1.bin/.cue`, `CD2.bin/.cue`, MODE2/2352) in ISO wandeln:
   ```sh
   bchunk "Rent a Hero CD1.bin" "Rent a Hero CD1.cue" cd1   # -> cd101.iso
   bchunk "Rent a Hero CD2.bin" "Rent a Hero CD2.cue" cd2   # -> cd201.iso
   ```
2. Beide ISOs mit `7z x` entpacken und in `RentAHero/` zusammenführen. Die 199 Pfade,
   die auf beiden CDs vorkommen (v. a. `GAME/COMMONCD`), sind byteidentisch
   (per `cmp` geprüft). Ergebnis: 2792 Dateien, 942 MB. Prüfsummen: `docs/manifests/`.

## Original unter Wine (Referenz)

```sh
export WINEPREFIX=$PWD/wineprefix
wineboot -i                       # WoW64-Prefix (reines win32 wird von Wine 11 WoW64 nicht unterstützt)
ln -sfn $PWD/RentAHero $WINEPREFIX/dosdevices/d:
wine reg add 'HKLM\Software\Wine\Drives' /v 'd:' /t REG_SZ /d cdrom /f
wine 'D:\SETUP\SETUP.EXE'         # Standardpfad, DirectX-6-Setup ablehnen
cd "$WINEPREFIX/drive_c/Program Files (x86)/neoSoftware/Rent-A-Hero" && wine Rent-A-Hero.exe
```
Getestet mit wine-11.17 (Staging): Spiel startet und läuft (Fenster 640×480).
Hochskalieren optional mit `gamescope -w 640 -h 480 -H 1440 -S fit -F nearest -f -- wine …`.

## Analysewerkzeuge

| Werkzeug | Version | Quelle / Ort |
|---|---|---|
| Ghidra | 12.2 PUBLIC | bereits vorhanden: `~/.local/opt/ghidra_12.2_PUBLIC` |
| JDK | Temurin 25.0.5+8 | bereits vorhanden: `~/.local/opt/jdk-25.0.5+8` |
| Python | 3.14 + pefile 2024.8.26 | System |
| bchunk, 7z, xorriso, objdump, strings | System | dnf |
| Wine | 11.17 Staging | System |
| gamescope | System | dnf |

Noch nicht installiert (benötigt root):
```sh
sudo dnf install -y rizin        # optional, ergänzend zu Ghidra
```
Frida (für Phase 7) später per `pip install --user frida-tools`.

## Ghidra-Projekt anlegen (reproduzierbar)

```sh
mkdir -p re/private/{ghidra,bin,out}
cp "wineprefix/drive_c/Program Files (x86)/neoSoftware/Rent-A-Hero/Rent-A-Hero.exe" re/private/bin/
JAVA_HOME=~/.local/opt/jdk-25.0.5+8 ~/.local/opt/ghidra_12.2_PUBLIC/support/analyzeHeadless \
  re/private/ghidra RahGame -import re/private/bin/Rent-A-Hero.exe -overwrite \
  -scriptPath re/scripts -postScript ExportSummary.java "$PWD/re/private/out"
```
Dauer ca. 25 s. Ergebnis: `re/private/out/functions.tsv`, `vtables.tsv`.
Die versionierte Fassung ohne Spiel-Strings liegt in `docs/symbols/functions.tsv`.
