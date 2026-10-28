# Inventarleiste (`CSzene+0x5B4`)

Objekt 0x42C Bytes, Ctor `0x00441750` (aus `CSzene_ctor`), Dtor-Teil `0x00441880`. Globaler
Zeiger `0x0047E640` (vom Ctor gesetzt; der Klick-Handler arbeitet nur darüber). Belegt an
`0x00441750`–`0x004422C0`, `CSzene_Update` `0x0044B280`, `Game_Idle` `0x004030C0`,
`CSzene_HitTest` `0x0044C3C0`, `CSzene_OnClick` `0x0044C950`, `CSzene_MouseMove`
`0x0044CBB0`, `CSzene_DispatchEvents` `0x0044C690`, Renderer `0x0043E600`/`0x0043E7C0`/
`0x0043E830`.

## Felder
| Off | Bedeutung | Ctor |
|---|---|---|
| `+0x000` | 50 Einträge à 0x14: `+0` Gegenstands-ID, `+4` belegt, `+8` `CAnimLayer*`, `+0xC` `CArea*` (nur freigegeben, nie angelegt), `+0x10` Ereignis-ID des Gegenstands | 0 |
| `+0x3E8` | zuletzt angeklickte ID | 0 |
| `+0x3EC` | Anzahl | 0 |
| `+0x3F0` | erster sichtbarer Eintrag (Blättern) | 0 |
| `+0x3F4` | Szene (`CSzene*`) | – |
| `+0x3F8` | Ausgaberechteck der belegten Plätze | `0x00441EA0` |
| `+0x408` | neu zeichnen | 0 |
| `+0x40C` | hell (Maus war in der Leiste) | 0 |
| `+0x410` | ganze Leiste ausgeben (nach Leeren/Entfernen) | 0 |
| `+0x414` | Eintrag unter der Maus (Schleifenanimation), −1 = keiner | −1 |
| `+0x418` | Gegenstands-Ereignisse aktiv | 0 |
| `+0x41C`/`+0x420` | Benutzungs-Handler / `CGame*` für `CGame_DescribeItem` (`0x00441E60`) | 0 |
| `+0x424` | Ereignis „Gegenstand benutzt“ (`CEvent*`) | 0 |
| `+0x428` | `CArea` „Inventory“, Rechteck (0, 360, 640, 420), aktiv, Funktions-Handler `0x00441FC0` | neu |

`CSzene+0x5BC` = Leiste aktiv: `Game_SwitchLocation` setzt es auf `g_CurGame` (≠ 0 im Spiel),
`CGame_dtor` auf 0.

## Ausgaberechteck `0x00441EA0`
Anzahl 0 → (0, 0, 0, 0); sonst (0, 360, min(Anzahl·64, 640), 420) (`Anzahl < 10` → `·64`,
sonst 640).

## Gegenstände
- `Inventory_Has(id)` `0x00441BF0`: Einträge `0 … Anzahl−1` mit ID und belegt.
- `Inventory_Add(id)` `0x00441960`: schon vorhanden → 1. Sonst erster Platz ohne Ebene (`+8 == 0`, 0…49):
  `CSzene_FindFile("Item%03d", "A16")` (Common: `Item001.a16` …, alle 60×60 mit Alpha);
  fehlt die Datei → 0. Neue `CAnimLayer`, Laden, Slot 3, `0x00446C90`, Name = Pfad,
  Zoom 100, fps **10**, `CLayer_Enable`; Eintrag (ID, belegt, Ebene, Ereignis 0), neu
  zeichnen, Anzahl + 1, Rechteck, `CAnimLayer_Start(12)` (einmal, zurück auf Bild 0) → 1.
  Laden scheitert → Ebene löschen, 0. Kein freier Platz → 0.
  **Eigenheit:** gesucht wird ein Platz ohne Ebene, nicht „Index = Anzahl“.
- `Inventory_Remove(id)` `0x00441B00`: erster Eintrag mit der ID (bis Anzahl): Ebene und
  Bereich löschen, Einträge `i+1 … 49` je um eins nach vorn kopieren (Schleife bis 0x31),
  letzten Platz nullen; neu zeichnen, Anzahl − 1, `0x00441F60`, Rechteck;
  `+0x414 == i` → −1 (**Eigenheit:** liegt `+0x414` dahinter, zeigt es danach auf den
  Nachfolger). Nicht gefunden → 0.
- `Inventory_Clear` `0x004418D0`: alle Einträge bis Anzahl leeren (ID, belegt, Ereignis 0;
  Ebene und Bereich löschen), Anzahl 0, neu zeichnen, `0x00441F60`, Rechteck.
  `+0x3F0` und `+0x414` bleiben unverändert.
- `0x00442240(id, ev)`: Platz mit der ID suchen (`0x004422C0`, alle 50 Plätze, auch
  unbelegte) → Ereignis-ID `+0x10 = ev`, `+0x418 = 1`, dann `0x00441DB0(+0x41C)`.
- `0x00441DB0(h)`: Ereignis `+0x424` anlegen, Handler `h`; `h ≠ 0`: aktivieren und bei
  `h ≠ +0x41C` → `+0x418 = 0`.
- `0x00441E60(h, game)`: `+0x41C = h`, `+0x420 = game`, `+0x418 = (h ≠ 0)`.

## Leiste leeren `0x00441F60`
`Blt` mit `DDBLT_COLORFILL` (Farbe 0) des Bereichsrechtecks `+0x428` (0, 360, 640, 420) auf
die Szenenoberfläche, `+0x408 = 1`, `+0x410 = 1`.

