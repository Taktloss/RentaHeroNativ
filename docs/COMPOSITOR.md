# Szenen-Compositor: Dirty-Rechtecke, Zeichenreihenfolge, Ausgabe

Belege: `CSzene_Compose` `0x0044B7D0` (bisher `FUN_0044b7d0`), Rechteckliste
`0x00445C50`–`0x00445EE0`, `CLayer_GetRect` `0x00447B80`, `CLayer_Draw` `0x004471C0`,
`ClipToScene` `0x004481E0`, `CImageTcc_Draw` `0x0043B900`, Ausgabe
`0x0043E210`/`0x0043E330`, `CSzene_SetDevice` `0x0044A680`, `Game_Idle` `0x004030C0`.

## Oberflächen und Koordinaten
`CSzene_SetDevice(hwnd, ddraw, surface, colorKey, yOffset, ?)` (`0x0044A680`) setzt
`+0x2CC`, `+0x2D0`, `+0x2D4` (Zieloberfläche), `+0x2D8` (Farbschlüssel), `+0x2F0`
(Y-Versatz der Ebenen), `+0x5C4`. Aufruf in `WinMain` (`0x004020FC`) mit
`surface = Renderer+0x18`, `colorKey = Renderer+0x04`,
`yOffset = Renderer+0x08 ? 60 : 0`.
- `Renderer+0x08` wird bei beiden Modus-Einrichtungen auf 0 gesetzt (`0x0043DE4B`,
  `0x0043E098`) → **Y-Versatz 0**. Die Zieloberfläche arbeitet in Szenenkoordinaten.
  Damit gilt für den Tiefentest `sceneTop = 0` (RENDERER.md).
- `Renderer+0x04 = DDColorMatch(surface, RGB(0,0,0))` (`0x0043D4C0`, Verfahren aus dem
  DirectX-SDK: Pixel per GDI setzen, gesperrt zurücklesen) → **Farbschlüssel = Wert von
  Schwarz = `0x0000` in RGB565**. Er wird an `CLayer_Load` weitergereicht
  (`CSzene_CreateLayer` → `Load(hwnd, ddraw, name, +0x2D8)`) und dort als `+0x1E4`
  gespeichert (`0x00446A92`); `0x00446C90` setzt ihn per `SetColorKey(DDCKEY_SRCBLT)`
  (unterer = oberer Wert).
- Die Oberfläche umfasst Szene (y 0…359) und unteren Streifen (y 360…419, Inventar).
  Ausgabe (`Game_Idle`): bei `Renderer+0x08 == 0` wird das Ausgaberechteck (Liste B,
  unten) über Renderer-Slot 0 gezeigt:
  - Vollbild `0x0043E210`: `Blt` nach `(l, t+60, r, b+60)`; **Sonderfall**: Rechteck mit
    `right == 640` und `bottom == 180` → Ziel unten = 420 (gestreckt).
  - Fenster `0x0043E330`: Ziel = Client-Bereich ohne oberes/unteres Achtel
    (bei 640×480: y 60…420) plus Rechteck; ist der Client nicht 640×480, wird
    `(0, 0, 640, 420)` gestreckt ausgegeben.
  - UNVERIFIED: Größe der Oberfläche `Renderer+0x18` (mindestens 640×420, siehe
    Quellrechteck in `0x0043E330`).

## Rechteckliste (`0x16C` Bytes)
| Off | Inhalt |
|---|---|
| `+0x000` | `RECT` Vereinigung (Modus 0) |
| `+0x010` | `RECT[20]` Einträge 1…20 bei `k·0x10` (Modus 1) |
| `+0x160` | Anzahl, `+0x164` Lesezeiger, `+0x168` Modus |

- Ctor `0x00445C50`: Anzahl, Lesezeiger, **Modus = 0**. Der Modus wird für die
  Szenenlisten nirgends geändert (alle Schreibzugriffe auf `+0x168` gehören anderen
  Klassen) → **beide Listen sind immer ein einziges Vereinigungsrechteck**; der
  Listenmodus ist toter Code.
- `Add(r)` `0x00445D20`: `r == NULL` → `(0, 0, 640, 360)`. Begrenzen **im Rechteck des
  Aufrufers**: `left, top ≥ 0`, `right ≤ 640`, `bottom ≤ 480` (nicht 360). Modus 0:
  `UnionRect` (Win32: leere Rechtecke zählen nicht). (Modus 1: gleiches Rechteck →
  nichts; mit Eintrag verschmelzen, wenn `⌊Fläche(∪)/10⌋·8 < Fläche(e)+Fläche(r)`;
  sonst anhängen; ab 20 Einträgen verworfen.)
- `First` `0x00445EA0`: Modus 0 → Vereinigung, wenn nicht leer, sonst NULL.
  `Next` `0x00445EE0`: Modus 0 → NULL. `Intersects(r)` `0x00445C90`: Schnitt nicht leer.
  `Clear` `0x00445C70`: alles 0.

Szene: Liste **A** `+0x2F4` = Hintergrund wiederherstellen, Liste **B** `+0xAC4` =
auszugeben.

