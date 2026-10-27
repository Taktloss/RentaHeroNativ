# Eingabe und Tastenkürzel des Originals

## Fensterprozedur `FUN_00402350` (WndProc, registriert in `FUN_00401e00`)

| Nachricht | Behandlung |
|---|---|
| `WM_KEYDOWN` (0x100) | `FUN_0044ce50(scene, vk)` (Spiel-Tastatur, noch offen) |
| `WM_SYSKEYDOWN` (0x104) mit `VK_RETURN` | `PostMessage(WM_COMMAND, 40080)` – Alt+Enter → „Fullscreen“; 40080 hat aber **keinen** Handler (siehe unten), wirkt also nicht |
| `WM_COMMAND` (0x111) | `FUN_00402870(LOWORD(wParam))` |
| `WM_SYSCOMMAND` (0x112) | `SC_SCREENSAVE` (0xF140) und `SC_MONITORPOWER` (0xF170) werden verschluckt, Rest an `DefWindowProc` |
| `WM_ACTIVATEAPP` (0x1C) | wParam 1/2 → `DAT_0047d6a8 = 1`, `FUN_00447e30`; sonst `DAT_0047d6a8 = 0`. Bei 0 ruft die Hauptschleife `WaitMessage()` → **Spiel pausiert ohne Fokus** (unter Wine beobachtet: Bild steht) |
| `WM_PAINT` (0xF) | nur wenn aktiv; Fenster-/Vollbild-Pfad über Renderer-vtable |
| `WM_MOUSEMOVE` (0x200) | `FUN_00402790(msg, x, y)` |
| `WM_LBUTTONDOWN/UP`, `WM_RBUTTONDOWN/UP` (0x201–0x204, 0x206) | `FUN_00402670(msg, x, y)` |
| `WM_INITMENUPOPUP` (0x117) | `FUN_00402f80` |
| `WM_DROPFILES` (0x233) | `FUN_00403030` (Editor-Rest) |
| `WM_DESTROY` (2) | `PostQuitMessage(0)` |

Am Disassembly geprüft (`0x00402379`–`0x0040239E`): Nachricht `0x1C` = `WM_ACTIVATEAPP`;
aktiv bei `wParam == 1 || wParam == 2`. `2` kommt bei WM_ACTIVATEAPP nie vor (vermutlich
Verwechslung mit `WM_ACTIVATE`/`WA_CLICKACTIVE`); für den Port gilt: aktiv ⇔ wParam ≠ 0.

## Befehlsverteiler `FUN_00402870`

Nur diese Befehls-IDs haben Code; alle übrigen Einträge des Editor-Menüs (`docs/EDITOR.md`)
sind im Release funktionslos.

| ID | Kürzel (Accelerator-Tabelle 110) | Wirkung |
|---:|---|---|
| 40079 (0x9C8F) | – (nur Editor-Menü „Start Game Menu“) | `DAT_00470808 = 1`, Spielmenü öffnen (`FUN_00403370`) |
| 40093 (0x9C9D) | **Ctrl+F1** | wie 40079: Menü-Modus 1 |
| 40094 (0x9C9E) | **Ctrl+F2** | Menü-Modus 2 (`DAT_00470808 = 2`); Unterschied zu Modus 1 UNVERIFIED |
| 40092 (0x9C9C) | – (nur Editor-Menü „Quit Game Menu“) | Spielmenü schließen (`FUN_00403480`) |
| 40081 (0x9C91) | **Ctrl+F6** | Musiklautstärke +10 |
| 40082 (0x9C92) | **Ctrl+F5** | Musiklautstärke −10 |
| 40083 (0x9C93) | **Ctrl+F7** | Effektlautstärke −10 |
| 40084 (0x9C94) | **Ctrl+F8** | Effektlautstärke +10 |
| 40085 (0x9C95) | **Ctrl+F9** | Sprachlautstärke −10 |
| 40086 (0x9C96) | **Ctrl+F10** | Sprachlautstärke +10 |
| 40087 (0x9C97) | **Ctrl+F3** | Maus-Verzögerung −200 ms |
| 40088 (0x9C98) | **Ctrl+F4** | Maus-Verzögerung +200 ms |
| 40090 (0x9C9A) | **Ctrl+F11** | Untertitel an (`DAT_00481f1c = 1`) |
| 40091 (0x9C9B) | **Ctrl+F12** | Untertitel aus |

Weitere Accelerator-Einträge ohne Handler: Ctrl+A (40075), Ctrl+R (40076), Ctrl+S (40068).
Die Accelerator-Tabelle wird in der Hauptschleife immer angewendet
(`TranslateAcceleratorA` in `FUN_00401e00`), die Kürzel gelten also im normalen Spiel.

Zuordnung der Lautstärke-Setter (aus `FUN_00404db0`, INI `[SOUNDS]`):
`FUN_00436a20` = Musik (Feld `+0x6C` des Audio-Objekts `0x0047BF10`),
`FUN_004369d0` = Effekte, `FUN_00436a70` = Sprache. Werte werden auf 0…100 begrenzt
(`FUN_00436a20`; die beiden anderen analog, UNVERIFIED).
`FUN_00436500`: Maus-Verzögerung `DAT_004764ac`, begrenzt auf 100…2000, INI-Default 250.

## Spielmenü (`FUN_00403370` / `FUN_00403480`)
Öffnen: nur wenn nicht schon offen (`DAT_0047d688`) und `DAT_0047d754 == 0`. Die aktuelle
Szene/Spielinstanz (`DAT_0047d264`/`DAT_0047d26c`) wird gesichert und durch die
Menü-Szene (`DAT_0047d268`/`DAT_0047d270`) ersetzt. Schließen stellt beides wieder her
und zeichnet neu.

## Laufzeitbeobachtung (Wine, 2026-10-27)
- Escape während des Intros überspringt die gesamte Intro-Sequenz; das Spiel landet
  direkt in Szene 7A (ein Testlauf).
- Tastendrücke über xdotool an den virtuellen Wine-Desktop kamen nicht an (Spiel
  pausiert, Bild unverändert) → Ctrl+F1/F2 zur Laufzeit noch ungeprüft.
