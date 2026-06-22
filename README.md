# Bitware

External cheat for Roblox (Windows x64). Targets `RobloxPlayerBeta.exe` with a **dual-path memory architecture** — runtime-switchable between direct-syscall usermode and a custom kernel driver (unsigned, mapped via kdmapper exploit). No injection, no DLL hooks.

---

## Architecture

```
Bitware.cxx → main()
  → Application::Init()
      ├── FakeStrings::Generate()       – plant decoy strings
      ├── AntiInjection::Start()        – monitor modules/threads/hooks
      ├── AntiDump::Enable()            – VEH + PAGE_NOACCESS protection
      ├── InitSeh()                     – SEH handler (separated to avoid C2712)
      ├── InitBackend()                 – 
      │   ├── Select backend: UsermodeMemory or KernelMemory (per Settings_KernelMode)
      │   ├── KernelMemory::Connect()   – CreateFileA → KernelLoader → kdmapper load
      │   └── init global g_Memory
      ├── InitProcess()                 – attach to RobloxPlayerBeta.exe via g_Memory
      ├── InitSDK()                     – resolve DataModel, VisualEngine, Players, Camera, Lighting
      ├── InputHook::Install()          – low-level keyboard hook
      ├── SpawnThreads()                – launch worker threads
      └── InitOverlay()                 – create D3D11 overlay window + ImGui
```

Execution model: **external process**. Memory backend is abstracted behind `MemoryInterface` — `g_Memory->Read<T>()` / `Write<T>()` resolves to either direct syscalls or IOCTL calls into a WDM driver mapped via kdmapper. No DLL injection. Overlay uses ImGui + D3D11 (resolved dynamically, no IAT entry for d3d11.dll).

---

## Project Structure