## Rechteck einer Ebene (`CLayer_GetRect` `0x00447B80`)
Ebenenfelder (double): X `+0x10`, Y `+0x18`, Z `+0x20`, Zoom `+0x28` (100.0 = keiner).
Grundrechteck:
- `DirtyAlpha` (`+0x19C`) und Bild (`+0x0C`) und Alpha (`+0x198`) und Zoom = 100 →
  Alpha-Rechteck des Bilds `image+0x35C`;
- sonst Zoom = 100 → `+0x1F0` = `(0, 0, w, h)`;
- sonst `+0x200` (skaliertes Rechteck).
Dann `left/right += ftol(X)`, `top/bottom += ftol(Y)` (Abschneiden Richtung 0).

`ClipToScene(r)` `0x004481E0`: Ergebnis 0 („unsichtbar“), wenn `right < 1`, `bottom < 1`,
`top > 359` oder `left > 639`; schneidet `r` in jedem Fall mit `(0, 0, 640, 360)`.

## Ebene zeichnen (`CLayer_Draw(surface, flag, srcRect, zmap)` `0x004471C0`)
1. Kein Bild geladen (`+0x1D8 == 0`) → 0. Nicht aktiv (`+0x1A4 == 0`) → dirty löschen, 1.
2. `srcRect.right < srcRect.left` → 0.
3. `r = GetRect()`; **altes Rechteck `+0x210 = r`**; bei `DirtyAlpha` `+0x210.right++`,
   `+0x210.bottom++`.
4. Außer für den unteren Streifen (Y = 360.0 und Höhe `+0x1EC` = 60):
   `ClipToScene(+0x210)` (in place); unsichtbar → dirty löschen, 1.
5. `zmap` ohne Daten (`zmap+0x138 == 0`) → keine Tiefe.
6. AVI-Ebenen (Typ 3) schalten Alpha `+0x198` je nach Zoom/Flag/`+0x30` um
   (`0x00446E20`) – Details bei den Videoebenen.
7. Ohne Alpha (`+0x198 == 0`, z. B. BMP): DirectDraw-Oberfläche der Ebene (`+0x08`,
   bei Bedarf `0x00446C90`):
   - Zoom = 100: `BltFast(r.left + src.left, r.top + src.top, src)` mit
     `DDBLTFAST_WAIT | (flag ? SRCCOLORKEY : 0)`.
   - sonst `Blt(r, src)` gestreckt, `DDBLT_WAIT | (flag ? KEYSRC : 0)`.
   Schlägt der Blit fehl → Rückgabe 0, **dirty bleibt gesetzt**. UNVERIFIED: `BltFast`
   ohne Clipper lehnt Ziele außerhalb der Oberfläche ab (DDERR_INVALIDRECT) – Ebene
   wird dann nicht gezeichnet; Streckverfahren von `Blt`.
8. Mit Alpha (TCC/A16): `image->Draw(surface, &r, flag, ZBuffered ? zmap : 0, ftol(Z),
   ftol(Zoom))` (`0x004472DA`–`0x0044730C`).
9. dirty (`+0x1A0`) = 0, Rückgabe 1.

`CImageTcc_Draw` (`0x0043B900`, Argumente wie oben): Rechteck clippen, merken;
außer für den unteren Streifen (`top == 360 && bottom == 420`) `ClipToScene`, unsichtbar
→ 1. **`flag == 0` → Rechteck vorher schwarz füllen.** Blitter (`0x0043B9ED`–`0x0043BA1C`):
**deckend**, wenn das Bild kein Alpha hat (`+0x334 == 0`) **oder `flag == 0`** oder
`Z ≥ 255` (`fild` des vorzeichenlosen Z gegen 255.0); sonst Alpha-Varianten nach
RENDERER.md.

Hilfsaufrufe: `0x00447150(layer, s, flag)` = `Draw(s, flag, +0x1F0, NULL)`;
`0x00447170(layer, s, flag, zmap)` = `Draw(s, flag, +0x1F0, zmap)`;
`0x004471A0(layer, s, flag, rect)` = `Draw(s, flag, rect, NULL)`.

## Ablauf `CSzene_Compose(force)` (`0x0044B7D0`, pro Frame aus `Game_Idle`)
Rückgabe 1 = Liste B ausgeben. Felder: Hintergrund `+0x5C0`, Figuren `+0x5E0/+0x5E4`,
Ebenen `+0x04[40]` (Ende beim ersten leeren Platz), Vollbildobjekt `+0x5DC`,
Z-Max `+0x2F8`, Z-Min `+0x2FC`, Z-Zähler `short +0x300[z]`, Tiefenkarte `+0x2BC`.

1. Nicht geladen (`+0x00`), keine Oberfläche, oder `g_SceneChanged` (`0x00481F28`) = 0
   und `force == 0` → 0.
