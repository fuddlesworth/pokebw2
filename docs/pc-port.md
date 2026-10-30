# 64-bit PC port: SDL3 and Vulkan

## Current milestone

`pc/` is an independent C99 host target using SDL3's Vulkan GPU backend. It compiles the recovered NitroSystem
G3D/G2D renderers, fixed-point math, and GFL archive/background routines directly with native 64-bit pointers.
All native targets require C99 with CMake extensions disabled; new host code is compiled with pedantic warnings
as errors. Recovered SDK headers still contain their original anonymous unions and zero-length arrays, so these
headers remain system includes rather than being described as strictly ISO C99. No C11 features are required.
Original models, textures, skeletal animation, and multi-cell sprite animation reach Vulkan through native
geometry FIFO, matrix state, texture memory and palette implementations. The BG shader reads the original
register state and VRAM to decode tile maps.
Model creation and texture uploads use the original GFL 3D system, with the original GFD managers allocating
texture and palette space for both models and sprites. The native GFL build enables allocation bug fixes for
absent texture kinds, compressed texture sizes, and allocation-failure cleanup; the ROM build retains its original behavior.

Verified resource paths include the Aspertia house interior, an animated model, and the intro's complete animated
Juniper sprite with its background. The background uses GFL_BGSys and arc_util; the sprite uses the original NNS
multi-cell animation and rendering code. The original boot screens, Game Freak movie, opening movie, title process,
and main menu now run. The title combines its 3D scene, logo and prompt on one canvas. Original OAM sprites and
key-repeat logic run through native VRAM and SDL input. Title STRM music and Pokémon cries use SDL audio;
SSEQ playback now uses a C interpreter/envelope engine with native sample channels; playback validation is in progress.
The original new-game intro, both name entries, the Aspertia flyover, and the first field scene now run. A visible
sanitizer replay completed 11,000 frames through field dialogue and map transitions without an error. The field's
3D view and the Aspertia flyover use the full 16:9 canvas, with a wider horizontal view that keeps characters'
size and proportions; the native window defaults to 1280×720 and remains resizable. The PC renderer retains the
field's last 3D frame during the original 30 FPS redraw skips, then clears it while a map transition loads. Unused
window space and the field's unmodeled interior edges are black. Original 2D windows are currently centered on it.
The scripted replay returns control to live keyboard input when its entries end.
Season and location banners span the canvas using their original background tiles. Location patterns repeat
through the extra width at the original scale; season splash margins sample the plain background edges.
The location icon/name and season label anchor to the left and right edges while retaining their original
sprite scale and animation. For repeating game artwork, extend the tile sampling before introducing new assets
or stretching a framebuffer. Fractional source rectangles keep the tile spacing continuous across the joins.
The field menu is accessible from the X button (S on the keyboard); its original lower-screen controls are
temporarily shown in the same window. Options opens the native widescreen Settings application.
The Aspertia script now reaches the starter choice and Bianca's Pokédex gift. A save made afterward resumes at the
lookout. The field menu opens the recovered Pokédex process: its list displays the acquired starter, its detail
page shows the real entry, and B returns to the field menu. Native palette reads use the PC VRAM buffers, and the
list's scroll bar handles a collection shorter than one page. The PC list now draws its header, species list,
Pokémon artwork, entry card, and footer at 1280x720 coordinates, following the one-screen Pokédex mockup. Its
header, footer, list rows, center backdrop, and entry frame are SVG components based on the mockup in
`pc/assets/dex/`; Vulkan composites them before drawing the live content. Static lettering uses vector paths and
dynamic text uses the bundled Pixel Operator fonts. Names, descriptions, both type badges, height, weight, footprints,
and the 96x96 Pokémon artwork come from the game resources. The type badges use `AppMenuCommon` sprites and palettes;
footprints use the original cell layout and form-specific archive selection. The Pokémon's reflection shares its
texture and fades beneath its feet. Z or clicking the Pokémon plays the displayed form's original cry. Unknown
entries remain hidden, and footprints, types and measurements require a caught species, like the original screen.
The native screen enters the list directly, skipping the DS cover's timed wait and animation. The original process
still owns the list's lifetime; the native UI handles browsing, name/number search, seasonal habitat listings and
recorded form selection. Tab or Q/W changes tabs; Left/Right changes the season or form on those pages.
S opens the filter card: the original search engine supplies number/name/size sorting and initial, two-type, color,
and shape filters. The regional/national selector respects the save's National Dex unlock, and the header and numbers
follow that selection. Z applies, X cancels, and R resets the draft. Name/number search remains available with /.
L cycles recorded foreign-language entries for species supported by the original game, including localized type
badges. Characters absent from the bundled font use the original game glyphs. The Area page decodes the original
Unova map and region masks; H switches to the original visited-habitat completion data. Click a location or press A
to select one; Backspace or clicking the lower card pages the habitat list. Story visibility and Surf/Fishing
availability come from the game's save data. Forms uses the original MCSS multi-cell renderer: Space plays/pauses,
V switches front/back, and C compares the selected appearance with the next recorded appearance. The controls have
aligned SVG rows, and original form names label the list. Animated sprites and reflections share a floor derived
from the resting pose's visible pixels. Cry playback displays a waveform from the actual PCM samples; C hides or
shows it on Entry.

