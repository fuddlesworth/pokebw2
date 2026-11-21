# Module map

Where each overlay's source files go. The overlay a file belongs to stays in its `delinks.txt` under `config/`; this
map is only the source layout. Overlays with a `/` split their files across more than one directory.

`src/` mirrors `include/`: every `src/<module>/<subdir>/` has a matching `include/<module>/<subdir>/`. The library and
header modules — `gfl/`, `pml/`, `save/`, `constants/`, `asm/` and the root headers (`types.h`, `struct_decls.h`) — are
organized by concern rather than mirroring a `src/` directory, so their headers stay flat.

## app/ — application UI, one feature subdirectory each
- `bag/` — ov142
- `battle_recorder/` — ov201, ov268, ov269, ov270, ov271, ov272, ov273, ov306
- `box/` — ov255
- `c_gear/` — ov079, ov080, ov088, ov200
- `codein/` — ov205
- `config/` — the trainer-card config panel
- `dowsing/` — ov086
- `drawing/` — the Drawing Board
- `dup_fitting/` — the duplicate-furniture feature
- `geonet/` — the Geonet
- `hall_of_fame_pc/` — ov256
- `mail/` — ov215
- `manual/` — ov319
- `marine_tube/` — the Marine Tube ride
- `mb_parent/` — the DS Download Play host of Unova Link's Memory Link (was `src/ov181`)
- `medalbox/` — ov145, ov187
- `monolith/` — ov143
- `move_tutor/` — ov258
- `mystery_gift/` — ov197
- `name_input/` — ov280, ov281
- `pms/` — the Pokémon Musical's name input
- `pokedex/` — ov296–ov304, ov313, ov314, ov315
- `pokelist/` — ov165
- `pokemon_select/` — the generic Pokémon selection screen
- `p_status/` — ov020, ov207
- `report/` — ov087 (the "report", i.e. the save-the-game screen)
- `research_radar/` — ov018, ov310
- `spin_trade/` — ov216
- `townmap/` — ov144
- `trainer_card/` — ov186
- `ui/` — ov139, ov184
- `unity_tower/` — ov311 (the Unity Tower floor select)
- `unova_link/` — ov332 (the Key System and Memory Link, was `key_system/`)

## battle/ — engine and battle UI direct; specialized in subdirectories
- (direct) — ov166, ov167, ov169, ov171, ov172, ov213, ov285, ov286, ov287, ov288, ov289, plus the field battle
  instance (`fld_btl_inst_*`), the PDC battle-return flow and the Pokéstar cutin
- `graphic/` — ov168's `btlv_bg.c`, `btlv_camera.c`, `btlv_clact.c`, `btlv_stage.c`, `btlv_field.c`,
  `btlv_finger_cursor.c`, `btlv_b_gauge.c`, `btlv_gauge.c`, `btlv_timer.c`, `btlv_input.c`, `btlv_mcss.c`
- `ai/` — ov170
- `select/`, `handler/`, `record/` — the battle engine's pickers, ability/move handlers and record keeping

## communication/ — link and trade features (direct entry/delivery files at the top)
- (direct) — `comm_entry_menu.c`, `comm_player.c`, `delivery_beacon.c`, `delivery_irc.c`, `fld_trade.c`
- `union/` — ov028, ov034, ov069, ov082
- `irc/` — ov174, ov175, ov176, ov177, ov178, ov179, ov180, ov338
- `gts/` — ov001, ov195
- `worldtrade/` — ov004, ov214
- `pokemon_trade/` — ov016, ov192, ov193, ov194
- `xtransceiver/` — ov017, ov198, ov253, ov257, ov259

## demo/ — intros, credits and cutscenes, one subdirectory each
- `intro/` — ov294
- `title/` — ov162, ov279 (title screen, start menu, delete-save, boot screens)
- `evolution/` — ov284
- `hall_of_fame/` — ov265
- `ending/` — ov266, ov267
- `demo3d/` — ov293
- `cdemo/` — ov264
- `egg/` — ov307
- `comm_battle/` — ov305

## entralink/ — the Entralink, Entree Forest and Funfest Missions
- ov033's `entree_forest.c`, `entree_forest_first_warp.c`, `entralink_warp.c`, `event_funfest_mission.c`,
  `scrcmd_funfest.c`, `pdw_postman.c`
- ov021, ov024, ov025, ov026, ov027, ov071, ov072, ov073, ov085, ov130, ov157, ov309, plus the field-side Funfest and
  Hidden Hollow files and the `festival.h` / `festival_app.h` pair

