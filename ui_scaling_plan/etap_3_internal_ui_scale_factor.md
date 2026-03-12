# Etap 3 - Refactor: Internal UI Scale Factor

**Cel:** Wprowadzić mechanizm skalowania UI z bezpiecznym clampingiem, jeszcze bez publicznego toggle.

**Scope:** Dodanie scale factor, anchoring, compositing - opcja developerska/config-only.

---

## 1. Problem do rozwiązania

### Dotychczasowe etapy
- **Etap 1:** Podzieliliśmy render path na world/UI passes
- **Etap 2:** Przygotowaliśmy logical UI coordinates dla inputu

### Co teraz potrzebujemy
- Rzeczywisty mechanizm skalowania UI
- Bezpieczne limity (clamping)
- Anchoring system dla pozycjonowania
- Compositing world + scaled UI

---

## 2. Docelowa architektura

### UI Scale Manager
```cpp
// Source/engine/ui/ui_scale_manager.h
class UiScaleManager {
private:
    float currentScale_ = 1.0f;
    float minScale_ = 0.8f;      // Safe minimum
    float maxScale_ = 2.0f;      // Safe maximum
    
    // Anchoring system
    enum class Anchor {
        TopLeft,
        TopCenter,
        TopRight,
        CenterLeft,
        Center,
        CenterRight,
        BottomLeft,
        BottomCenter,
        BottomRight
    };
    
public:
    // Scale management
    void SetScale(float scale);
    float GetScale() const { return currentScale_; }
    
    // Clamping
    float ClampScale(float scale) const;
    bool IsScaleValid(float scale) const;
    
    // Anchoring
    Point GetAnchoredPosition(Anchor anchor, const Size& elementSize, const Rect& container) const;
    
    // Transformacje
    Rect ScaleRect(const Rect& logicalRect) const;
    Point ScalePoint(const Point& logicalPoint) const;
    Size ScaleSize(const Size& logicalSize) const;
};
```

### Render Pipeline Integration
```cpp
// W głównym frame render
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
    // Create temporary render target for scaled UI
    auto uiSurface = CreateUiSurface();
    
    // Render UI do logical space
    RenderUiFrameToSurface(uiSurface);
    
    // Scale i composite
    ScaleAndCompositeUi(uiSurface);
}
```

---

## 3. Implementacja krok po kroku

### Krok 1: Dodaj UiScaleManager
```cpp
// Source/engine/ui/ui_scale_manager.cpp
UiScaleManager::UiScaleManager() {
    // Load from config (developerska opcja)
    currentScale_ = LoadConfigFloat("ui_scale_factor", 1.0f);
    currentScale_ = ClampScale(currentScale_);
}

float UiScaleManager::ClampScale(float scale) const {
    return std::clamp(scale, minScale_, maxScale_);
}

bool UiScaleManager::IsScaleValid(float scale) const {
    return scale >= minScale_ && scale <= maxScale_;
}

Point UiScaleManager::GetAnchoredPosition(Anchor anchor, const Size& elementSize, const Rect& container) const {
    Point pos;
    
    switch (anchor) {
        case Anchor::TopLeft:
            pos = {container.x, container.y};
            break;
        case Anchor::TopCenter:
            pos = {container.x + (container.w - elementSize.w) / 2, container.y};
            break;
        case Anchor::TopRight:
            pos = {container.x + container.w - elementSize.w, container.y};
            break;
        // ... pozostałe anchory
    }
    
    return ScalePoint(pos);
}
```

### Krok 2: Zintegruj z coordinate mapperem
```cpp
// Rozszerz UiCoordinateMapper z etapu 2
class UiCoordinateMapper {
private:
    const UiScaleManager* scaleManager_;
    
public:
    LogicalUiPoint ScreenToLogical(int screenX, int screenY) const {
        // Reverse transform przez scale
        float invScale = 1.0f / scaleManager_->GetScale();
        return {
            static_cast<int>(screenX * invScale),
            static_cast<int>(screenY * invScale)
        };
    }
    
    int LogicalToScreenX(int logicalX) const {
        return static_cast<int>(logicalX * scaleManager_->GetScale());
    }
    
    int LogicalToScreenY(int logicalY) const {
        return static_cast<int>(logicalY * scaleManager_->GetScale());
    }
};
```

### Krok 3: Zmodyfikuj render pipeline

