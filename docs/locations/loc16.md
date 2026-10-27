# Loc16 (automatische Übersicht)

Funktionen `0x0041B290`–`0x0041CEFF` (10 Stück), Ereignis-Handler `0x0041BDC0`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S58_To57` | `0x0041B830` |
| 2 | `S58_To59` | `0x0041B830` |
| 3 | `S58_Dragon` | `0x0041B830` |
| 4 | `S58_Hut (Ebene)` | `0x0041B830` |
| 5 | `S58_Hand (Ebene)` | `0x0041B830` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 0041bdc0.c FUN_0041bdc0 @ 0041bdc0
  Anfang: ruft FUN_0041bdc0
  case 1: → 15/57 | ruft CActor_WalkTo CGame_Random CAudioBuffer_SetVolume
  case 2: Sprache 058_DRA_34 058_ROD_35 058_DRA_33 | → 17/59 | S? 3FC8 3FB8 | ruft FUN_0041c9a0 CGame_HeroSayWait FUN_0041ca50 CActor_WalkTo CGame_Speak CActor_WalkTo2 CGame_Random CAudioBuffer_SetVolume
  case 3: S? 3FB4 | ruft CActor_WalkTo FUN_0041c9f0 CGame_Random CAudioBuffer_SetVolume
  == 1: Sprache 058_ROD_07 058_DRA_07 058_ROD_19 058_DRA_19 | S= 3FAC=2 | ruft CSzene_DisableInput CGame_HeroSayWait CActor_Hide CAnimLayer_PlayRange FUN_0041c9a0 CActor_Show CSzene_EnableInput FUN_0041ca50 CGame_Speak
  == 9: Sprache 058_ROD_15 058_DRA_15 | → 17/59 | S= 3FB8=1 3FAC=iVar2 + 1 | S? 3FAC | ruft CSzene_DisableInput FUN_0041ca50 CGame_HeroSayWait CGame_Speak FUN_0041c860 CActor_WalkTo CSzene_EnableInput FUN_0041c9f0
  case 4: Sprache 058_ROD_28 058_DRA_28 058_ROD_29 | S= 3FC0=0 | ruft FUN_0041c9f0 CGame_HeroSayWait CActor_WalkTo CActor_Hide CAnimLayer_PlayRange CLayer_Disable CGame_PlaySfxRandom3D CGame_Random CEvent_SetTimerHandler CAudioBuffer_SetVolume
  case 5: Sprache 058_DRA_30 058_ROD_30 058_DRA_31 | S= 3FC4=0 | S? 3FB4 | ruft CSzene_DisableInput CActor_WalkTo FUN_0041c9a0 FUN_0041c9f0 FUN_0041c890 CSzene_EnableInput CActor_Hide CAnimLayer_PlayRange CLayer_Disable CActor_Show
  case 6: Sprache 058_ROD_32 058_DRA_32 058_ROD_18 058_DRA_18 | S? 3FB4 | ruft FUN_0041c890 FUN_0041c9f0 CActor_WalkTo FUN_0041ca50
  == 4: Sprache 058_ROD_22 058_DRA_22 | ruft CSzene_DisableInput CActor_Turn CActor_Hide CAnimLayer_PlayEndEvent CActor_Show CGame_HeroSayWait CActor_WalkTo CAnimLayer_PlayRange FUN_0041c9a0 CSzene_EnableInput
  == 6: Sprache 058_ROD_24 058_DRA_24 | S= 3F74=2 3FB0=iVar2 + 1 | S? 3F74 3FB0 | ruft FUN_0041c9f0
  case 7: S? 3E80 | ruft CGame_Random
```
