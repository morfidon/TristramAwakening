---
description: Refactor render path into logical world/overlay/window/HUD stages
---

# UI Scaling - Phase 1: Render Path Refactor

**Cel:** Make the existing render call order explicit and easier to evolve for future UI-scaling work, bez zmiany zachowania dla gracza.

**Strategia commitów:** Podzielić implementację na 3 mini-commity dla łatwiejszego review:
1. extract helpery z DrawView()
2. extract RenderMainHudPanel() 
3. cleanup nazw i komentarzy bez zmiany behavior

## Non-goals / semantic guardrails
- no gameplay changes
- no input/hit-testing changes  
- no save/load/state flag changes
- no new render model
- no surface separation
- no movement of touch / present logic
- no cross-file abstraction unless strictly necessary

**Helper localization rule:** Helpery mają być lokalne dla scrollrt.cpp, jeśli to możliwe. Zamiast od razu robić nowe klasy, nowe moduły albo przenosić rzeczy między plikami:
- wyciągasz helpery lokalnie w scrollrt.cpp
- zachowujesz istniejący flow
- nie robisz szerokiej abstrakcji, dopóki nie będzie realnie potrzebna

**Path preservation rule:** PR1 must preserve the semantics of both the main frame path and the lightweight scrollrt_draw_game_screen() path. To odpowiada zasadzie "Audit the full pipeline" - nie patrz tylko na główną ścieżkę, sprawdź też boczne/pomocnicze ścieżki, które dotykają cursor/present. W aktualnym kodzie scrollrt_draw_game_screen() robi osobno UndrawCursor(out), DrawCursor(out), DrawMain(...) i RenderPresent().

To pokazuje, że nie zgadujesz nowych mechanik, nie przeciążasz istniejących pojęć, i bierzesz najmniejszą semantycznie poprawną zmianę.

## Definition of success
**Expected visible outcome:**
- no intended visual change
- no intended input change  
- no intended change in draw timing semantics
- any visible ordering difference is a regression

To jest bardzo mocne i bardzo dobre dla review - brutalnie proste kryteria sukcesu.

## Implementation warnings
**Jedyna realna rzecz, na którą bym uważał - nie w samym planie, tylko w wykonaniu:**

Maintainerom może się nie spodobać, jeśli w trakcie implementacji PR1 niechcący zacznie robić coś z tych rzeczy:
- przenosić odpowiedzialność między DrawView() a DrawAndBlit()
- "upraszczać" redraw flags
- ruszać scrollrt_draw_game_screen()
- zmieniać semantykę debug/touch/present
- wynosić helpery poza scrollrt.cpp bez realnej potrzeby

To są exactly te rzeczy, które mogą spowodować reject PR.

**Implementation notes for helper signatures:**

**RenderWorldOverlays z parametrami:**
Zostawiasz sygnatury dokładnie takie, jak wynikają z aktualnych call sites. Jeśli DrawFloatingNumbers dziś bierze startPosition i offset, to helper też je bierze - mechanicznie. W PR description piszesz wprost: "RenderWorldOverlays takes world-space parameters because DrawFloatingNumbers is world-aware - this matches existing call semantics." Maintainer widzi, że nie wymyślasz, tylko opisujesz to, co już tam jest.

**Warunkowa logika w RenderWindowPanels:**
Też zostawiasz 1:1. W opisie PR-a lub w komentarzu do kodu możesz dodać: "Conditional logic preserved as-is from DrawView() - no behavior change intended." To sygnalizuje, że widziałeś ten chaos i świadomie go nie ruszałeś.

## Scope
- Zgrupowanie renderingu w logiczne etapy: world/overlay/window/HUD
- Bez widocznych zmian dla użytkownika
- Make the existing render call order explicit and easier to evolve for future UI-scaling work
- Bez rozdzielania surface'ów ani render targetów

## Kluczowe pliki
- `Source/engine/render/scrollrt.cpp` - główny scene/HUD pipeline
- `Source/engine/dx.cpp` - reference only: confirms that virtual gamepad is appended during RenderPresent() and stays out of scope for phase 1
- Opcjonalnie pomocniczo: `Source/controls/touch/renderers.h`

## Funkcje do refactorowania
- `DrawView()` - główny scene rendering pipeline
- `DrawAndBlit()` - backbuffer composition before DrawMain()/RenderPresent()

**Uwaga:** `DrawCursor()` i `RenderPresent()` pozostają poza zakresem etapu 1.

## 1. Gdzie patrzeć najpierw

Najważniejsze miejsce to dziś `Source/engine/render/scrollrt.cpp`. Tam `DrawGame()` rysuje bazę świata - `DrawFloor`, `DrawTileContent`, `DrawOOB` - i uwzględnia też to, że boczne panele mogą zasłaniać viewport. Potem `DrawView()` dokłada kolejne warstwy w konkretnej kolejności. W tym samym pliku są też helpery `DrawCursor` / `UndrawCursor`, więc kursor najpewniej jest obsługiwany jako osobna, bardzo późna warstwa, a nie część samego `DrawView()`.

## 2. Warstwy stanowe - czyli co zasłania co

Z samej kolejności w `DrawView()` wychodzi taki bardzo praktyczny obraz:

### Świat
- `DrawGame`

