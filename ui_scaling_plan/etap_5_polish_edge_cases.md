# Etap 5 - Polish: Fix Scaled UI Layout and Navigation Edge Cases

**Cel:** Dopieścić szczegóły UI scaling, które wychodzą dopiero po realnym użyciu.

**Scope:** Tooltipy, overlapy, controller navigation, konkretne ekrany, edge cases.

---

## 1. Problem do rozwiązania

### Dotychczasowe etapy
- **Etap 1-4:** Podstawowy mechanizm UI scaling działa

### Co teraz potrzebujemy
- Naprawić problemy, które wychodzą dopiero przy rzeczywistym użyciu
- Dopieścić szczegóły UX
- Upewnić się, że wszystko działa przy wszystkich presetach

---

## 2. Najczęstsze problemy po włączeniu scalingu

### Tooltip positioning
- Tooltipy wychodzą poza ekran
- Tooltipy nakładają się na UI elements
- Tooltip positioning nie uwzględnia scale

### UI element overlapping
- Panele nachodzą na siebie przy dużym scale
- Text nie mieści się w przyciskach
- Icons są ucinane

### Controller navigation
- Focus rectangle nie skaluje się poprawnie
- Directional navigation działa dziwnie
- Button hitboxy nie pokrywają się z visual elements

### Specific screens
- Store screens mają layout breaking
- Spell book icons nie mieszczą się
- Inventory grid alignment
- Character panel stats positioning

---

## 3. Implementacja krok po kroku

### Krok 1: Tooltip system overhaul
```cpp
// Source/engine/ui/tooltip_system.h
class TooltipManager {
private:
    struct Tooltip {
        std::string text;
        Point logicalPosition;
        Size logicalSize;
        UiScaleManager::Anchor anchor = UiScaleManager::Anchor::TopRight;
    };
    
    std::vector<Tooltip> activeTooltips_;
    
public:
    void ShowTooltip(const std::string& text, Point screenPos, UiScaleManager::Anchor preferredAnchor = UiScaleManager::Anchor::TopRight);
    void UpdateAndRender();
    
private:
    Point GetBestTooltipPosition(const Size& tooltipSize, Point preferredPos, UiScaleManager::Anchor anchor) const;
    bool IsPositionValid(Point pos, const Size& size) const;
    Point AdjustPositionForBounds(Point pos, const Size& size) const;
};

Point TooltipManager::GetBestTooltipPosition(const Size& tooltipSize, Point preferredPos, UiScaleManager::Anchor anchor) {
    // Start z preferred position
    Point pos = uiScaleManager.GetAnchoredPosition(anchor, tooltipSize, GetScreenRect());
    
    // Adjust żeby nie wyjść poza ekran
    if (!IsPositionValid(pos, tooltipSize)) {
        pos = AdjustPositionForBounds(pos, tooltipSize);
    }
    
    return pos;
}

bool TooltipManager::IsPositionValid(Point pos, const Size& size) const {
    Rect screenRect = GetScreenRect();
    Rect tooltipRect = {pos.x, pos.y, size.w, size.h};
    
    return screenRect.contains(tooltipRect);
}

Point TooltipManager::AdjustPositionForBounds(Point pos, const Size& size) const {
    Rect screenRect = GetScreenRect();
    
    // Horizontal adjustment
    if (pos.x < screenRect.x) {
        pos.x = screenRect.x + 5;  // Small margin
    } else if (pos.x + size.w > screenRect.x + screenRect.w) {
        pos.x = screenRect.x + screenRect.w - size.w - 5;
    }
    
    // Vertical adjustment
    if (pos.y < screenRect.y) {
        pos.y = screenRect.y + 5;
    } else if (pos.y + size.h > screenRect.y + screenRect.h) {
        pos.y = screenRect.y + screenRect.h - size.h - 5;
    }
    
    return pos;
}
```

### Krok 2: Dynamic layout adaptation
```cpp
// Source/engine/ui/layout_adapter.h
class LayoutAdapter {
public:
    // Dynamiczne dostosowywanie layoutów
    static Rect AdaptPanelRect(const Rect& baseRect, UiScalePreset scale);
    static Size AdaptButtonSize(const Size& baseSize, UiScalePreset scale);
    static int AdaptFontSize(int baseSize, UiScalePreset scale);
    static int AdaptSpacing(int baseSpacing, UiScalePreset scale);
    
private:
    static bool NeedsLayoutAdjustment(UiScalePreset scale);
    static float GetLayoutScale(UiScalePreset scale);
};

Rect LayoutAdapter::AdaptPanelRect(const Rect& baseRect, UiScalePreset scale) {
    if (!NeedsLayoutAdjustment(scale)) {
        return baseRect;
    }
    
    float layoutScale = GetLayoutScale(scale);
    Rect adapted = baseRect;
    
    // Scale dimensions
    adapted.w = static_cast<int>(baseRect.w * layoutScale);
    adapted.h = static_cast<int>(baseRect.h * layoutScale);
    
    // Re-center jeśli potrzeba
    Rect screenRect = GetScreenRect();
    if (adapted.w > screenRect.w) {
        adapted.w = screenRect.w - 20;  // Margin
        adapted.x = 10;
    }
    
    if (adapted.h > screenRect.h) {
        adapted.h = screenRect.h - 20;
        adapted.y = 10;
    }
    
    return adapted;
}

bool LayoutAdapter::NeedsLayoutAdjustment(UiScalePreset scale) {
    return scale == UiScalePreset::Large || scale == UiScalePreset::Larger;
}
```

