# Etap 2 - Refactor: Logical UI Coordinates for Hit-Testing

**Cel:** Przygotować system współrzędnych UI pod przyszłe skalowanie bez zmiany zachowania.

**Scope:** Centralizacja mapowania ekran → logical UI space, przygotowanie hitboxów.

---

## 1. Problem do rozwiązania

### Aktualny stan
- Input używa bezpośrednich współrzędnych ekranowych
- Tooltipy pozycjonują się w screen space
- Controller focus operuje na pikselach
- Brak centralnego punktu transformacji współrzędnych

### Problem po przyszłym skalowaniu UI
- Kliknięcia będą niepoprawne po powiększeniu UI
- Tooltipy pojawią się w złych miejscach  
- Controller navigation będzie się gubić
- Hitboxy nie będć skalować razem z UI

---

## 2. Docelowa architektura

### Logical UI Space
```cpp
// Nowy system współrzędnych
struct LogicalUiPoint {
    int x, y;  // Współrzędne w logical UI space (nie screen space)
};

struct LogicalUiRect {
    int x, y, w, h;  // Prostokąt w logical UI space
};

class UiCoordinateMapper {
public:
    // Screen → Logical UI
    LogicalUiPoint ScreenToLogical(int screenX, int screenY) const;
    
    // Logical UI → Screen  
    int LogicalToScreenX(int logicalX) const;
    int LogicalToScreenY(int logicalY) const;
    
    // Transformacja hitboxów
    void TransformHitbox(const LogicalUiRect& logicalRect, /* out */ ScreenRect& screenRect) const;
    
    // Aktualizacja przy zmianie scale (dla przyszłych etapów)
    void UpdateScale(float uiScale);
};
```

### Miejsca do refaktoryzacji

#### Input System
- Mouse click handling
- Mouse motion tracking  
- Touch input (jeśli istnieje)
- Controller button mapping

#### Tooltip System  
- Tooltip positioning
- Tooltip anchoring to UI elements
- Tooltip bounds checking

#### Controller Navigation
- Focus rectangle positioning
- Directional navigation calculations
- Button hitbox mapping

#### UI Element Hit Testing
- Panel click detection
- Button hit testing
- Inventory slot selection
- Spell icon clicking

---

## 3. Implementacja krok po kroku

### Krok 1: Dodaj UiCoordinateMapper
```cpp
// Source/engine/ui/coordinate_mapper.h
class UiCoordinateMapper {
private:
    float currentUiScale_ = 1.0f;  // Na razie zawsze 1.0
    
public:
    LogicalUiPoint ScreenToLogical(int screenX, int screenY) const;
    int LogicalToScreenX(int logicalX) const;
    int LogicalToScreenY(int logicalY) const;
    void UpdateScale(float scale);  // Dla przyszłych etapów
};
```

### Krok 2: Zidentyfikuj wszystkie miejsca użycia współrzędnych UI

#### Input handling
- `Source/control/` - mouse input processing
- `Source/controls/` - controller input
- Event handlers w panelach

#### Tooltip positioning  
- `Source/DiabloUI/` - UI tooltip system
- Inventory tooltip handlers
- Spell tooltip handlers

#### Controller navigation
- Focus management w `Source/controls/`
- Directional navigation logic

#### Hit testing
- Panel click detection w `Source/DiabloUI/`
- Inventory slot selection
- Button click handlers

### Krok 3: Refactor input system

#### Przed:
```cpp
void OnMouseClick(int screenX, int screenY) {
    if (IsPointInRect(screenX, screenY, buttonRect)) {
        // Handle click
    }
}
```

#### Po:
```cpp
void OnMouseClick(int screenX, int screenY) {
    auto logicalPoint = coordMapper.ScreenToLogical(screenX, screenY);
    if (IsPointInLogicalRect(logicalPoint.x, logicalPoint.y, buttonLogicalRect)) {
        // Handle click
    }
}
```

### Krok 4: Refactor tooltip system

#### Przed:
```cpp
void ShowTooltip(int screenX, int screenY, const char* text) {
    tooltipRect.x = screenX + 10;
    tooltipRect.y = screenY - 20;
}
```

#### Po:
```cpp
void ShowTooltip(int screenX, int screenY, const char* text) {
    auto logicalPoint = coordMapper.ScreenToLogical(screenX, screenY);
    tooltipLogicalRect.x = logicalPoint.x + 10;
    tooltipLogicalRect.y = logicalPoint.y - 20;
    
    // Transform do screen space przy renderowaniu
    tooltipScreenRect = coordMapper.LogicalToScreen(tooltipLogicalRect);
}
```

---

## 4. Kryteria ukończenia

### Techniczne
- [ ] Wszystkie UI input używa logical coordinates
- [ ] Tooltipy pozycjonują się w logical space
- [ ] Controller navigation używa logical space
- [ ] Centralny mapper dla transformacji współrzędnych
- [ ] `UiCoordinateMapper::UpdateScale()` przygotowany (ale nie używany)

### Testowe
- [ ] Kliknięcia działają identycznie jak wcześniej
- [ ] Tooltipy pojawiają się w tych samych miejscach
- [ ] Controller focus działa identycznie
- [ ] Inventory slot selection działa bez zmian
- [ ] Panel clicking działa bez zmian

### Brak regresji
- [ ] Mouse input w grze (movement, attacks)
- [ ] Interface navigation
- [ ] Inventory management
- [ ] Spell casting
- [ ] Store interactions

---

## 5. Ryzyka i uwagi

### Największe ryzyka
- **Performance** - dodatkowe transformacje przy każdym input event
- **Complexity** - wiele miejsc używających współrzędnych
- **Edge cases** - boundary conditions przy transformacji
- **Controller vs mouse** - różne systemy muszą być spójne

### Czego NIE robić
- Nie dodawać jeszcze publicznego UI scale
- Nie zmieniać domyślnego zachowania
- Nie optymalizować przedwcześnie
- Nie mieszać inputu gry (movement) z UI input

---

## 6. Plan testowy

### Manual test checklist
- [ ] Klikanie przycisków w menu
- [ ] Klikanie ikon w spellbook
- [ ] Inventory slot selection
- [ ] Store item clicking
- [ ] Character panel navigation
- [ ] Tooltip positioning nad wszystkimi elementami
- [ ] Controller navigation między elementami
- [ ] Mouse hover effects

### Automated test areas
- [ ] Unit testy dla `UiCoordinateMapper`
- [ ] Hit testing accuracy tests
- [ ] Coordinate transformation round-trip tests

---

## 7. Integration z etapem 1

Po etapie 1 mamy:
- `RenderWorldFrame()`, `RenderUiFrame()`, etc.

Po etapie 2 dodajemy:
- `UiCoordinateMapper` dostępny w całym UI systemie
- Wszystkie input/tooltip/hitbox używają logical coordinates

To tworzy solidny fundament pod etap 3 (actual scaling).

---

## 8. Next Steps

Po tym etapie:
- Etap 3: Internal UI scale factor z realnym skalowaniem
- Etap 4: Publiczne UI scale presets  
- Etap 5: Polish edge cases i controller navigation

Etap 2 jest kluczowy - bez niego skalowanie UI zepsuje input system.