### Overlay świata
- `DrawAutomap`
- `DrawItemNameLabels`
- `DrawMonsterHealthBar`
- `DrawFloatingNumbers`

### Panele/okna
- store / inv / spellbook
- char / quest / stash
- qtext / spell list / gold split / gold withdraw
- help / chatlog

### Overlay stanów gry
- death screen albo pause
- diablo msg

### HUD / overlay końcowy
- controller hints
- player messages
- game menu
- doom
- info box
- life/mana flasks

### Prawdopodobnie jeszcze później
- kursor software/hardware, bo helpery kursora siedzą obok tego pipeline'u, ale nie są częścią samej sekwencji `DrawView()`.

## 3. Co tu jest najważniejsze dla etapu 1

Najważniejszy wniosek jest taki:

Nie masz dziś prostego podziału tylko na „świat" i „UI". Masz raczej:
- world
- world overlays  
- panel/modal UI
- HUD/top-most overlays
- cursor

I to jest właśnie dobra wiadomość, bo etap 1 nie musi od razu robić idealnej architektury. Wystarczy, że rozdzielisz to logicznie na wyższe stages, nawet jeśli nie wszystko od razu trafi do osobnych surface'ów.

## 4. Moja robocza rozpiska do refactoru

Ja bym to sobie zapisał dokładnie tak:

### A. World stage
`DrawGame`
- `DrawGame(...)`
- `DrawFloor(...)`
- `DrawTileContent(...)`
- `DrawOOB(...)`
- `Zoom(...)`

**Uwaga:** World rendering już dziś jest panel-aware - zmniejsza viewport gdy lewy/prawy panel jest otwarty.

### B. World overlay stage
`DrawAutomap`
- `DrawAutomap(...)`
- `DrawItemNameLabels(...)`
- `DrawMonsterHealthBar(...)`
- `DrawFloatingNumbers(...)`

**Charakter:** World-aware ale rysowane "na wierzchu" świata, przed panelami. Dodatkowo `DrawFloatingNumbers(out, startPosition, offset)` dostaje parametry świata, więc to bardzo mocno sugeruje warstwę world-aware, nie zwykły panel HUD. `DrawAutomap` też jest rysowany na subregionie viewportu, od razu po świecie.

### C. Panel / modal UI stage
`DrawSText`
- `DrawSText(...)` - sklep/rozmowy
- `DrawInv(...)` - inventory
- `DrawSpellBook(...)` - księga zaklęć
- `DrawDurIcon(...)` - durability icon
- `DrawChr(...)` - character panel
- `DrawQuestLog(...)` - dziennik zadań
- `DrawStash(...)` - stash
- `DrawLevelButton(...)` - level button
- `DrawUniqueInfo(...)` - unique info
- `DrawQText(...)` - quest text
- `DrawSpellList(...)` - lista zaklęć
- `DrawGoldSplit(...)` - dzielenie złota
- `DrawGoldWithdraw(...)` - wypłata złota
- `DrawHelp(...)` - pomoc
- `DrawChatLog(...)` - chat log

**Charakter:** To są klasyczne panele UI. `DrawSTextBack` też używa `GetUIRectangle().position` i półprzezroczystego tła dla store/talk panelu.
`DrawChatLog` używa `DrawQTextBack(out)` i `GetUIRectangle()`, czyli to kolejny pełnoekranowy panel UI.

### D. Top overlay / HUD stage
death / pause
- Death/Pause screens
- `DrawDiabloMsg(...)`
- `DrawControllerModifierHints(...)`
- `DrawPlrMsg(...)`
- `gmenu_draw(...)`
- `doom_draw(...)`
- `DrawInfoBox(...)`
- `UpdateLifeManaPercent()`
- `DrawLifeFlaskUpper(...)`
- `DrawManaFlaskUpper(...)`

**Charakter:** To jest późny overlay, ale wciąż część DrawView().

### E. Cursor boundary handling
`UndrawCursor(...)` - before redraw
`DrawCursor(...)` - after HUD composition, before `DrawMain(...)`

**Charakter:** To nie jest jeden zwarty stage, tylko dwie graniczne operacje:
- `UndrawCursor()` jest pre-frame restore step
- `DrawCursor()` jest late post-HUD draw
- `RenderPresent()` jest jeszcze później

**Architektoniczna uwaga:** Nie tworzą jednego zwykłego stage, więc nie powinny być nazywane "Cursor stage" czy "RenderCursorFrame()".

To nie musi być finalna architektura. Ale jako mini-notes przed etapem 1 - to już jest bardzo sensowna mapa.

## 5. Największe ryzyka w etapie 1

To są miejsca, gdzie bym najbardziej uważał:

### Draw order - coś nagle zacznie być pod panelem zamiast nad nim
- Najważniejsze ryzyko - przypadkowe zmiany kolejności rysowania
- Musi być zachowana dokładna sekwencja z `DrawView()`

### Cursor sync issues
- DrawCursor()/UndrawCursor() muszą pozostać na obecnej boundary pozycji - poza DrawView(), po main HUD composition i przed DrawMain()
- Helpery kursora siedzą obok głównego pipeline'u

### Item labels / chat / console / info overlays - one często są na pograniczu "czy to jeszcze świat czy już UI"
- `DrawItemNameLabels` - world overlay ale może być traktowane jako UI
- `DrawChatLog` - pełnoekranowy panel UI
- `DrawInfoBox` - HUD ale może zależeć od UI state

