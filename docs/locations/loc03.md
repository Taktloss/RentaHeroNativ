# Loc03 – Brücke vor Smashville (Szene 12)

Funktionen `0x0040A4E0`–`0x0040B8BF` (7 Stück), Ereignis-Handler `0x0040AE30`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S12_Bridge` | `0x0040A9C0` |
| 2 | `S12_Schild` | `0x0040A9C0` |
| 3 | `S12_To13` | `0x0040A9C0` |
| 4 | `S12_Pflanze` | `0x0040A9C0` |
| 5 | `S12_RodGlider (Ebene)` | `0x0040A9C0` |
| 6 | `S12_SabSword (Ebene)` | `0x0040A9C0` |
| 7 | `S12_SabGlider (Ebene)` | `0x0040A9C0` |
| 8 | `S12_Rab1Talk (Ebene)` | `0x0040A9C0` |
| 9 | `S12_Rab2 (Ebene)` | `0x0040A9C0` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0040ae30.c FUN_0040ae30 @ 0040ae30
  Anfang: ruft FUN_0040ae30
  case 1: Sprache 012_ROD_03 012_RA1_01 | ruft FUN_0040ae10 CGame_WalkAndComment FUN_0040adc0 CGame_Random CAudioBuffer_SetVolume
  case 2: Sprache 012_ROD_01 012_RA2_01 | ruft FUN_0040ae10 CGame_WalkAndComment FUN_0040adc0 CGame_Speak FUN_00442f90 CGame_Random CAudioBuffer_SetVolume
  case 3: Sprache 012_RA1_02 | → 2/13 | ruft CActor_WalkTo FUN_0040ae10
  case 4: Sprache 012_ROD_02 012_RA2_02 | ruft FUN_0040ae10 CGame_WalkAndComment FUN_0040adc0 CGame_Speak CGame_Random CAudioBuffer_SetVolume
  case 5: → 1/7 | ruft FUN_0040ae10 CActor_WalkTo CSzene_DisableInput CLayer_Disable CSzene_SetModal CAnimLayer_PlayRange CAudioBuffer_SetFreq CAudioBuffer_FadeFreq
  case 9: Sprache 012_ROD_06 012_RA1_03 012_RA2_03 012_RA1_04 012_RA1_05 012_RA1_06 012_RA2_05 012_ROD_07 012_SAB_01 012_ROD_08 | S= 3ECC=1 | ruft FUN_0040ae10 CActor_WalkTo CSzene_DisableInput CActor_UnblockRect FUN_00440400 CGame_Speak CAnimLayer_PlayRange CAnimLayer_ShowFrame CLayer_Disable CAnimLayer_AddCue FUN_00442f90 FUN_004475b0 CActor_WalkTo2 CAnimLayer_ClearCues CLayer_Enable CSzene_EnableInput CActor_Turn CGame_Random CAudioBuffer_SetVolume CGame_PlaySfxRandom3D CEvent_SetTimerHandler
  case 7: ruft FUN_0040ae10 CGame_Random
  == 1: Sprache 012_ROD_04 012_ROD_05 | ruft CGame_WalkAndComment FUN_0040adc0
  case 10: ruft CGame_Random
  case 11: ruft FUN_0040adc0
  case 12: ruft CLayer_Disable CAnimLayer_ClearCues CLayer_Enable CAudioBuffer_SetFreq CAnimLayer_PlayRange
  case 13: ruft CAudioBuffer_FadeFreq CAnimLayer_PlayRange
  case 14: ruft CGame_Random
```

## Analyse (2026-10-29, Port `src/game/loc03.cpp`)
Konstruktor `0x0040A4E0`: von Szene 15 zusätzlich `S12_PirateShip` und Musik
`Loc09_SmashvilleAttack`, sonst `Loc01_Smashville`; Hall (16, 0.111, 2.767, 0.224), Anteil 0.1.
Solange `0x3ECC == 0`: Ladebit 1 (Wachen `S12_Rab1Run/Rab1Talk/Rab2`, Säbelfrau
`S12_SabInGlider/SabSword/SabGlider`, `SabWalk`), Wachen-Timer `+0xB8`, Schwertklänge.
Abbau `0x0040A880` löscht die Schrittklänge `+0xC8/+0xCC` nicht (Original-Fehler, nachgebildet).

### Start `0x0040A9C0`
Wachen aktiv (nur `0x3ECC == 0` und nicht von Szene 15): `CSzene_ClearFlag510`, Bereiche
6 (Säbel), 7 (Gleiter), 8/9 (Wachen), Sperrrechteck (407, 296)–(538, 341), Timer 4–9 × 333 ms.
Zugang je Vorgänger-Szene: 8 Anflug (`S12_Anflug`, Gleiterklang 90 → 35 %), Held (635, 334, 2)
→ (581, 311); 13 Held (634, 270, 2) → (568, 280); 15 Piratenschiff (0–58, `012_GA1_01`, 59–94,
`012_GA2_01`, 95–Ende), `0x3EC8 = 1`, Musikwechsel, Held (238, 294, 6) → (315, 348).
Andere Vorgänger: Held bleibt unsichtbar (wie im Original). Umgebungsklänge alle 3–6 s (Ereignis 14).

### Ereignisse `0x0040AE30`
1/2/4/7 Kommentare (Wachen-Timer dabei angehalten); 3 → Szene 13 (Loc02); 5 Abflug
(`S12_Wegflug` 0–30, 31–68, 69–Ende) → Szene 7. 6/8/9: Gespräch mit den Wachen, sie laufen
davon (Klang-Marken, `MoveTo` 625/215 bzw. 615/240 in 15 Schritten), Säbelfrau tritt auf und
geht zum Gleiter (`MoveTo(411, 177, 164, 52)` in 52 Schritten → Ereignis 12, Einsteigen →
13, Abflug), `0x3ECC = 1`. 10: Wachen-Spruch (nie derselbe zweimal hintereinander, globale
`0x0047D774`); Bereiche der Wachen bis Ereignis 11 inaktiv.
