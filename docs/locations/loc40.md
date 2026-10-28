# Loc40 – Intro (Szenen 114, 115, 116)

Belege: `Loc40_ctor` `0x00431920`, Start `0x00432CA0`, Handler `0x00432EF0` (S114),
`0x004330D0` (S115), `0x00433100` (S116), Destruktor `0x00432A40`. Start von
„Neues Spiel“ = Loc 40 / Szene 114 (savegame.md).

## Ablauf
| Szene | Animation (AVI) | Musik | Ende → |
|---|---|---|---|
| 114 | `S114_IntroPart1` | `Loc40_IntroPart1` | Loc 35 / Szene 1 |
| 115 | `S115_IntroPart2` | `Loc36_IntroDragonAttack` | Loc 37 / Szene 3 |
| 116 | `S116_IntroPart3` | `Loc36_IntroDragonAttack` | Loc 1 / Szene 7 (Spielbeginn) |

Gemeinsames Muster beim Start (`0x00432CA0`):
- Musik starten, Musik-Puffer-Eigenschaften `0x00437560`/`0x004375B0` (UNVERIFIED:
  vermutlich DirectSound-3D-/Hall-Parameter).
- `CGame_SetEventProc` auf den Szenen-Handler.
- **`CSzene_SetClickEvent(EventValue(2))`** (`0x0044C880`, Ereignis `CSzene+0x2C4`,
  Typ 0x16): ein Klick ins Bild löst dasselbe Ereignis 2 aus wie das Animationsende →
  **Intro überspringen per Mausklick**, je Teil einzeln.
- `CSzene_SetModal(anim)`; `CAnimLayer_SetFrameEvent(EventValue(1), bild)`
  (`0x00443400`, Ereignis `+0x1CC`, Zielbild `+0x2CC`);
  `CAnimLayer_PlayEndEvent(10, EventValue(2))` (`0x00443030`, Ende-Ereignis `+0x1C4`,
  dann `CAnimLayer_Start(10)`), bei S116 stattdessen `CAnimLayer_PlayRange`
  (`0x00442F90`, Bilder 0…120, Ereignis 11).
- `CSzene+0x518 = 1` während des Intros (vor jedem Wechsel wieder 0), `CSzene+0x510 = 0`
  (`0x0044B190`), `CSzene_EnableInput`.

## Ereignisse
- **1 (Bild erreicht)**: Sprachzeile abspielen und nächstes Zielbild setzen (Kette).
  - S114: Bild 10 `016_SPR_01` → 150 `016_SPR_02` → 530 `016_SPR_03` → 740
    `021_SPR_01` → 850 `021_SPR_02` → 960 `023_SPR_01` → 1077 Musikwechsel
    `Loc09_SmashvilleAttack` → 1248 `024_ST2_01` (Ende der Kette). Bei 150 und 960
    werden die Musik-Puffer-Parameter umgestellt.
  - S116: Bild 1 `003_PRI_05` → 21 `003_PRI_06` → 87 `004_PRI_01` (mit Ende-Ereignis 3)
    → 98 Musik `Loc35_RodrigoOnGlider` (Einblenden 100 % über 2000 ms), Meer-Puffer
    `+0xD0` auf 1 % / Frequenz 50 % über 5000 ms.
- **2 (Animationsende oder Klick)**: `CSzene+0x518 = 0`, `CGame_ChangeScene` (Tabelle).
  S116 setzt vorher `CGame+0x2C = 5` → `RequestLocation` speichert **5** als vorherige
  Szene (Original-Verhalten).
- **3** (S116, Ende von `004_PRI_01`): `CAnimLayer_Play…(10, EventValue(2), 121, −1)`
  (`0x00442E70`) – spielt ab Bild 121 zu Ende.

Ton: S114 lädt 21 Effekte und setzt 78 Animations-Cues (AUDIO.md), S115/S116 weitere;
Werte stehen im Port-Code, nicht hier.

## Port
`src/game/loc40.*` (GAME.md). Abweichungen bis zum Audio-Port: keine Musik/Effekte/Sprache
hörbar; Sprachdauer nach Textlänge (SPEECH.md „Port“).