### Automap - bo to pół-overlay, pół-osobny tryb
- Rysowany na subregionie viewportu
- Może mieć specyficzne dependencies od UI panels

### Inventory / spellbook / stores / help - bo to już mocno UI-centric
- Wszystkie używają `GetPanelPosition()` lub `GetUIRectangle()`
- Muszą pozostać w UI stage

### Fade / palette / clipping - jeśli jakiś efekt zakłada konkretną kolejność rysowania
- Efekty visual mogą zakładać określoną kolejność warstw
- Muszą być zachowane po refaktorze

### Panel-aware elements
- World rendering już dziś jest panel-aware (zmniejsza viewport przy otwartych panelach)
- `DrawPlrMsg` zmienia szerokość w zależności od otwartych paneli
- Te zachowania muszą być zachowane

### Touch/Mobile controls
- Virtual gamepad jest doklejany w `RenderPresent()`, nie w głównym pipeline
- Touch/mobile ma całkowicie oddzielony render path
- Refactor etapu 1 nie powinien wpłynąć na touch/mobile controls

Repo ma wiele takich warstw UI i paneli w osobnych plikach/modułach, więc to nie jest czysta jedna ścieżka renderu. Widać katalogi controls, panels, pliki help.cpp, inv.cpp, interfac.cpp, cursor.cpp, automap.cpp, gamemenu.cpp.

## 6. Co faktycznie maluje main panel

### DrawMain() nie rysuje main panelu od zera
`DrawMain()` nie rysuje main panelu od zera. To jest głównie partial present / partial blit na output surface. Widać to po tym, że robi `DoBlitScreen(...)` dla konkretnych prostokątów: górna część ekranu, belt, info box, fragmenty HP/Mana, przyciski i obszary kursora. Czyli ono zakłada, że te rzeczy są już wcześniej narysowane w backbufferze.

### Co naprawdę składa pełną klatkę
To, co naprawdę składa pełną klatkę, siedzi w `DrawAndBlit()`. Tam kolejność jest taka:
1. `DrawView(out, ViewPosition)` - świat i duża część overlayów
2. Potem runtime painterzy panelu i HUD:
   - `DrawMainPanel(out)`
   - `DrawLifeFlaskLower(...)`
   - `DrawManaFlaskLower(...)`
   - `DrawSpell(out)`
   - `DrawMainPanelButtons(out)`
   - `DrawInvBelt(out)`
   - `DrawChatBox(out)`
   - `DrawXPBar(out)`
   - opcjonalnie flask values / floating info box / party panel
3. Potem `DrawCursor(out)`, `DrawFPS(out)`
4. I dopiero na końcu `DrawMain(...)` + `RenderPresent()`

**Wniosek:** pełny paint jest w `DrawAndBlit()` do `GlobalBackBuffer()`, a `DrawMain()` to już etap prezentacji wybranych regionów.

### Co z mainpanel.cpp
`Source/panels/mainpanel.cpp` wygląda bardziej na przygotowanie assetów panelu, a nie główny runtime render frame-by-frame. Tam widać pre-render tłumaczonych stanów przycisków do `PanelButtonDown`, a nawet rysowanie pewnych przycisków do `BottomBuffer` podczas ładowania zasobów. To mocno sugeruje: ten plik przygotowuje elementy panelu, ale nie jest centralnym miejscem składania każdej klatki.

### Wniosek do etapu 1
Jeśli chcesz rozdzielać render na stages, to punktem cięcia nie powinno być `DrawMain()`. Punktem cięcia powinno być `DrawAndBlit()`, bo to ono naprawdę układa frame pipeline.

## 7. Checklist do etapu 1

**Najważniejszy wniosek przed refactorem jest taki:** nie ruszasz w PR 1 `DrawMain()` / `DoBlitScreen()`, nie ruszasz `DrawCursor()` / `UndrawCursor()` jako granicy dirty-blit, i nie ruszasz `RenderPresent()`. Dodatkowo touch/mobile nie siedzi w `DrawView()`, tylko jest dokładany przez `RenderVirtualGamepad(...)` przy `RenderPresent()` dla `ControlTypes::VirtualGamepad`, więc to też powinno zostać poza etapem 1.

### A. Najpierw rozdziel to mentalnie na 4 warstwy

#### 1. World stage
To, co jest czystym światem:
- `DrawGame(...)`

To jest najczystszy kandydat na osobny stage. `DrawGame()` robi floor, tile content, out-of-bounds i zoom. Dodatkowo już dziś jest panel-aware, bo uwzględnia zasłanianie viewportu przez panele boczne.

#### 2. World overlay stage
Rzeczy jeszcze związane z widokiem gry, ale już nad światem:
- `DrawAutomap(...)`
- `DrawItemNameLabels(...)`
- `DrawMonsterHealthBar(...)`
- `DrawFloatingNumbers(...)`

To wszystko dzieje się zaraz po `DrawGame()` i przed oknami/panelami.

