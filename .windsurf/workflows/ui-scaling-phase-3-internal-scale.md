UI Scaling - Phase 3: Backend Validation Spike + Minimal Hidden Scaling Path
Cel: Zweryfikować, czy pośrednie surface scaling jest bezpieczny w DevilutionX, i wprowadzić minimalny działający mechanizm skalowania dla **jednej wybranej rodziny in-game UI** przez renderowanie do pośredniego surface'a i złożenie jej z powrotem na ekran, z minimalnym wymaganym modelem transformacji i bez publicznego eksponowania funkcji.
Scope (izolowany i bezpieczny)
config-only hidden scale value
minimal in-game UI transform struct
intermediate surface for the targeted in-game UI path
inverse transform wired into stage-2 mouse/UI hit-testing
no world input changes
no controller unification
no DiabloUI

Phase 3 starts with a rendering-backend spike to validate indexed-surface scaling/compositing behavior in the current build.
Kluczowe pliki
Source/engine/render/scrollrt.cpp - integracja z render pipeline'em (modyfikacja wywołań helperów z Etapu 1)
Source/engine/render/ui_scale_transform.h (Nowe) - minimalny struct transformu UI (local-first helper; header/cpp split only if it proves reusable beyond the immediate callsites)
Miejsca implementacji UiHitTesting (z Etapu 2) - aktualizacja o korzystanie z transformu
Source/options.cpp - dodanie ukrytego parametru (użycie istniejącego systemu opcji)
Problem do rozwiązania
Wszystkie panele w Diablo rysowane są piksel po pikselu (często funkcją CelDraw). Próba skalowania każdego elementu z osobna zniszczyłaby layout (powstałyby luki między przyciskami i ramkami).
Najbardziej obiecujące podejście na etapie 3 to renderowanie docelowej warstwy in-game UI do pośredniego surface'a i złożenie jej z powrotem na ekran.
Po przeskalowaniu obrazka, kliknięcia myszką muszą przechodzić przez "odwrotną transformację", by trafić w odpowiednie logiczne miejsca.

Docelowa architektura (minimalny transform)
code
C++
struct UiScaleTransform {
    float scale = 1.0f;
    Point origin; // origin is the screen-space origin where the scaled in-game UI image is composited
    
    // Krytyczne helpery dla etapu 3
    Point ScreenToLogical(const Point& screenPoint) const; // dla hit-testing, applies only to the currently scaled in-game UI path and must not be assumed to cover unrelated UI contexts. must account for both scale and composited screen-space origin, including offset from final placement policy
    static float ClampConfigValue(float configScale); // Returns safe hidden scale within implementation-defined range validated by fit testing. Initial path may limit to scale >= 1.0
    
    // Możliwe rozszerzenia (nie wymagane na start)
    // Rect LogicalToScreen(const Rect& logicalRect) const;
};
Render Pipeline Integration (wewnątrz DrawAndBlit)
Wykorzystujemy dokładnie te same helpery, które wydzieliliśmy w Etapie 1.
code
C++
void DrawAndBlit() {
    // ... setup i logiki (zgodnie z Etapem 1) ...
    const Surface &out = GlobalBackBuffer();

    // 1. Render World (zawsze w skali 1.0)
    // IMPORTANT: Phase 3 requires explicit splitting of DrawView() orchestration
    // The current DrawView() contains both world and overlay rendering.
    // For UI scaling, we must separate these to avoid double-rendering
    // when redirecting targeted UI families to intermediate surfaces.
    DrawView(out, ViewPosition); // world + overlays - will need further splitting

    // 2. Render UI & Composite
    const auto& uiTransform = GetCurrentUiTransform();
    if (uiTransform.scale != 1.0f) {
        // Ścieżka ze skalowaniem
        auto uiSurface = allocate an intermediate engine-compatible UI surface for the targeted path; // Create an intermediate UI surface using the same indexed-surface assumptions as the backbuffer, preferably via the existing OwnedSurface / SDL_PIXELFORMAT_INDEX8 path unless investigation proves a different path is safe.
        uiSurface.Clear(TransparentColor); // transparent regions handled using the engine-compatible approach, most likely color keying or equivalent indexed-surface transparency semantics
        
        // Single UI family first (initial targeted path)
        // Phase 3 may start with only one in-game UI family
        // to avoid placement conflicts between different UI families.
        // Examples: main HUD only, or window panels only.
        RenderWindowPanels(uiSurface);
        // RenderMainHudPanel(uiSurface); // May be added in later phase
        // top overlays audit-only unless clearly needed for scaled path
        
        // Skalowanie bufora i nałożenie na główny ekran
        ScaleAndCompositeUi(uiSurface, out, uiTransform); // implementation must follow the existing rendering backend and surface semantics rather than introducing a parallel rendering path 
    } else {
        // Normalna ścieżka (Zero overhead)
        RenderWindowPanels(out);
        // RenderMainHudPanel(out); // Mirror scaled path structure
        // RenderTopOverlays(out);
    }
    
    // ... DrawCursor, DrawMain, etc. (zgodnie z Etapem 1) ...
}
Konfiguracja (developerska)
code
Ini
; Zmiana wymaga wpisu w pliku konfiguracyjnym (nie ma jeszcze suwaka w grze)
; Użycie istniejącego systemu opcji, ukryta/non-user-facing opcja
; exact option name and section should follow the existing options system rather than introducing a parallel config convention
[Development]
ui_scale_factor = 1.2
Kryteria ukończenia

