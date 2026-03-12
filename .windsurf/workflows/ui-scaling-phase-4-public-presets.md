---
description: Expose UI scaling to users through safe presets
---

# UI Scaling - Phase 4: Public UI Scale Presets

**Cel:** Eksponować UI scaling dla użytkowników przez bezpieczne presety.

## Scope
- Dodanie opcji w settings
- 2-3 bezpieczne presety
- Safety validation dla rozdzielczości
- Fallback logic

## Kluczowe pliki
- `Source/engine/ui/ui_scale_presets.h/.cpp` - system presetów
- `Source/DiabloUI/options.cpp` - integration z options menu
- Config loading/saving

## Docelowe presety
```cpp
enum class UiScalePreset {
    Normal = 100,    // 1.0x - default
    Large = 125,     // 1.25x
    Larger = 150     // 1.5x
};

class UiScalePresets {
public:
    static const std::array<UiScalePreset, 3> GetAllPresets();
    static float GetScaleValue(UiScalePreset preset);
    static const char* GetPresetName(UiScalePreset preset);
    static bool IsPresetSafeForResolution(UiScalePreset preset, int screenWidth, int screenHeight);
};
```

## Safety validation
```cpp
bool UiScalePresets::IsPresetSafeForResolution(UiScalePreset preset, int screenWidth, int screenHeight) {
    float scale = GetScaleValue(preset);
    int scaledUiWidth = static_cast<int>(BASE_UI_WIDTH * scale);
    int scaledUiHeight = static_cast<int>(BASE_UI_HEIGHT * scale);
    const int margin = 50;
    
    bool fitsWidth = (scaledUiWidth + margin) <= screenWidth;
    bool fitsHeight = (scaledUiHeight + margin) <= screenHeight;
    
    if (screenWidth < 800 || screenHeight < 600) {
        return preset == UiScalePreset::Normal;
    }
    
    return fitsWidth && fitsHeight;
}
```

## Settings menu layout
```
┌─────────────────────────────────────┐
│ Video Options                       │
│ ├─ Resolution: [1920x1080 ▼]       │
│ ├─ Fullscreen: [Yes]               │
│ ├─ VSync: [Yes]                     │
│ └─ UI Scale: [Large ▼]              │ ← Nowa opcja
│                                     │
│ └─ Help: [Adjusts UI size...]      │
└─────────────────────────────────────┘
```

## Preset Selection Flow
1. User clicks "UI Scale" button
2. Cycles through: Normal → Large → Larger → Normal
3. Each change:
   - Validates for current resolution
   - Applies immediately
   - Saves to config
   - Shows preview feedback

## Error handling
- **Resolution too low**: Force Normal, show notification
- **Preset not available**: Skip unsafe presets, show warning
- **Config corruption**: Fallback to Normal

## Runtime validation
```cpp
void OnResolutionChanged(int newWidth, int newHeight) {
    if (!UiScalePresets::IsPresetSafeForResolution(options.uiScale, newWidth, newHeight)) {
        options.uiScale = UiScalePreset::Normal;
        uiScaleManager.SetScale(1.0f);
        ShowMessageBox("UI scale reset to Normal due to resolution change.");
        SaveConfigInt("ui_scale_preset", static_cast<int>(UiScalePreset::Normal));
    }
}
```

## Help text
```cpp
const char* GetUiScaleHelpText() {
    return "Adjusts the size of UI elements for better readability.\n"
           "Normal: Default size\n"
           "Large: 25% larger\n"
           "Larger: 50% larger\n\n"
           "Note: Larger scales may not be available at low resolutions.";
}
```

## Kryteria ukończenia
- [ ] UI Scale Presets system z 3 opcjami
- [ ] Safety validation dla rozdzielczości
- [ ] Integration z options menu
- [ ] Config save/load
- [ ] Runtime resolution change handling
- [ ] Intuicyjny cycling przez presety
- [ ] Immediate preview
- [ ] Help/tooltips
- [ ] Error messages dla problematycznych przypadków
- [ ] Wszystkie presety działają na standardowych rozdzielczościach
- [ ] Fallback działa przy niskich rozdzielczościach
- [ ] Resolution change nie psuje UI scale
- [ ] Config persistence działa
- [ ] Input accuracy przy wszystkich presetach

## Test plan

### Standard resolutions
- 1920x1080 - wszystkie presety
- 1366x768 - Normal + Large
- 1280x720 - Normal + Large
- 1024x768 - tylko Normal
- 800x600 - tylko Normal

### Edge cases
- Ultrawide (3440x1440) - wszystkie presety
- 4K (3840x2160) - wszystkie presety
- Dynamiczna zmiana rozdzielczości
- Windowed mode resize

### User scenarios
- User selects preset too large for resolution
- User changes resolution with custom preset
- Config corruption/migration
- First-time user (default Normal)

## Ryzyka
- Layout breaking przy niektórych presetach
- Performance przy higher scales
- User confusion dlaczego niektóre presety nie są dostępne
- Regression w existing UI elements

## Czego NIE robić
- Nie dodawać dowolnego slidera (trzymać się presetów)
- Nie dodawać zbyt wielu presetów (3 to optymalnie)
- Nie ignorować safety validation
- Nie zapominać o migration dla existing configs

## Szacowany czas: 2-3 dni

## Next steps
Implementacja po ukończeniu etapu 3.

## Integration z poprzednimi etapami
- Etap 1-3: Foundation ready ✓
- Etap 4: Public feature exposure, safety nets, user-friendly interface