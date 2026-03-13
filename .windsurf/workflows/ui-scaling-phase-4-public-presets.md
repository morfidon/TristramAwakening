---
description: Expose UI scaling to users through safe presets
---

# UI Scaling - Phase 4: Public UI Scale Presets

**Cel:** Udostępnić użytkownikowi **bezpieczny, ograniczony wybór presetów UI scale**, oparty na rzeczywistych ograniczeniach layoutu potwierdzonych po etapie 3.

## Scope
* add hidden/internal scale choice to public options UI
* expose a small preset list
* validate presets against actual phase-3 scaled UI bounds
* apply safe fallback policy for runtime resolution changes
* follow existing options-menu semantics

## Non-goals
* no freeform slider
* no modal warnings on resize
* no new rendering model
* no DiabloUI scaling unless already proven safe
* no guessing fit from hardcoded constants unless validated

## Kluczowe pliki
- `Source/options.h / options.cpp` - integracja ze strukturą sgOptions i serializacją do diablo.ini.
- `options UI implementation files (exact callsites to confirm during implementation)` - podpięcie pozycji w menu Video Options.
- Local helper functions near existing options handling (start small, extract only if needed).

## Docelowe presety
```cpp
enum class UiScalePreset {
    Normal,     // Default scale (1.0x)
    Large,      // Larger scale (exact % TBD after phase 3 fit testing)
    Larger      // Largest scale (exact % TBD after phase 3 fit testing)
};

// Helper functions (start small, local):
static float GetScaleValue(UiScalePreset preset);
static const char* GetPresetName(UiScalePreset preset);
static bool IsPresetSafeForResolution(UiScalePreset preset, int screenWidth, int screenHeight);
static UiScalePreset GetMaxSafePreset(int screenWidth, int screenHeight);
```

**Note:** Exact percentage values (100/125/150) to be determined after phase 3 real-world fit testing.

## Safety validation (Reguły)

**Critical:** Validate against the actual targeted scaled UI family/families from phase 3, not guessed constants.

```cpp
bool IsPresetSafeForResolution(UiScalePreset preset, int screenWidth, int screenHeight) {
    if (preset == UiScalePreset::Normal) return true; // Normal always safe
    
    // Use real phase-3 scaled UI bounds, not BASE_UI_WIDTH/HEIGHT guesses
    // Bounds from actual layout composition path, not just preset metadata
    // GetActualScaledUiBounds() must be based on real phase-3 scaled UI path
    UiBounds scaledBounds = GetActualScaledUiBounds(preset);
    // Safety margin derived from actual compositing/layout requirements
    const int margin = GetRequiredSafetyMargin(); // Exact value TBD after testing
    
    return (scaledBounds.width + margin) <= screenWidth && 
           (scaledBounds.height + margin) <= screenHeight;
}
```

**Fit test must use real composed bounds, not guessed constants, unless investigation proves constants are reliable.**

**Critical:** `GetActualScaledUiBounds()` must be derived from the actual phase-3 scaled in-game UI family/families and not from abstract preset metadata alone. If phase 3 scales only one targeted family, phase 4 validation must use bounds for that family only.

## Settings Menu Layout & Interaction
```
┌─────────────────────────────────────┐
│ Video Options                       │
│ ├─ Resolution: [1920x1080 ▼]       │
│ ├─ Fullscreen: [Yes]               │
│ ├─ VSync: [Yes]                     │
│ └─ UI Scale: [Large]                │ ← Nowa opcja
│                                     │
│ └─ Help: [Adjusts UI size...]      │
└─────────────────────────────────────┘
```

## Zasady interakcji (Zgodnie z konwencją DevilutionX):
Opcja nie jest dropdownem, lecz pozycją cykliczną (Enter / kliknięcie zmienia na następną wartość).
Cykl omija wartości niebezpieczne dla wybranej w menu rozdzielczości (np. na 800x600 kliknięcie w Normal nie robi nic, bo Large jest zablokowane).
**Cycling should operate over currently safe presets only, or provide lightweight feedback if no larger preset is available.**
**Avoid "click and nothing happens" interactions.**
**Preferred behavior: cycle only through currently safe presets.**
**Follow existing options-menu apply/persist semantics** - nie zakładaj natychmiastowego zapisu.