Zbudowany minimalny UiScaleTransform struct z helperami.

Działa renderowanie UI do osobnego uiSurface i poprawny scaled blit z correct transparent compositing using an engine-compatible intermediate surface format.

Prosta reguła pozycjonowania (np. bottom-centered for the lower HUD, subject to fit testing) trzyma UI w odpowiednim miejscu na dużym ekranie.

Funkcja UiHitTesting::ScreenToUiPoint korzysta z ScreenToLogical – klikanie w powiększone elementy ekwipunku i dolnego paska działa idealnie.

Characterization Test z Etapu 0 wykazuje ZERO ZMIAN (0 diff) i brak spadków FPS przy ui_scale_factor = 1.0 (zero diff expectation applies to the ui_scale_factor = 1.0 path only). At non-1.0 scale, the initial success bar is correctness of placement and mouse hit-testing for the targeted in-game UI path, not full parity for all overlays.
Non-goals (Explicit Guardrails)
no full anchor framework yet
no public settings menu
no world zoom / perspective changes
no assumption that all overlays belong to the scaled UI path
no attempt to preserve full top-overlay parity in phase 3
top overlays remain audit-only unless explicitly pulled into the scaled path
no controller/touch rewrite
no hidden singleton/global manager in phase 3
transform must be explicit and passed to required callsites only
Ryzyka
Performance: Rezerwowanie pamięci na uiSurface i operacja sprzętowego (bądź programowego) skalowania może być kosztowna. Kluczowe jest utrzymanie pętli if (scale != 1.0f), by starzy gracze nie odczuli żadnego narzutu.
Artefakty skalowania (Quality): Pixel-art źle znosi skalowanie o ułamki (np. 1.25x). Może być konieczne przetestowanie, czy SDL_RenderCopy używa filtrowania "Nearest Neighbor" czy "Linear" i dobranie tego, które wygląda lepiej dla interfejsu Diablo.
Backend compatibility: classic in-game rendering assumes direct writes into an 8-bit palettized surface. An intermediate UI surface must preserve those assumptions or the result may be corrupted or crash-prone. The compositing step must follow the existing backend semantics instead of introducing a parallel rendering model.
Integration z poprzednimi etapami
Etap 0: Narzędzie gwarantuje, że przy skali 1.0 nic nie popsuliśmy.
Etap 1: Dał nam helpery RenderWindowPanels i RenderMainHudPanel, dzięki czemu możemy je teraz przekierować do uiSurface zamiast głównego out.
Etap 2: Dał nam izolowany namespace UiHitTesting, do którego teraz wpinamy transform danych - cały input myszki "naprawia się" pod nową skalę bez retroaktywnych zmian w etapie 2.

Ważna uwaga architektoniczna
Przy implementacji zachować elastyczność co do zawartości uiSurface - nie zakładać automatycznie, że RenderWindowPanels i RenderMainHudPanel muszą zawsze być skalowane razem jako jeden obraz. Możliwe, że będą to dwie różne rodziny requiring different placement policies.

Important implementation guardrail
The intermediate UI surface must be compatible with the existing 8-bit indexed rendering path used by Surface and GlobalBackBuffer().
Do not assume a generic 32-bit alpha UI buffer.
Prefer an engine-compatible indexed surface approach first, and verify transparency/compositing through the existing backend semantics.
Exact compositing behavior should be validated against CreateBackBuffer(), Surface, and the current blit/present pipeline.