The field menu's Pokémon option now opens a native party screen on the same 1280x720 grid. Six SVG roster cards
show the actual party, original animated icons, HP and status; the selected Pokémon uses the original MCSS
animation, front/back sprites and reflection. The Party page shows nickname, species, level, gender, original type
badges, held item and ability. Eggs use the game's egg graphic and hide their species, HP, types and ability.
The party and Pokédex share the font renderer, type-badge loader and panel components. Arrows or the wheel browse,
Z/Enter opens actions, X/Escape returns, and V changes front/back. Clicking a roster card opens its actions;
clicking a destination completes a switch or item move. Click the header tabs or use Tab/Q/W to switch between Party
and Overview. Overview shows the selected Pokémon's six stats, nature, experience and four moves with current/max
PP; browsing the roster keeps this page open, and selecting an action returns to Party. Eggs hide those details.
The original party process owns action labels, eligibility, messages, party reordering and item transfers.
`party_view.c` owns the roster, animated preview, details and Overview layout for both field and battle callers.
`ui_frame.c` owns menu header titles, section labels and footer content. SVGs supply geometry; render footer text
once through the shared component. When integrating another game mode, adapt its data and actions to the existing
view instead of copying coordinates, drawing code or embedded labels into a second screen. Menu sprite rendering
flushes pending game geometry before measuring the sprite and isolates its camera from the battle camera.
The native UI framework separates original game controllers from presentation:

- `ui_theme` owns the palette, spacing, frame bounds and semantic typography. Use text roles such as
  `PC_UI_TEXT_BODY`, `PC_UI_TEXT_TAB` and `PC_UI_TEXT_VALUE`; font faces, preferred sizes and line spacing live
  in one table. Screens do not choose raw font sizes or weights.
- `ui_widgets` supplies bounded labels, aligned values, paragraphs and stat rows. Single-line labels fit their
  available bounds and use glyph ink metrics for centering. Paragraphs reflow the original DS line breaks,
  split long words, and reduce their preferred size only when the complete text needs more room.
- `ui_components` draws tabs, buttons and rows and registers their actions using the same bounds. Begin each
  view with its application owner and phase; pointer events resolve only controls from that owner and phase.
  Disabled controls and gaps have no action. `ui_skin` holds shared bevel and slice measurements so resizing a
  control preserves its diagonal slope.
- `ui_frame` owns screen headers, footer content and key hints. `assets/menu/profile.svg` supplies the common
  Summary/Trainer/Save card shell; their smaller SVGs contain only screen-specific details. SVG layers use
  premultiplied source-over composition, keeping transparent overlays and underlying panels intact.
- `menu_ui` routes lifecycle, drawing and input to separate Bag, Summary, Mail, phrase and map controllers.
  `ui_input` supplies shared key mappings and application-scoped input state. Queued actions are discarded on
  application/phase changes; held-key edges survive page changes. Field and battle Bag share `bag_view`,
  including selection-keyed item descriptions. Field and battle Party share
  `party_view`, which preserves original slot identities even when partner parties contain gaps.

When adding a screen, adapt its original state into these views, choose semantic text roles, and attach actions
while drawing controls. Do not duplicate hit rectangles in its event handler or add a separate font-size policy.
The CPU tests cover sloped control boundaries, row gaps, owner/phase transitions, keyboard mappings, font failure
recovery, text bounds, all text roles and original native matrix/save behavior. Visual comparisons remain necessary
for composition and original-game transitions.