## Zmiany w czasie rzeczywistym (Runtime Fallback)

**Important:** Separate user preference from effective runtime scale to avoid overloading state meanings.

**No modal dialogs during runtime resize or window drag.**
**Runtime resize may clamp only the effective preset, not the stored user preference.**

```cpp
// Hook w miejscu obsługi zdarzenia SDL_WINDOWEVENT_RESIZED
void OnResolutionChanged(int newWidth, int newHeight) {
    UiScalePreset userPreference = sgOptions.Graphics.uiScalePreset;
    
    if (!IsPresetSafeForResolution(userPreference, newWidth, newHeight)) {
        // Cichy fallback - nie blokujemy wątku modalnym okienkiem!
        UiScalePreset safePreset = GetMaxSafePreset(newWidth, newHeight);
        ApplyRuntimeUiScalePreset(safePreset); // Apply effective scale
        
        // User preference stays unchanged - persistence policy explicit
        // Optional: Save effective scale separately if needed
    }
}
```

**Do Not Overload Existing Flags / state meanings** - user preference vs effective scale should be distinct.

## Explicit Fallback Policy

* if current effective preset becomes unsafe after resize, clamp only the effective runtime value
* if the user later returns to a larger resolution, re-apply the highest safe value consistent with the stored preference

This ensures consistent UX behavior across resolution changes and preserves user intent.

## Tekst Pomocy (Help Text)

Simple description of option functionality. Final text to be determined during implementation based on actual preset values.

## Kryteria ukończenia (Definition of Done)

Struktura opcji zawiera nowy wpis, który poprawnie czyta i zapisuje się do pliku konfiguracyjnego.

Niepoprawne wartości w configu (np. wpisane z palca uiScalePreset = 999) są automatycznie zbijane do Normal.

Cykl opcji w menu Wideo działa intuicyjnie i blokuje presety zbyt duże dla ekranu.

Resize okna w trybie Windowed automatycznie redukuje skalę do bezpiecznej, bez wysypywania gry.

Skala aplikuje się na żywo i jest od razu widoczna podczas przełączania w menu.

**subject to existing options-menu apply semantics** - jeśli repo ma konkretny rytm apply/save, nowa opcja go respektuje.

Interfejs z Etapu 3 bezbłędnie reaguje na zmiany napędzane przez sgOptions.

## Test plan

### Standard resolutions
Test a range of common resolutions:
- 1920x1080, 1366x768, 1280x720, 1024x768, 800x600
- Ultrawide (3440x1440), 4K (3840x2160)

### Validation approach
- Verify which presets remain safe after real fit validation using actual phase-3 bounds
- **Do not hardcode expected allowed presets until phase-3 bounds are confirmed**
- Test dynamic resolution changes and windowed mode resize

### User scenarios
- User selects preset too large for resolution
- User changes resolution with custom preset
- Config corruption/migration
- First-time user (default Normal)
- **Resize down then resize up: verify effective preset returns to highest safe value consistent with stored preference**

## Ryzyka

**Primary Risk:** Using guessed constants (`BASE_UI_WIDTH/HEIGHT`) instead of real phase-3 scaled UI bounds for validation.

Event looping during graphics options changes (scale change -> reinit -> scale change).

User preference vs effective scale state confusion if not properly separated.

## Czego NIE robić
Nie dodawać MessageBoxów ani alertów przerywających grę przy re-size'owaniu okna. Cicha korekta to poprawny UX.
Nie tworzyć swobodnych sliderów odświeżanych co klatkę.
**Nie używaj magicznych liczb bez uzasadnienia - safety margin musi wynikać z realnych potrzeb layout/compositing.**

## Szacowany czas: 2-3 dni

## Next steps
Implementacja po ukończeniu etapu 3.

## Integration z poprzednimi etapami
- Etap 1-3: Foundation ready ✓
- Etap 4: Public feature exposure, safety nets, user-friendly interface