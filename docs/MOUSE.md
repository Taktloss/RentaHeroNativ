# Maus: Bewegung, Zeiger und Bereichstext

Belege per Adresse; ergänzt EVENTS.md, AREAS.md, TIMING.md (Timer), RENDERER.md (Schrift).
Port: `Scene::onMouseMove`, `Scene::compose` (Schritte 5/12), `render/font`, `formats/pe`.

## `WM_MOUSEMOVE` → `OnMouseMove` (`0x00402790`)
Fenstermodus: Koordinaten umrechnen (`0x00447E50`); Editor im Vollbild: `y + 19`. Editor-Rest
„Set Area“ (`0x0047D138`) speichert `(x, y − 60)`. Dann `CSzene_MouseMove(x, y)`
(`0x0044CBB0`).

## `CSzene_MouseMove(x, y)` `0x0044CBB0` (Bildschirmkoordinaten)
1. `+0x500/+0x504 = x, y`; `alt = Renderer_GetCursor()` (`0x0043EA60`); `neu = 0`.
2. Nur wenn Hilfe `+0x9F8` nicht offen und Vollbildobjekt `+0x5DC` nicht (aktiv und läuft):
   - `y < 60` oder `y > 479`: ist der Text nicht sichtbar (`0x00481F24 == 0`) →
     Umschalt-Ereignis deaktivieren, `pending = 0`.
   - sonst: `vorher = +0x2C8`; `+0x2C8 = HitTest(x, y − 60)`; Treffer → `neu = Zeiger des
     Bereichs (+0xA8)`.
     Inventarleiste (`+0x5B4`, ohne Editor): Treffer = deren Bereich `+0x428` → dessen
     Ereignis Typ **0x0D** markieren, `+0x2C8 = 0`; sonst `0x00441D40(leiste, 1)`.
     Hat sich der Bereich geändert **oder** ist der Text nicht sichtbar:
     - `neuZeichnen = +0x5CC && Text sichtbar && vorher ≠ 0 && vorher ≠ +0x2C8`;
     - Text unsichtbar (`0x00481F24 = 0`), Umschalt-Ereignis deaktivieren, `pending = 0`;
     - `+0x2C8 ≠ 0`: `neu = Zeiger des Bereichs`; bei `+0x5CC`: Textposition
       `+0x508 = x`, `+0x50C = y < 390 ? y + 25 : y − 20` (vorzeichenlos), Umschalt-Ereignis
       aktivieren und `CEvent_SetTimer(g_MouseDelay, einmalig)`. `+0x2C8 == 0`: `neu = 0`.
     - `neuZeichnen` → `CSzene_InvalidateAll`.
     Treffer = Inventarbereich → `+0x2C8 = 0`.
3. `neu ≠ alt` → `Renderer_SetCursor(neu)` (`0x0043E960`).

`g_MouseDelay` (`0x004764AC`) = **250** ms (Vorgabe; geändert von `0x0043650E/0x00436520`).
`+0x5CC` (Texte zeigen) = 1 im `CSzene_ctor` (`0x00448481`).

## Umschalt-Ereignis (`*0x00481F2C`) und `Scene_ToggleTimerEvent` `0x0044C8D0`
Im `CSzene_ctor` einmalig angelegt (nicht im Editor), aktiviert = 0, Funktions-Handler
`0x0044C8D0`; `0x00481F30` zeigt auf die Szene. Der Handler (Timer feuert Typ 0x15):
- Text unsichtbar → sichtbar (`0x00481F24 = 1`), Ereignis aktiv, Timer **2000 ms**
  einmalig, `g_SceneChanged = 1`.
- sichtbar → unsichtbar, Ereignis deaktivieren, `pending = 0`, `CSzene_InvalidateAll`,
  `+0x2C8 = 0`.