Escape is Back in gameplay and native menus, including controller-generated keyboard events. It only quits the
standalone diagnostic viewer; closing the window still exits the game.
Window close stops game updates immediately. Host teardown closes audio and save services while game memory is
still valid, then releases graphics, remaining resource streams, and all native heaps (including independent roots
and aligned allocator blocks). It never injects buttons or waits for game scripts to reach an exit state.
Events that survive a field close/open must refresh their cached field pointers from `GSYS_GetField` after the
new field opens. Host allocations do not preserve the DS heap's address reuse. The map-change owner refreshes
its pointer before creating arrival transitions, as the field-menu and shortcut owners do after returning from apps.
The Route 19 capture-tutorial replay passes ASan/UBSan through map arrival, Bianca's scripted battle, the catch,
and the return dialogue (`0x40a3 = 1`, story work `0x40a1 = 8`). Battle effect sprite positions use defined signed
multiplication for native fixed-point conversion, including the negative coordinates in the Poké Ball animation.

Summary, Bag, Mail, phrase input and the Fly map now have native SVG layouts on the same design grid. Menu text uses
the bold Pixel Operator face throughout, including names and descriptions; the thin face is not used for UI text.
Summary reads the original trainer memo, moves, PP, stats, ability, markings and Ribbons. Q/W or the header tabs
switch pages; selecting a move enters the original detail/reorder or move-forgetting flow. Bag pocket tabs, item
icons, descriptions, action menus and quantity controls use the original inventory process. Q/W switches pockets,
T opens sort/filter, A moves an item, and D registers eligible items. Mail and its phrase picker use the original
sentence templates, word availability, search, completion checks and return data. Fly destinations come from the
original Town Map visibility and travel flags. These screens still need integration coverage beyond the paths
listed below. Battle party selection uses the same native party view through its own original controller.

Visible integration checks use a copied save with a six-Pokémon party and additional items. They cover Summary
page changes, original Ribbon graphics, move reordering, giving/replacing/taking held items with inventory-count
checks, Mail composition with one- and two-word phrases, category and letter search, word-list paging, attaching
and reading Mail, and cancelling both phrase input and Mail without consuming an item. The Fly fixture enters
through the original party-result routing before the story has unlocked Fly; the map uses the save's actual
destination flags, returns the original destination, and completes the transition back to the field. A separate
move-learning fixture checks the original Summary-to-party handoff, replacement of a selected move, HM
restrictions, and cancellation without changing the moves. The normal `visible-demo.sav` is not changed by these
fixtures. Broader Bag modes, every Mail design, and every
field destination have not been exhaustively exercised.

The field menu's Trainer option opens a native card with Profile, Records, Badges and Signature tabs. It uses the
original portrait, player data, record visibility rules, badge sprites, earned dates and badge descriptions.
Badge cards place the complete 32x64 sprite to the left of the name and earned status. Profile edits use the
original class, nature and greeting handlers; the saved-greeting list and phrase editor share the native layout.
The signature pad uses the original brush and tiled pixels, with Pen, Eraser, Clear and Undo controls. Closing the
card stores the signature and profile changes in the game's save blocks; the normal game save writes them to disk.
Native UI text and artwork retain their authored colors during the original applications' DS brightness effects.

Visible Trainer checks use a separate copied save. They cover all four tabs, class and nature changes, composing
and applying a greeting, single-click saved-phrase selection, cancellation without replacing the card's greeting,
signature drawing, clearing, undoing and persistence across closing and reopening the card. An earned-badge
fixture checks all eight complete original icons and their date/description selection. The original
badge polishing/3D display, other-player cards and Medal detail application are not part of this native screen yet.

The field menu's Save option uses the same native SVG panels and bold fonts. It shows the player, current location
and date, original party icons, play time, badges, Pokédex count and last-save timestamp. Save and Cancel feed the
original report event's Yes/No menu; the original save writer controls progress, completion, failure and the
existing-save restriction. Input cannot dismiss a write in progress. The original backup copies and checksums
remain in use, and the native screen reports success only after the original writer succeeds. Keyboard arrows,
Z/Enter and X/Escape work alongside the mouse buttons. The save-screen integration fixture has its own copied save.
Visible checks cover cancellation without changing the file, writing and reloading the saved game, the existing-save
restriction, and a forced file-write failure that leaves the previous save intact. Failure retains the last successful save's timestamp;
Continue closes the native message with one click, without the hidden DS page-break prompts.

