# GOOD UI → Bitware Integration Plan

## Overview

Integrate the GOOD framework UI into Bitware by adapting it as an `IMenuRenderer`
implementation, replacing the 955-line `LegacyMenuRenderer` with GOOD's full
widget library (tabs, sliders, checkboxes, keybinds, color pickers, dropdowns,
search, notifications).  Bitware's existing DX11 overlay, ImGui instance, and
rendering pipeline remain untouched — GOOD runs inside them.

---

## Phase 0 — Code Layout

```
Bitware/Source/Core/UI/
├── IMenuRenderer.h              ← unchanged
├── LegacyMenuRenderer.h/cpp     ← kept but no longer the default
├── GoodMenuRenderer.h/cpp       ← NEW: IMenuRenderer adapter
└── GOOD/                        ← NEW: entire GOOD framework copy
    ├── gui.cpp
    ├── headers/
    │   ├── includes.h
    │   ├── flags.h
    │   ├── functions.h
    │   ├── draw.h
    │   ├── fonts.h
    │   └── widgets.h
    ├── helpers/
    │   ├── draw.cpp
    │   └── fonts.cpp
    ├── settings/
    │   ├── colors.h
    │   ├── elements.h
    │   └── variables.h
    ├── widgets/
    │   ├── helpers.cpp
    │   ├── notify.cpp
    │   ├── text_field.cpp
    │   ├── widgets.cpp
    │   └── window.cpp
    └── data/
        ├── fonts.h               (4.2 MB — Inter font bytes)
        └── uicons/
            ├── uicons-regular-rounded.ttf
            └── LICENSE
```

GOOD's `thirdparty/imgui/` and `thirdparty/freetype/` **are NOT copied** —
Bitware already has its own ImGui fork at `Dependencies/ImGui/` and FreeType at
`Dependencies/FreeType/`.

---

## Phase 1 — Build System

### 1.1  Add GOOD sources to `Bitware.vcxproj`

Insert these five new `ClCompile` items:

```xml
<ClCompile Include="Source\Core\UI\GOOD\gui.cpp" />
<ClCompile Include="Source\Core\UI\GOOD\helpers\draw.cpp" />
<ClCompile Include="Source\Core\UI\GOOD\helpers\fonts.cpp" />
<ClCompile Include="Source\Core\UI\GOOD\widgets\helpers.cpp" />
<ClCompile Include="Source\Core\UI\GOOD\widgets\notify.cpp" />
<ClCompile Include="Source\Core\UI\GOOD\widgets\text_field.cpp" />
<ClCompile Include="Source\Core\UI\GOOD\widgets\widgets.cpp" />
<ClCompile Include="Source\Core\UI\GOOD\widgets\window.cpp" />
```

Also add the new adapter file:

```xml
<ClCompile Include="Source\Core\UI\GoodMenuRenderer.cpp" />
```

### 1.2  Add include path for GOOD headers

Add to the existing `AdditionalIncludeDirectories` (Release|x64, line 77):

```
Source\Core\UI\GOOD\headers
```

GOOD's includes use `"includes.h"`, `"flags.h"`, `"draw.h"`, etc. relative to
this path, so it needs to be on the include search path.  However, there is a
**header name collision**: GOOD's `includes.h` includes `"imgui.h"` and
`"imgui_internal.h"` bare, which resolves to the *first* matching directory on
the include path.  GOOD's framework headers also `#include "../settings/colors.h"`
etc. using relative paths from `headers/`.  We must ensure that:

- GOOD's `includes.h` can find `imgui.h` from Bitware's `Dependencies/ImGui/`
- GOOD's relative `../settings/colors.h` resolves inside `Source/Core/UI/GOOD/`

