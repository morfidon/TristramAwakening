---
description: Refactor render path to separate world and UI passes
---

# UI Scaling - Phase 1: Render Path Refactor

**Cel:** Przygotować render path pod przyszłe skalowanie bez zmiany zachowania dla gracza.

## Scope
- Podzielenie renderingu na world/UI passes
- Bez widocznych zmian dla użytkownika
- Przygotowanie architektury pod future scaling

## Kluczowe pliki
- `Source/engine/render/scrollrt.cpp` - główny render pipeline
- Funkcje do refactorowania: `DrawGame()`, `DrawView()`, `DrawCursor()`

## Aktualna struktura
```
DrawGame() → DrawView() → [world → overlays → panels → HUD] → cursor
```

## Docelowa struktura
```
RenderWorldFrame() → RenderWorldOverlayFrame() → 
RenderUiFrame() → RenderTopOverlayFrame() → RenderCursorFrame()
```

## World Pass
- `DrawGame(...)`
- `DrawFloor(...)`
- `DrawTileContent(...)`
- `DrawOOB(...)`
- `Zoom(...)`

## World Overlay Pass
- `DrawAutomap(...)`
- `DrawItemNameLabels(...)`
- `DrawMonsterHealthBar(...)`
- `DrawFloatingNumbers(...)`

## Panel/Modal UI Pass
- `DrawSText(...)` - sklep/rozmowy
- `DrawInv(...)` - inventory
- `DrawSpellBook(...)` - księga zaklęć
- `DrawChr(...)` - character panel
- `DrawQuestLog(...)` - dziennik zadań
- `DrawStash(...)` - stash
- `DrawQText(...)` - quest text
- `DrawSpellList(...)` - lista zaklęć
- `DrawGoldSplit(...)` - dzielenie złota
- `DrawGoldWithdraw(...)` - wypłata złota
- `DrawHelp(...)` - pomoc
- `DrawChatLog(...)` - chat log

## Top Overlay/HUD Pass
- Death/Pause screens
- `DrawDiabloMsg(...)`
- `DrawControllerModifierHints(...)`
- `DrawPlrMsg(...)` - wiadomości gracza (panel-aware!)
- `gmenu_draw(...)` - menu gry
- `doom_draw(...)` - doom screen
- `DrawInfoBox(...)`
- `UpdateLifeManaPercent()`
- `DrawLifeFlaskUpper(...)`
- `DrawManaFlaskUpper(...)`

## Cursor Pass
- `DrawCursor(...)`
- `UndrawCursor(...)`

## Kryteria ukończenia
- [ ] Jedno miejsce orkiestrujące cały frame
- [ ] Logiczny podział na 5 passes
- [ ] Bez zmiany draw order
- [ ] Bez nowych ustawień/config keys
- [ ] Screenshoty before/after identyczne
- [ ] Brak regresji w town, dungeon, inventory, etc.

## Ryzyka
- Draw order changes
- Cursor sync issues
- Panel-aware elements breaking
- Partial updates interference

## Czego NIE robić
- Nie dodawać UI scale settings
- Nie zmieniać współrzędnych inputu (to etap 2)
- Nie tworzyć osobnych framebufferów (jeszcze nie)
- Nie mieszać zoomu świata i UI

## Szacowany czas: 2-3 dni

## Next steps
Dyskusja z maintainerami, potem implementacja etapu 1.

## PR Description Template
```
Summary: Refactor frame rendering to separate world and UI passes
No intended behavior change - preparatory step for future UI scaling

What changed:
- Grouped world rendering into dedicated pass
- Grouped world overlays into dedicated pass  
- Grouped UI panels into dedicated pass
- Grouped top overlays into dedicated pass
- Centralized frame composition order

Why:
- Issue #8300 shows real readability problems at higher resolutions
- Issue #8388 discusses independent scaling as long-term direction
- This PR intentionally limits scope to render-path preparation
```