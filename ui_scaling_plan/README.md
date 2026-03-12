# DevilutionX UI Scaling Project

Plan implementacji niezależnego skalowania UI dla DevilutionX - rozwiązanie problemu czytelności interfejsu na wysokich rozdzielczościach (issue #8300).

## Problem

Gracze na wysokich rozdzielczościach zgłaszają, że UI staje się zbyt małe i trudne do odczytania. Obecny system nie pozwala na niezależne skalowanie interfejsu względem świata gry.

## Cel strategiczny

Wprowadzić niezależne skalowanie UI i świata gry zgodnie z kierunkiem z issue #8388, ale w kontrolowany sposób przez serię małych PR-ów.

## Plan etapowy

### Etap 1: Refactor render path
**Cel:** Przygotowanie architektury pod przyszłe skalowanie
- Podzielenie renderingu na world/UI passes
- Bez widocznych zmian dla użytkownika
- [Szczegóły](etap_1_render_path_refactor.md)

### Etap 2: Logical UI coordinates
**Cel:** Przygotowanie systemu współrzędnych pod skalowanie
- Centralizacja mapowania ekran → logical UI space
- Przygotowanie hitboxów i input systemu
- [Szczegóły](etap_2_logical_ui_coordinates.md)

### Etap 3: Internal UI scale factor
**Cel:** Wprowadzenie mechanizmu skalowania
- Scale factor z clampingiem (0.8-2.0)
- Anchoring system
- Config-only (developerska opcja)
- [Szczegóły](etap_3_internal_ui_scale_factor.md)

### Etap 4: Public UI scale presets
**Cel:** Eksponowanie funkcji dla użytkowników
- 3 bezpieczne presety: Normal/Large/Larger
- Safety validation dla rozdzielczości
- Integration z options menu
- [Szczegóły](etap_4_public_ui_scale_presets.md)

### Etap 5: Polish edge cases
**Cel:** Dopieszczenie szczegółów UX
- Tooltip positioning
- Controller navigation
- Specific screen fixes
- Edge case handling
- [Szczegóły](etap_5_polish_edge_cases.md)

## Dlaczego etapowe podejście?

1. **Małe ryzyko** - Każdy PR jest reviewable i łatwy do cofnięcia
2. **Kontrola maintainerów** - Możliwość zatwierdzenia kierunku krok po kroku
3. **Testowanie** - Każdy etap może być przetestowany niezależnie
4. **Architektura** - Stopniowe budowanie solidnych fundamentów

## Kluczowe pliki do analizy

- `Source/engine/render/scrollrt.cpp` - główny render pipeline
- `Source/DiabloUI/` - panele i interfejs
- `Source/controls/` - controller navigation
- `Source/control/` - input handling

## Integration z istniejącym kodem

### Aktualna struktura renderingu
```
DrawGame() → DrawView() → [world → overlays → panels → HUD]
```

### Docelowa struktura
```
RenderWorldFrame() → RenderWorldOverlayFrame() → 
RenderUiFrame() → RenderTopOverlayFrame() → RenderCursorFrame()
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