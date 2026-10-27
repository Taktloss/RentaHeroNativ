# HLP – Hilfetexte

Belege: `CHelp_Show` `0x004414C0`, `Hlp_Decode` `0x004416F0`, Layout `0x00441040` /
`0x00441080`. Aufruf aus `0x00440C40`. 17 Dateien in `GAME/COMMONCD` (`NN_NNN_X.HLP`);
die `DIRECTX/*.HLP` sind echte Windows-Hilfedateien des DirectX-Setups und irrelevant.

## Format
Reiner Text (ANSI/CP1252), ohne Kopf. **Ist das erste Byte ≥ 0x80**, wird jedes Byte
mit `0x80` XOR-verknüpft; sonst bleibt der Text unverändert. Alle 17 Dateien sind
verschlüsselt und ergeben danach reinen Text. Puffer = Dateigröße + 100 Nullbytes.

## Anzeige
- Pfad über `FUN_00448b30(szene, name, "hlp")` (Endung klein).
- Figur fehlt oder steht rechts (`CActor+0x15D0 > 319`): Text **links**, Rechteck
  (20, 20)–(300, 340), Bild `+0x0C` an (0, 0).
- sonst Text **rechts**, Rechteck (340, 20)–(620, 340), Bild `+0x10` an (320, 0).
- `DrawTextA(…, DT_CENTER | DT_VCENTER | DT_WORDBREAK)` (0x15), weiße Schrift,
  transparenter Hintergrund, Schrift `+0x100`. Direkt auf die Oberfläche gezeichnet
  (GetDC an Slot `+0x44`), dann Bildschirm aktualisiert.
- Port: Zeilenumbruch und Schrift müssen GDI-`DrawText` nachbilden (Schrift wird beim
  Renderer analysiert – offen).