#### 3. Window / panel UI stage
Okna i większe panele:
- `DrawSText(...)`
- `DrawInv(...)`
- `DrawSpellBook(...)`
- `DrawDurIcon(...)` ← przeniesione z HUD stage
- `DrawChr(...)`
- `DrawQuestLog(...)`
- `DrawStash(...)`
- `DrawLevelButton(...)` ← przeniesione z HUD stage
- `DrawUniqueInfo(...)` ← przeniesione z HUD stage
- `DrawQText(...)`
- `DrawSpellList(...)`
- `DrawGoldSplit(...)`
- `DrawGoldWithdraw(...)`
- `DrawHelp(...)`
- `DrawChatLog(...)`

To jest sensowna grupa, bo te rzeczy są klasycznym UI. Sklep używa wspólnego UI boxa i `GetUIRectangle()`, quest/help/chatlog też bazują na tym samym typie ekranowego panelu, a spellbook używa pozycji panelu (`GetPanelPosition(...)`).

**Ważna poprawka:** `DrawDurIcon`, `DrawLevelButton`, `DrawUniqueInfo` są tutaj, bo w aktualnym `DrawView()` kolejność jest:
1. `DrawInv`/`DrawSpellBook`
2. `DrawDurIcon`
3. `DrawChr`/`DrawQuestLog`/`DrawStash`
4. `DrawLevelButton`
5. `DrawUniqueInfo`
6. dopiero potem reszta

**Uwaga:** Nazwa helpera to `RenderWindowPanels()`, bo to jeszcze nie jest docelowy UI stage w sensie osobnego surface'a - tylko porządkowanie draw calli.

#### 4. HUD / top overlay stage
To, co jest późnym overlayem:
- death / pause overlay
- `DrawDiabloMsg(...)`
- `DrawControllerModifierHints(...)`
- `DrawPlrMsg(...)`
- `gmenu_draw(...)`
- `doom_draw(...)`
- `DrawInfoBox(...)`
- `UpdateLifeManaPercent()`
- `DrawLifeFlaskUpper(...)`
- `DrawManaFlaskUpper(...)`

`DrawPlrMsg()` jest tu dobrym przykładem hybrydy - jest ekranowe, ale liczy szerokość i pozycję względem main panelu oraz side paneli.

### B. Osobno wydziel "main panel composition stage"
Ta grupa nie należy do `DrawMain()`, tylko do runtime składania backbuffera w `DrawAndBlit()`:
- `DrawMainPanel(out)`
- `DrawLifeFlaskLower(...)`
- `DrawManaFlaskLower(...)`
- `DrawSpell(out)`
- `DrawMainPanelButtons(out)`
- `DrawInvBelt(out)`
- `DrawChatBox(out)`
- `DrawXPBar(out)`
- `DrawFlaskValues(...)`
- `DrawFloatingInfoBox(out)`
- `DrawPartyMemberInfoPanel(out)`

To jest praktycznie osobny stage: main panel shell + jego elementy + dolny HUD. Belt też jest wyraźnie zakotwiczony do `GetMainPanel().position`.

**Moja rada:** nazwij to sobie roboczo np. `RenderMainHudPanel()`. Bo to nie jest ani czysty world stage, ani zwykłe modalne okno.

### C. DrawMain() zostaw na razie w spokoju
Na etapie 1 nie próbuj go "naprawiać" ani zmieniać semantyki.

**Checklist:**
- nie zmieniać logiki `DoBlitScreen(...)`
- nie przenosić jeszcze dirty-rect logic do nowych klas
- nie mieszać tego z independent scaling
- ewentualnie dodać komentarz, że to jest present / dirty-region blit, nie full paint

To zmniejszy ryzyko. `DrawMain()` jest dziś ważne dla częściowego odświeżania i kursora.

### D. Konkretna checklista plik po pliku

#### scrollrt.cpp
- wyciągnąć z `DrawView()` helper `RenderWorldPass(...)`
- wyciągnąć helper `RenderWorldOverlays(...)` z parametrami world data
- wyciągnąć helper `RenderWindowPanels(...)`  ← nazwa zmieniona z `RenderUiStage`
- wyciągnąć helper `RenderTopOverlays(...)`
- w `DrawAndBlit()` wyciągnąć helper `RenderMainHudPanel(...)` z flagami redrawu
- zachować identyczną kolejność draw calls
- nie zmieniać jeszcze inputu
- nie zmieniać jeszcze surface targetów
- nie zmieniać jeszcze zoom/scaling behavior

To jest najważniejszy plik etapu 1.

#### stores.cpp
- nic nie zmieniać w logice sklepu
- tylko upewnić się, że `DrawSText(...)` zostaje wołane z nowego helpera `RenderWindowPanels(...)` 
- zanotować, że store UI używa `GetUIRectangle()` i wspólnego store/text box background

#### minitext.cpp
- potraktować `DrawQText(...)` jako klasyczne fullscreen-ish UI window
- nie mieszać go z world overlay
- zanotować zależność od `DrawQTextBack(...)`

#### help.cpp
- potraktować `DrawHelp(...)` jak panel UI korzystający z `DrawQTextBack(...)`
- nie przenosić jeszcze żadnej logiki scrolla
- zostawić behavior 1:1

#### qol/chatlog.cpp
- potraktować `DrawChatLog(...)` jako panel UI w tej samej rodzinie co help/qtext
- nie mieszać z `DrawPlrMsg(...)`
- `DrawChatLog()` też siada na `DrawQTextBack(...)`, więc to nie jest HUD tylko okno/panel.