**Resolution**: add `Source/Core/UI/GOOD` to the include path as well (the
parent directory, so that GOOD's relative `../` includes work), AND verify the
include order so Bitware's normal TU's still find `Dependencies/ImGui/imgui.h`
via the existing `..\Dependencies;..\Dependencies\ImGui;Source;..\Dependencies\Auth`
path.

The final includes for the project should be (new items **bolded**):

```
..\Dependencies\FreeType\include
..\Dependencies\Auth
Source
..\Dependencies
Source\Core\UI\GOOD\headers    ← NEW (for GOOD headers themselves)
Source\Core\UI\GOOD             ← NEW (for GOOD's relative ../ paths)
```

(All relative to `Bitware\Bitware.vcxproj`.)

### 1.3  Verify no ClCompile conflicts

Bitware already compiles `Dependencies/ImGui/imgui.cpp` etc.
GOOD's `gui.cpp` and widget files **do not** duplicate any of those — they are
separate source files that call into ImGui's public API.  No conflict.

---

## Phase 2 — ImGui Fork Reconciliation

Bitware and GOOD both carry a custom Dear ImGui fork.  The key question is
whether GOOD's widget code relies on internal (`imgui_internal.h`) structs or
behaviors that differ between the two forks.

### 2.1  What GOOD's fork has

GOOD's fork adds/modifies:

- `gui_style` parallel style system (in `flags.h`, standalone)
- `c_vec2 = ImVec2`, `c_vec4 = ImVec4`, `c_rect = ImRect`, `c_col = ImColor`
  (in `flags.h`, all type aliases — no ABI change)
- `window_flags_`, `child_flags_`, `gui_cond_` enums (in `flags.h` — just
  integer constants, not part of ImGui itself)
- Custom `s_()` macro for DPI scaling — preprocessor-only, no ABI impact

**None of this requires changes to `imgui.h` or `imgui_internal.h`.**  GOOD's
widgets only use the public ImGui API and the typedefs in `flags.h`.

### 2.2  Conclusion

**GOOD can compile against Bitware's existing ImGui fork as-is.**
We do NOT need to swap or merge the two forks.  If GOOD's fork had modifications
to e.g. `ImGuiWindow` internals that its widget code relied on, that would be a
problem, but after inspecting `widgets/widgets.cpp`, `widgets/window.cpp`, and
`widgets/text_field.cpp`, all operations go through `ImGui::` public API,
`ImDrawList`, or `ImGuiWindow` (via `c_window` alias, which is just
`ImGuiWindow*`).  No internal struct layout assumptions.

### 2.3  One caveat — font loading

GOOD's `helpers/fonts.cpp` includes:

```cpp
#include "../../thirdparty/imgui/imgui_freetype.h"
#include "imgui_impl_dx11.h"
```

These paths must be adjusted to point to Bitware's copies:

- `Dependencies/ImGui/misc/imgui_freetype.h`
- `Dependencies/ImGui/backends/imgui_impl_dx11.h`

And the include guards / relative includes in GOOD's files must be patched.
Specifically, `helpers/fonts.cpp` line 1 needs to change from:

```cpp
#include "../../thirdparty/imgui/imgui_freetype.h"
```

to:

```cpp
#include <imgui/misc/imgui_freetype.h>
```

(since `Dependencies\ImGui` is already on the include path).

Line 2 from:

```cpp
#include "imgui_impl_dx11.h"
```

to:

```cpp
#include <imgui/backends/imgui_impl_dx11.h>
```

---

## Phase 3 — Font Loading

GOOD needs three fonts that Bitware doesn't currently load:

| Font | File | Size | How to load |
|---|---|---|---|
| Inter SemiBold | `GOOD/framework/data/fonts.h` (byte array `inter_semibold`) | ~4.2 MB (combined) | `AddFontFromMemoryTTF` in `Create_Imgui()` |
| Inter Medium | same file (byte array `inter_medium`) | (included above) | `AddFontFromMemoryTTF` in `Create_Imgui()` |
| Flaticon UIcons | `GOOD/framework/data/uicons/uicons-regular-rounded.ttf` | 1.0 MB | `AddFontFromFileTTF` or embed as bytes |

### 3.1  Where to load them

In `Graphics::Create_Imgui()` (`Graphics.cpp:202`), after the existing Tahoma
font loading:

```cpp
// existing
IO.Fonts->AddFontFromMemoryTTF(const_cast<unsigned char*>(Tahoma), sizeof(Tahoma), Verdana_Size, &font_configuration);
Tahoma_BoldXP = IO.Fonts->AddFontFromMemoryTTF(..., Tahoma_Bold, ...);

// NEW — GOOD fonts
extern unsigned char inter_semibold[];
extern unsigned int inter_semibold_len;
extern unsigned char inter_medium[];
extern unsigned int inter_medium_len;

ImFontConfig good_font_cfg = font_configuration;
good_font_cfg.FontDataOwnedByAtlas = false;  // needed since data is static

ImFont* inter_semibold_font = IO.Fonts->AddFontFromMemoryTTF(
    inter_semibold, inter_semibold_len, 14.0f * MainScale, &good_font_cfg);
ImFont* inter_medium_font = IO.Fonts->AddFontFromMemoryTTF(
    inter_medium, inter_medium_len, 11.0f * MainScale, &good_font_cfg);

// Flaticon icon font (loaded as private/separate set)
IO.Fonts->AddFontFromFileTTF(
    "Source/Core/UI/GOOD/data/uicons/uicons-regular-rounded.ttf",
    12.5f * MainScale, &good_font_cfg, IO.Fonts->GetGlyphRangesDefault());
```

Then expose these pointers so GOOD's `c_font` class can use them.  The simplest
approach: store them alongside `Tahoma_BoldXP` in `Graphics.h`:

```cpp
inline ImFont* Inter_SemiBold = nullptr;
inline ImFont* Inter_Medium   = nullptr;
inline ImFont* Icon_Font      = nullptr;
```

Then patch GOOD's `c_font::get()` in `helpers/fonts.cpp` to return the
pre-loaded pointer instead of trying to load its own.

### 3.2  GOOD font size expectations

GOOD uses `font->get(inter_semibold, 14)` and `font->get(inter_medium, 11)`
throughout.  The `c_font::get()` method currently loads dynamic fonts via
`AddFontFromMemoryTTF` on demand.  Since we're pre-loading everything, the
simplest approach is to make `c_font::get()` ignore the `data` parameter and
return the pre-loaded pointer:

```cpp
// helpers/fonts.cpp — simplified get()
ImFont* c_font::get(const std::vector<unsigned char>& data, float size)
{
    if (&data == &inter_semibold)
        return Inter_SemiBold;
    if (&data == &inter_medium)
        return Inter_Medium;
    return ImGui::GetIO().Fonts->Fonts[0];  // fallback
}
```

---

## Phase 4 — Create `GoodMenuRenderer`

### 4.1  `GoodMenuRenderer.h`

```cpp
#pragma once
#include "IMenuRenderer.h"

class GoodMenuRenderer : public IMenuRenderer {
public:
    GoodMenuRenderer();
    virtual const char* Name() const override { return "GOOD"; }
    virtual void Render() override;

private:
    bool m_Initialized = false;
    void Initialize();
    void RenderLogin();
    void RenderDashboard();
};
```

### 4.2  `GoodMenuRenderer.cpp` (conceptual)

```cpp
#include "GoodMenuRenderer.h"
#include "GOOD/headers/includes.h"   // brings in all GOOD singletons
#include <Core/Graphics/Graphics.h>

GoodMenuRenderer::GoodMenuRenderer()
{
}

void GoodMenuRenderer::Initialize()
{
    if (m_Initialized) return;
    m_Initialized = true;

    // Initialize GOOD UI state
    var->gui.strict_license = false;  // skip license screen
    var->gui.tab = 1;                 // start at dashboard
    var->gui.tab_stored = 1;

    // Map Bitware settings to GOOD elements
    elements->window.name = "Bitware";
}

void GoodMenuRenderer::Render()
{
    Initialize();
    gui->render();  // GOOD's main render function
}
```

### 4.3  Wiring it in `Application::InitOverlay()`

In `Application.cpp:148-161`, change from:

```cpp
if (!m_MenuRenderer)
{
    m_MenuRenderer = std::make_unique<LegacyMenuRenderer>();
}
```

to:

```cpp
if (!m_MenuRenderer)
{
    m_MenuRenderer = std::make_unique<GoodMenuRenderer>();
}
```

The `Graphics` class already holds the renderer via `SetMenuRenderer()` and
calls `m_MenuRenderer->Render()` from `Graphics::Render()`.  No further changes
needed to the rendering pipeline.

---

## Phase 5 — GOOD Adaptations for Bitware

### 5.1  Skip login screen

GOOD's `gui.cpp` uses `var->gui.strict_license` and checks license keys.
Bitware has its own auth system.  The simplest approach: set
`var->gui.strict_license = false` and/or force `var->gui.tab = 1` (dashboard)
immediately.

Alternatively, patch `gui.cpp` around line 832 where it checks `var->gui.tab == 0`
to always jump to the dashboard path.  **Prefer the configuration approach**
since it avoids forking the framework file.

### 5.2  Disable GOOD's keybind handler

`handle_feature_keybinds()` in `gui.cpp` scans GOOD-specific keybind state
every frame.  Bitware has its own keybind system via `InputHook`.  Either:

- Make `handle_feature_keybinds()` a no-op (call it but it does nothing because
  no GOOD keybinds are configured), or
- Comment out the call in `gui::render()`.

**Prefer the no-op approach** to keep the framework file clean.  GOOD's keybind
state is in `keybind_t` structs owned by the visual panels; if those panels
aren't populated, `handle_feature_keybinds()` finds nothing to do.

### 5.3  Handle menu visibility (Running flag)

Bitware toggles the menu with a hotkey (F6 by default), which sets `Running` in
the `Graphics` class.  `Graphics::Render()` only calls
`m_MenuRenderer->Render()` when `Running` is true.

GOOD's `gui->render()` always renders.  This is fine — wrapping it behind
`Running` means the menu appears/disappears like the original LegacyMenuRenderer.

**One subtlety**: GOOD may need to know about visibility for e.g. not eating
mouse input when hidden.  GOOD's `gui->render()` calls `ImGui::Begin()` for its
main window, so when `Running` is false, those ImGui windows simply aren't
rendered and don't consume input.  No conflict.

### 5.4  DPI handling

GOOD's `s_()` macro and `var->gui.dpi` (default 1.5) scale all UI elements.
Bitware doesn't currently handle DPI.  We need to:

1. Set `var->gui.dpi` based on the actual monitor DPI at startup (same DPI that
   Bitware already gets via `ImGui_ImplWin32_GetDpiScaleForMonitor` in
   `Create_Imgui()`).
2. Store the computed `MainScale` value and pass it to `var->gui.dpi`:

```cpp
// In Create_Imgui(), after MainScale is computed:
var->gui.dpi = MainScale;
var->gui.stored_dpi = static_cast<int>(MainScale * 100.0f);
var->gui.dpi_changed = true;
```

### 5.5  Window management

GOOD does `gui->set_next_window_size(elements->window.size)` and
`gui->set_next_window_pos(c_vec2(0, 0))` in its render function.  Since the
GOOD window is rendered inside Bitware's overlay (which covers the whole
screen), this positions the GOOD UI at the top-left of the overlay, which is
correct.