The first rival battle uses a 16:9 arena and native SVG HP cards, move buttons, action buttons, dialogue and
level-up statistics based on the battle mockup. Move selection feeds the original battle input handler, so PP,
damage, animations and story outcomes remain controlled by the game. Type badges use the original resources.
The footer measures its labels to place SVG keycaps with consistent gaps. The command prompt is centered in its
panel, HP/EXP text aligns with the bars, and the original gauge's gender mark follows the measured nickname.
The rival cut-in spans the window. A visible ASan/UBSan replay covers the cut-in, both send-outs, multiple turns,
victory, level-up and return to Hugh's field dialogue. The normal save remains unchanged by this copied-save test.
Battle Bag and Pokémon buttons now open native panels backed by the original battle applications. The party
view is shared with the field menu; a battle adapter supplies current HP, PP, held items and switch eligibility.
Run uses the original escape action and refusal messages. Visible checks cover party switching, Summary/Overview
and Moves, fainted-Pokémon refusals, cancellation without inventory changes, a Potion healing its target, an Ether
restoring the selected move, trainer Poké Ball rejection and trainer escape rejection. ASan/UBSan checks cover
the item flows and party display. Multi-battler target layouts still need native integration.

Geometry matrix store/restore follows the active matrix mode: MCSS switches between orthographic sprites and
perspective shadows inside a draw. Restoring only position matrices collapses the shadows to stray dots and
leaves effects with the wrong projection. Native UI uses unused polygon-attribute bit 23; bits 24-29 belong to
the game's polygon IDs, including particles. Reusing bit 29 misclassifies effects as UI, changing their viewport
and allowing them over dialogue. The CPU renderer regression covers both cases. Native battle state becomes
unavailable when its source parties are released, before the view itself is deleted, to avoid teardown reads.
Native font buffers pad the complete rows and prefetch byte consumed by each converter, including the small
16x8 HP font. The serialized Pokémon nickname/ribbon block keeps its four-byte alignment on 64-bit hosts;
ABI checks protect its size, ribbon offset, alignment and the surrounding saved Pokémon sizes.

This is **not yet completion of the user's goal**. Windows/blending, capture effects and the
remaining single-screen layouts remain unfinished. Some BG modes have implementation paths but
have not yet been visually validated. Compilation alone does not demonstrate support.

Without resources the executable displays a diagnostic canvas. Tab toggles its panel; Escape exits. The diagnostic
pointer mapping and geometry renderer share the same aspect-preserving viewport when the window is resized.

The PC target requires 64-bit pointers. Linux LP64 and Windows LLP64 are the intended data models; the current
validation is on Linux x86-64. ARM64 is allowed by the build but has not been tested. There is no 32-bit PC target.

The DS build remains `python3 configure.py` and `ninja` at the repository root. The host target neither adds files
to the ROM delinks nor changes the matching compiler's include paths.

## Build and run

Install a native 64-bit C compiler, CMake 3.21+, Ninja, Python 3.11+, SDL3 3.2+ development files with the Vulkan
GPU backend, `glslc`, FreeType, librsvg 2.52+ (including Cairo), and a working Vulkan driver. Real rendering checks
use extracted game resources; diagnostic and CPU tests need no ROM.
The CMake configure step generates the native headers from repository data without downloading the DS compiler.