| Path | Description |
|---|---|
| `Bitware/Source/Bitware.cxx` | Entry point (`main()`) |
| `Bitware/Source/Globals.hxx` | Global singletons + settings references |
| `Bitware/Source/Core/Application.h/.cpp` | App lifecycle: init, run, shutdown |
| `Bitware/Source/Core/Settings/Settings.h` | All cheat settings (toggles, values, colors, keybinds) |
| `Bitware/Source/Core/Settings/Types.h` | Color4, KeyBind, BindMode types |
| `Bitware/Source/Core/Graphics/Graphics.h/.cpp` | D3D11 overlay window + ImGui setup |
| `Bitware/Source/Core/Graphics/Fonts/Tahoma.h` | Embedded Tahoma font |
| `Bitware/Source/Core/Graphics/Fonts/Tahoma_Bold.h` | Embedded Tahoma Bold font |
| `Bitware/Source/Core/UI/IMenuRenderer.h` | Menu renderer interface |
| `Bitware/Source/Core/UI/LegacyMenuRenderer.h/.cpp` | ImGui-based cheat menu |
| `Bitware/Source/Core/UI/UIBridge.h` | UI bridge (unused) |
| `Bitware/Source/Core/Input/InputHook.h/.cpp` | Low-level keyboard input hook |
| `Bitware/Source/Core/Features/Cache/Cache.h/.cpp` | Player data caching (Rescan + Runtime threads) |
| `Bitware/Source/Core/Features/Cache/PhantomForces/PhantomForces.h` | PhantomForces-specific caching (disabled) |
| `Bitware/Source/Core/Features/Cheats/Aimbot/Aimbot.h/.cpp` | Mouse/camera aimbot |
| `Bitware/Source/Core/Features/Cheats/Aimbot/Silent/Silent.h/.cpp` | Silent aim (InputObject CFrame write) |
| `Bitware/Source/Core/Features/Cheats/Triggerbot/Triggerbot.h/.cpp` | Auto-fire triggerbot |
| `Bitware/Source/Core/Features/Cheats/Visuals/Visuals.h/.cpp` | ESP rendering (boxes, health, skeleton, chams, snaplines) |
| `Bitware/Source/Core/Features/Cheats/World/World.h/.cpp` | World modifiers (skybox, fog, ambience, brightness, exposure, FOV) |
| `Bitware/Source/Core/Features/Cheats/Misc/Misc.h/.cpp` | Speed hack + jump hack |
| `Bitware/Source/Core/Features/Cheats/Common/WallCheck.h/.cpp` | OBB raycasting visibility checks |
| `Bitware/Source/Core/Features/Cheats/Common/PlayerUtils.h` | Knock detection utility |
| `Bitware/Source/Core/Features/Explorer/Explorer.h/.cpp` | Instance tree browser, script viewer, teleport |
| `Bitware/Source/Driver/Driver.h/.cpp` | `UsermodeMemory` — direct syscall read/write (non-kernel backend) |
| `Bitware/Source/Driver/MemoryInterface.h` | Abstract base `MemoryInterface` + global `g_Memory` pointer |
| `Bitware/Source/Driver/KernelMemory.h/.cpp` | `KernelMemory` — kernel-driver client via CreateFile + DeviceIoControl |
| `Bitware/Source/Driver/KernelLoader.h/.cpp` | `LoadDriver()` — tries direct connection, falls back to kdmapper |
| `Bitware/Source/Driver/IoctlDefs.h` | Shared IOCTL codes + packed structs (user + kernel) |
| `Bitware/Source/Driver/kdmapper/` | kdmapper exploit (Intel vulnerable driver loader, PE mapper, service mgmt) |
| `Bitware/Source/Driver/SSN.h` | Dynamic SSN resolution + runtime stub generation |
| `Bitware/Source/Driver/luck.asm` | **Dead** — all functions removed; kept for MSBuild MASM integration |
| `Bitware/Source/Engine/Engine.h` | Roblox SDK class declarations |
| `Bitware/Source/Engine/Classes/*` | SDK implementations (Instance, Datamodel, Camera, Humanoid, Part, etc.) |
| `Bitware/Source/Engine/Math/Math.h` | Vector2/3/4, Matrix3/4, xorshift PRNG |
| `Bitware/Source/Engine/Offsets/Offsets.h` | 390+ memory offsets for Roblox classes |
| `Bitware/Source/Infrastructure/ApiHiding.h` | Dynamic Win32 API resolution |
| `Bitware/Source/Infrastructure/Obfuscation.h` | ObfuscatedValue, OBF_PROLOGUE, junk code, opaque predicates |
| `Bitware/Source/Infrastructure/AntiDump.h` | VEH + PAGE_NOACCESS anti-dump (VEH handler in `.vcdata` section, outside `.text` range) |
| `Bitware/Source/Infrastructure/ThreadObf.h` | Random window class/thread names, random string generation |
| `Bitware/Source/Infrastructure/MiniVM.h` | 26-opcode stack VM for bytecode obfuscation |
| `Bitware/Source/Infrastructure/ResourceEnc.h` | Encrypted resource loading |
| `Bitware/Source/Infrastructure/Logger.h` | Debug logging to `bitware_debug.log` |
| `Bitware/Source/Infrastructure/SyscallObf.h` | Runtime syscall stub generator with garbage instructions, XOR encryption, RW→RX pages |
| `Bitware/Source/Miscellaneous/Config/ConfigManager.h` | Encrypted JSON config save/load |
| `Bitware/Source/Miscellaneous/Output/Output.h` | Console output with colored prefixes |
| `Bitware/Source/Miscellaneous/Protection/AntiInjection.h/.cpp` | Module/thread/hook monitoring |
| `Bitware/Source/Miscellaneous/Protection/FakeStrings.h/.cpp` | Decoy string pool in memory |
| `Bitware/Source/Miscellaneous/Protection/External/oxorany.h` | Compile-time literal obfuscation |
| `Bitware/Dependencies/ImGui/` | ImGui v1.91+ (Win32 + DX11 backends, FreeType) |
| `Bitware/Dependencies/ImGui/addons/` | Custom animated widgets, theme colors |
| `Bitware/Dependencies/Clipper2Lib/` | Clipper2 polygon clipping (used by chams) |
| `Bitware/Dependencies/FreeType/` | FreeType font rasterizer |
| `Bitware/Dependencies/Auth/skStr.h` | skCrypt compile-time string encryption |
| `BitwareDrv/` | WDM kernel driver project (6 sources: entry, IOCTL dispatch, read/write, process finder, module finder) |

---

## Application Lifecycle