However, if `gui->begin()` creates an ImGui window with no title bar /
decoration, it may need to be the *only* ImGui window in the frame.  GOOD
writes to `ImGui::GetBackgroundDrawList()` and `GetForegroundDrawList()`, which
are global — this is fine since GOOD is the only custom UI renderer.

### 5.6  Color singleton adaptation

GOOD's colors (`clr->accent`, `clr->child`, `clr->text`, etc.) are defined in
`settings/colors.h` with hardcoded default values.  To match Bitware's intended
color scheme, edit `colors.h` to use Bitware's accent colors (e.g. the purple
theme visible in Bitware's current UI).

---

## Phase 6 — Feature Tab Implementation

GOOD's dashboard renders 10 tabs (Player, Shapes, World, Polish, Combat, Aim,
Loot, Movement, Visuals, HUD) with per-tab sub-tabs and visual panels.  Bitware
currently has 6 tabs in `LegacyMenuRenderer`: Silent Aim, Aimbot, Visuals,
Players, Settings, Configs.

### 6.1  Tab mapping (initial)

| Bitware Tab | GOOD Tab Slot | Notes |
|---|---|---|
| Aimbot | tab 5 (Combat) + tab 6 (Aim) | Split aimbot and silent aim |
| Silent Aim | tab 6 (Aim) | Sub-tab under Aim |
| Visuals | tab 9 (Visuals) + tab 10 (HUD) | External ESP + crosshair |
| Players | tab 1 (Player) | Player labels, filtering |
| Settings | tab 8 (Movement) or new | Misc toggles, performance |
| Configs | tab 4 (Polish) or new | Config save/load |