## Takt `0x00441CE0`
Aus `CSzene_Update` am Ende, wenn `+0x408` oder Stufe-1-Tick. Einträge `0 … Anzahl−1`,
Abbruch bei unbelegtem Eintrag oder fehlender Ebene: Update-Slot der Ebene
(`CAnimLayer_Update`); danach Ebene dirty → `+0x408 = 1`.

## Zeichnen `0x00441C30` (Game_Idle, nicht bei offenem Spielmenü)
`+0x408 == 0` → 0. Sonst ab `i = +0x3F0`, `x = 0` solange `i < Anzahl`, `x ≤ 639`, Eintrag
belegt und Ebene vorhanden: `CLayer_SetColor(+0x40C ? (0, 0, 0) : (−128, −128, −128))`
(**jedes Mal** neu dekodiert; Tönung über die A16-Deltabilder kumuliert, tcc.md),
`SetPos(x, 360)`, `Draw(Szenenoberfläche, 0, ganzes Bild)` (flag 0: Ziel vorher schwarz
füllen, kein Tiefentest); `x += 64`. Dann `+0x408 = 0` → 1.

`Game_Idle` gibt danach (wenn 1) `0x00441F00` aus: `+0x410` → Bereichsrechteck
(0, 360, 640, 420) und `+0x410 = 0`, sonst `+0x3F8`; Renderer-Slot 0 (Szenenoberfläche →
Bildschirm, y + 60), dann `0x0043E830` (Statustext erneut, falls nicht leer).

## Maus und Klick
- `CSzene_HitTest`, `360 ≤ y ≤ 420`: nur bei `+0x5BC` und Leiste: Bereich `+0x428`
  (aktiv, `PtInRect`) → Treffer.
- `CSzene_MouseMove` (ohne Editor): Treffer = Leistenbereich → dessen Ereignis mit Typ
  **0x0D** markieren (Sofort-Flag → auslösen), `+0x2C8 = 0`; sonst `0x00441D40(leiste, 1)`.
- `CSzene_OnClick`: Treffer = Leistenbereich → Typ 0x0B (allgemein) und
  `0x00441D40(leiste, 0)`, Ende.
- `CSzene_DispatchEvents` (ohne Editor, am Ende): Ereignis des Bereichs `+0x428`, dann
  `+0x424`, jeweils mit gespeichertem Typ.

### `0x00441D40(leiste, verlassen)`
- `+0x40C` und `verlassen` → Statustext löschen (`0x0043E7C0`), `+0x40C = 0`, neu zeichnen.
- `+0x414 ≠ −1` → `CAnimLayer_Stop(0)` (Slot 9) dieses Eintrags (Ende-Ereignis, beendet),
  neu zeichnen, `+0x414 = −1`.

Der Klick ruft es also mit 0: die Schleifenanimation endet, die Leiste bleibt hell.

### Handler `0x00441FC0(ereignis, typ)` (Typ 0x0D Maus, 0x0B Klick)
`x = CSzene+0x500` (Bildschirm-x, vorzeichenlos):
1. `x < 5` und `+0x3F0 > 0` → `+0x3F0 − 1`, neu zeichnen, Ende.
2. `x > 634` und `Anzahl − +0x3F0 > 10` → `+0x3F0 + 1`, neu zeichnen, Ende.
   (Blättert bei jeder Mausbewegung am Rand um einen Platz.)
3. `i = x / 64 + +0x3F0`. `i ≠ +0x414` und `+0x414 ≠ −1` → `0x00441D40(leiste, 0)`.
4. `+0x40C == 0` → `+0x40C = 1`, neu zeichnen.
5. `i > Anzahl − 1` → Ende.
6. Typ 0x0D: nur wenn `+0x414 == −1`: Ebene `+0x1B8 = 0`, `CAnimLayer_Start(13)` (Schleife),
   `+0x414 = i`, Text `0x00442160(ID)`.
7. sonst (Klick): `+0x3E8 = ID`. Ereignis `+0x424` vorhanden:
   - `+0x418 == 0` → markieren mit Typ = ID (Sofort-Flag → auslösen), Ende;
   - sonst Ereignis-ID des Eintrags ≠ 0 → markieren mit dieser, Ende.
   Sonst `CGame_DescribeItem(+0x420, ID)` (ger.md).

### Text `0x00442160(id)`
`DefInvent.GER` (`CSzene_FindFile`), `[SPEECH]` `TEXT%03d`, Vorgabe `NULL`; fehlt der
Schlüssel → `[TEXT%03d]`. Statustext löschen, dann `0x0043E600((40, 424, 600, 475), text)`.

## Statustext des Renderers
- `0x0043E600(rect, text)`: Text nach `Renderer+0x210`; nicht leer → Rechteck nach `+0x200`.
  Leer → Rechteck `+0x200` um −60 verschoben über Slot 0 aus der Szenenoberfläche neu
  ausgeben. Sonst mit `GetDC` der Ausgabeoberfläche (`+0x14`) direkt auf den Bildschirm:
  Schrift `+0x4C` (Arial 12, RENDERER.md), Hintergrund transparent, Weiß,
  `DrawTextA(…, 0x15)` = `DT_CENTER | DT_VCENTER | DT_WORDBREAK` im Rechteck – ohne
  `DT_SINGLELINE` wirkt `DT_VCENTER` nicht: Text oben, je Zeile zentriert, umgebrochen. (Vollbildmodus:
  `ClientToScreen`-Versatz.)
- `0x0043E7C0`: Text = leer (`0x0047D770`), Rechteck `+0x200` (−60) neu ausgeben.
- `0x0043E830`: Text nicht leer → `0x0043E600(+0x200, +0x210)`.

Der Text liegt also nicht in der Szenenoberfläche; jede Ausgabe der Leiste übermalt ihn,
`Game_Idle` zeichnet ihn danach neu.
