---
description: Polish edge cases and finalize UI scaling implementation
---

# UI Scaling - Phase 5: Polish Edge Cases

**Cel:** Dopieścić szczegóły UI scaling, które wychodzą dopiero po realnym użyciu.

## Scope
- Tooltip positioning i bounds checking
- Controller navigation fixes
- Dynamic layout adaptation
- Specific screen fixes
- Edge case handling

## Kluczowe pliki
- `Source/engine/ui/tooltip_system.h/.cpp` - tooltip overhaul
- `Source/engine/ui/layout_adapter.h/.cpp` - dynamic layouts
- `Source/controls/controller_navigation.cpp` - navigation fixes
- Specific screen files (stores, spellbook, inventory)

## Najważniejsze problemy do naprawy
- Tooltipy wychodzące poza ekran
- Panele nachodzą na siebie przy dużym scale
- Controller focus rectangle nie skaluje się poprawnie
- Text nie mieści się w przyciskach
- Layout breaking w konkretnych ekranach

## Docelowe systemy
```cpp
class TooltipManager {
    Point GetBestTooltipPosition(const Size& tooltipSize, Point preferredPos, Anchor anchor) const;
    bool IsPositionValid(Point pos, const Size& size) const;
    Point AdjustPositionForBounds(Point pos, const Size& size) const;
};

class LayoutAdapter {
    static Rect AdaptPanelRect(const Rect& baseRect, UiScalePreset scale);
    static Size AdaptButtonSize(const Size& baseSize, UiScalePreset scale);
    static int AdaptFontSize(int baseSize, UiScalePreset scale);
};
```

## Edge cases
- Very small screens (< 1024x768)
- Ultrawide screens (aspect ratio > 2.0)
- Dynamic resolution change
- Windowed mode resize

## Kryteria ukończenia
- [ ] Tooltipy nigdy nie wychodzą poza ekran
- [ ] Panele automatycznie dostosowują się do dużych skal
- [ ] Controller navigation działa poprawnie
- [ ] Wszystkie ekrany są użyteczne przy wszystkich dostępnych presetach
- [ ] Stable FPS przy wszystkich presetach

## Szacowany czas: 4-5 dni

## Next steps
Implementacja po ukończeniu etapu 4.

## Project completion
Po tym etapie cały UI scaling feature jest kompletny i gotowy do production use.