### 6.2  Widget mapping

Bitware's `LegacyMenuRenderer` uses raw ImGui:

```
Checkbox   →  ImGui::Checkbox
Slider     →  ImGui::SliderFloat
Combo      →  ImGui::Combo
ColorEdit  →  ImGui::ColorEdit4
Keybind    →  raw ImGui button + key detection
```

GOOD equivalents:

```
widgets->checkbox("Name", "Description", &variable)
widgets->slider("Name", "Description", &variable, min, max, "%.1f")
widgets->dropdown("Name", "Description", &variable, {"A", "B", "C"})
widgets->color_picker("Name", &color)
widgets->keybind("Name", "Desc", &keybind_struct)
widgets->text_field("Name", buf, size)
```

### 6.3  Visual panel structure

Each tab's content is rendered inside `begin_visual_section()` / `end_visual_section()`
blocks.  Bitware's settings are accessed via `Globals::Settings::*` (defined in
`Settings.h`).  The pattern for each panel:

```cpp
if (var->gui.tab == 5)  // Combat tab
{
    begin_visual_section("Aimbot", half_section_height);
    widgets->checkbox("Enable Aimbot", "Auto-aim at target", &Globals::Settings::AimbotEnabled);
    widgets->slider("FOV", "Aimbot field of view", &Globals::Settings::AimbotFOV, 0.f, 360.f);
    widgets->dropdown("Target bone", "Aim at which bone", &Globals::Settings::AimbotBone,
        {"Head", "Torso", "Random"});
    end_visual_section();
}
```