### `Application::Init()`
```
 1. FakeStrings::Generate()           – plant decoy strings in memory
 2. AntiInjection::Start()            – monitor modules/threads/hooks
 3. AntiDump::Enable()                – VEH + PAGE_NOACCESS on .text section
 4. InitSeh()                         – install SEH handler (isolated to avoid C2712 with ~objects)
 5. InitBackend()                     – select + connect memory backend:
   a. Settings_KernelMode ? KernelMemory::Connect() : UsermodeMemory()
   b. KernelMemory::Connect() → KernelLoader::LoadDriver() → kdmapper → CreateFile
   c. sets global g_Memory (abstract MemoryInterface*)
 6. InitProcess()                     – attach to RobloxPlayerBeta.exe via g_Memory
 7. InitSDK()                         – resolve DataModel, VisualEngine, Players, Camera, Lighting
 8. InputHook::Install()              – install low-level keyboard hook
 9. SpawnThreads()                    – launch Cache, Aimbot, Silent, Triggerbot, World, Misc threads
10. InitOverlay()                     – create overlay window + D3D11 + ImGui
```

### `Application::Run()`
Main loop: `PeekMessage` → `Graphics::NewFrame()` → `DrawCursor()` → `Visuals::RunService()` → `Graphics::Render()` → `ImGui::Render()` → `Present()`. FPS capped by performance mode (60/144/240). F7 panic key clears globals and exits.

### `Application::Shutdown()`
Disables anti-dump, stops anti-injection, removes input hook, joins all worker threads.

---

## Driver Layer — Dual-Path Memory Architecture

All memory operations go through the abstract `MemoryInterface` base, dispatched at runtime via the global `g_Memory` pointer. Two backends are available, chosen at startup via `Settings_KernelMode`.

### Memory Backend Selection

```
Application::InitBackend()
  │
  ├─ Settings_KernelMode == false
  │   └─ g_Memory = new UsermodeMemory()   ← direct syscalls (existing)
  │
  └─ Settings_KernelMode == true
      └─ g_Memory = new KernelMemory()
          └─ Connect()
              ├─ CreateFileA("\\.\\BitwareDevice")  → already loaded?
              │   └─ if yes, done
              ├─ KernelLoader::LoadDriver()
              │   ├─ intel_driver::Load()           → register + start iqvw64e.sys
              │   ├─ kdmapper::MapDriver()          → map BitwareDrv.sys in kernel
              │   ├─ intel_driver::Unload()         → stop + remove iqvw64e.sys
              │   └─ Sleep(200)                     → wait for driver to initialize
              └─ CreateFileA("\\.\\BitwareDevice")  → connect to our driver
```

### `MemoryInterface` (`MemoryInterface.h`)

Shared abstract interface used by all feature code:

| Method | Description |
|---|---|
| `Read<T>(address)` | Read typed value from remote process |
| `Write<T>(address, value)` | Write typed value to remote process |
| `Read_Raw(address, buffer, size)` | Raw memory read |
| `Write_Raw(address, buffer, size)` | Raw memory write |
| `Read_String(address)` | Read Roblox-style string |
| `Write_String(address, value)` | Write string value |
| `Find_Process(name)` | Find PID by process name |
| `Attach_Process(name)` | Open process handle with required access |
| `Find_Module(name)` | Get module base address (PEB walking) |
| `Get_Process_Id()` | Return attached process ID |

All feature code (`Aimbot`, `Visuals`, `Cache`, etc.) uses `g_Memory->Read<T>()` / `g_Memory->Write<T>()` — the backend is transparent.

---

### Backend 1: `UsermodeMemory` (`Driver.h/.cpp`)

Direct syscall approach (original Bitware method). Inherits `MemoryInterface`.

| Component | Description |
|---|---|
| **Syscall stubs** | `NtOpenProcess`, `NtReadVirtualMemory`, `NtWriteVirtualMemory` resolved via SSN + EAT parsing |
| **Stub generation** | `SyscallObf::GenerateSyscallStub()` — XOR-encrypted at rest, RW→RX pages, randomized garbage |
| **Process operations** | `NtOpenProcess` via direct syscall, PEB walking for modules |
| **SSN resolution** | `SSN.h` — dynamic SSN via EAT parsing, no hardcoded values |

### Runtime Syscall Stubs (`SSN.h` + `SyscallObf.h`)

| Stub | Method |
|---|---|
| `NtReadVirtualMemory` | SSN resolved via EAT parsing → `GenerateSyscallStub()` |
| `NtWriteVirtualMemory` | SSN resolved via EAT parsing → `GenerateSyscallStub()` |
| `DriverWriteMousePosition` | Runtime-generated via `GenerateWriteStub(0xEC, 0xF0)` |

