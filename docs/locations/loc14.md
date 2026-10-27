# Loc14 (automatische Übersicht)

Funktionen `0x00419650`–`0x0041A32F` (6 Stück), Ereignis-Handler `0x00419C90`.
Erzeugt mit `tools/gen_loc_docs.py`; Bedeutung der Flags und Abläufe folgt bei der Einzelanalyse (UNVERIFIED bis dahin). In der Fallübersicht ist `== N` ein Zweig einer If-Kette – Ereignis-ID **oder** Wert eines Flags/Zählers.

## Klickbereiche → Ereignis-IDs
| ID | Bereich / Auslöser | gesetzt in |
|---:|---|---|
| 1 | `S90_Krankenbett` | `0x004199C0` |
| 2 | `S90_Doppelbett` | `0x004199C0` |
| 3 | `S90_Blumen` | `0x004199C0` |
| 4 | `S90_Exit` | `0x004199C0` |
| 5 | `S90_Trog` | `0x004199C0` |
| 6 | `S90_Regale` | `0x004199C0` |
| 10 | `S90_KaminArea` | `0x004199C0` |
| 11 | `S90_CynTalk (Ebene)` | `0x004199C0` |

## Ereignis-Handler (Zusammenfassung je Fall)
```
# 00419c90.c FUN_00419c90 @ 00419c90
  Anfang: ruft FUN_00419c90
  case 1: Sprache 090_ROD_11 090_ROD_02 090_CYN_02 | ruft CGame_WalkAndComment CGame_HeroSayWait FUN_0041a300
  case 2: Sprache 090_ROD_12 090_CYN_12 090_ROD_16 090_ROD_13 090_CYN_13 090_ROD_14 090_CYN_14 090_ROD_15 090_CYN_15 090_ROD_03 090_CYN_03 | S= 3F8C=2 3F8C=3 | ruft CActor_WalkTo CGame_HeroSayWait FUN_00435ba0 FUN_0041a300 CActor_Turn CSzene_DisableInput CGame_PlayMusic FUN_00437fd0 FUN_00437560 FUN_004375b0 FUN_004406e0 CMusic_FadeIn CSzene_EnableInput
  case 3: Sprache 090_ROD_17 090_CYN_17 090_ROD_04 090_CYN_04 | S= 3F80=1 3F88=4 | ruft CActor_WalkTo CGame_HeroSayWait FUN_00435ba0 FUN_0041a300 CSzene_DisableInput CActor_Turn CGame_Delay CSzene_EnableInput
  case 4: Sprache 090_ROD_21 090_CYN_21 090_ROD_05 090_CYN_05 090_ROD_06 090_ROD_07 | → 12/91 | S= 3F88=5 3F7C=1 | ruft CActor_WalkTo CGame_HeroSayWait FUN_00440400 CSzene_DisableInput FUN_0041a300 CActor_Turn CSzene_EnableInput
  case 5: Sprache 090_ROD_22 090_ROD_08 090_CYN_08 | ruft CGame_WalkAndComment CGame_HeroSayWait FUN_0041a300
  case 6: Sprache 090_ROD_18 | ruft CGame_WalkAndComment
  case 10: Sprache 090_ROD_19 090_CYN_19 090_ROD_20 090_CYN_20 | S= 3F84=1 3F88=1 | S? 3F80 3F88 | ruft CActor_WalkTo CActor_Turn CGame_HeroSayWait FUN_0041a300
  case 11: Sprache 090_ROD_09 090_CYN_09 090_ROD_10 090_CYN_10 | S= 3F88=2 | ruft CActor_WalkTo CGame_HeroSayWait FUN_0041a300
```