### Krok 3: Controller navigation fixes
```cpp
// Source/controls/controller_navigation.cpp
class ControllerNavigationManager {
private:
    struct NavigableElement {
        Rect logicalHitbox;
        std::string id;
        int navigationOrder;
    };
    
    std::vector<NavigableElement> elements_;
    
public:
    void RegisterElement(const std::string& id, const Rect& logicalHitbox, int order);
    void UpdateElementHitbox(const std::string& id, const Rect& newHitbox);
    Point GetCurrentFocusPosition() const;
    bool NavigateToNext();
    bool NavigateToPrevious();
    bool NavigateToDirection(Direction dir);
    
private:
    void UpdateHitboxesForCurrentScale();
    Rect ScaleHitboxForNavigation(const Rect& logicalRect) const;
};

void ControllerNavigationManager::UpdateHitboxesForCurrentScale() {
    float currentScale = uiScaleManager.GetScale();
    
    for (auto& element : elements_) {
        // Scale hitbox z marginesem dla lepszego feel
        Rect scaled = ScaleHitboxForNavigation(element.logicalHitbox);
        
        // Dodaj small margin żeby nie było "pixel perfect"
        const int margin = 2;
        scaled.x -= margin;
        scaled.y -= margin;
        scaled.w += margin * 2;
        scaled.h += margin * 2;
        
        // Update w systemie navigation
        UpdateNavigationHitbox(element.id, scaled);
    }
}

Rect ControllerNavigationManager::ScaleHitboxForNavigation(const Rect& logicalRect) const {
    return uiScaleManager.ScaleRect(logicalRect);
}
```

### Krok 4: Specific screen fixes

#### Store screen
```cpp
// Source/DiabloUI/stores.cpp
void DrawStoreScreen() {
    // Adapt layout dla current scale
    Rect storeRect = LayoutAdapter::AdaptPanelRect(GetBaseStoreRect(), options.uiScale);
    
    // Draw background
    DrawPanelBackground(storeRect);
    
    // Adapt item grid
    Size itemSize = LayoutAdapter::AdaptButtonSize({32, 32}, options.uiScale);
    int spacing = LayoutAdapter::AdaptSpacing(4, options.uiScale);
    
    DrawStoreItems(storeRect, itemSize, spacing);
    
    // Adapt text positions
    int fontSize = LayoutAdapter::AdaptFontSize(12, options.uiScale);
    DrawStoreText(storeRect, fontSize);
}
```

#### Spell book
```cpp
// Source/DiabloUI/spellbook.cpp
void DrawSpellBook() {
    Rect bookRect = LayoutAdapter::AdaptPanelRect(GetBaseSpellBookRect(), options.uiScale);
    
    // Calculate grid layout
    int iconsPerRow = CalculateIconsPerRow(bookRect.w, options.uiScale);
    Size iconSize = LayoutAdapter::AdaptButtonSize({32, 32}, options.uiScale);
    int spacing = LayoutAdapter::AdaptSpacing(2, options.uiScale);
    
    DrawSpellIcons(bookRect, iconsPerRow, iconSize, spacing);
    
    // Update controller navigation
    controllerNavigation.ClearElements();
    RegisterSpellIconsForNavigation(bookRect, iconsPerRow, iconSize, spacing);
}
```

#### Inventory
```cpp
// Source/DiabloUI/inventory.cpp
void DrawInventory() {
    Rect invRect = LayoutAdapter::AdaptPanelRect(GetBaseInventoryRect(), options.uiScale);
    
    // Grid layout adaptation
    Size cellSize = LayoutAdapter::AdaptButtonSize({28, 28}, options.uiScale);
    int gridSpacing = LayoutAdapter::AdaptSpacing(1, options.uiScale);
    
    DrawInventoryGrid(invRect, cellSize, gridSpacing);
    
    // Item label positioning
    UpdateItemLabelPositions(options.uiScale);
}
```

---

## 4. Edge case handling

### Very small screens
```cpp
bool IsSmallScreenMode() {
    return GetScreenWidth() < 1024 || GetScreenHeight() < 768;
}

void HandleSmallScreenLayout() {
    if (IsSmallScreenMode()) {
        // Force Normal scale
        if (options.uiScale != UiScalePreset::Normal) {
            options.uiScale = UiScalePreset::Normal;
            uiScaleManager.SetScale(1.0f);
            
            ShowMessageBox("UI scale reset to Normal for small screen compatibility.");
        }
        
        // Additional layout adjustments
        EnableCompactLayoutMode();
    }
}
```