## field/ — the field system direct; commands and effects in subdirectories
- (direct) — the core field system: `field.c`, `field_task.c`, `sound_obj.c` and their shared headers
- `actor/` — field actors (NPCs), the follower and the field-communication actors
- `beacon/` — the beacon view and beacon search
- `camera/` — the field camera and its scene areas
- `comm/` — the field multiplayer (other players' actors, WFBC)
- `effect/` — field-move and field visual effects (`fldeff_*`, `field_effect`, fades and transitions)
- `encounter/` — wild/trainer encounter spawning and the encounter cut-ins (`encounter_effect_*`, `battle_cutin_db`)
- `event/` — field events (`event_*`) and the field facilities (Day Care, Trial House, cruise, hidden items, forms)
- `gimmick/` — location gimmicks (gyms, bridges, frigates, the league)
- `map/` — the field map, chunks, matrix, props, symbols and zones
- `menu/` — the field menu, subscreen and message windows
- `pass_power/` — Pass Powers
- `player/` — the player character and its movement
- `rail/` — the rail/ledge movement system
- `resort/` — the Resort (Join Avenue)
- `script/` — the script VM and the general `scrcmd_*` commands
- `script_facility/` — the facility-specific `scrcmd_*` commands (Battle Subway, World Tournament, league, ruins)
- `weather/` — weather, lighting, fog and the lens flare

The overlays stay ov010–ov136, ov155, ov156, ov308, ov013, ov083, ov084, ov328 (field) and ov050–ov068 (facility
scripts) and ov146–ov154 (encounter cut-ins), split across the directories above.

## moveeffect/ — the move-effect VM
- ov168's `btlv_effect.c`, `btlv_effvm.c`

## musical/ — the Pokémon Musical
- ov012's `musical_event.c`, `musical_dressup_sys.c`, `musical_stage_sys.c`, `musical_program.c`
- ov209, ov210, ov211

## pokewood/ — Pokéstar Studios
- ov023, ov062, ov321, ov322, ov323, ov324, ov325

## save/ — the save overlay and the save-data structures
- `save_outside.c` — ov331
- `include/save/` holds the save-data structure headers (`records.h`, `player_info.h`, `box.h`, …). The serialization
  declarations (`*_savedata.h`, `save_control.h`, `save_table.h`) live beside their implementations in `system/savedata/`.

## system/ — the ARM9 main executable
- (direct) — the game loop, field event/sound/status systems, UI widgets, Pokémon data helpers, net and sound
- `savedata/` — the per-block save/load serialization (`*_savedata.c` and their `*_savedata.h`, `save_control.c`,
  `save_table.c`, `save_error.c`)
- `iss/` — the interactive sound system

## tournament/ — the World Tournament and Battle Subway
- ov005, ov009, ov022, ov055, ov081, ov134, ov135, ov141, ov191, ov317, ov320, ov326, ov327
- ov033's `bsubway_scr.c`, `bsubway_trainers.c`, and the World Tournament stadium files from `field/`

## wifi/ — wireless features and device drivers
- `battle_match/` — ov002, ov173, ov260, ov261, ov262, ov263, ov290, plus the Battle Championship and Live Battlematch
- `note/` — ov204
- `login/` — ov190
- `2dmap/` — ov203
- `gsync/` — ov000, ov199
- `device/` — ov011, ov029, ov030, ov031, ov182, ov189
- `net/` — ov032, ov164, ov196
- (direct) — the Wi-Fi Club (`wificlub.h`, `event_wificlub.c`)

## gfl/, pml/ — libraries
- `gfl/` is the Game Freak library (graphics, sound, heap, net, procs).
- `pml/` is the Pokémon library: `src/pml/` holds the pure library implementations (`hm_check.c`, `item.c`, `met_data.c`,
  `species_names.c`) and `include/pml/` the API headers. The Pokémon data and logic it wraps (`poke_personal.c`,
  `pokegra.c`, `pokeparty.c`, `move_tool.c`, …) stays in `system/`, where the core game data lives.

## Unclassified (81 empty overlays, no source yet)
ov006, ov007, ov019, ov037–ov049, ov138, ov158–ov161, ov163, ov183, ov188, ov202, ov206, ov212, ov217–ov252, ov254,
ov274–ov278, ov282, ov283, ov291, ov292, ov329, ov333–ov337, ov339–ov343
