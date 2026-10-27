# Loc01 – Smashville (Szenen 7, 1007, 8, 9, 46)

Belege: `Loc01_ctor` `0x00405530`, Start `0x00405DC0`, Handler `0x004065C0`,
Hilfsfunktionen `0x004070D0`–`0x00407DC0`. Musik `Loc01_Smashville`. Tabellen erzeugt
mit `tools/loc_areas.py` und `tools/case_summary.py` (re/private/decomp-loc).

## Szenen
| Szene | SZN-Teile / Inhalt |
|---|---|
| 7 | Platz, Teilmaske beim Laden: Bit 0 `S[0x3E94] ≠ 0`, Bit 1 Gegenstand 7 im Inventar, Bit 2 `S[0x3E94]` und Gegenstand 13 (`CSzene_Load(name, maske)`) |
| 1007 | Platz, unterer Stock (Lift, Hebel) |
| 8 | Abflug mit dem Gleiter: Ziele Büro, Kloster, Kneipe, zurück |
| 9 | oben am Hebel: Lift, Pflanze, Schilder, Tür, Klingel, Besen |
| 46 | Kanal (nur von Szene 47 kommend), Teil abhängig von `S[0x3E94]` |
Jede andere Szenennummer wird im Konstruktor zu **7** (`CGame+0x2C = 7`).

## Flags
| Flag | Start | Bedeutung (aus dem Code) |
|---|---|---|
| `S[0x3E94]` | 1 | Kanalwesen ist da; 0 nach `0x004079E0` (Gegenstand 13 benutzt) |
| `S[0x3E98]` | 0 | Gesprächsstufe am Kanal: 1 nach Ablegen von Gegenstand 7, 2…5 in `0x00407B80` |
| `S[0x3E9C]` | 0 | Gegenstand 7 abgelegt (`0x00407860`, am Ende der Sequenz) |
| `S[0x3EC4]` | 0 | Gesprächsstufe 4 erreicht (`0x00407B80`) |
Hinweis `01_046_A` (HINTS.md) prüft genau `S[0x3E9C] && S[0x3E94] && S[0x3EC4] && !INV(13)`.

## Sequenzen und Abbruch
Sequenzen wie `0x00407860` (Gegenstand 7 ablegen) sperren zuerst die Eingabe
(`CSzene_DisableInput`) und prüfen nach jedem Schritt `CGame_Wait`; erst am Ende wird
die Eingabe wieder freigegeben. Da `CSzene_OnClick` bei gesperrter Eingabe jeden Klick
verwirft (EVENTS.md), kann der Spieler diese Sequenzen nicht abbrechen. Ein Abbruch
(Rückgabe `false`) beendet die Sequenz an der jeweiligen Stelle – Port: Reihenfolge der
Flag-Änderungen wörtlich übernehmen.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 0 | `Gegenstand 13` | `0x004079E0` |
| 1 | `007a_Tuer` | `0x00405DC0` |
| 2 | `007a_Treppe` | `0x00405DC0` |
| 4 | `007a_Pflanze` | `0x00405DC0` |
| 5 | `007a_Schild` | `0x00405DC0` |
| 6 | `GliderLayer` | `0x00405DC0` |
| 10 | `007b_LiftRunter` | `0x00405DC0` |
| 10 | `007b_UntererStock` | `0x00405DC0` |
| 11 | `007b_Pflanze` | `0x00405DC0` |
| 12 | `007b_Treppe` | `0x00405DC0` |
| 13 | `007b_Lift` | `0x00405DC0` |
| 14 | `007b_Schild` | `0x00405DC0` |
| 20 | `009_Lift` | `0x00405DC0` |
| 21 | `009_Pflanze` | `0x00405DC0` |
| 22 | `009_SchildLinks` | `0x00405DC0` |
| 23 | `009_SchildRechts` | `0x00405DC0` |
| 24 | `009_Tuer` | `0x00405DC0` |
| 25 | `009_Klingel` | `0x00405DC0` |
| 26 | `009_Besen` | `0x00405DC0` |
| 30 | `008_Office` | `0x00405DC0` |
| 31 | `008_Kloster` | `0x00405DC0` |
| 32 | `008_Kneipe` | `0x00405DC0` |
| 33 | `008_Raus` | `0x00405DC0` |
| 40 | `046_Gang` | `0x00405DC0` |
| 41 | `046_Kanal` | `0x00405DC0` |
| 42 | `046_Lift` | `0x00405DC0` |
| 43 | `046_Schild` | `0x00405DC0` |
| 44 | `046_Schild2` | `0x00405DC0` |
| 45 | `KanalTalk` | `0x00405DC0` |
| 101 | `Gegenstand 13` | `0x00405DC0` |