Ablauf also: Bereich überfahren → nach 250 ms Text → nach weiteren 2000 ms weg. Erst ein
Wechsel des Bereichs (oder Verlassen des Szenenstreifens) startet erneut.
`CSzene_OnClick` außerhalb des Szenenstreifens: Text unsichtbar → Ereignis deaktivieren.

## Zeichnen (`CSzene_Compose` `0x0044B7D0`)
- Schritt 5 (ohne Editor-Vollbild `+0x5C8`): `+0x5CC`, `+0x2C8` und Text sichtbar →
  `r = (x − 160, y − 90, x + 160, y − 50)` mit `x, y = +0x508, +0x50C`; `ClipToScene(r)`
  sichtbar → `r` in A und B (Hintergrund wiederherstellen, ausgeben).
- Schritt 12 (gleiche Bedingung): `GetDC` der Szenenoberfläche, `CArea_DrawText(bereich,
  hdc, x, y − 60)`, `ReleaseDC`.
- **Eigenheit:** Texte breiter als 320 px oder höher als das Rechteck werden außerhalb von
  `r` nicht wiederhergestellt (bis zum nächsten `InvalidateAll` beim Ausblenden).

## `CArea_DrawText(hdc, x, y)` `0x00436230`
- Text = Anzeigetext `+0x42`; **leer → interner Name `+0x10`** (z. B. `007a_Tuer`); beides
  leer → nichts.
- Ausrichtung nach Schätzbreite `w = len · 8 / 2` (vorzeichenlos): `x < w` →
  `TA_BASELINE|TA_LEFT`; `x + w ≤ 640` → `TA_BASELINE|TA_CENTER`; sonst
  `TA_BASELINE|TA_RIGHT`.
- Schrift (einmal angelegt, `0x0047E5F0`): `CreateFontA(15, 0, 0, 0, 700, kursiv, …,
  "Arial")`, Ersatz `SYSTEM_FONT`. Hintergrund transparent.
- Schwarz an `(x−1, y−1)`, `(x−1, y+1)`, `(x+1, y−1)`, `(x+1, y+1)`, dann weiß an `(x, y)`.
- Port: Liberation Sans Bold Italic (bzw. Arial des Systems) über FreeType, einfarbig
  gerastert, Größe so, dass Ascent + Descent (OS/2 `usWin*`) = 15 px (GDI-Zellenhöhe).
  Bewusste Abweichung (Entscheidung 2026-10-27): GDI-Hinting ist nicht nachbildbar.

## Zeiger `Renderer_SetCursor(n)` `0x0043E960`
Die Zeiger sind 32×32-Schwarzweiß-Cursor (`RT_GROUP_CURSOR`) der **installierten**
`Rent-A-Hero.exe` (die `RENT-A-HERO.EXE` im CD-Wurzelordner ist ein anderes Programm
ohne diese Ressourcen), geladen mit `LoadCursorA` (`0x0043E870`):

| Typ | Ressource | Feld | | Typ | Ressource | Feld |
|---:|---|---|---|---:|---|---|
| 0 | 0x95 | `+0x610` | | 6 | 0x9B | `+0x62C` |
| 1 | 0x97 | `+0x61C` | | 7 | 0x9C | `+0x630` |
| 2 | 0x98 | `+0x620` | | 8 | 0x9D | `+0x634` |
| 3 | 0x99 | `+0x624` | | 9 | 0x9E | `+0x638` |
| 4 | 0x9A | `+0x628` | | 10 | 0x9F | `+0x614` |
| | | | | 11 | 0x96 | `+0x618` |

Typ 5 und unbekannte Typen: **keine Änderung**. `Renderer_GetCursor` (`0x0043EA60`) liefert
den Typ des aktuellen Systemzeigers, unbekannt → 99. Port: Quelle der EXE ist
`<Common>/../Rent-A-Hero.exe` oder `--exe`; fehlt sie, bleibt der Systemzeiger.
Kenndaten ausgeben: `python3 -I tools/pe_cursors.py <exe>`.
