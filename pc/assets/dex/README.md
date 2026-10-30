# Pokédex SVG components

`reference.png` is the supplied 1672×940 single-screen mockup. Runtime artwork is rebuilt as editable SVG components in a 1280×720 design coordinate system. These components contain no embedded raster images or cropped bitmap patches.

| Asset | Placement and purpose |
| --- | --- |
| `header.svg` | 0,0; title, regional label, search frame and top trim |
| `top-junction.svg` | 356,81; the complete header notch, extending below the header's bounds |
| `header-corner.svg` | 1256,81; the white fold joining the header rail to the right-hand frame |
| `center.svg` | 375,81; grid, Poké Ball backdrop, platform and browse arrows |
| `row.svg` | 0,90+61n; complete inactive list row |
| `selection.svg` | selected row; complete cyan outline and arrow |
| `information-card.svg` | 796,144; pale card, emblem and straight divider |
| `stats.svg` | 855,354; Entry-only height/weight background and rules; captions, numbers and units share a baseline, with separate numeric and unit columns |
| `description-card.svg` | 796,424; description panel, bevels and emblem |
| `tabs.svg` | 794,101; three 486×43 state strips sharing the same tab paths and red edge trim |
| `footer.svg` | 0,646; bottom trim and navigation controls |
| `filter-row.svg` | 848,y; two 400×32 focus strips for the pale and dark cards |
| `form-controls.svg` | 850,295; aligned key, action and value columns for playback, view and comparison |

Each tab's active fill uses its entire outline, including both angled edges. The same paths draw inactive borders. All three states live in one SVG so geometry cannot drift between separate state assets.

List rows and their selection overlay are both 375×61. The selected outline's 4px stroke is inset relative to the inactive 2px stroke so their visible top, right, bottom and beveled edges coincide. Compare stroke extents, not just the path coordinates.

The information and description panels share a continuous white shell. Both SVGs meet at design y=424 with the white strip spanning local x=434..478. The upper card's dark bottom rule ends before this strip; the lower panel's inset bevel exposes the shoulder and right spine, which join its white bottom rim. Preserve this connection instead of enclosing each panel in a separate box.

The center frame's upper-right bevel stops before the Entry tab. At its lower-left corner, the grey outline turns inward above a solid white fold; that fold and the footer share the point (375,646) and the same fill. These are separate inset and outer frame edges, not crossing strokes.

Straight bevels use 45-degree diagonals (equal horizontal and vertical changes). Card fills, outline strokes and grid clipping reuse one frame path; corner accents follow that path rather than a separately estimated polygon. Detached bevel accents use filled polygons, not stroked diagonal segments. Their long sides remain parallel to the frame with a consistent clear gap. Their end cuts follow the adjacent frame geometry: horizontal for the tabs and footer trim; vertical and horizontal for the description corner. Keep cross-component junctions within a component that includes their full bounds. The compositor rounds both ends of every destination rectangle so shared boundaries remain coincident at fractional window scales.

The C99 loader rasterizes SVGs through librsvg/Cairo at twice the design resolution, then uploads them to SDL's Vulkan GPU backend. Static lettering is converted to vector paths, so it cannot fall back to a system font. Dynamic text uses the same Pixel Operator family through FreeType. Font licensing is in `../fonts/CC0-PixelOperator.txt`, from the author's [distribution](https://www.dafont.com/pixel-operator.font). Typography approximates the generated mockup; the reference does not contain a complete reusable font. Lettering paths were exported with Pango/Cairo from the bundled fonts and positioned in each component's viewBox.

Pokémon artwork, both type badges and footprints come from original game archives. Footprints use the original cell layout and form-specific resource selection, and appear only for caught Pokémon. The center sprite stands on a shared floor at y=473; its reflection reuses the same texture with reversed vertical coordinates, a compressed height and fading opacity. Paragraphs wrap by word to the card width instead of retaining DS line breaks. Search results and recorded forms use the current save data.

Z, the footer's Cry control, or clicking the Pokémon plays its original cry for the displayed form. Selecting another Pokémon or form stops playback. The native screen skips the DS cover's five-second input wait and slide/zoom sequence; `POKEBW2_TRACE_DEX=1` logs time from Pokédex procedure resolution to its first native frame, along with cry playback state.

The filters replace the right card's content and suppress its measurement strip. Up/Down selects a row, Left/Right
changes the value, Z applies, X/Escape cancels, and R resets. Wheel input selects a filter row while this card is open.
The original search engine supplies the results; the current save controls National Dex access.
Keyboard presses remain pending until a simulation tick consumes them, so a press and release between ticks
does not drop the action.

Area uses the original town-map layers and habitat masks with translucent cyan highlights. H switches to visited
habitat completion (green for complete, amber for incomplete); A selects the next location and Backspace pages the
species list. The original habitat builder applies story and encounter-method conditions. Forms uses MCSS animations
and original form-name messages. Space toggles playback, V switches front/back, and C compares adjacent recorded
appearances. L on Entry cycles the species' recorded languages; original glyphs and localized type badges accompany
the foreign messages. Cry playback adds the PCM waveform below the platform; C toggles its visibility.

SVG pieces are drawn through the skin GPU pipeline with source-over blending of Cairo's premultiplied pixels.
The control rows contain opaque backgrounds matching their host cards; this keeps dark text readable. Existing
detached accents and card connectors retain their approved geometry.

Validation uses a separate copy of the starter save, with temporary debugger fixtures for recorded species,
languages, forms and visited habitats. The normal save is not unlocked by these checks. Live checks cover the
original alphabetical order, all 48 Fire results, an empty Dragon/Normal intersection, Japanese and Korean entries
and badges, male/female and shiny comparisons, Unown's 56 recorded appearances, and all 57 habitat records.
Unown's letter labels distinguish the original form-name messages, which all say "One form".

Compare a live screenshot with the reference at the same size after every visual change. Check Entry, Area, Forms, search and row selection. Keep only one visible game process running. SVG provides consistent geometry, but proportions, font metrics and compositing still require screenshot verification.