#### plrmsg.cpp
- zostawić w top overlay / HUD stage
- oznaczyć jako hybrid HUD, bo zależy od main panelu i side paneli
- nic nie zmieniać w timeoucie ani layout policy

#### inv.cpp
- `DrawInvBelt(...)` zostawić w `RenderMainHudPanel(...)`
- nie mieszać beltu z `DrawInv(...)`
- zanotować, że belt jest pozycjonowany względem `GetMainPanel().position`

To ważne, bo inventory window i belt to dwa różne typy UI.

#### panels/spell_book.cpp
- `DrawSpellBook(...)` trzymać w `RenderWindowPanels(...)`
- nie wrzucać go do main panelu
- zanotować, że używa `GetPanelPosition(UiPanels::Spell, ...)`

#### panels/mainpanel.cpp
- na razie nie traktować go jako centrum render pipeline
- rozumieć go głównie jako asset/pre-render setup dla przycisków
- nie robić tam etapu 1, jeśli nie musisz

### E. Czego nie dotykać w etapie 1
- `DrawMain()` / `DoBlitScreen()` - dirty-rect semantics
- `DrawCursor()` / `UndrawCursor()` - granica dirty-blit
- `RenderPresent()` - touch/mobile jest doklejany tutaj
- `RenderVirtualGamepad(...)` dla `ControlTypes::VirtualGamepad`
- hit-testing
- kliknięcia i mapping mouse → UI
- controller navigation
- tooltip placement
- osobne surface'y dla UI/world
- publiczne UI scaling options

**Kluczowe:** Touch/mobile nie siedzi w `DrawView()`, tylko jest renderowany przez `RenderVirtualGamepad(...)` przy `RenderPresent()`. To musi pozostać poza etapem 1.

To wszystko powinno wejść dopiero później. Z samej struktury `DrawAndBlit()` i `DrawMain()` widać, że już teraz pipeline ma kilka warstw i dirty-region assumptions, więc dokładanie input/scalingu do pierwszego PR-a tylko go napompuje.

### F. Nazewnictwo helperów w PR 1

**Mała korekta do nazewnictwa:**

Ja bym nawet rozważył nazwę:
- `RenderWindowPanels` zamiast `RenderUiStage`

Bo na tym etapie to jeszcze nie jest docelowy UI stage w sensie architektury osobnego surface'a. To po prostu porządkowanie draw calli. Dzięki temu opis PR-a będzie uczciwy.

**Ostateczne nazwy helperów:**
- `RenderWorldPass(...)`
- `RenderWorldOverlays(...)`
- `RenderWindowPanels(...)`  ← zmienione z `RenderUiStage`
- `RenderTopOverlays(...)`
- `RenderMainHudPanel(...)`

I potem:
- `DrawView(...)` staje się cienkim orkiestratorem tych helperów
- `DrawAndBlit(...)` dalej robi backbuffer composition + partial present

To brzmi zwyczajnie. I właśnie o to chodzi.

### G. Najważniejszy praktyczny wniosek
Etap 1 nie ma jeszcze rozdzielać surface'ów. Ma tylko sprawić, że z kodu będzie jasno wynikało:
- co jest światem,
- co jest overlayem świata,
- co jest oknem UI,
- co jest HUD-em,
- co jest composition dolnego panelu,
- a co jest już tylko present/blit.

## 9. Czego nie ruszać w tym szkicu

**Tego bym pilnował bardzo twardo:**

- nie przenoś `DrawCursor(out)` do `DrawView()`
- nie przenoś `DrawMain(...)`
- nie przenoś `RenderPresent()`
- nie próbuj jeszcze wyciągać osobnych surface'ów
- nie dotykaj hit-testingu, tooltipów, controller navigation
- nie mieszaj touch/gamepad renderingu do backbuffera

**To ostatnie jest istotne, bo virtual gamepad ma własne API renderujące dla `SDL_Renderer*` i `SDL_Surface*`, a jego render jest dopinany przy present, nie w głównym world/UI stage'u.**

## 8. Touch/Mobile i Virtual Gamepad - Important Finding

### Touch/Mobile nie wchodzi w DrawView() ani DrawAndBlit()
Mam już ważny wynik: touch/mobile nie wchodzi w `DrawView()` ani `DrawAndBlit()` jako zwykły draw call do backbuffera. Virtual gamepad jest doklejany dopiero w `RenderPresent()`, więc to mocny argument, żeby zostawić present path poza etapem 1.

**To oznacza, że touch/mobile rendering jest całkowicie oddzielony od głównego pipeline'u renderowania gry i jest dodawany na samym końcu procesu prezentacji.**

### Implikacje dla etapu 1
- ✅ Wzmacnia to decyzję, żeby nie dotykać `DrawMain()` i `RenderPresent()` w etapie 1
- ✅ Potwierdza, że `DrawAndBlit()` jest właściwym punktem cięcia dla refactoru
- ✅ Touch/mobile jest już naturalnie oddzielony od głównego render pipeline
- ✅ Touch/mobile should remain unaffected as long as RenderPresent() and virtual gamepad rendering stay untouched

### Dlaczego to jest ważne
Virtual gamepad rendering is appended during RenderPresent() and should remain unaffected as long as RenderPresent() and virtual gamepad rendering stay untouched. Touch/mobile controls często mają swoje własne systemy renderowania i mogą używać różnych surface'ów lub technik kompozycji. Fakt, że są doklejane dopiero w `RenderPresent()`, oznacza że:
1. Są renderowane po głównym frame bufferze gry
2. Mają niezależny pipeline od UI scaling
3. Touch/mobile should remain unaffected as long as RenderPresent() and virtual gamepad rendering stay untouched

