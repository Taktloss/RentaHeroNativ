# Ereignisfluss: Mausklick → Bereich → `OnGenericEvent`

Belegt durch Dekompilate (Adressen jeweils angegeben) und die Codegenerator-Vorlage
im Binary (`%s->SetEvent(MakeEventValue(CLICKED_%s))`, `switch (thisEvent->Param)`).

## 1. Fenster → Szene
`WndProc` (`0x00402350`) leitet `WM_LBUTTONDOWN/UP`, `WM_RBUTTONDOWN/UP`,
`WM_LBUTTONDBLCLK` (0x201–0x204) und `WM_RBUTTONDBLCLK` (0x206) an
`OnMouseButton(msg, x, y)` (`0x00402670`):
- Fenstermodus: Koordinaten umrechnen (`FUN_00447e50`).
- `DAT_0047d138 != 0`: Editor-Rest „Set Area“ (zwei Klicks spannen ein Rechteck auf,
  `y - 0x3C`), im Spiel nie aktiv.
- `WM_LBUTTONDBLCLK` und Figur `CSzene+0x5E0` vorhanden → `FUN_00435320(figur)`, Ende.
- sonst: Figur vorhanden → `FUN_00435330(figur)`; dann `CSzene_OnClick(x, y)`.
  **Die Nachrichtenart wird nicht weitergegeben** (Disassembly `0x00402779`–`0x00402783`:
  nur `push y; push x`) – jeder Tasten-Down/Up-Event führt also zu `CSzene_OnClick`.
  Klicks innerhalb eines Frames fallen zusammen, weil Ereignisse nur markiert und erst in
  `CSzene_DispatchEvents` ausgelöst werden (Abschnitt 4). Ob Down und Up in getrennten
  Frames zweimal auslösen, ist noch im Testlauf zu prüfen (UNVERIFIED).

## 2. `CSzene_OnClick` (`0x0044C950`)
Speichert `x, y` in `CSzene+0x500/+0x504`. Bildschirmaufteilung (Szene 640×360 ab y = 60):
- `CSzene+0x9F8` (Hilfe, HINTS.md) offen (`[1] != 0`) → schließen (`FUN_004416c0`), Ende.
- `CSzene+0x514 == 0` → Klicks gesperrt, Ende.
- weitere Sperren/Sonderfälle: `+0x518`, `+0x5DC`, `+0x510` mit globalem Ereignis `0x00481EE8`
  (Typ 0x16) – Bedeutung offen.
- **y < 60** (oberer Balken): bei `x ≥ 0x245` (581) Hilfe-Symbol → `CGame_ShowHint` (HINTS.md);
  sonst nichts.
- **60 ≤ y < 480**: `area = CSzene_HitTest(x, y - 60)` → `CSzene+0x2C8`.
  - Treffer: Ereignis `area+0xB0` mit **Typ 0x0B** markieren (`pending = 1`, Typ/Param =
    0x0B; sofort `CEvent_Fire` nur bei Sofort-Flag `+0x0C`).
  - Treffer gleich Bereich des Objekts `+0x5B4` (+0x428) → `FUN_00441d40`, Ende.
  - Treffer, aber nicht der Bereich der Figur → Ende.
  - Kein (anderer) Treffer und Figur aktiv (`+0x1A4`) → `CActor_WalkToClick(figur, x, y-60)`
    (SCRIPTING.md).
  - sonst Szenen-Ereignis `CSzene+0x2C4` mit **Typ 0x16** markieren.

## 3. Treffer-Prüfung `CSzene_HitTest` (`0x0044C3C0`, Szenenkoordinaten)
- **y in [360, 420]** (unterer Streifen = Bildschirm-y 420…480): nur der Bereich von
  `CSzene+0x5B4` (+0x428), wenn `CSzene+0x5BC` gesetzt.
