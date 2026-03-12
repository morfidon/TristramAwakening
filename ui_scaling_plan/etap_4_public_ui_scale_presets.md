# Etap 4 - Feature: Add Limited UI Scale Presets

**Cel:** Eksponować UI scaling dla użytkowników przez bezpieczne presety.

**Scope:** Dodanie opcji w settings, 2-3 bezpieczne presety, fallback logic.

---

## 1. Problem do rozwiązania

### Dotychczasowe etapy
- **Etap 1:** Podzielony render path
- **Etap 2:** Logical UI coordinates  
- **Etap 3:** Internal scale factor z clampingiem

### Co teraz potrzebujemy
- Publicznie dostępna opcja UI scaling
- Bezpieczne, przetestowane presety
- Łatwy UX w settings menu
- Fallback dla problematycznych konfiguracji

---

## 2. Docelowa architektura

### UI Scale Presets
```cpp
// Source/engine/ui/ui_scale_presets.h
enum class UiScalePreset {
    Normal = 100,    // 1.0x - default
    Large = 125,     // 1.25x
    Larger = 150     // 1.5x
};

class UiScalePresets {
public:
    static const std::array<UiScalePreset, 3> GetAllPresets() {
        return {UiScalePreset::Normal, UiScalePreset::Large, UiScalePreset::Larger};
    }
    
    static float GetScaleValue(UiScalePreset preset) {
        switch (preset) {
            case UiScalePreset::Normal:  return 1.0f;
            case UiScalePreset::Large:   return 1.25f;
            case UiScalePreset::Larger:  return 1.5f;
        }
        return 1.0f;
    }
    
    static const char* GetPresetName(UiScalePreset preset) {
        switch (preset) {
            case UiScalePreset::Normal:  return "Normal";
            case UiScalePreset::Large:   return "Large";
            case UiScalePreset::Larger:  return "Larger";
        }
        return "Normal";
    }
    
    static bool IsPresetSafeForResolution(UiScalePreset preset, int screenWidth, int screenHeight);
};
```

### Settings Integration
```cpp
// W options menu
struct GameOptions {
    // ... inne opcje
    UiScalePreset uiScale = UiScalePreset::Normal;
};

// W config loading/saving
void LoadOptions() {
    options.uiScale = static_cast<UiScalePreset>(
        LoadConfigInt("ui_scale_preset", static_cast<int>(UiScalePreset::Normal))
    );
    
    // Validate preset dla aktualnej rozdzielczości
    if (!UiScalePresets::IsPresetSafeForResolution(options.uiScale, GetScreenWidth(), GetScreenHeight())) {
        options.uiScale = UiScalePreset::Normal;  // Fallback
    }
    
    // Apply do scale manager
    uiScaleManager.SetScale(UiScalePresets::GetScaleValue(options.uiScale));
}
```

---

## 3. Implementacja krok po kroku

### Krok 1: Dodaj safety validation
```cpp
bool UiScalePresets::IsPresetSafeForResolution(UiScalePreset preset, int screenWidth, int screenHeight) {
    float scale = GetScaleValue(preset);
    
    // Sprawdź czy UI zmieści się w ekranie po skalowaniu
    int scaledUiWidth = static_cast<int>(BASE_UI_WIDTH * scale);
    int scaledUiHeight = static_cast<int>(BASE_UI_HEIGHT * scale);
    
    // Dodaj margines błędu
    const int margin = 50;
    
    bool fitsWidth = (scaledUiWidth + margin) <= screenWidth;
    bool fitsHeight = (scaledUiHeight + margin) <= screenHeight;
    
    // Specjalne case dla bardzo małych ekranów
    if (screenWidth < 800 || screenHeight < 600) {
        return preset == UiScalePreset::Normal;
    }
    
    return fitsWidth && fitsHeight;
}
```

### Krok 2: Dodaj UI w options menu
```cpp
// W Source/DiabloUI/options.cpp
void DrawOptionsUi() {
    // ... inne opcje
    
    // UI Scale section
    DrawLabel("UI Scale:", x, y);
    
    int currentPresetIndex = static_cast<int>(options.uiScale);
    const char* currentPresetName = UiScalePresets::GetPresetName(options.uiScale);
    
    if (DrawButton(currentPresetName, x + 100, y)) {
        // Cycle przez presety
        auto allPresets = UiScalePresets::GetAllPresets();
        int nextIndex = (currentPresetIndex + 1) % allPresets.size();
        
        auto nextPreset = allPresets[nextIndex];
        
        // Validate przed zmianą
        if (UiScalePresets::IsPresetSafeForResolution(nextPreset, GetScreenWidth(), GetScreenHeight())) {
            options.uiScale = nextPreset;
            uiScaleManager.SetScale(UiScalePresets::GetScaleValue(nextPreset));
            
            // Save config
            SaveConfigInt("ui_scale_preset", static_cast<int>(nextPreset));
        } else {
            // Show warning
            ShowMessageBox("This UI scale is not compatible with your current resolution.");
        }
    }
    
    // Preview/current scale info
    DrawSmallText("Current: " + std::string(currentPresetName), x + 250, y);
}
```