### Ultrawide screens
```cpp
void HandleUltrawideLayout() {
    float aspectRatio = static_cast<float>(GetScreenWidth()) / GetScreenHeight();
    
    if (aspectRatio > 2.0f) {  // Ultrawide
        // Center UI instead of stretching
        CenterUiLayout();
        
        // Adjust anchoring dla better composition
        uiScaleManager.SetAnchorMode(UiScaleManager::AnchorMode::Centered);
    }
}
```

---

## 5. Kryteria ukończenia

### Tooltip system
- [ ] Tooltipy nigdy nie wychodzą poza ekran
- [ ] Tooltip positioning inteligentnie wybiera najlepszą lokalizację
- [ ] Tooltipy nie nakładają się na ważne UI elements
- [ ] Tooltip text mieści się w tooltip box

### Layout adaptation
- [ ] Panele automatycznie dostosowują się do dużych skal
- [ ] Text mieści się w przyciskach przy wszystkich presetach
- [ ] Icons nie są ucinane
- [ ] Spacing i margins skalują się sensownie

### Controller navigation
- [ ] Focus rectangle pokrywa się z actual elements
- [ ] Directional navigation działa poprawnie
- [ ] Hitboxy są odpowiednio duże dla wygody
- [ ] Navigation flow jest spójny

### Specific screens
- [ ] Store screen działa przy wszystkich presetach
- [ ] Spell book icons i layout są poprawne
- [ ] Inventory grid alignment jest maintained
- [ ] Character panel stats positioning jest poprawny

### Edge cases
- [ ] Very small screens handled gracefully
- [ ] Ultrawide screens work correctly
- [ ] Dynamic resolution change nie psuje layout
- [ ] Windowed mode resize działa

---

## 6. Plan testowy

### Comprehensive screen testing
- [ ] Main menu - wszystkie presety
- [ ] Character creation - wszystkie presety
- [ ] In-game UI - wszystkie presety
- [ ] Inventory - wszystkie presety
- [ ] Character sheet - wszystkie presety
- [ ] Spell book - wszystkie presety
- [ ] All store types - wszystkie presety
- [ ] Quest log - wszystkie presety
- [ ] Options menu - wszystkie presety

### Interaction testing
- [ ] Mouse clicking przy wszystkich presetach
- [ ] Tooltip positioning na wszystkich ekranach
- [ ] Controller navigation na wszystkich ekranach
- [ ] Keyboard shortcuts
- [ ] Drag and drop (inventory)

### Resolution testing
- [ ] 800x600 - tylko Normal
- [ ] 1024x768 - Normal + Large
- [ ] 1920x1080 - wszystkie presety
- [ ] 2560x1440 - wszystkie presety
- [ ] 3840x2160 - wszystkie presety
- [ ] Ultrawide 3440x1440 - wszystkie presety

---

## 7. Performance considerations

### Rendering optimization
```cpp
// Cache scaled layouts
class LayoutCache {
private:
    std::unordered_map<std::string, CachedLayout> cache_;
    
public:
    CachedLayout GetOrCreateLayout(const std::string& layoutId, UiScalePreset scale);
    void InvalidateCache();
    void InvalidateForScale(UiScalePreset scale);
};
```

### Memory management
- Unikaj częstego reallocation dla tooltipów
- Cache scaled textures jeśli możliwe
- Pool dla layout calculations

---

## 8. Documentation

### User-facing help
```cpp
const char* GetUiScaleHelpText() {
    return "UI Scale adjusts the size of interface elements:\n"
           "• Normal: Default size (recommended for most screens)\n"
           "• Large: 25% larger (better readability)\n"
           "• Larger: 50% larger (maximum size)\n\n"
           "Note: Larger scales may not be available at low resolutions.\n"
           "The game will automatically adjust if needed.";
}
```

### Developer notes
- Layout adaptation principles
- Controller navigation considerations
- Performance optimization guidelines

---

## 9. Final validation

### User acceptance criteria
- [ ] UI scaling poprawia czytelność bez psucia funkcjonalności
- [ ] Wszystkie ekrany są użyteczne przy wszystkich dostępnych presetach
- [ ] Transition między presetami jest smooth
- [ ] Error handling jest user-friendly

### Technical validation
- [ ] No memory leaks
- [ ] Stable FPS przy wszystkich presetach
- [ ] Consistent behavior across platforms
- [ ] Proper config migration

---

## 10. Project completion

Po tym etapie cały UI scaling feature jest kompletny:
- ✅ Podstawowy mechanizm (etapy 1-3)
- ✅ Public exposure (etap 4)
- ✅ Polish i edge cases (etap 5)

Feature jest gotowy do production use.