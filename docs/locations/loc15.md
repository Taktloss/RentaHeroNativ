# Loc15 (automatische Übersicht)

Funktionen `0x0041A330`–`0x0041B28F` (6 Stück), Ereignis-Handler `0x0041ACF0`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S57_Kanal` | `0x0041A8B0` |
| 2 | `S57_To38` | `0x0041A8B0` |
| 3 | `S57_To58` | `0x0041A8B0` |
| 4 | `S57_Schlucht` | `0x0041A8B0` |
| 5 | `S57_Gleiter (Ebene)` | `0x0041A8B0` |
| 6 | `S57_Sack (Ebene)` | `0x0041A8B0` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0041acf0.c FUN_0041acf0 @ 0041acf0
  Anfang: ruft FUN_0041acf0
  case 1: → 7/53 | S= 3EFC=0 3F00=0 | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide CAnimLayer_PlayEndEvent CSzene_EnableInput CGame_Random CAudioBuffer_SetVolume
  case 2: → 10/38 | S= 3F38=1 | ruft CActor_WalkTo CGame_Random CAudioBuffer_SetVolume
  case 3: Sprache 057_ROD_02 | → 16/58 | S= 3F94=0 | S? 3F94 | ruft CGame_HeroSayWait CActor_WalkTo CGame_Random CAudioBuffer_SetVolume
  case 4: Sprache 057_ROD_01 | ruft CActor_WalkTo CActor_Hide CAnimLayer_PlayEndEvent CActor_Show CGame_HeroSayWait CGame_PlaySfxRandom3D CGame_Random CEvent_SetTimerHandler CAudioBuffer_SetVolume
  case 5: → 31/113 | S= 3F9C=0 | ruft CActor_WalkTo CSzene_DisableInput CLayer_Disable CActor_Hide CAnimLayer_PlayRange CAudioBuffer_SetFreq CAudioBuffer_FadeFreq
  case 6: Sprache 057_ROD_03 | S= 3FA0=0 | Inv Add(7) | ruft CActor_WalkTo CSzene_DisableInput CActor_Hide CAnimLayer_PlayRange CLayer_Disable CActor_Show CSzene_EnableInput CGame_HeroSayWait
  case 11: S? 3E80 | ruft CGame_Random
```
