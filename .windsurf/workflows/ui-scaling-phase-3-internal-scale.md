---
description: Implement internal UI scale factor with clamping and anchoring
---

# UI Scaling - Phase 3: Internal UI Scale Factor

**Cel:** Wprowadzić mechanizm skalowania UI z bezpiecznym clampingiem, jeszcze bez publicznego toggle.

## Scope
- Dodanie scale factor z clampingiem (0.8-2.0)
- Anchoring system dla pozycjonowania
- Compositing world + scaled UI
- Config-only (developerska opcja)

## Kluczowe pliki
- `Source/engine/ui/ui_scale_manager.h/.cpp` - główny system skalowania
- `Source/engine/ui/coordinate_mapper.cpp` - integracja z coordinate mapperem
- Render pipeline integration

## Problem do rozwiązania
- Potrzebujemy rzeczywistego mechanizmu skalowania UI
- Bezpieczne limity (clamping)
- Anchoring system dla pozycjonowania
- Compositing world + scaled UI

## Docelowa architektura
```cpp
class UiScaleManager {
private:
    float currentScale_ = 1.0f;
    float minScale_ = 0.8f;      // Safe minimum
    float maxScale_ = 2.0f;      // Safe maximum
    
    enum class Anchor {
        TopLeft, TopCenter, TopRight,
        CenterLeft, Center, CenterRight,
        BottomLeft, BottomCenter, BottomRight
    };
    
public:
    void SetScale(float scale);
    float GetScale() const { return currentScale_; }
    float ClampScale(float scale) const;
    bool IsScaleValid(float scale) const;
    Point GetAnchoredPosition(Anchor anchor, const Size& elementSize, const Rect& container) const;
    Rect ScaleRect(const Rect& logicalRect) const;
    Point ScalePoint(const Point& logicalPoint) const;
    Size ScaleSize(const Size& logicalSize) const;
};
```

## Render Pipeline Integration
```cpp
void RenderFrame() {
    // 1. Render world (bez zmian)
    RenderWorldFrame();
    
    // 2. Przygotuj scaled UI surface
    if (uiScaleManager.GetScale() != 1.0f) {
        RenderScaledUiFrame();
    } else {
        RenderUiFrame();  // Normal path
    }
    
    // 3. Composite
    CompositeFinalFrame();
}

void RenderScaledUiFrame() {
    auto uiSurface = CreateUiSurface();
    RenderUiFrameToSurface(uiSurface);
    ScaleAndCompositeUi(uiSurface);
}
```

## Konfiguracja (developerska)
```ini
[Development]
ui_scale_factor = 1.2
```

## Debug commands
```cpp
void DebugSetUiScale(float scale) {
    if (IsDebugMode()) {
        uiScaleManager.SetScale(scale);
    }
}
```

## Kryteria ukończenia
- [ ] `UiScaleManager` z clampingiem (0.8 - 2.0)
- [ ] Scaled UI render path działa
- [ ] Anchoring system dla paneli
- [ ] Coordinate mapper uwzględnia scale
- [ ] Config-only option (nie w UI settings)
- [ ] UI poprawnie się skaluje przy różnych wartościach
- [ ] Input działa poprawnie po skalowaniu
- [ ] Tooltipy pozycjonują się poprawnie
- [ ] Controller navigation działa
- [ ] Panele nie wychodzą poza ekran (anchoring)
- [ ] Cursor działa poprawnie
- [ ] Scale clamping działa
- [ ] Edge case (bardzo mały/duży ekran) handled
- [ ] Performance nie spada drastycznie
- [ ] Memory usage w kontrolach

## Ryzyka
- Performance - dodatkowy render target i scale operation
- Memory - temporary surface dla scaled UI
- Quality - scaling artifacts (pixelated vs blurry)
- Layout breaking - elementy mogą nie mieścić się przy skalowaniu

## Czego NIE robić
- Nie eksponować jeszcze publicznego UI scale slider
- Nie dodawać zbyt wielu presetów
- Nie zmieniać domyślnego scale (nadal 1.0)
- Nie optymalizować przedwcześnie

## Szacowany czas: 3-4 dni

## Next steps
Implementacja po ukończeniu etapu 2.

## Integration z poprzednimi etapami
- Etap 1: Render path separated ✓
- Etap 2: Logical coordinates ready ✓
- Etap 3: Dodajemy actual scaling mechanism