### 6.4  Search integration

GOOD's feature search (top of dashboard) already works with this structure —
each widget is wrapped in `visual_widget_filter` which checks the search query
before rendering.  As long as widget calls use the `widgets->` macro inside the
`visual_widgets` scope, search filtering is free.

---

## Phase 7 — Notifications

GOOD has a `c_notify` system (`notify.cpp`).  Wire it to Bitware's existing
output/logging:

```cpp
// In GoodMenuRenderer or wherever needed:
notify->add_notify("Aimbot", "Target acquired", success);

// From Bitware's output system:
Output::print(OUTPUT_SUCCESS, "Aimbot target acquired");
```

GOOD's notifications render automatically inside `gui->render()` via
`gui->initialize()` which calls `notify->setup_notify()`.

Bitware's `Output::print` writes colored text to a debug console; GOOD's
notifications are in-screen toast popups.  They can coexist — the console is
for development, toasts are for in-game feedback.

---

## Phase 8 — Risks & Mitigations

| Risk | Severity | Mitigation |
|---|---|---|
| ImGui fork incompatibility | Medium | Test first by compiling GOOD's `widgets.cpp` alone against Bitware's imgui |
| Font byte array size (4.2 MB) | Low | Added to `.rdata` via `const` byte array; acceptable for Release build |
| GOOD uses `imgui_impl_dx11.h` — path mismatch | Low | Patch include paths in `helpers/fonts.cpp` |
| `ImGuiFreeType` include mismatch | Low | GOOD includes `imgui_freetype.h` from its own thirdparty; point to Bitware's |
| `gui.cpp` assumes standalone app | Medium | `gui->render()` works fine inside the overlay frame; tested by adapter pattern |
| DPI scaling mismatch | Low | Initialize `var->gui.dpi` from Bitware's existing DPI calculation |
| Feature keybind conflict | Low | GOOD's `handle_feature_keybinds()` finds nothing if no keybinds configured |
| Memory overlay (MiniVM, obfuscation) | None | GOOD is pure UI code, no interaction with memory protection |
| Build time increase from 4.2 MB font header | Low | Acceptable; precompiled headers help |