2. Vollbildobjekt `+0x5DC` (Video/Menü): Sonderfälle, bei aktivem Objekt direkt zu 12.
3. Kein Hintergrund → 0.
4. Figuren klassifizieren (nur wenn dirty oder aktiv): `zc = ftol(Figur-Z)`;
   Klasse 1 = `zc > Zmax`, 2 = im Bereich, 3 = `zc < Zmin` (sonst 0 = nicht zeichnen).
5. Editor-Vollbild `+0x5C8`: `Add(NULL)` in A und B, alle aktiven Ebenen/Figuren dirty
   (weiter bei 9). Sonst Bereichsnamen-Text unter der Maus (`+0x5CC`, Bereich `+0x2C8`,
   `0x00481F24`): Rechteck `(mx−160, my−90, mx+160, my−50)`, falls sichtbar, in A und B.
6. Rechteck aus A clippen und nach B.
7. Ebenen 0…39: dirty → `GetRect` → `ClipToScene` → B; altes Rechteck `+0x210` nicht
   leer → bei **Z ≠ 255 nach A** und nach B.
8. Figuren (zweimal, damit eine von Figur 1 ausgelöste Figur 0 noch erfasst wird):
   aktiv, nicht dirty, schneidet B → dirty. Dirty → Rechteck nach B, altes Rechteck nach
   A und B.
   Ausbreitung: jede aktive, nicht dirty Ebene, die B schneidet → dirty, Rechteck nach
   B, altes Rechteck (Z ≠ 255) nach A und B; danach Figuren erneut prüfen und
   **Ebenensuche von vorn** beginnen.
9. Hintergrund für das Rechteck aus A zeichnen: `0x004471A0(bg, s, 0, A)` (flag 0 =
   ohne Farbschlüssel), A nach B; A leeren.
10. Nicht dirty gewordene Figuren werden nicht gezeichnet.
    Ebenen mit **Z = 255.0** und dirty: `0x00447150(layer, s, 0)` – flag 0, also ohne
    Farbschlüssel bzw. deckend und vorher schwarz, ohne Tiefe.
11. Zeichenreihenfolge: Zähler der Figuren-Z vorübergehend erhöhen; Figurenreihenfolge
    `z0 ≤ z1` → erst Figur 1, dann 0, sonst umgekehrt. Für `z = Zmax+1` abwärts bis
    `Zmin` (Abbruch bei `z == 0`): erst alle dirty Ebenen mit `ftol(Z) == z` und
    `Z ≠ 255.0` → `0x00447170(layer, s, 1, zmap)`; dann die Figuren in ihrer Reihenfolge,
    wenn Klasse 1, oder Klasse 2 mit `zc == z`, oder Klasse 3 mit `z == Zmin`
    (`CActor_Draw(s, 1, zmap)` `0x00435AE0`, danach nicht mehr). Danach Zähler
    zurück und noch nicht gezeichnete Figuren zeichnen.
    → **Größeres Z wird zuerst gezeichnet (hinten)**; Figuren liegen vor Ebenen gleichen Zs.
12. Bereichsname (`CArea_DrawText` bei `(mx, my−60)`) bzw. Editor-Bereichsrahmen;
    letzte Ebene `+0x5D8` Slot `+0x10(this+0x9FC)`; `g_SceneChanged = 0`; Rückgabe 1
    (0, wenn Vollbildobjekt aktiv).

`g_SceneChanged` setzen u. a. Szenenlader, `CSzene_CreateLayer`, `0x0044C1E0`
(`Add(NULL)` in A und B = alles neu) und `CSzene_Update`: nach jedem Update eines
Objekts mit gesetztem dirty-Flag; ein Hintergrund-Update fügt außerdem `NULL` (ganze
Szene) in A ein (`0x0044B3E6`).

## Eigenheiten, die der Port übernimmt
- Ein einziges Vereinigungsrechteck je Liste (kein Mehrfachrechteck).
- Ebenen mit Z = 255 lösen keine Hintergrund-Wiederherstellung aus und werden deckend
  bzw. ohne Farbschlüssel gezeichnet.
- `Add` begrenzt unten auf 480, `ClipToScene` auf 360.
- Farbschlüssel Schwarz: BMP-Pixel, die in RGB565 0 ergeben (R < 8, G < 4, B < 8),
  sind in Ebenen mit flag 1 durchsichtig. UNVERIFIED: Umwandlung 24 → 16 Bit beim
  Laden über GDI (Abschneiden wie `pack565` angenommen).

## Aktivierung
Neue Ebenen werden **nicht** aktiviert: `CSzene_CreateLayer` ruft `CLayer_Enable` nur, wenn
der Dateiname NULL ist (`0x0044AC48`, `ebx` = Dateiname), was nie vorkommt. Auch der
Hintergrund (`0x00449E70`) bleibt inaktiv. Die Schauplätze aktivieren Ebenen über
`CSzene_GetObject` und den Hintergrund über `CGame+0x60` (von `CGame_GetViewObjects`
`0x0043F6C0` aus `CSzene+0x5C0` gesetzt; 22 Schauplätze). `CSzene_EnableAll` (`0x0044B020`)
wird nur aus der Editor-Tastatur (`0x0044CF00`) aufgerufen.