### Krok 3: Dodaj runtime validation
```cpp
// Przy zmianie rozdzielczości
void OnResolutionChanged(int newWidth, int newHeight) {
    // Sprawdź czy current preset jest still safe
    if (!UiScalePresets::IsPresetSafeForResolution(options.uiScale, newWidth, newHeight)) {
        // Auto fallback do Normal
        options.uiScale = UiScalePreset::Normal;
        uiScaleManager.SetScale(1.0f);
        
        // Notify user
        ShowMessageBox("UI scale reset to Normal due to resolution change.");
        
        // Save
        SaveConfigInt("ui_scale_preset", static_cast<int>(UiScalePreset::Normal));
    }
}
```

### Krok 4: Dodaj help/tooltips
```cpp
// Tooltip dla UI scale option
const char* GetUiScaleHelpText() {
    return "Adjusts the size of UI elements for better readability.\n"
           "Normal: Default size\n"
           "Large: 25% larger\n"
           "Larger: 50% larger\n\n"
           "Note: Larger scales may not be available at low resolutions.";
}
```

---

## 4. UX Design

### Settings Menu Layout
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

### Preset Selection Flow
1. User clicks "UI Scale" button
2. Cycles through: Normal → Large → Larger → Normal
3. Each change:
   - Validates for current resolution
   - Applies immediately
   - Saves to config
   - Shows preview feedback

### Error Handling
- **Resolution too low**: Force Normal, show notification
- **Preset not available**: Skip unsafe presets, show warning
- **Config corruption**: Fallback to Normal

---

## 5. Kryteria ukończenia

### Techniczne
- [ ] UI Scale Presets system z 3 opcjami
- [ ] Safety validation dla rozdzielczości
- [ ] Integration z options menu
- [ ] Config save/load
- [ ] Runtime resolution change handling

### UX
- [ ] Intuicyjny cycling przez presety
- [ ] Immediate preview
- [ ] Help/tooltips
- [ ] Error messages dla problematycznych przypadków

### Testowe
- [ ] Wszystkie presety działają na standardowych rozdzielczościach
- [ ] Fallback działa przy niskich rozdzielczościach
- [ ] Resolution change nie psuje UI scale
- [ ] Config persistence działa
- [ ] Input accuracy przy wszystkich presetach

---

## 6. Plan testowy

### Standard resolutions
- [ ] 1920x1080 - wszystkie presety
- [ ] 1366x768 - Normal + Large
- [ ] 1280x720 - Normal + Large
- [ ] 1024x768 - tylko Normal
- [ ] 800x600 - tylko Normal

### Edge cases
- [ ] Ultrawide (3440x1440) - wszystkie presety
- [ ] 4K (3840x2160) - wszystkie presety
- [ ] Dynamiczna zmiana rozdzielczości
- [ ] Windowed mode resize

### User scenarios
- [ ] User selects preset too large for resolution
- [ ] User changes resolution with custom preset
- [ ] Config corruption/migration
- [ ] First-time user (default Normal)

---

## 7. Ryzyka i uwagi

### Największe ryzyka
- **Layout breaking** przy niektórych presetach
- **Performance** przy higher scales
- **User confusion** dlaczego niektóre presety nie są dostępne
- **Regression** w existing UI elements

### Czego NIE robić
- Nie dodawać dowolnego slidera (trzymać się presetów)
- Nie dodawać zbyt wielu presetów (3 to optymalnie)
- Nie ignorować safety validation
- Nie zapominać o migration dla existing configs

---

## 8. Integration z poprzednimi etapami

### Etap 1-3 ✓
- Foundation ready

### Etap 4 (teraz)
- Public feature exposure
- Safety nets
- User-friendly interface

---

## 9. Next Steps

Po tym etapie:
- Etap 5: Polish edge cases, tooltip positioning, controller navigation

Etap 4 jest przełomowy - to tutaj funkcja staje się dostępna dla użytkowników.