### Action items dla etapu 1
- Potwierdzić, że `RenderPresent()` nie jest zmieniany
- Upewnić się, że touch/mobile pozostaje poza zakresem etapu 1
- Dodać notatkę w PR description, że touch/mobile should remain unaffected as long as RenderPresent() and virtual gamepad rendering stay untouched

## 10. Kolejność w DrawView() - trzeba zachować 1:1

**Tekstowo ta kolejność jest dziś taka:**

1. wyczyścić debug mapę współrzędnych w debug buildzie,
2. policzyć offset przez `CalcFirstTilePosition(startPosition, offset)`,
3. narysować świat przez `DrawGame(out, startPosition, offset)`,
4. jeśli automapa jest aktywna - narysować automapę,
5. w debug buildzie - narysować debug grid / debug text / RedrawEverything(),
6. narysować item labels,
7. narysować monster health bar,
8. narysować floating numbers,
9. jeśli gracz jest w sklepie i nie ma qtextflag - narysować store text,
10. jeśli invflag - narysować inventory, w przeciwnym razie jeśli SpellbookFlag - spellbook,
11. narysować durability icon,
12. jeśli CharFlag - character panel, inaczej jeśli QuestLogIsOpen - quest log, inaczej jeśli IsStashOpen - stash,
13. narysować level button,
14. jeśli ShowUniqueItemInfoBox - unique info,
15. jeśli qtextflag - quest text,
16. jeśli SpellSelectFlag - spell list,
17. jeśli DropGoldFlag - gold split,
18. zawsze wywołać `DrawGoldWithdraw`,
19. jeśli HelpFlag - help,
20. jeśli ChatLogFlag - chat log,
21. jeśli gracz nie żyje - RedBack + death text, inaczej jeśli pauza - pause overlay,
22. jeśli jest komunikat Diablo - `DrawDiabloMsg`,
23. narysować controller modifier hints,
24. narysować player messages,
25. narysować game menu,
26. narysować doom overlay,
27. narysować info box,
28. zaktualizować procenty życia/many,
29. narysować górną część flaszki życia,
30. narysować górną część flaszki many.

**To jest właśnie ta logiczna sekwencja, której bym nie ruszał semantycznie w PR 1. Możesz ją tylko pociąć na helpery, ale order musi zostać taki sam.**

### Jak ta sekwencja ma być podzielona na helpery:

```cpp
void DrawView(Surface &out, Point startPosition) {
    // Setup
    ClearDebugMapCoordinates();  // pseudocode only - helper name illustrative, current code uses DebugCoordsMap.clear()
    CalcFirstTilePosition(startPosition, offset);
    
    // World stage
    RenderWorldPass(out, startPosition, offset);
    
    // World overlay stage  
    RenderWorldOverlays(out, startPosition, offset);  ← UWAGA: potrzebuje world data!
    
    // Window panels stage
    RenderWindowPanels(out);
    
    // Top overlay stage
    RenderTopOverlays(out);
}
```

**Gdzie:**
- **Setup** zawiera kroki 1-2 (debug clear, CalcFirstTilePosition)
- `RenderWorldPass()` zawiera krok 3 (DrawGame)
- `RenderWorldOverlays()` zawiera kroki 4-8 (automapa, debug, labels, health bar, floating numbers)
- `RenderWindowPanels()` zawiera kroki 9-20 (store, inv/spellbook, durability, char/quest/stash, level button, unique info, quest text, spell list, gold split/withdraw, help, chat log)
- `RenderTopOverlays()` zawiera kroki 21-30 (death/pause, diablo msg, controller hints, player messages, game menu, doom, info box, life/mana flasks)

**Ważna uwaga:** `RenderWorldOverlays()` potrzebuje `startPosition` i `offset`, bo `DrawFloatingNumbers(out, startPosition, offset)` jest wołane z tymi parametrami w aktualnym kodzie.

## 11. Kolejność w DrawAndBlit() - też musi zostać

**Poza DrawView() obecny pipeline jest taki:**

1. sprawdzić early return: `!gbRunGame || HeadlessMode`,
2. policzyć flagi redrawu (`drawHealth`, `drawMana`, `drawControlButtons`, `drawBelt`, `drawChatInput`, `drawInfoBox`, `drawCtrlPan`, `hgt`),
3. pobrać `GlobalBackBuffer()`,
4. zrobić `UndrawCursor(out)`,
5. zrobić `nthread_UpdateProgressToNextGameTick()`,
6. wywołać `DrawView(out, ViewPosition)`,
7. jeśli `drawCtrlPan` - `DrawMainPanel(out)`,
8. jeśli `drawHealth` - dolna część flaszki życia,
9. jeśli `drawMana` - dolna część flaszki many, a potem `DrawSpell(out)`,
10. jeśli `drawControlButtons` - `DrawMainPanelButtons(out)`,
11. jeśli `drawBelt` - `DrawInvBelt(out)`,
12. jeśli `drawChatInput` - `DrawChatBox(out)`,
13. zawsze `DrawXPBar(out)`,
14. opcjonalnie wartości HP,
15. opcjonalnie wartości many,
16. opcjonalnie floating info box,
17. opcjonalnie party member info panel,
18. dopiero teraz `DrawCursor(out)`,
19. potem `DrawFPS(out)`,
20. potem `lua::GameDrawComplete()`,
21. potem `DrawMain(...)` - czyli partial blit/present logic na dirty rectach,
22. w debug buildzie `DrawConsole(out)`,
23. potem `RedrawComplete()` i czyszczenie flag komponentów,
24. na końcu `RenderPresent()`