## Ereignis-Handler (Zusammenfassung je Fall; `== N` = Zweig einer If-Kette: Ereignis-ID oder Flag-Wert)
```
# 004065c0.c FUN_004065c0 @ 004065c0
  Anfang: ruft FUN_004065c0
  case 1: ruft FUN_00407500 CGame_Random CAudioBuffer_SetVolume
  case 2: ruft CGame_Random CAudioBuffer_SetVolume
  case 3: ruft FUN_00407640
  case 4: Sprache 007_ROD_05 | ruft CActor_WalkTo FUN_004403e0 CGame_Random CAudioBuffer_SetVolume
  case 6: ruft FUN_00407790
  case 13: ruft CActor_WalkTo CSzene_DisableInput CLayer_Disable CActor_Hide CAnimLayer_PlayRange CSzene_EnableInput
  == 10: → 1/46 1/9 | ruft FUN_004406e0
  case 11: Sprache 007_ROD_05 | ruft CActor_WalkTo FUN_00440400
  case 12: ruft CActor_WalkTo
  case 14: ruft CActor_WalkTo CGame_Random
  == 2: Sprache 007_ROD_03 046_KAN_07 | ruft FUN_00440400 CGame_Speak
  == 3: Sprache 007_ROD_04 007_ROD_02 046_KAN_08 046_KAN_06 | → 7/47 | ruft FUN_00440400 CGame_Speak CSzene_DisableInput CSzene_SetModal CAnimLayer_PlayRange
  case 20: → 1/1007 | ruft CActor_WalkTo CSzene_DisableInput CLayer_Disable CActor_Hide CAnimLayer_PlayRange CSzene_EnableInput
  case 21: Sprache 009_ROD_03 | ruft CActor_WalkTo FUN_00440400
  case 22: Sprache 009_ROD_01 | ruft CActor_WalkTo FUN_00440400
  case 23: Sprache 009_ROD_02 | ruft CActor_WalkTo FUN_00440400
  case 24: → 5/10 | ruft CActor_WalkTo
  case 25: Sprache 009_ROD_05 | ruft CGame_WalkAndComment
  case 26: Sprache 009_ROD_04 | ruft CGame_WalkAndComment
  case 30: → 1/7
  case 31: Inv Has(14)
  == 0: Sprache 008_ROD_01 | → 6/26 | ruft CAudioBuffer_SetFreq CAudioBuffer_FadeFreq FUN_004406e0 FUN_00440400
  case 32: → 3/12 | ruft CAudioBuffer_SetFreq CAudioBuffer_FadeFreq FUN_004406e0
  case 33: → 31/113 | ruft CAudioBuffer_SetFreq CAudioBuffer_FadeFreq FUN_004406e0
  case 40: Sprache 046_ROD_08 | ruft CActor_WalkTo FUN_00440400
  case 41: S? 3E94 | ruft CActor_WalkTo CGame_Random
  case 42: → 1/1007 | S? 3E94 | ruft CActor_WalkTo CSzene_DisableInput CAnimLayer_PlayRange CSzene_EnableInput
  == 43: ruft CActor_WalkTo CGame_Random
  case 45: S? 3E94 | ruft FUN_00407b80
  case 101: Sprache 046_ROD_09 | S? 3E94 | ruft FUN_004079e0 FUN_00440400 CGame_DescribeItem
  case 200: ruft CGame_Random
  case 9: ruft CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D CEvent_SetTimerHandler
# 00407b80.c FUN_00407b80 @ 00407b80
  Anfang: S? 3E98 3E9C | ruft FUN_00407b80 CActor_WalkTo
  == 1: Sprache 046_ROD_01 046_KAN_01 046_ROD_02 | S= 3E98=2 | ruft FUN_00440400 CGame_Speak CGame_WalkAndComment
  case 1: Sprache 046_ROD_03 046_KAN_02 | ruft FUN_00440400
  case 2: Sprache 046_ROD_04 046_KAN_03 | S= 3E98=3 | ruft FUN_00440400 CGame_Speak
  case 3: Sprache 046_ROD_05 046_KAN_04 | S= 3E98=4 3EC4=1 | ruft FUN_00440400 CGame_Speak
  case 4: Sprache 046_ROD_06 046_KAN_05 046_ROD_07 | S= 3E98=5 | ruft FUN_00440400 CGame_Speak
```
