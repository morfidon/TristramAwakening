# Etap 1 - Refactor: Separate World and UI Render Passes

**Cel:** Przygotować render path pod przyszłe niezwiązne skalowanie UI bez zmiany zachowania dla gracza.

**Scope:** Logiczny podział renderingu na osobne passes, bez nowych opcji, bez widocznych zmian.

---

## 1. Aktualna mapa renderingu (Source/engine/render/scrollrt.cpp)

### World Pass (bazowy świat)
- `DrawGame(...)`
- `DrawFloor(...)`
- `DrawTileContent(...)`
- `DrawOOB(...)`
- opcjonalnie `Zoom(...)`

**Uwaga:** World rendering już dziś jest panel-aware - zmniejsza viewport gdy lewy/prawy panel jest otwarty.

### World Overlay Pass (elementy na świecie)
- `DrawAutomap(...)`
- `DrawItemNameLabels(...)`
- `DrawMonsterHealthBar(...)`
- `DrawFloatingNumbers(...)`

**Charakter:** World-aware ale rysowane "na wierzchu" śwwiata, przed panelami.

### Panel/Modal UI Pass (okna i panele)
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

**Charakter:** Klasyczne panele UI, często używające `GetPanelPosition()` i `GetUIRectangle()`.

### Top Overlay/HUD Pass (najwyższe warstwy)
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

**Charakter:** Zawsze na wierzchu, często panel-aware.

### Cursor Pass (ostatnia warstwa)
- `DrawCursor(...)`
- `UndrawCursor(...)`

---

## 2. Docelowa struktura po refaktorze

```cpp
// Nowa struktura (bez zmiany behavior)
void RenderWorldFrame(...)
{
    DrawGame(...);
}

void RenderWorldOverlayFrame(...)
{
    DrawAutomap(...);
    DrawItemNameLabels(...);
    DrawMonsterHealthBar(...);
    DrawFloatingNumbers(...);
}

void RenderUiFrame(...)
{
    // Panele i okna modalne
    DrawSText(...);
    DrawInv(...);
    DrawSpellBook(...);
    DrawChr(...);
    DrawQuestLog(...);
    DrawStash(...);
    DrawQText(...);
    DrawSpellList(...);
    DrawGoldSplit(...);
    DrawGoldWithdraw(...);
    DrawHelp(...);
    DrawChatLog(...);
}

void RenderTopOverlayFrame(...)
{
    // Death/pause + diablo msg + hints + player messages
    // + menu + doom + info box + flasks
}

void RenderCursorFrame(...)
{
    DrawCursor(...);
}
```

**Ważne:** Wszystkie passes nadal rysują do tego samego targetu - nie tworzymy jeszcze osobnych surface'ów.

---

## 3. Kryteria ukończenia

### Techniczne
- [ ] Jedno miejsce orkiestrujące cały frame (central composition point)
- [ ] Logiczny podział na 5 passes
- [ ] Bez zmiany draw order
- [ ] Bez nowych ustawień/config keys

### Testowe
- [ ] Screenshoty before/after identyczne
- [ ] Brak regresji w: town, dungeon, inventory, character panel, spellbook, help/menu, store, automap, item labels, chat/console
- [ ] Cursor działa poprawnie
- [ ] Panel-aware rendering nadal działa (lewy/prawy panel wpływa na viewport)

---

## 4. Ryzyka i uwagi

### Największe ryzyka
- **Draw order** - przypadkowe zmiany kolejności
- **Cursor** - software/hardware cursor sync
- **Panel-aware elements** - `DrawPlrMsg` i world rendering muszą zachować panel awareness
- **Partial updates** - `DrawMain` i related blit/update pipeline

### Czego NIE robić
- Nie dodawać UI scale settings
- Nie zmieniać współrzędnych inputu (to etap 2)
- Nie tworzyć osobnych framebufferów (jeszcze nie)
- Nie mieszać zoomu śwwiata i UI
- Nie poprawiać "przy okazji" innych rzeczy

---

## 5. Plan implementacji

1. **Zmapowanie** - Potwierdź aktualny flow w `DrawView()`
2. **Eksportacja** - Wyciągnij world rendering do `RenderWorldFrame()`
3. **Eksportacja** - Wyciągnij world overlays do `RenderWorldOverlayFrame()`
4. **Eksportacja** - Wyciągnij UI panels do `RenderUiFrame()`
5. **Eksportacja** - Wyciągnij top overlays do `RenderTopOverlayFrame()`
6. **Orkiestracja** - Stwórz centralny composition point
7. **Testy** - Porównaj screenshoty i zachowanie

---

## 6. PR Description Template

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
- Keeps review small while enabling future UI scaling work
```

---

## 7. Next Steps

Po tym etapie:
- Etap 2: Logical UI coordinates dla input/hit-testing
- Etap 3: Internal UI scale factor (bez publicznego toggle)
- Etap 4: Limited public UI scale presets
- Etap 5: Polish edge cases (tooltipy, overlap, controller nav)