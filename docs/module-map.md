# Module map

Where each overlay's source files go. The overlay a file belongs to stays in its `delinks.txt` under `config/`; this
map is only the source layout. Overlays with a `/` split their files across more than one directory.

`src/` mirrors `include/` (`app/`, `battle/`, `field/`, `save/`, `system/`, `gfl/`, `pml/`, `demo/`), plus the new
modules the overlays add: `communication/`, `wifi/`, `moveeffect/`, `pokewood/`, `musical/`, `entralink/`,
`tournament/`.

## app/ — application UI, one feature subdirectory each
- `bag/` — ov142
- `battle_recorder/` — ov201, ov268, ov269, ov270, ov271, ov272, ov273, ov306
- `box/` — ov255
- `c_gear/` — ov079, ov080, ov088, ov200
- `codein/` — ov205
- `dowsing/` — ov086
- `hall_of_fame_pc/` — ov256
- `key_system/` — ov332
- `mail/` — ov215
- `manual/` — ov319
- `medalbox/` — ov145, ov187
- `monolith/` — ov143
- `mystery_gift/` — ov197
- `name_input/` — ov280, ov281
- `pokedex/` — ov296, ov297, ov298, ov299, ov300, ov301, ov302, ov303, ov304, ov313, ov314, ov315
- `pokelist/` — ov165
- `p_status/` — ov020, ov207
- `record/` — ov087
- `research_radar/` — ov018, ov310
- `spin_trade/` — ov216
- `townmap/` — ov144
- `trainer_card/` — ov186
- `move_tutor/` — ov258
- `ui/` — ov139, ov184

## battle/ — engine and battle UI direct; specialized in subdirectories
- (direct) — ov166, ov167, ov169, ov171, ov172, ov213, ov285, ov286, ov287, ov288, ov289
- `graphic/` — ov168's `btlv_bg.c`, `btlv_camera.c`, `btlv_clact.c`, `btlv_stage.c`, `btlv_field.c`,
  `btlv_finger_cursor.c`, `btlv_b_gauge.c`, `btlv_gauge.c`, `btlv_timer.c`, `btlv_input.c`, `btlv_mcss.c`
- `ai/` — ov170

## communication/ — link and trade features (direct)
- union/colosseum — ov028, ov034, ov069, ov082
- irc — ov174, ov175, ov176, ov177, ov178, ov179, ov180, ov338
- gts — ov001, ov195
- worldtrade — ov004, ov214
- pokemon_trade — ov016, ov192, ov193, ov194
- xtransceiver — ov017, ov198, ov253, ov257, ov259

## demo/ — intros, credits and cutscenes, one subdirectory each
- `intro/` — ov294
- `title/` — ov162, ov279
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
- ov021, ov024, ov025, ov026, ov027, ov071, ov072, ov073, ov085, ov130, ov157, ov309

## field/ — the field system direct; commands and effects in subdirectories
- (direct) — ov010, ov012 (field files), ov033 (field files), ov035, ov036 (field files), ov074, ov075, ov076,
  ov077, ov078, ov089, ov090, ov091, ov092, ov093, ov094, ov095, ov096, ov097, ov098, ov099, ov100, ov101, ov102,
  ov103, ov104, ov105, ov106, ov107, ov108, ov109, ov110, ov111, ov112, ov113, ov114, ov115, ov116, ov117, ov118,
  ov119, ov120, ov121, ov122, ov123, ov124, ov125, ov126, ov127, ov128, ov129, ov131, ov132, ov133, ov136, ov155,
  ov013, ov083, ov084, ov308, ov137, ov014, ov156, ov328
- `eventcmd/` — ov050, ov051, ov052, ov053, ov054, ov055, ov056, ov057, ov058, ov059, ov060, ov061, ov062, ov063,
  ov064, ov065, ov066, ov067, ov068 (scrcmd_*), and the encounter cut-ins ov146, ov147, ov148, ov149, ov150, ov151,
  ov152, ov153, ov154

## moveeffect/ — the move-effect VM
- ov168's `btlv_effect.c`, `btlv_effvm.c`

## musical/ — the Pokémon Musical
- ov012's `musical_event.c`, `musical_dressup_sys.c`, `musical_stage_sys.c`, `musical_program.c`
- ov209, ov210, ov211

## pokewood/ — Pokéstar Studios
- ov023, ov062, ov321, ov322, ov323, ov324, ov325

## save/
- ov331

## tournament/ — the World Tournament and Battle Subway
- ov005, ov009, ov022, ov055, ov081, ov134, ov135, ov141, ov191, ov317, ov320, ov326, ov327
- ov033's `bsubway_scr.c`, `bsubway_trainers.c`

## wifi/ — wireless features and device drivers
- `battle_match/` — ov002, ov173, ov260, ov261, ov262, ov263, ov290
- `note/` — ov204
- `login/` — ov190
- `2dmap/` — ov203
- `gsync/` — ov000, ov199
- `device/` — ov011, ov029, ov030, ov031, ov182, ov189
- `net/` — ov032, ov164, ov196

## gfl/, system/, pml/ — unchanged (ARM9 main)

## Unclassified (81 empty overlays, no source yet)
ov006, ov007, ov019, ov037–ov049, ov138, ov158–ov161, ov163, ov183, ov188, ov202, ov206, ov212, ov217–ov252, ov254,
ov274–ov278, ov282, ov283, ov291, ov292, ov329, ov333–ov337, ov339–ov343