Stubs are XOR-encrypted at rest, decrypted before use. Allocated as `PAGE_READWRITE`, then protected to `PAGE_EXECUTE_READ` (no RWX pages at rest). No static ASM bytes remain in `.text` — `luck.asm` is empty.

---

### Backend 2: `KernelMemory` (`KernelMemory.h/.cpp`, `IoctlDefs.h`)

Kernel-driver client. Uses `CreateFileA("\\.\\BitwareDevice")` + `DeviceIoControl` to communicate with a WDM driver. Handles are obtained via `KernelLoader`.

| IOCTL | Code | Description |
|---|---|---|
| `IOCTL_BITWARE_READ_MEMORY` | `0x00226004` | Read from target process (METHOD_BUFFERED) |
| `IOCTL_BITWARE_WRITE_MEMORY` | `0x0022A005` | Write to target process (METHOD_BUFFERED) |
| `IOCTL_BITWARE_FIND_PROCESS` | `0x00226008` | Find PID by process name |
| `IOCTL_BITWARE_FIND_MODULE` | `0x0022600C` | Get module base by name |

IOCTL input/output structs are defined in `IoctlDefs.h` as packed structs (portable to both user and kernel modes).

---

### Driver Loader (`KernelLoader.h/.cpp`)

`KernelLoader::LoadDriver()` handles the complete driver loading sequence:

1. **Direct check** — tries `CreateFileA("\\\\.\\BitwareDevice")` (in case already loaded)
2. **Intel vulnerable driver** — loads `iqvw64e.sys` via service registration → provides physical memory access
3. **kdmapper** — maps `BitwareDrv.sys` (embedded as byte array, never written to disk) into kernel space via Intel driver IOCTL
4. **Cleanup** — unloads Intel driver (service stop + delete) to leave no trace
5. **Retry** — waits 200ms, retries `CreateFileA` to connect to our driver

### kdmapper (`Bitware/Source/Driver/kdmapper/`)