**Kluczowe obserwacje:**
- `DrawMain()` jest dopiero po `DrawCursor()` i `DrawFPS()` - to potwierdza, że to jest partial blit, nie full paint
- `RenderPresent()` jest na samym końcu - tu jest doklejany touch/gamepad
- `DrawMainPanel()` i inne elementy main panel są renderowane przed `DrawCursor()`
- Flaszki mają osobne flagi dla górnej i dolnej części

### Jak DrawAndBlit() ma być podzielone w etapie 1:

```cpp
void DrawAndBlit() {
    // Early checks
    if (!gbRunGame || HeadlessMode) return;
    
    // Setup
    CalculateRedrawFlags();  // drawCtrlPan, drawHealth, drawMana, drawControlButtons, drawBelt, drawChatInput, drawInfoBox, drawCtrlPan, hgt
    auto &out = GlobalBackBuffer();
    UndrawCursor(out);
    nthread_UpdateProgressToNextGameTick();
    
    // Main frame rendering
    DrawView(out, ViewPosition);
    
    // Main HUD panel stage
    RenderMainHudPanel(out, drawCtrlPan, drawHealth, drawMana, drawControlButtons, drawBelt, drawChatInput);
    
    // Post-frame elements
    DrawCursor(out);
    DrawFPS(out);
    lua::GameDrawComplete();
    
    // Present pipeline (nie ruszać w etapie 1)
    DrawMain(...);           // partial blit
    DrawConsole(out);        // debug only
    RedrawComplete();        // flag cleanup
    RenderPresent();        // final present + touch/gamepad
}
```

**Gdzie `RenderMainHudPanel()` przyjmuje flagi redrawu i zawiera kroki 7-17:**
- `DrawMainPanel(out)` (jeśli `drawCtrlPan`)
- dolna część flaszki życia (jeśli `drawHealth`)
- dolna część flaszki many + `DrawSpell(out)` (jeśli `drawMana`) ← **ważne: DrawSpell musi pozostać sprzężone z drawMana**
- `DrawMainPanelButtons(out)` (jeśli `drawControlButtons`)
- `DrawInvBelt(out)` (jeśli `drawBelt`)
- `DrawChatBox(out)` (jeśli `drawChatInput`)
- `DrawXPBar(out)` (zawsze)
- opcjonalne wartości HP/many/info box/party panel

**Ważne:** Helper musi przyjmować flagi redrawu, bo to jest część semantyki, której nie chcemy zmieniać. `DrawAndBlit()` dziś bardzo wyraźnie bazuje na tych flagach przed rysowaniem panelu, flaszek, przycisków, beltu i czatu.

**Czego nie ruszać w etapie 1:**
- `DrawMain()` - partial blit logic
- `RenderPresent()` - final present + touch/gamepad
- `DrawCursor()` / `UndrawCursor()` - cursor handling
- `DrawConsole(out)` - zostaje tam, gdzie jest dziś, tylko w _DEBUG, po DrawMain(...), przed bookkeepingiem i RenderPresent()
- Flagi redrawu - zostają jak są

### Dodatkowa uwaga: scrollrt_draw_game_screen()
Warto dodać do notatek:
- istnieje `scrollrt_draw_game_screen()`
- używa `UndrawCursor(out)`, `DrawCursor(out)`, `DrawMain(...)`, `RenderPresent()`
- Phase 1 nie powinien zmieniać jego semantyki
- Phase 1 must not change `scrollrt_draw_game_screen()` semantics or attempt to route it through the new `DrawView` helper split

To jest mała rzecz, ale pokazuje, że cursor/present boundary nie żyją wyłącznie w `DrawAndBlit()`.

## Aktualna struktura
```
DrawAndBlit() -> setup / UndrawCursor -> DrawView() -> main HUD composition -> DrawCursor() -> DrawFPS() -> lua hook -> DrawMain() -> debug console -> redraw bookkeeping -> RenderPresent()
```

## 12. Najważniejsze granice, których nie wolno pomylić

**Dla PR 1 trzy granice są święte:**
1. `DrawView()` odpowiada za główną zawartość sceny i overlaye
2. `DrawAndBlit()` dokłada dolny HUD, kursor, FPS i odpala `DrawMain(...)`
3. `RenderPresent()` musi zostać na samym końcu

**Czyli w praktyce:**

- nie przenosisz `DrawCursor(out)` do `DrawView()`, bo dziś jest rysowany dopiero po HUD-zie i przed `DrawMain(...)`
- nie przenosisz `DrawMain(...)` do środka nowego stage, bo to jest już etap blit/present logic, a nie zwykłe malowanie sceny
- nie ruszasz `RenderPresent()`, bo to finalny koniec pipeline'u