- sonst, falls Editor (`+0xAC8`) oder `FUN_004364c0(x, y)`:
  1. Ebenen nach **aufsteigendem Z** (`+0x2FC` = min … `+0x2F8` = max; Zähler je Z in
     `short`-Array `+0x300`), Z = 255 ausgenommen. Pro Ebene: aktiv (`+0x1A4`), Bereich
     `layer+0x1C0` vorhanden und aktiv, `PtInRect`, und – falls Maske – `FUN_00447b00 == 0`.
     **Niedrigstes Z gewinnt.**
  2. Ebenen mit **Z = 255.0** (double, oberes Dword `0x406FE000`).
  3. Freie Bereiche `CSzene+0xA4[0..59]` ohne Ebene, `PtInRect` (+ Maske).
  → Rückgabe: Bereich (`CArea*`) oder 0.

## 4. `CEvent` und Auslösen
`CEvent` (0x24 Bytes, Ctor `0x0043F130`, 48× per `new` erzeugt):

| Offset | Feld | Init |
|---|---|---|
| `+0x00` | aktiviert | 0 |
| `+0x04` | ausstehend | 0 |
| `+0x08` | ? | 1 |
| `+0x0C` | Sofort-Flag: beim Markieren direkt auslösen | 0 |
| `+0x10` | Typ | 0 |
| `+0x14` | ? | 0 |
| `+0x18` | `CTimer*` (TIMING.md) | 0 |
| `+0x1C` | Handler: **< 0xFFF = Ereignis-ID**, sonst Funktionszeiger | 0 |
| `+0x20` | `Param` | 0 |

`CEvent_Fire(event, typ)` (`0x0043F3C0`): nur wenn aktiviert, Handler gesetzt und
ausstehend; `+0x04` wird gelöscht.
- Handler < 0xFFF: `Param = Handler` (die ID), Aufruf `g_EventProc(event, typ)`.
- sonst: direkter Aufruf `handler(event, typ)`.
`g_EventProc` (`0x0047E60C`) wird von `CGame_SetEventProc` über `FUN_0043f1c0` gesetzt
(vorheriger Wert → `g_PrevEventProc` `0x0047E610`) und ist das `OnGenericEvent` des
aktuellen Schauplatzes (z. B. `Loc05_OnGenericEvent` `0x0040DB30`).

Normalfall: Auslöser setzen nur `pending`, Typ und `Param`; `CSzene_DispatchEvents`
(`0x0044C690`) ruft einmal pro Frame `CEvent_Fire` für alle bekannten Ereignisse und die
Timerliste auf.

Ereignistypen bisher: **0x01** = Animation fertig, **0x0B** = Klick auf Bereich,
**0x15** = Timer, **0x16** = Klick ins Leere / Szene, **0x29** = Figur angekommen/gedreht,
**0x2A** = Laufbefehl abgebrochen (SCRIPTING.md).
`DAT_0047e608` und `event+0x08` steuern eine Variante beim Aufruf (Bedeutung offen).

## 5. `CArea` (Klickbereich)
Aus `CSzene_HitTest`/`CSzene_OnClick`: `RECT` bei `+0x00`, Maske/Bildreferenz `+0xAC`,
`CEvent*` bei `+0xB0`, aktiv-Flag `+0xB8`. Ebenen verweisen über `layer+0x1C0` auf
ihren Bereich (automatisch in `CSzene_CreateLayer` angelegt), freie Bereiche liegen in
`CSzene+0xA4[60]` (aus `AreaN` der SZN). Größe 0xBC, Ctor `0x00435F80`; angelegt mit
`aktiv = 0`; Anzeigetext aus `[AREA_TEXT]` der `LocNN.GER` (formats/ger.md).

## Folgen für den Port
- Klick-Priorität exakt so umsetzen: unterer Streifen → Ebenen nach Z aufsteigend
  (ohne 255) → Z = 255 → freie Bereiche → Figur läuft → Szenen-Ereignis 0x16.
- Ereignis-IDs pro Schauplatz stammen aus den `LocNN_OnGenericEvent`-Switches – diese
  werden in Phase 4 je Schauplatz dokumentiert.
