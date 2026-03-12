---
description: Implement logical UI coordinates for hit-testing and input
---

# UI Scaling - Phase 2: Logical UI Coordinates

**Cel:** Przygotować system współrzędnych UI pod przyszłe skalowanie bez zmiany zachowania.

## Scope
- Centralizacja mapowania ekran → logical UI space
- Przygotowanie hitboxów i input systemu
- Bez widocznych zmian dla użytkownika

## Kluczowe pliki
- `Source/engine/ui/coordinate_mapper.h/.cpp` - nowy system
- `Source/control/` - mouse input processing
- `Source/controls/` - controller input
- `Source/DiabloUI/` - UI tooltip system

## Problem do rozwiązania
- Input używa bezpośrednich współrzędnych ekranowych
- Tooltipy pozycjonują się w screen space
- Controller focus operuje na pikselach
- Brak centralnego punktu transformacji współrzędnych

## Docelowa architektura
```cpp
struct LogicalUiPoint {
    int x, y;  // Współrzędne w logical UI space
};

struct LogicalUiRect {
    int x, y, w, h;  // Prostokąt w logical UI space
};

class UiCoordinateMapper {
public:
    LogicalUiPoint ScreenToLogical(int screenX, int screenY) const;
    int LogicalToScreenX(int logicalX) const;
    int LogicalToScreenY(int logicalY) const;
    void TransformHitbox(const LogicalUiRect& logicalRect, ScreenRect& screenRect) const;
    void UpdateScale(float uiScale);  // Dla przyszłych etapów
};
```

## Miejsca do refaktoryzacji

### Input System
- Mouse click handling
- Mouse motion tracking
- Touch input (jeśli istnieje)
- Controller button mapping

### Tooltip System
- Tooltip positioning
- Tooltip anchoring to UI elements
- Tooltip bounds checking

### Controller Navigation
- Focus rectangle positioning
- Directional navigation calculations
- Button hitbox mapping

### UI Element Hit Testing
- Panel click detection
- Button hit testing
- Inventory slot selection
- Spell icon clicking

## Przykładowy refactor

### Przed:
```cpp
void OnMouseClick(int screenX, int screenY) {
    if (IsPointInRect(screenX, screenY, buttonRect)) {
        // Handle click
    }
}
```

### Po:
```cpp
void OnMouseClick(int screenX, int screenY) {
    auto logicalPoint = coordMapper.ScreenToLogical(screenX, screenY);
    if (IsPointInLogicalRect(logicalPoint.x, logicalPoint.y, buttonLogicalRect)) {
        // Handle click
    }
}
```

## Kryteria ukończenia
- [ ] Wszystkie UI input używa logical coordinates
- [ ] Tooltipy pozycjonują się w logical space
- [ ] Controller navigation używa logical space
- [ ] Centralny mapper dla transformacji współrzędnych
- [ ] `UiCoordinateMapper::UpdateScale()` przygotowany (ale nie używany)
- [ ] Kliknięcia działają identycznie jak wcześniej
- [ ] Tooltipy pojawiają się w tych samych miejscach
- [ ] Controller focus działa identycznie
- [ ] Inventory slot selection działa bez zmian
- [ ] Panel clicking działa bez zmian

## Ryzyka
- Performance - dodatkowe transformacje przy każdym input event
- Complexity - wiele miejsc używających współrzędnych
- Edge cases - boundary conditions przy transformacji
- Controller vs mouse - różne systemy muszą być spójne

## Czego NIE robić
- Nie dodawać jeszcze publicznego UI scale
- Nie zmieniać domyślnego zachowania
- Nie optymalizować przedwcześnie
- Nie mieszać inputu gry (movement) z UI input

## Szacowany czas: 2-3 dni

## Next steps
Implementacja po ukończeniu etapu 1.

## Integration z etapem 1
Po etapie 1 mamy: `RenderWorldFrame()`, `RenderUiFrame()`, etc.
Po etapie 2 dodajemy: `UiCoordinateMapper` dostępny w całym UI systemie.

To tworzy solidny fundament pod etap 3 (actual scaling).