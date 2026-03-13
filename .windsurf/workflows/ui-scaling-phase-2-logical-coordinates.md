---
description: Mouse/UI hit-testing logical coordinates (local-first approach)
---

# UI Scaling - Phase 2: Mouse/UI Hit-Testing Logical Coordinates

**Cel:** Wydzielić wspólny mapping screen-space → logical UI point dla klasycznego mouse/UI hit-testingu, bez zmiany widocznego zachowania i bez narzucania wspólnego modelu dla wszystkich inputów.

## Scope (zawężony do in-game UI)
- **Tylko** mouse/UI hit-testing dla **in-game UI**: panels, buttons, inventory slots, spell icons
- **DiabloUI audit-only** - main menu, character creation, multiplayer menu są osobnym kontekstem
- Lokalny helper blisko miejsca użycia (nie nowy subsystem)
- Mapping przy implicit scale = 1 (bez parametru skali)
- Audit controller/touch/tooltip contexts (bez forced unification)

## Kluczowe pliki do zbadania
- `Source/control/` - mouse input processing (control_panel.cpp, control_gold.cpp, etc.)
- `Source/panels/` - panel system (inventory, spellbook, etc.)
- `Source/DiabloUI/` - **audit only** (main menu, character creation - osobny kontekst)
- `Source/controls/` - **audit only** (gamepad-centric, nie zakładaj shared rule)

## Problem do rozwiązania (zawężony)
- Mouse/UI hit-testing używa bezpośrednich współrzędnych ekranowych
- Brak jednego wspólnego punktu transformacji screen → UI logical point
- Klasyczne UI elementy (panels, buttons, inventory) używają tego samego wzorca

## Docelowa architektura (minimalna)
```cpp
// Prosta struktura - nie pełny system
struct UiPoint {
    int x, y;  // Współrzędne w logical UI space (scale = 1)
};

// Lokalny helper - nie globalny system
namespace UiHitTesting {
    UiPoint ScreenToUiPoint(int screenX, int screenY);
    // Brak UpdateScale() - to jest w etapie 3
    // Brak TransformHitbox() - prosty mapping point-to-point
}
```

## Miejsca do refaktoryzacji (tylko mouse/UI hit-testing)
- Panel click detection (Source/control/)
- Inventory slot selection (Source/panels/ / Source/inv.cpp)
- Spell/icon clicking (Source/panels/ / Source/control/)
- In-game button hit testing (Source/panels/ / Source/control/)

### Contexts to Audit (nie refactor w etapie 2)
- **Controller navigation** - sprawdź czy pixel-based czy selection-based
- **Touch input** - sprawdź czy używa tego samego wzorca co mouse
- **Tooltip placement** - sprawdź czy potrzebuje tego samego mappingu

## Przykładowy refactor (minimalny)

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
    auto uiPoint = UiHitTesting::ScreenToUiPoint(screenX, screenY);
    if (IsPointInRect(uiPoint.x, uiPoint.y, buttonRect)) {
        // Handle click
    }
}
```

## Kryteria ukończenia (realistyczne i reviewable)
- [ ] Istnieje jeden jawny punkt transformacji screen-space → UI logical point dla mouse/UI hit-testingu
- [ ] Panel clicking działa identycznie jak wcześniej
- [ ] Inventory slot selection działa identycznie jak wcześniej  
- [ ] Button hit testing działa identycznie jak wcześniej
- [ ] Brak widocznych zmian dla użytkownika
- [ ] Controller navigation został zmapowany i oceniony (ale nie musi używać wspólnego helpera)
- [ ] Tooltip placement został zaudytowany (ale nie musi używać wspólnego helpera)
- [ ] Brak zmiany semantyki inputu świata gry

## Non-goals (explicit)
- **Bez zmian** world-space gameplay input
- **Bez założenia** że controller navigation używa tej samej reguły co mouse hit-testing
- **Bez wymuszania** unifikacji wszystkich UI coordinate consumers
- **Bez publicznych** scaling controls
- **Bez parametru** scale (to jest w etapie 3)
- **Bez rewrite** tooltip system bez dowodu że używa tej samej reguły

## Ryzyka (zawężone)
- Performance - dodatkowe transformacje przy mouse events
- Complexity - wiele miejsc używających współrzędnych w UI
- Edge cases - boundary conditions przy prostym mappingu

## Czego NIE robić w etapie 2
- **Nie ruszać** world input (movement, camera)
- **Nie wymuszać** wspólnego mappera dla gamepad/touch bez dowodu
- **Nie dodawać** UpdateScale() (to jest w etapie 3)
- **Nie tworzyć** dużego nowego subsystemu w engine/ui bez potrzeby
- **Nie zakładać** że controller focus jest pixel-based bez audytu
- **Nie optymalizować** przedwcześnie

## Szacowany czas: 1-2 dni (zawężony scope)

## Next steps
Implementacja po ukończeniu etapu 1, z auditem controller/touch/tooltip contexts.

## Integration z etapem 1
Po etapie 1 mamy: `RenderWorldPass()`, `RenderWorldOverlays()`, `RenderWindowPanels()`, `RenderTopOverlays()`, `RenderMainHudPanel()`.
Po etapie 2 dodajemy: `UiHitTesting::ScreenToUiPoint()` dostępny w **in-game** mouse/UI hit-testing.

## Integration z etapem 3
Etap 2 przygotowuje punkt transformacji.
Etap 3 nada mu parametr skali i zachowania skalowania.