# Klickbereiche (`CArea`) und Treffer-Prüfung

Ergänzt EVENTS.md (Abschnitte 2, 3, 5). Belege jeweils mit Adresse; Port:
`src/engine/area.*`, `Scene::addArea/getArea/hitTest/onClick`, Tests `tests/test_area.cpp`.

## `CArea` (0xBC Bytes, Ctor `0x00435F80`)
| Offset | Feld | Init |
|---|---|---|
| `+0x00` | `RECT` (Szenenkoordinaten) | leer (`0x00436040`) |
| `+0x10` | Name (`0x00436190`: Länge > 50 → bei 49 abgeschnitten) | "" |
| `+0x42` | Anzeigetext (`CArea_SetText` `0x004361F0`: Länge > 100 → bei 99 abgeschnitten) | – |
| `+0xA8` | Mauszeiger-Typ (`0x004361C0`: 0 → **10**) | 10 |
| `+0xAC` | Masken-Ebene (`CLayer*`), sonst 0 | 0 |
| `+0xB0` | `CEvent*` (eigener, Rückverweis Event `+0x14`) | neu |
| `+0xB4` | Parameter von `SetRect` (`0x004363C0`), Bedeutung offen | 0 |
| `+0xB8` | aktiv | 0 |

- `CArea_SetEvent(id)` `0x00436440`: id = 0 → nichts; sonst Handler = id, Ereignis
  aktiviert, **Bereich aktiv**. Das ist der einzige Weg, einen Bereich zu aktivieren –
  `CLayer_Enable` berührt ihn nicht. Erzeugter Schauplatzcode:
  `%s->SetEvent(MakeEventValue(CLICKED_%s))`.
- `CLayer_Disable` `0x00447490` deaktiviert den Bereich der Ebene (`+0xB8 = 0`).
- `0x00436060` (Ebene anhängen): Bereich `+0xAC = Ebene`, Ebene `+0x1C0 = Bereich`,
  Rechteck = `CLayer_GetRect` (ohne Raster-Markierung).
- Rechteck aktualisieren: `CLayer_SetPos` `0x00446E70` und weitere Setzer (`0x00446EC0`,
  `0x00446F10`, `0x00446FD0`, `CLayer_UpdateZoomRect` `0x00447060`, `0x00447100`,
  `0x004474C0`, `CLayer_Update` `0x00447890`) setzen bei vorhandener Maske
  Rechteck = `CLayer_GetRect` und markieren das Raster.

## Treffer-Raster `0x0047D77C`
Globales Byte-Raster: Zelle = 8×8 Pixel, 80 Zellen je Zeile, Index `x/8 + (y/8)·80`
(Division mit Abschneiden zur 0).
- Markieren `0x004360A0(rect)`: Kopie mit `ClipToScene`; nicht sichtbar oder Name beginnt
  mit `"Rodrigo"` (strncmp, Groß-/Kleinschreibung beachtet) → nichts. Sonst
  `for (y = top; y < bottom; y += 8) for (x = left; x < right; x += 8) grid[idx] = 1`
  (nur `idx < 0xE74`). **Eigenheit:** Die Schleife beginnt bei `left/top`, nicht an der
  Zellgrenze – eine Randzelle rechts/unten wird ausgelassen, wenn `right − left` bzw.
  `bottom − top` sie nicht mehr erreicht (z. B. left = 5, right = 12: nur Zelle 0).
- Löschen `0x00436150`: `rep stos` über **0xE10** Bytes (45 Zeilen = y < 360). Aufrufer:
  `Game_SwitchLocation` (`0x004036C3`), `CSzene_ctor` (`0x00448603`), `CSzene_Clear`,
  `0x004489B6`. Das Raster wird beim Entfernen eines Bereichs **nicht** gelöscht – markierte
  Zellen bleiben bis zum nächsten Löschen.
- Prüfen `0x004364C0(x, y)`: Byte ≠ 0, ohne Bereichsprüfung.

## `CSzene_AddArea(name, rect, cursor)` `0x0044ADD0`
1. `rect` (falls ≠ NULL) normalisieren: left/right bzw. top/bottom tauschen, falls verkehrt.
2. Ersten freien Platz in `CSzene+0xA4[60]`; keiner → 0.
3. `rect` ≠ NULL → `SetRect(rect, 0)` (Kopie, Raster markieren, `+0xB4 = 0`).
4. `name` ≠ NULL → Name, Zeiger-Typ; ist die Schauplatznummer `CSzene+0x5D0 ≠ 0`:
   `Loc%02d` + `GER` per `CSzene_FindFile`, `[AREA_TEXT] name` (Puffer 100, Vorgabe
   `"NULL"`) → Text, falls ≠ `"NULL"`.
5. aktiv = 0.