---

## Phase 9 — Step-by-Step Implementation Order

1. **Copy GOOD sources** into `Source/Core/UI/GOOD/`
2. **Create `GoodMenuRenderer.h/cpp`** — stub that calls `gui->render()`
3. **Update `Bitware.vcxproj`** — add all GOOD .cpp files + include paths
4. **Fix include paths** in GOOD's `helpers/fonts.cpp` for Bitware's imgui
5. **Load Inter + icon fonts** in `Graphics::Create_Imgui()`
6. **Initialize `var->gui.dpi`** from monitor DPI
7. **Set `var->gui.strict_license = false`** to skip login
8. **Build and verify** the GOOD UI renders inside the overlay
9. **Replace `LegacyMenuRenderer`** with `GoodMenuRenderer` in `Application.cpp`
10. **Wire Bitware features** into GOOD tab panels (one tab at a time)
11. **Adjust colors** in `settings/colors.h` to match Bitware theme
12. **Remove LegacyMenuRenderer** when parity is reached

---

## Key Files Reference

### Bitware files that need modification

| File | Change |
|---|---|
| `Bitware.vcxproj` | Add GOOD sources + include paths |
| `Graphics.cpp` | Load Inter/icon fonts; expose font pointers |
| `Graphics.h` | Add `Inter_SemiBold`, `Inter_Medium`, `Icon_Font` globals |
| `Application.cpp` | Swap `LegacyMenuRenderer` → `GoodMenuRenderer` |
| `Application.h` | Add forward decl / include for `GoodMenuRenderer` |

### Bitware files that need creation

| File | Purpose |
|---|---|
| `Source/Core/UI/GoodMenuRenderer.h` | Adapter class declaration |
| `Source/Core/UI/GoodMenuRenderer.cpp` | Adapter implementation |

### GOOD files that need modification

| File | Change |
|---|---|
| `helpers/fonts.cpp` | Fix imgui include paths; make `get()` return pre-loaded fonts |
| `headers/includes.h` | May need to adjust `#include "imgui.h"` resolution order |
| `gui.cpp` | Optionally disable login / keybind sections |
| `settings/colors.h` | Tune colors to Bitware palette |
| `settings/variables.h` | Keep defaults, may tune DPI default |
| `settings/elements.h` | Change window name from "Lumin" to "Bitware" |

### GOOD files that need NO modification

| File | Reason |
|---|---|
| `headers/flags.h` | Type aliases only; no Bitware dependency |
| `headers/functions.h` | Pure ImGui wrapper, no external dependency |
| `headers/draw.h` | Pure draw-list wrapper |
| `headers/widgets.h` | Widget API declarations |
| `widgets/*.cpp` | All use public ImGui API; no Bitware dependency |
| `helpers/draw.cpp` | Pure draw-list implementations |
| `data/fonts.h` | Raw byte arrays; no code dependency |
| `data/uicons/*` | Static font asset |