#### Przed (etap 1):
```cpp
void RenderFrame() {
    RenderWorldFrame();
    RenderUiFrame();
    RenderTopOverlayFrame();
    RenderCursorFrame();
}
```

#### Po (etap 3):
```cpp
void RenderFrame() {
    RenderWorldFrame();
    
    if (uiScaleManager.GetScale() != 1.0f) {
        RenderScaledUiFrame();
    } else {
        RenderUiFrame();
        RenderTopOverlayFrame();
    }
    
    RenderCursorFrame();
}

void RenderScaledUiFrame() {
    // Stwórz temporary surface
    auto uiTarget = CreateScaledUiSurface();
    
    // Render UI do logical space na temporary surface
    SetRenderTarget(uiTarget);
    RenderUiFrame();
    RenderTopOverlayFrame();
    SetRenderTarget(mainTarget);
    
    // Scale i composite na główny target
    DrawScaledSurface(uiTarget);
}
```

### Krok 4: Dodaj anchoring dla paneli

#### Przed:
```cpp
void DrawInv() {
    // Fixed position
    DrawPanelAt(INVENTORY_PANEL_X, INVENTORY_PANEL_Y);
}
```

#### Po:
```cpp
void DrawInv() {
    auto anchoredPos = uiScaleManager.GetAnchoredPosition(
        UiScaleManager::Anchor::TopRight,
        GetInventorySize(),
        GetScreenRect()
    );
    
    DrawPanelAt(anchoredPos.x, anchoredPos.y);
}
```

---

## 4. Konfiguracja (developerska)

### Config key
```ini
# W devilutionx.ini (developerska sekcja)
[Development]
ui_scale_factor = 1.2
```

### Runtime toggle (debug)
```cpp
// Debug commands dla testowania
void DebugSetUiScale(float scale) {
    if (IsDebugMode()) {
        uiScaleManager.SetScale(scale);
    }
}
```

---

## 5. Kryteria ukończenia

### Techniczne
- [ ] `UiScaleManager` z clampingiem (0.8 - 2.0)
- [ ] Scaled UI render path działa
- [ ] Anchoring system dla paneli
- [ ] Coordinate mapper uwzględnia scale
- [ ] Config-only option (nie w UI settings)

### Testowe
- [ ] UI poprawnie się skaluje przy różnych wartościach
- [ ] Input działa poprawnie po skalowaniu
- [ ] Tooltipy pozycjonują się poprawnie
- [ ] Controller navigation działa
- [ ] Panele nie wychodzą poza ekran (anchoring)
- [ ] Cursor działa poprawnie

### Bezpieczeństwo
- [ ] Scale clamping działa
- [ ] Edge case (bardzo mały/duży ekran) handled
- [ ] Performance nie spada drastycznie
- [ ] Memory usage w kontrolach

---

## 6. Ryzyka i uwagi

### Największe ryzyka
- **Performance** - dodatkowy render target i scale operation
- **Memory** - temporary surface dla scaled UI
- **Quality** - scaling artifacts (pixelated vs blurry)
- **Layout breaking** - elementy mogą nie mieścić się przy skalowaniu

### Czego NIE robić
- Nie eksponować jeszcze publicznego UI scale slider
- Nie dodawać zbyt wielu presetów
- Nie zmieniać domyślnego scale (nadal 1.0)
- Nie optymalizować przedwcześnie

---

## 7. Plan testowy

### Manual test checklist
- [ ] Test różnych scale values (0.8, 1.2, 1.5, 2.0)
- [ ] Panel positioning przy różnych scale
- [ ] Input accuracy przy skalowanym UI
- [ ] Tooltip positioning
- [ ] Controller navigation
- [ ] Inventory management
- [ ] Store interactions
- [ ] Performance check (FPS)

### Edge cases
- [ ] Bardzo mały ekran (640x480)
- [ ] Bardzo duży ekran (4K)
- [ ] Ultrawide aspect ratio
- [ ] Windowed vs fullscreen

---

## 8. Integration z poprzednimi etapami

### Etap 1 ✓
- Render path separated

### Etap 2 ✓  
- Logical coordinates ready

### Etap 3 (teraz)
- Dodajemy actual scaling mechanism
- Integrujemy z coordinate mapperem
- Dodajemy anchoring system

---

## 9. Next Steps

Po tym etapie:
- Etap 4: Publiczne UI scale presets
- Etap 5: Polish edge cases

Etap 3 jest kluczowy - to tutaj faktycznie wprowadzamy skalowanie, ale w kontrolowany sposób.