Fork of [eddeeh/kdmapper](https://github.com/eddeeh/kdmapper) adapted for indirect loading:

| File | Purpose |
|---|---|
| `intel_driver.hpp/.cpp` | `\\.\Nal` IOCTL interface (`0x80862007`) for physical read/write + map |
| `intel_driver_resource.hpp` | Embedded `iqvw64e.sys` binary (209 KB) |
| `kdmapper.hpp/.cpp` | `MapDriver()` — PE relocations, import resolution, kernel call |
| `nt.hpp` | NT structures (`SYSTEM_HANDLE_INFORMATION_EX`, `POOL_TYPE`) |
| `portable_executable.hpp/.cpp` | PE parsing |
| `utils.hpp/.cpp` | `GetKernelModuleAddress`, file I/O |
| `service.hpp/.cpp` | SCM service register/start/stop/remove |
| `bitware_driver_resource.hpp` | Auto-generated embedded `BitwareDrv.sys` byte array |

kdmapper works by loading the vulnerable Intel driver (access to physical memory), then using it to allocate kernel memory, write PE sections, resolve imports, call `DriverEntry`, and hook `NtGdiDdDDIReclaimAllocations2` in `win32kbase.sys` as an execution gadget.

---

### WDM Driver (`BitwareDrv/`)

Custom WDM kernel driver (no KMDF dependencies, no generated type tables):

| Source | Purpose |
|---|---|
| `DriverEntry.cpp` | Driver entry — registers `DRIVER_DISPATCH` for create/close/device control |
| `MemoryOps.cpp` | Read/write via `KeStackAttachProcess` with custom `BITWARE_KAPC_STATE` |
| `ProcessFinder.cpp` | `ZwQuerySystemInformation` with `SystemProcessInformation` for PID lookup |
| `ModuleFinder.cpp` | `PsGetProcessSectionBaseAddress` for module base retrieval |
| `IoctlDefs.h` | Shared IOCTL definitions (copied from user project) |
| `BitwareDrv.vcxproj` | WDK project targeting `10.0.28000.0` via NuGet WDK package |

Build output: `Build/BitwareDrv.sys` (6,144 bytes). Embedded into the cheat executable as a C byte array at build time via a manual step (`bitware_driver_resource.hpp`).

---

## Roblox SDK (`Engine/`)

### Base Class: `SDK::Instance`
Wraps a Roblox Instance pointer. Methods use offset-based memory reads:

| Method | Description |
|---|---|
| `Name()` | Read Instance name |
| `Class()` | Read ClassName |
| `Text()` | Read Text property |
| `Parent()` | Get parent Instance |
| `Children()` | Get all children |
| `Find_First_Child(name)` | Find child by name |
| `Find_First_Child_Of_Class(class)` | Find child by class name |

### Class Hierarchy

| Class | Key Methods |
|---|---|
| `Datamodel` | `Get_PlaceID()`, `Get_GameID()`, `Get_CreatorID()`, `Get_ServerIP()` |
| `VisualEngine` | `Get_Dimensions()`, `Get_ViewMatrix()`, `World_To_Screen(Vector3)` |
| `Players` | `Get_UserID()`, `Get_Team()`, `Get_DisplayName()`, `Get_Local_Player()` |
| `Camera` | `Get_CameraPos()`, `Get_CameraRot()`, `Set_CameraPos()`, `Set_CameraRot()`, `FetchViewPort()`, `SetViewPort()` |
| `Humanoid` | `Get_Health()`, `Get_MaxHealth()`, `Get_RigType()`, `Kill()` |
| `Part` | `Get_Position()`, `Get_Rotation()`, `Get_Size()`, `Get_CFrame()`, `Get_Velocity()`, `Get_Primitive()`, `Set_PartPosition()`, `Write_Velocity()`, `Set_Rotation()`, `Set_MeshID()`, `Set_Transparency()` |
| `Lighting` | `SetAmbient()`, `SetFog()`, `SetBrightness()`, `SetExposure()`, `SetFOV()` |
| `Renderview` | `GetRenderview()`, `InvalidateLighting()`, `ValidateSkybox()` |

### `SDK::Player` (Cache.h)
Full player data structure cached by `Cache::RunService`:

| Field | Description |
|---|---|
| `Name`, `Display_Name` | Player name strings |
| `UserID` | Roblox user ID |
| `Health`, `MaxHealth` | Current/max HP |
| `Distance` | Distance from camera |
| `Local_Player` | Is this the local player |
| `Character` | Character Instance |
| `Head`, `HumanoidRootPart` | Core body parts |
| `Torso` / `UpperTorso`, `LowerTorso` | R6 / R15 torso parts |
| `LeftArm`, `RightArm`, `LeftLeg`, `RightLeg` | R6 limb parts |
| `LeftUpperArm`, `LeftLowerArm`, `LeftHand`, etc. | R15 limb parts |
| `Humanoid` | Humanoid Instance |
| `Team` | Team Instance |
| `Rig_Type` | R6 (0) or R15 (1) |
| `Tool_Name` | Currently held tool name |
| `Bones` | All bone instances |
| `CharacterVersion` | Incremented on character change |

---

## Cheat Features

### Aimbot (`Aimbot.cpp`)
Two modes controlled by `Aimbot_type`:

| Mode | Index | Method |
|---|---|---|
| Mouse | 0 | `SendInput(INPUT_MOUSE)` — moves cursor with smoothing |
| Camera | 1 | Writes CFrame to Camera — rotates view with matrix lerp smoothing |

- **Target acquisition**: Finds closest player to crosshair within FOV
- **HitPart selection**: Head (0), Torso (1), LowerTorso (2), Random (4)
- **FOV circle**: Optional draw with spin animation and fill
- **Sticky aim**: Persists target lock
- **Humanization**: Independent X/Y smoothing, shake randomizer with per-axis amplitude
- **WallCheck**: OBB raycasting to skip obscured targets
- **Knocked check**: Skips knocked/downed players
- **Keybind**: Configurable key with hold/toggle mode

### Silent Aim (`Silent.cpp`)
Writes `CFrame` directly to `MouseService.InputObject`, bypassing normal aiming:

- **Gun-based FOV**: Per-weapon FOV presets (Double-Barrel, Tactical Shotgun, Revolver)
- **Target part**: Head or Torso selection
- **FOV circle**: Optional draw with spin animation and fill
- **Sticky aim**: Persists target lock
- **SpoofMouse**: Writes mouse position via custom syscall
- **WallCheck**: OBB raycasting to skip obscured targets
- **Knocked check**: Skips knocked/downed players
- **Keybind**: Configurable key with hold/toggle mode

### Triggerbot (`Triggerbot.cpp`)
Auto-fires when crosshair is over a target:

| Setting | Description |
|---|---|
| HitPart | Head, Torso, LowerTorso, or **All** (fires if over any of the three) |
| Delay (ms) | Delay before firing (0–500ms) |
| Randomize (ms) | Random variation applied to delay (0–250ms) |
| FOV | Pixel distance threshold for target detection |
| FireMode | Fire mode selection |
| Knocked check | Skips knocked players |
| WallCheck | OBB raycasting to skip obscured targets |
| Key bind | Hold/toggle activation |

### Visuals — ESP (`Visuals.cpp`)
Per-player overlay rendering with wall-check occlusion support:

| Feature | Description |
|---|---|
| Box | 2D bounding or corner boxes with optional fill |
| Box Fill | Solid color or animated gradient (3 gradient modes: horizontal, vertical, radial) |
| Healthbar | Static or gradient health bar (adjustable gap/thickness) |
| Health | Health text overlay |
| Name | Name display (username / display name / both) |
| Distance | Distance text in meters |
| Rig Type | R6/R15 indicator |
| Tool | Currently held tool name |
| Skeleton | Bone lines for R6 and R15 rigs |
| Chams | Convex hull highlight using Clipper2 union, with optional fade animation |
| WallCheck | OBB raycasting — occluded players rendered in a separate color |
| Team color | Color by team affiliation |
| Whitelist color | Custom color for whitelisted players |
| Render distance | Configurable maximum render distance |

All colors: fully customizable with per-feature color pickers.

### World (`World.cpp`)
Modifies `Lighting` and `Camera` properties:

| Feature | Properties |
|---|---|
| Skybox | Presets (Bitware.fun, Space, Pink Sky, Minecraft, Nebula, etc.) with rotation |
| Atmosphere | Ambient color override |
| Fog | Color + distance |
| Brightness | 0–10 range |
| Exposure | -3 to +3 range |
| FOV override | 70–120° |

### Misc (`Misc.cpp`)
| Feature | Description |
|---|---|
| Speed hack | Writes custom WalkSpeed to Humanoid (restores on disable) |
| Jump hack | Writes custom JumpPower to Humanoid (restores on disable) |

Both features have configurable keybinds with hold/toggle modes.

### Explorer (`Explorer.cpp`)
Instance tree browser with interactive features:

| Feature | Description |
|---|---|
| Tree browser | Recursive DataModel hierarchy with expandable nodes |
| Properties panel | Shows path, memory address, copy-to-clipboard |
| Part properties | Teleport to any Part's position |
| Model/Actor teleport | Teleport to PrimaryPart or first child Part |
| Script viewer | Read script bytecode (LocalScript, ModuleScript, Script) |
| Bytecode viewer | Hex dump and extracted strings views |

### WallCheck (`WallCheck.cpp`)
OBB (Oriented Bounding Box) raycasting system for visibility checks:

| Feature | Description |
|---|---|
| OBB intersection | Full 3-axis slab-based ray-OBB intersection test |
| Workspace caching | Background thread caches all workspace Part obstacles |
| Thread-safe | Mutex-protected, non-blocking visibility queries |
| Used by | Aimbot, Silent Aim, Triggerbot, Visuals (occluded player color) |

---

## Protection Stack

### Compile-Time
| Layer | What |
|---|---|
| `skStr.h` / `skCrypt()` | XOR-encrypts string literals at compile time |
| `oxorany.h` / `_o_()` | Obfuscates numeric/string literals |
| `WRAPPER_MARCO()` | Wrapper for `_o_()` — used throughout for display strings |

### Runtime
| Layer | What |
|---|---|
| `SyscallObf.h` | Runtime syscall stub generator — XOR-encrypted at rest, RW→RX allocation, random garbage instructions |
| `ApiHiding.h` | All Win32 APIs (including MessageBoxA, D3D11CreateDeviceAndSwapChain) resolved dynamically — no IAT entries |
| `Obfuscation.h` | `ObfuscatedValue<T>` (XOR-encrypted integers), opaque predicates, junk code blocks |
| `AntiDump.h` | VEH handler + `PAGE_NOACCESS` on `.text` section (handler in `.vcdata`, outside `.text` range) |
| `AntiInjection.cpp` | Monitors module loads, detects NTDLL hooks, detects debuggers (all APIs via ApiHiding) |
| `ThreadObf.h` | Random window class/thread names, random string generation |
| `FakeStrings.cpp` | Pool of decoy strings (AES_KEY, Backdoor, License_Key, etc.) |
| `MiniVM.h` | 26-opcode bytecode interpreter for obfuscated code sections |
| `ResourceEnc.h` | AES-XOR-shift encrypted embedded resources |
| `InputHook.h` | Low-level keyboard hook for input capture |

---

## UI — Menu System

### LegacyMenuRenderer (`LegacyMenuRenderer.cpp`)
ImGui-based cheat menu with animated custom widgets:

| Tab | Contents |
|---|---|
| Aimbot | Aimbot settings, FOV settings, Triggerbot, Silent aim |
| Visuals | ESP toggles, World modifiers (skybox/fog/ambience/brightness) |
| Players | Player list with whitelist checkboxes, whitelist settings |
| Settings | Streamproof toggle, performance mode (Low=60/Medium=144/High=240) |
| Configs | Save/Load/Delete encrypted JSON config profiles |

Menu keybind: configurable (default `Insert`).

### Explorer Panel
Separate window for instance browsing, script viewing, and teleporting.

### Custom UI Widgets (`imgui_addons.cpp`)
All rendered with animated transitions, custom dark theme:
- `Menu::CheckBox` — animated checkbox with shadow label
- `Menu::SliderFloat` / `Menu::SliderInt` — animated sliders with value text
- `Menu::Combo` — dropdown with animated items
- `Menu::ColorEdit4` — color square button that opens a popup color picker
- `Menu::KeyBindEx` — keybind capture with toggle/hold mode
- `Menu::Tabs` — animated tab bar
- `Menu::SelectableLabel` — animated selectable text

---

## Settings System

Settings are defined as `inline` variables in `Settings.h` (namespace `SettingsStore`) and referenced via `Globals::Xxx` aliases in `Globals.hxx`. Config profiles are saved/loaded as encrypted JSON via `ConfigManager`.

### Key Settings Namespaces
- `Globals::Aimbot` — aimbot toggles, smoothing, FOV, keybinds, wall check
- `Globals::Visuals` — ESP toggles, colors, box/healthbar styles, wall check, render distance
- `Globals::World` — skybox, fog, ambience, brightness, exposure, FOV
- `Globals::Triggerbot` — delay, randomization, hit part, FOV, wall check, keybind
- `Globals::Silent` — silent aim FOVs, gun-based FOV, target part, wall check, keybind
- `Globals::Whitelist` — whitelist enable, color, user IDs
- `Globals::Settings` — streamproof, performance mode, team/client check, wall check method, kernel mode toggle
- `Globals::Misc` — speed hack, jump hack with keybinds
- `Globals::Explorer` — explorer panel state

---

## Threading Model

| Thread | Purpose | Frequency |
|---|---|---|
| Main | Message loop, render, menu, ESP draw calls | Per-frame (60/144/240 FPS) |
| Cache::Rescan | Re-reads DataModel pointers, detects game changes | Every 100ms |
| Cache::Runtime | Caches player data (health, parts, bones, distance) | Every 150ms |
| Aimbot | Target acquisition + mouse/camera movement | Continuous |
| Silent Aim | InputObject CFrame writes | Continuous |
| Triggerbot | Crosshair target check + auto-fire | Continuous |
| Misc::Speed | WalkSpeed writes while active | 1ms interval |
| Misc::Jump | JumpPower writes while active | 10ms interval |
| World | Lighting/Camera property updates | Continuous |
| WallCheck::cache_loop | Caches workspace OBB obstacles | Background |
| AntiInjection | Module/hook/debugger monitoring | Periodic |

---

## Math Library (`Engine/Math/Math.h`)

| Type | Description |
|---|---|
| `SDK::Vector2` | 2D vector with `+`, `-`, `*`, `distance()` |
| `SDK::Vector3` | 3D vector with `+`, `-`, `*`, `/`, `magnitude()`, `distance()`, `normalize()`, `cross()` |
| `SDK::Matrix4` | 4x4 matrix (`float data[16]`) |
| `SDK::Matrix3` | 3x3 matrix (`float data[9]`) with `Vector3` multiplication |
| `SDK::Vector4` | 4D vector (`x, y, z, w`) |
| `SDK::xorshift64()` | Thread-local xorshift PRNG |
| `SDK::fast_rand_float()` | Fast float in [0, 1) using xorshift |

---

## Memory Offset System (`Offsets.h`)

390+ offsets sourced from `offsets.imtheo.lol` (version `version-8884371d30284041`). Covers:

| Class | Key Offsets |
|---|---|
| Instance | Children, Parent, Name, ClassName, IsA |
| DataModel | GameId, JobId, PlaceId |
| Player | UserId, AccountAge, Character, DisplayName, Team, Health, MaxHealth |
| BasePart | Position, Size, CFrame, Velocity, Transparency, Color, Material, Primitive |
| Humanoid | Health, MaxHealth, WalkSpeed, WalkSpeedCheck, JumpPower, UseJumpPower, RigType, State |
| Camera | CFrame, Focus, CameraSubject, FieldOfView, ViewportSize |
| Lighting | Ambient, Brightness, FogColor, FogEnd, FogStart |
| InputObject | UserInputType, UserInputState, Position, Delta |
| VisualEngine | ViewMatrix pointer |
| RenderView | ViewMatrix, Width, Height |
| Model | PrimaryPart |
| LocalScript | ByteCode |
| ModuleScript | ByteCode |
| ByteCode | Pointer, Size |
| GuiObject | Visible |
| Primitive | Position |
| Misc | Value (used by knock detection) |

---

## Build

### Prerequisites

- Visual Studio 2022 with **Desktop development with C++** workload
- Windows SDK 10.0.26100.0 (or matching installed version)
- **For the kernel driver**: WDK NuGet package (`Microsoft.Windows.WDK.x64.10.0.28000.1839`) — restored automatically by NuGet in `BitwareDrv.vcxproj`
- MASM (ML64) — for `luck.asm` (empty stub, kept for MSBuild integration)

### Projects

| Project | File | Output |
|---|---|---|
| **Bitware** (main cheat) | `Bitware/Bitware.vcxproj` | `Build/Bitware.exe` |
| **BitwareDrv** (kernel driver) | `Bitware/BitwareDrv/BitwareDrv.vcxproj` | `Build/BitwareDrv.sys` |

### Bitware (main cheat)

- **Toolset**: v145, Windows SDK 10.0, C++20
- **Platform**: x64 Release
- **Entry**: `main()` via `mainCRTStartup` (subsystem: Windows)
- **Libraries**: `d3d11.lib`, `d3dcompiler.lib`, `dxgi.lib`, `Ole32.lib`, `Advapi32.lib`, `Shell32.lib`, `winmm.lib`, `freetype.lib`
- **UAC**: RequireAdministrator (Release)
- **Configurations**: Debug / Release
- **Release post-build**: Cleans PDB, iobj, ipdb, and tlog build artifacts

### BitwareDrv (kernel driver)

- **WDK**: NuGet package `Microsoft.Windows.WDK.x64.10.0.28000.1839`
- **SDK**: `10.0.28000.0` (provided by NuGet WDK package)
- **Toolset**: v145, KMDF (uses WDM directly — no KMDF library dependency)
- **Build**: `BuildBitwareDrv.bat` or manual MSBuild of `BitwareDrv.vcxproj`
- **Output**: `Build/BitwareDrv.sys` (6,144 bytes)

### Driver Resource Embedding

After building `BitwareDrv.sys`, the binary must be converted to a C byte array and placed at `Bitware/Source/Driver/kdmapper/bitware_driver_resource.hpp`. This header provides `bitware_driver_resource::driver[]` and `bitware_driver_resource::size` for embedding the driver directly in the cheat executable (never written to disk).

### Note on NuGet Packages

The main `Bitware.vcxproj` intentionally does **not** reference `Microsoft.Windows.SDK.CPP` or `Microsoft.Windows.WDK` NuGet packages. These packages override standard SDK include/library paths and expect SDK version `10.0.28000.0` which may not match the installed SDK. They are only needed in `BitwareDrv.vcxproj` for kernel-mode headers and libraries.