Aufrufer: SZN-Lader (`AreaN`: Name = Sektionsname, Rechteck X1/Y1/X2/Y2, `CursorType`) und
`CSzene_CreateLayer` (Name = Dateiname ohne Pfad/Endung via `_splitpath` `0x00447F00`,
Rechteck NULL, Zeiger 0 → 10, dann Ebene anhängen). Ebenenbereiche liegen also **in
derselben Liste** wie die freien.
`CSzene_CreateLayer` ruft danach den Update-Slot der Ebene auf, wenn `CSzene+0x5BC == 0`.

`CSzene_GetArea(name)` `0x0044AFB0`: Liste bis zum ersten leeren Platz, `lstrcmpi` mit dem
Namen; kein Treffer → `MessageBox(name)`, 0.

## `CSzene_HitTest(x, y)` `0x0044C3C0` (Szenenkoordinaten)
- `360 ≤ y ≤ 420`: nur Bereich `+0x428` der Inventarleiste `+0x5B4` (wenn `+0x5BC`).
- sonst nur, wenn Editor (`+0xAC8`) oder Rasterzelle gesetzt:
  1. Z = zMin … zMax (vorzeichenlos), Z ≠ 255 und Zähler[Z] ≠ 0: Ebenen bis zum ersten
     leeren Platz; aktiv, `ftol(Z) == z`, Bereich vorhanden und aktiv, `PtInRect`, und
     (Maske 0 oder `0x00447B00` = nicht transparent) → Treffer. **Niedrigstes Z gewinnt.**
  2. Ebenen mit `Z == 255.0` (double exakt) ebenso.
  3. Bereichsliste bis zum ersten leeren Platz: **nur Bereiche ohne Maske**, aktiv,
     `PtInRect`. Ebenenbereiche werden hier nie getroffen.
- `PtInRect`: `left ≤ x < right`, `top ≤ y < bottom`.
- **Eigenheit:** TCC-Ebenen haben immer DirtyAlpha; ihr Bereich ist das Alpha-Rechteck mit
  einschließendem Maximum (`CImage_CalcAlphaBounds`, COMPOSITOR.md). Die letzte Spalte und
  Zeile mit Alpha ≠ 0 sind daher nicht anklickbar (Test `area_hit_priority_and_activation`).

### Transparenz `0x00447B00` (Ebene, x, y)
Kein Bild (`+0x0C`, z. B. BMP-Ebene nach dem Anlegen der Oberfläche) → 0 (nicht transparent).
`dx = x − ftol(X)`, `dy = y − ftol(Y)`; Zoom ≠ 100.0 → `dx = dx·100 / ftol(zoom)` (ebenso dy).
Dann Bild-Slot 8 = `CImage_IsTransparent` `0x00438680` (TCC und A16):
Index `breite·dy + dx` **ohne Bereichsprüfung**; mit Alpha: `(~a & 0xFF) >> 7` (a < 0x80);
ohne: R = G = B = 0.

## `CSzene_OnClick(x, y)` `0x0044C950` (Bildschirmkoordinaten)
1. `+0x500/+0x504 = x, y`. Ohne Inventarleiste (`+0x5BC == 0`): `0x0044CB60` –
   Figur `+0x5E0` (bzw. `+0x5E4`, wenn sie die zuletzt angelegte Ebene `+0x5D8` ist) läuft
   zum Klick (`CActor_WalkToClick`).
2. Hilfe `+0x9F8` offen → schließen, Ende.
3. `+0x514 == 0` (Ctor: 1) → Ende.
4. `+0x518 == 0` und Vollbildobjekt `+0x5DC` aktiv und läuft → Ende.
5. `+0x510 ≠ 0` (Ctor: 1), kein Editor und globales Ereignis `0x00481EE8` aktiviert →
   dieses mit Typ 0x16 markieren (Sofort-Flag `+0x0C` → auslösen), Ende.
6. `y < 60`: bei Hilfe-Objekt, kein Editor und `x ≥ 581` (vorzeichenlos) → Hinweis
   (`0x004416E0`); sonst Sprung zu 9.
7. `y < 480`: `area = HitTest(x, y − 60)` → `+0x2C8`. Treffer: Bereichsereignis Typ 0x0B
   markieren (Sofort-Flag → auslösen). Treffer = Inventarbereich → `0x00441D40`, Ende.
   Treffer und nicht der Bereich der Figur → Ende. Figur aktiv → `CActor_WalkToClick`,
   Ende. Sonst Szenen-Ereignis `+0x2C4` Typ 0x16 markieren.
8. Ende.
9. (`y < 60` oder `y ≥ 480`): `0x00481F24 == 0` → Umschalt-Ereignis `*0x00481F2C`
   deaktivieren und `pending = 0`.