CMake fetches the MIT-licensed [libntr](https://github.com/cybervisi0n/libntr) at commit
`1555d911ac80c0190ac1c321f99f1323de8b3c09` for three C sound-engine files. Its simulator and C++ sources are not
built. `pc/patches/libntr-envelope.patch` replaces undefined signed shifts in the envelope calculation with
multiplication. Native bank/wave decoding and sample mixing remain in `pc/sequence.c`.

```sh
cmake -S pc -B build/pc -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/pc
ctest --test-dir build/pc --output-on-failure
./build/pc/pokebw2-pc
```

Run the entry movies with `--boot --data-root build/b2_us/files`. `--title` and `--menu` start directly at those
original processes for testing. Z/X map to A/B, Enter to Start, Backspace to Select, arrows to the D-pad, A/S to
Y/X and Q/W to L/R. The original save-block code uses a 512 KiB file-backed flash adapter. The default save is
`black2.sav` under SDL's `pokebw2/pokebw2-pc` preferences directory; `POKEBW2_SAVE_FILE` overrides the path for
testing. The backup test checks persistence, bounds, callback completion, CRC recovery and rejection of corrupt
copies and incorrectly sized files.

`POKEBW2_TRACE_DEX=1` logs the elapsed time and simulation ticks from opening the Pokédex procedure to its first
native UI frame, and the species, form and playback state when a cry is requested. This measures UI construction,
not GPU presentation latency.
Set `POKEBW2_GPU_DEBUG=1` when GPU validation is needed; normal play leaves it disabled.

`POKEBW2_INPUT_REPLAY` accepts a text file of frame numbers and hexadecimal DS button masks (one pair per line).
Each mask remains held until the next entry; use a zero mask to release buttons. Replay exercises the original
input and game processes. `--new-game` starts the original new-game flow directly. Use a separate save path for
these development runs.

Clang address/undefined sanitizer builds using GNU ld need `-Wl,-z,start-stop-gc` in
`CMAKE_EXE_LINKER_FLAGS` so sanitizer registration sections do not retain every unused application descriptor.
This preserves global instrumentation while allowing the native build to link its selected applications.

After extracting the supported Black 2 ROM with the normal repository build, exercise the original intro rendering:

```sh
./build/pc/pokebw2-pc --data-root build/b2_us/files --frames 120 --resize-test --screenshot build/pc/intro.bmp
```

This loads the intro background and multi-cell animation, the real font archive, and greeting message 6 from
message file 54 through the original GFL loaders and text renderer. It is a rendering check, not the intro's full
script or an interactive new-game flow. `--help` lists the model, external texture and skeletal-animation checks.

Pass `-DSDL3_DIR=/path/to/SDL3/cmake` if CMake cannot find SDL3. On Windows use a 64-bit toolchain and make the
SDL3 runtime available beside the executable or on PATH. The full Windows application remains unverified.
SDL owns Vulkan device/swapchain creation and synchronization through its GPU API. CMake compiles GLSL with
`glslc`; no offline asset conversion is involved. The selected backend is checked and logged;
failure does not silently switch to another graphics API.

The display-independent tests cover viewport bounds, letterboxing, pointer mapping, scalar/pointer widths,
original G3D resource lookup, SDK sorting and CLI validation. The native sort adapter checks ordering and complete
record preservation for duplicate keys, extreme keys, and each list size through 601 entries. On a machine with a
display and Vulkan device, enable the presentation test:

Use `ctest --test-dir build/pc -L cpu --output-on-failure` for checks without windows. Graphical tests have the
`graphics` label and share a CTest display lock. Run those on an isolated display when available; on the user's
desktop, run only one selected graphical test or gameplay replay at a time, never both concurrently.

For automated runs, start Xvfb on an unused display, then set `DISPLAY=:99 SDL_VIDEODRIVER=x11
SDL_AUDIODRIVER=dummy` for the game or CTest. This preserves the Vulkan renderer without desktop windows. SDL's
`offscreen` video driver instead requires `VK_EXT_headless_surface`, which the local NVIDIA driver lacks;
Xvfb with SDL's X11 driver is verified to present successfully on that same driver. The local test server is
`build/pc/headless/usr/bin/Xvfb :99 -screen 0 1152x648x24 -nolisten tcp`.
`POKEBW2_VSYNC=0` selects immediate presentation; normal play uses VSync. Simulation uses a monotonic fixed
60 Hz clock regardless of presentation rate. This is the game's nominal rate (`src/gfl/ui.c`) and keeps the
native audio mixer's 800 samples per tick at 48 kHz; the DS's approximately 59.83 Hz physical refresh is not
reproduced exactly. The original main loop waits for VBlank once per update (`src/system/main.c`), while
30 Hz field work keeps its original alternating-frame behavior. Each simulation tick advances input, replay,
VBlank tasks, audio, fades, animations and process callbacks once. Original drawing callbacks also mutate state,
so their completed geometry is retained and presented again on display frames between game ticks.

Catch-up runs at most five ticks per presentation; longer stalls discard whole overdue ticks and retain the
fractional tick. A debugger stop or suspend therefore does not cause an unbounded burst of game updates.
`--frames N` still counts successful presentations, not simulation ticks. Replay timestamps count simulation
ticks; disabling VSync no longer fast-forwards replays. Use enough presentation frames (or omit the limit)
to allow the intended replay duration at the display's frame rate. The CPU `pc-fixed-timing` test verifies
equal simulation progress at 24–1000 presentations per second, fractional timing and bounded stall recovery.

```sh
cmake -S pc -B build/pc -DPC_GPU_TESTS=ON
cmake --build build/pc
ctest --test-dir build/pc --output-on-failure
./build/pc/pokebw2-pc --frames 120 --resize-test
```

The GPU tests exercise the diagnostic canvas, original model resources, skeletal animation, texture allocation,
and swapchain resizing. Set `-DPC_GAME_DATA=/absolute/path/to/build/b2_us/files` to include the original intro,
background and font rendering test. These check successful API submission, not pixel-perfect GPU output. A null swapchain while
minimized is skipped. A bounded run fails if it cannot present for ten seconds, and CTest has an outer timeout.
Debug device creation enables available GPU validation; availability depends on the installed Vulkan layers.

Initial verification uses GCC and Clang on Linux x86-64 with SDL 3.4.18. Configuring with `-m32` reaches the explicit
64-bit-toolchain rejection. All 21 G3D files also compile without warnings with the Windows x86-64 LLVM-MinGW
toolchain, and the ABI test links against the original resource accessor. That Windows test has not been executed;
the full Windows application remains untested. Address/undefined-behavior sanitizer
tests pass with leak detection disabled for the GPU run. With leak detection enabled, the local SDL/Vulkan stack
reports 5,520 bytes in 54 allocations at exit (DBus and unloaded modules); an independent SDL window/device-only
program reproduces exactly that report without any port code or texture uploads. This is a recorded validation
limitation, not a clean LeakSanitizer result. No sanitizer suppression is built into the application or tests.

## Single-screen direction

The target is one integrated widescreen view, with field/battle content and contextual menus sharing the screen.
DS main/sub engines are compatibility inputs, not two permanent rectangles in the final UI. Menus, battle commands,
battle party selection and C-Gear functions still need scene-aware presentation and action routing.
A temporary dual-screen debug view may help compare against the ROM, but does not define the product interface.

The checked-in Pokédex mockup is `pc/assets/dex/reference.png`; its editable screen components are in
`pc/assets/dex/`. The current small field-menu overlay is only for accessing the original applications during
bring-up. The native Vulkan renderer already rasterizes at the output window resolution; the 384x216 canvas describes
legacy game coordinates.
The Pokédex and party UIs use a 1280x720 design grid scaled to the 16:9 output. The diagnostic's colors and panel remain
calibration fixtures, and the CPU-upload path is only a bring-up aid.

## 64-bit boundaries

Do not put the existing `include/` directory on the host shell's include path: it includes MWCC C-library headers
as well as DS SDK types. In particular, `include/types.h` defines `u32` as `unsigned long`, correct for MWCC ARM32
but eight bytes on Linux LP64. Changing it globally would also violate the matching SDK declaration policy.

Before compiling a recovered game module on PC, introduce a deliberate native SDK/header boundary and verify it
with both native tests and the two ROM builds:

| Value | PC representation |
| --- | --- |
| DS scalar, saved word, register, binary offset | Explicit `uint8_t`/`uint16_t`/`uint32_t`/`uint64_t`, signed equivalents |
| Live host object/function pointer | Native pointer to its actual type |
| Address stored as an integer | `uintptr_t`/`intptr_t`, only for actual host addresses |
| Allocation size, host array extent | `size_t`, with checked conversions at DS format boundaries |
| Pointer encoded in ROM data | A decoded 32-bit DS address/relocation resolved to a host object |
| Native pointer-containing structure | Native layout; allocate with `sizeof`, not recovered ARM32 byte counts |
| Save, NARC, model, or other serialized structure | Preserve the original format and accessors; verify scalar widths, offsets, alignment and endianness before using a mapped structure |

Windows LLP64 has four-byte `long` and eight-byte pointers. A Linux-only `long` substitution is not a portability
solution. Do not truncate pointers into `u32`, map every object below 4 GiB as a workaround, or assume widening
the `uptr` alias alone repairs stored callbacks, fixed allocation sizes, or binary relocations. Existing examples
of these risks are in [Native port notes](porting-notes.md).

## Integration sequence

The PC port compiles BW2's recovered rendering code directly. Native work replaces hardware/platform interfaces
and fixes host ABI assumptions; it does not build a separate asset viewer or introduce a Python preprocessing
pipeline. This is the user's explicit direction on 2026-10-23. GLSL compilation belongs in the native build.

1. **Native types and a small recovered module.** Compile a curated, hardware-independent game module from the
   original source with the native header boundary. Test its behavior on 64-bit hosts before expanding the list.
   Audit pointer-bearing structs, allocations, callbacks, casts and binary overlays in each dependency closure.
2. **Platform services and boot closure.** Implement required heap, file/archive, overlay, input, timer and OS
   services against their BW2 callers. Track unresolved symbols explicitly; unsupported services must fail loudly.
   `src/system/main.c` reaches `SystemInit`, `GFLAppInit`, IRQ setup and `GameInit`; `GameInit` loads overlays 12/27
   and queues `EVENT_TITLE_FUNCS`. `pc/boot.c` runs this host boot closure and the original process manager.
3. **Scheduling and rendering fidelity.** Preserve the game's update/VBlank ordering, validate the DS clock from
   recovered constants. `PcGameClock` advances simulation at the DS rate independently of presentation refresh.
   Maintain 2D BG/OBJ, palette/VRAM and 3D command handling; compare known scenes against
   DS output before enhancing them. Vulkan rendering replaces hardware effects, not game logic.
4. **Owned ROM and saves.** Validate the supported B2/W2 dumps, reuse the existing extraction tools, and decode
   assets into host representations. Keep binary save layouts stable and use atomic persistence under a host user
   data directory. No ROM or extracted assets belong in distributed PC binaries.
5. **One playable single-screen scene.** Integrate field movement and a contextual menu, then battle commands,
   using the mockups as visual acceptance references. Add keyboard/gamepad actions and pointer hit testing without
   losing access to secondary-screen functions. Audio, complete boot/gameplay, and packaging follow tested slices.

## References inspected

Inspected on 2026-10-23 as architectural references; no source was copied or vendored.

| Reference | Evidence and lesson |
| --- | --- |
| [Platinum PC branch](https://github.com/cybervisi0n/pokeplatinum/tree/pc_port) | `README.md` and `meson.build`: shared game source with ROM/native build targets and replacement libntr SDK dependencies. Use the `pc_port` branch, not `main`. |
| [Project Holon eidos](https://github.com/Project-Holon/eidos) | `README.md`, `docs/architecture.md`, `docs/runtime.md`: separate game source, SDK services, hardware rendering and platform layers; useful boot/VBlank and pointer-layout lessons. Some architecture/runtime sections describe older milestones, so their status claims are not treated as validation for BW2. |
| Local `../pokeemerald-native` at `a5589b181c0ecab001ac5554079cab1f076fe832` | `INSTALL_PC.md`, `src/platform/sdl2.c`, `src/platform/system.c`: host rendering/input and hardware services separated from game logic. Its SDL2/GBA paths are references, not a DS backend. |
| [SDL GPU device creation](https://wiki.libsdl.org/SDL3/SDL_CreateGPUDevice) and [swapchain acquisition](https://wiki.libsdl.org/SDL3/SDL_WaitAndAcquireGPUSwapchainTexture) | Explicit Vulkan selection, minimized-window handling, and command submission ownership. |

The references inform boundaries; BW2's source, recovered types, and ROM behavior remain the authority for its
game semantics. Any later reuse of external libraries needs a deliberate API and license integration review.


### Native settings

The Options menu uses the settings mockups' sidebar, light option rows, contextual help, and Apply/Revert/Restore
Defaults controls. Shared text roles, buttons, pointer registration, and frame components keep it consistent with
the other native menus. The sidebar and selection outline are SVG assets.

Apply stores text speed, battle animation/style, save-before-IR, stereo/mono output, master volume, window size,
fullscreen, VSync, drawing frame limit, MSAA, integer scaling, hardware-sprite filtering, controller input/deadzone,
wheel direction, and primary/alternate gameplay keyboard bindings in the same `.sav` file. Gameplay
preferences also update the original Config, and are restored after the original save loads. The drawing frame
limit does not change the fixed game simulation rate. Keyboard conflicts show a Swap/Cancel prompt; Escape cancels capture, and Delete clears the selected alternate. Native menus retain their documented shortcuts. Tab switches category/row focus; Page Up/Down changes
the binding page. Back warns before discarding a modified draft.

The original 512 KiB flash region is unchanged. An optional trailer holds two alternating 272-byte records:
`BW2PCSET`, a little-endian generation, a 256-byte versioned payload, and a 32-bit FNV-1a checksum. Flash writes
cannot address this trailer. A partial/corrupt newest record falls back to the preceding valid record; legacy saves
load defaults. The payload encodes bytes explicitly rather than writing a host C struct. PC settings tests cover
round trips, original flash preservation, ordinary game writes, invalid bindings, and corrupt-record recovery.

The recovered US Options app has exactly five rows: text speed, battle scene, battle style, sound, and save-before-IR.
All are exposed; sound lives in Audio. Language and C-Gear state are separate game configuration, not extra rows in
that Options app.

Payload version 2 adds optional alternate bindings and graphics/controller/mouse fields. Version 3 adds a preferred
controller GUID, twelve gameplay button bindings, and optional left-stick movement. Version 4 records a device slot
and path hash alongside the GUID so two controllers of the same model can have distinct preferences. Versions 1–3
load with their existing preferences intact and receive defaults for fields they do not contain; a version 3 GUID
without a saved path selects the first matching controller until a specific device is chosen. The Device tab selects
Automatic or a connected device, enables input/stick movement, adjusts deadzone, and shows live stick/button input.
A disconnected preferred device waits for reconnection rather than silently switching controllers. The Bindings tab
uses the shared binding/conflict editor and Swap/Cancel
confirmation. Native menus keep standard controls even when gameplay buttons are rebound. Y switches controller
tabs; View/Back changes binding pages; LB/RB switch categories. Apply/Revert/defaults include controller preferences.
CPU tests use an explicitly selected virtual controller and cover B/Back, remapping, stable menu controls, deadzone,
disabled input, disconnection, save round trips, invalid bindings, and migration from older payload versions.

MSAA resolves the game scene before the native text/UI pass. The original DS capture path keeps single-sample
pipelines and its 256x192 framebuffer. Both the multisample and following single-sample depth attachments use STORE:
discarding depth at the pass transition caused NVIDIA depth-target faults/device loss in visible testing. Preserving
both passes' depth fixes boot and the Options-to-field transition with 4x MSAA. Linear sprite sampling interpolates
premultiplied colors to avoid dark transparency fringes and leaves native text unchanged. Integer scaling uses the
same viewport for presentation, native pointer controls, and legacy touch input, including high-DPI windows.

Building-entry capture retains the original 256x192 VRAM image for game state and also records the full 384x216 field
frame for PC presentation. The captured frame covers the new map until the original alpha fade reveals it, then the
game clears both captures. The host marks VBlank while running its callbacks so the original affine entrance zoom
advances. The widescreen capture stays visible through the field process's brief handoff and when VRAM bank D is
temporarily remapped. The field renderer holds its last complete 3D list through the brief actor reload after the fade,
keeping sprites visible until the new list is ready. Field wipe masks use the original game-frame progress and cover
the full PC canvas for the circle and four directional patterns; the DS window registers still drive the original game
logic.
The main screen's white master-brightness fade also colors empty canvas pixels, so it reaches the edges.

Aspertia Gate's weather board reads the `ZONE_MAX` marker past the final zone header. NitroFS returns zero bytes at
end-of-file without failing; the PC file adapter now does the same, while still reporting actual stream errors. A
replay that warps into Aspertia Gate completes its map load and renders the gate.

Aspect ratio remains fixed at the native 16:9 layout; arbitrary aspect selection is not implemented. Separate Run/Back
and Open C-Gear actions and a captured field preview are still absent from the mockup design.
The right panel shows the selected binding/conflict while rebinding, and a themed emblem on other pages.