**Te granice definiują:**
- **Scene rendering:** `DrawView()` - świat + window panels + top overlays
- **HUD composition:** `DrawAndBlit()` do `DrawMain()` - main panel + kursor + FPS
- **Present phase:** `DrawMain()` + `RenderPresent()` - partial blit + final present + touch/gamepad

**Dlaczego to jest ważne:**
- `DrawCursor()` musi być narysowany po wszystkich elementach HUD, żeby był na wierzchu
- `DrawMain()` zakłada, że wszystko jest już narysowane w backbufferze - to partial blit, nie full paint
- `RenderPresent()` to miejsce gdzie touch/gamepad jest doklejany do finalnego obrazu

**W PR 1 tylko:**
- Refaktoryzujesz wewnątrz `DrawView()` na helpery
- Refaktoryzujesz wewnątrz `DrawAndBlit()` (do `DrawMain()`) na `RenderMainHudPanel()`
- Nie ruszasz granic między tymi trzema fazami

## 13. Najprostsza wersja do zapisania sobie obok kodu

**Możesz to sobie skrócić do takiego "ściągawkowego" orderu:**

### DrawView:
world → automap/debug → item/monster/floating overlays → windows/panels → death/pause/diablo → hints/messages/menus → info box → upper flasks

### DrawAndBlit:
undraw cursor → update tick progress → DrawView → main panel shell → lower flasks/spell → buttons/belt/chat/xp → optional value/info/party overlays → cursor → FPS → lua hook → DrawMain → debug console → redraw bookkeeping → present

## Docelowa struktura (po etapie 1)
```
DrawAndBlit()
  -> setup / redraw flags / UndrawCursor
  -> DrawView()
       -> setup (debug clear, CalcFirstTilePosition)
       -> RenderWorldPass()
       -> RenderWorldOverlays()
       -> RenderWindowPanels()
       -> RenderTopOverlays()
  -> RenderMainHudPanel()
  -> DrawCursor() / DrawFPS() / lua hook
  -> DrawMain() / DrawConsole() / bookkeeping
  -> RenderPresent()
```

## World Stage
- `DrawGame(...)`
- `DrawFloor(...)`
- `DrawTileContent(...)`
- `DrawOOB(...)`
- `Zoom(...)`

## World Overlay Stage
- `DrawAutomap(...)`
- `DrawItemNameLabels(...)`
- `DrawMonsterHealthBar(...)`
- `DrawFloatingNumbers(...)`

## Window/Panel Stage
- `DrawSText(...)` - sklep/rozmowy
- `DrawInv(...)` - inventory
- `DrawSpellBook(...)` - księga zaklęć
- `DrawDurIcon(...)` - durability icon
- `DrawChr(...)` - character panel
- `DrawQuestLog(...)` - dziennik zadań
- `DrawStash(...)` - stash
- `DrawLevelButton(...)` - level button
- `DrawUniqueInfo(...)` - unique item info
- `DrawQText(...)` - quest text
- `DrawSpellList(...)` - lista zaklęć
- `DrawGoldSplit(...)` - dzielenie złota
- `DrawGoldWithdraw(...)` - wypłata złota
- `DrawHelp(...)` - pomoc
- `DrawChatLog(...)` - chat log

## Top Overlay/HUD Stage
- Death/Pause screens
- `DrawDiabloMsg(...)`
- `DrawControllerModifierHints(...)`
- `DrawPlrMsg(...)` - wiadomości gracza (panel-aware!)
- `gmenu_draw(...)` - menu gry
- `doom_draw(...)` - doom screen
- `DrawInfoBox(...)`
- `UpdateLifeManaPercent()`
- `DrawLifeFlaskUpper(...)`
- `DrawManaFlaskUpper(...)`

## Kryteria ukończenia
- [ ] Jedno miejsce orkiestrujące cały frame
- [ ] Logiczny podział na 4 scene/UI stages w DrawView() + 1 HUD composition helper w DrawAndBlit(), przy zachowaniu osobnych granic dla cursor/present
- [ ] Bez zmiany draw order
- [ ] Bez nowych ustawień/config keys
- [ ] Screenshoty before/after identyczne
- [ ] Brak regresji w town, dungeon, inventory, etc.

## Ryzyka
- Draw order changes
- Cursor sync issues
- Panel-aware elements breaking
- Partial updates interference

## Czego NIE robić
- Nie dodawać UI scale settings
- Nie zmieniać współrzędnych inputu (to etap 2)
- Nie tworzyć osobnych framebufferów (jeszcze nie)
- Nie mieszać zoomu świata i UI

## Next steps
Dyskusja z maintainerami, potem implementacja etapu 1.

## PR Description Template
```
Summary: Refactor render path into logical world / overlay / window / HUD stages
No intended behavior change - preparatory step for future UI scaling

What changed:
- Grouped world rendering into dedicated stage
- Grouped world overlays into dedicated stage  
- Grouped UI panels into dedicated stage
- Grouped top overlays into dedicated stage
- Centralized frame composition order
- Added RenderMainHudPanel() helper for bottom panel composition

This phase does not separate UI and world into different surfaces. It only makes the existing call order explicit and easier to evolve for future UI-scaling work.

Why:
- Issue #8300 shows real readability problems at higher resolutions
- Issue #8388 discusses independent scaling as long-term direction
- This PR intentionally limits scope to render-path preparation
- No surface separation or render target changes in this phase
```