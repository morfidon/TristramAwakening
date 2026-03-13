---
description: Plan implementacji niezależnego skalowania UI dla DevilutionX
---

# DevilutionX UI Scaling Project

Plan implementacji niezależnego skalowania UI dla DevilutionX - rozwiązanie problemu czytelności interfejsu na wysokich rozdzielczościach (issue #8300).

## Problem

Gracze na wysokich rozdzielczościach zgłaszają, że UI staje się zbyt małe i trudne do odczytania. Obecny system nie pozwala na niezależne skalowanie interfejsu względem świata gry.

## Cel strategiczny

Wprowadzić niezależne skalowanie UI i świata gry zgodnie z kierunkiem z issue #8388, ale w kontrolowany sposób przez serię małych PR-ów.

## Plan etapowy

### Etap 0: Local render pipeline verification
**Cel:** Przygotowanie prywatnego narzędzia weryfikacyjnego
- Lokalny mechanizm logowania render pipeline event order
- Weryfikacja że refactoring nie zmienia kolejności renderowania
- Prywatne narzędzie - nie wchodzi do upstream
- Szacowany czas: 0.5 dnia

### Etap 1: Refactor render path
**Cel:** Podzielenie renderingu na logiczne warstwy bez zmiany zachowania
- Wydzielenie helperów: `RenderWorldPass()`, `RenderWorldOverlays()`, `RenderWindowPanels()`, `RenderTopOverlays()`, `RenderMainHudPanel()`
- Zachowanie dokładnej kolejności draw calls z `DrawView()` i `DrawAndBlit()`
- Bez widocznych zmian dla użytkownika, bez zmiany inputu, surface'ów ani render targetów
- Szacowany czas: 2-3 dni

### Etap 2: Logical UI coordinates
**Cel:** Przygotowanie systemu współrzędnych pod skalowanie
- Prepare UI-space mapping
- UI coordinate mapping + UI hit-testing prep
- Exact behavior TBD after maintainer feedback
- Szacowany czas: 2-3 dni

### Etap 3: Internal UI scale factor
**Cel:** Wprowadzenie mechanizmu skalowania
- Introduce internal scale plumbing
- Internal UI scale factor with clamping
- Exact bounds TBD based on layout fit and maintainer feedback
- Anchoring system (exact behavior TBD)
- Config-only (developerska opcja)
- Szacowany czas: 3-4 dni

### Etap 4: Public UI scale presets
**Cel:** Eksponowanie funkcji dla użytkowników
- Expose a safe user-facing UI scaling control
- Safe user-facing presets (exact preset count and allowed scale values TBD based on layout fit)
- Safety validation dla rozdzielczości
- Integration z options menu
- Szacowany czas: 2-3 dni

### Etap 5: Polish edge cases
**Cel:** Dopieszczenie szczegółów UX
- Tooltip positioning
- Controller navigation
- Specific screen fixes
- Edge case handling
- Szacowany czas: 4-5 dni

### Etap 6: World zoom / perspective policy discussion
**Cel:** Oddzielenie dyskusji o world zoom od UI scaling
- Separate discussion from UI scaling
- Decide whether #8388 balance proposals are desired
- Only then plan implementation
- Szacowany czas: 1-2 dni (discussion only)

## Dlaczego etapowe podejście?

1. **Małe ryzyko** - Każdy PR jest reviewable i łatwy do cofnięcia
2. **Kontrola maintainerów** - Możliwość zatwierdzenia kierunku krok po kroku
3. **Testowanie** - Każdy etap może być przetestowany niezależnie
4. **Architektura** - Stopniowe budowanie solidnych fundamentów

## Kluczowe pliki do analizy

- `Source/engine/render/scrollrt.cpp` - centrum etapów 1-3 (render path refactor, coordinates, scale plumbing)
- `Source/DiabloUI/` - panele i interfejs (głównie od etapu 4 wzwyż)
- `Source/panels/` - panele UI (głównie od etapu 4 wzwyż)
- `Source/controls/` - controller navigation (tylko gdy realnie wejdziesz w controller navigation)
- `Source/control/` - input handling

## Integration z istniejącym kodem

### Aktualna struktura renderingu
```
DrawGame() → DrawView() → [world → overlays → panels → HUD]
```

### Docelowa struktura
```
DrawAndBlit() 
  → setup & UndrawCursor 
  → DrawView() [RenderWorldPass → RenderWorldOverlays → RenderWindowPanels → RenderTopOverlays] 
  → RenderMainHudPanel() 
  → DrawCursor() 
  → DrawMain() & RenderPresent()
```

## Safety considerations

### Clamping i validation
- UI scale limited do 0.8-2.0 range
- Resolution-based safety checks
- Automatic fallback dla niekompatybilnych setupów

### Performance
- Cache dla scaled layouts
- Minimalny overhead przy normalnym scale (1.0)
- Efficient render target management

## Test plan

### Standard resolutions
- 800x600: tylko Normal
- 1024x768: Normal + Large  
- 1920x1080: wszystkie presety
- 4K: wszystkie presety

### Edge cases
- Ultrawide screens
- Very small screens
- Dynamic resolution change
- Windowed mode resize

## Next steps

1. **Dyskusja z maintainerami** - komentarz pod issue #8388 z planem etapowym
2. **Etap 1 implementation** - refactor render path bez zmiany behavior
3. **Validation** - testy i screenshoty before/after
4. **Kolejne etapy** - w zależności od feedbacku

## Expected outcome

Po ukończeniu projektu:
- Gracze będą mogli powiększyć UI dla lepszej czytelności
- System będzie bezpieczny i stabilny
- Architektura będzie gotowa na przyszłe rozszerzenia
- DevilutionX będzie bardziej accessible

---

**Status:** Planowanie - czekamy na feedback maintainerów