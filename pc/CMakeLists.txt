cmake_minimum_required(VERSION 3.21)
project(pokebw2_pc VERSION 0.1.0 LANGUAGES C)
find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
file(GLOB_RECURSE PC_HEADER_INPUTS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../generated/*.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/../res/text/*.txt
    ${CMAKE_CURRENT_SOURCE_DIR}/../res/*.json)
file(GLOB PC_GENERATOR_SCRIPTS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../tools/text/*.py
    ${CMAKE_CURRENT_SOURCE_DIR}/../tools/data/*.py)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${PC_HEADER_INPUTS}
    ${PC_GENERATOR_SCRIPTS}
    ${CMAKE_CURRENT_SOURCE_DIR}/../configure.py
    ${CMAKE_CURRENT_SOURCE_DIR}/../tools/build/rom_blob.py
    ${CMAKE_CURRENT_SOURCE_DIR}/../tools/decomp/compile_all.py
    ${CMAKE_CURRENT_SOURCE_DIR}/../tools/data/gen_constants.py
    ${CMAKE_CURRENT_SOURCE_DIR}/../tools/text/pms_words.py
    ${CMAKE_CURRENT_SOURCE_DIR}/../tools/text/text_ids.py)
execute_process(
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/decomp/compile_all.py --generate-headers-only
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    RESULT_VARIABLE PC_HEADER_GENERATION_RESULT)
if(NOT PC_HEADER_GENERATION_RESULT EQUAL 0)
    message(FATAL_ERROR "Could not generate headers required by the PC build")
endif()
if(POLICY CMP0205)
    cmake_policy(SET CMP0205 NEW)
endif()

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "The BW2 PC port requires a 64-bit toolchain (x86-64 or ARM64).")
endif()

find_package(SDL3 3.2 CONFIG REQUIRED)
find_package(Freetype REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(RSVG REQUIRED IMPORTED_TARGET librsvg-2.0>=2.52)
foreach(font PixelOperator.ttf PixelOperator-Bold.ttf PixelOperator8-Bold.ttf PixelOperatorHB8.ttf CC0-PixelOperator.txt)
    configure_file(assets/fonts/${font} assets/fonts/${font} COPYONLY)
endforeach()
find_program(GLSLC glslc REQUIRED)
foreach(stage vert frag)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/skin_${stage}.h
        COMMAND ${GLSLC} -mfmt=c ${CMAKE_CURRENT_SOURCE_DIR}/shaders/skin.${stage}
            -o ${CMAKE_CURRENT_BINARY_DIR}/skin_${stage}.h
        DEPENDS shaders/skin.${stage} VERBATIM)
    add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/gx_${stage}.h
        COMMAND ${GLSLC} -mfmt=c ${CMAKE_CURRENT_SOURCE_DIR}/shaders/gx.${stage}
            -o ${CMAKE_CURRENT_BINARY_DIR}/gx_${stage}.h
        DEPENDS shaders/gx.${stage} VERBATIM)
endforeach()

# Compile the recovered renderer directly. The native type vocabulary is selected before SDK headers.
file(GLOB NNS_G3D_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/nnsys/src/g3d/*.c)
file(GLOB NNS_G2D_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/nnsys/src/g2d/*.c)
file(GLOB NNS_GFD_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/../lib/nnsys/src/gfd/*.c)
add_library(bw2_g3d STATIC ${NNS_G3D_SOURCES} ${NNS_G2D_SOURCES} ${NNS_GFD_SOURCES})
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/native-include)
foreach(header gfl system app constants field save pml battle demo struct_decls.h)
    file(CREATE_LINK ${CMAKE_CURRENT_SOURCE_DIR}/../include/${header}
        ${CMAKE_CURRENT_BINARY_DIR}/native-include/${header} SYMBOLIC COPY_ON_ERROR)
endforeach()
target_include_directories(bw2_g3d SYSTEM PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/native-include)
target_include_directories(bw2_g3d SYSTEM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../build/include/generated)
target_sources(bw2_g3d PRIVATE
    ../lib/nitro/src/fx/fx.c ../lib/nitro/src/fx/fx_cp.c
    ../lib/nitro/src/fx/fx_mtx22.c
    ../lib/nitro/src/fx/fx_mtx33.c ../lib/nitro/src/fx/fx_mtx43.c
    ../lib/nitro/src/fx/fx_mtx44.c ../lib/nitro/src/fx/fx_vec.c ../lib/nitro/src/fx/fx_sincos.c
    ../lib/nitro/src/gx/g3imm.c ../lib/nitro/src/gx/gx_bgcnt.c ../lib/nitro/src/gx/g2.c ../lib/nitro/src/gx/g3x.c
    ../lib/nitro/src/gx/gx.c
    ../src/gfl/arc_tool.c ../src/gfl/bg_sys.c ../src/gfl/areaman.c ../src/gfl/arc_util.c ../src/gfl/net.c
    ../src/gfl/calctool.c
    ../src/system/gf_font.c ../src/gfl/bmp.c ../src/gfl/bmp_win.c ../src/gfl/strbuf.c ../src/system/printsys.c
    ../src/gfl/msgdata.c
    ../src/gfl/g3d_system.c ../src/gfl/g3d_util.c ../src/gfl/g3d_camera.c ../src/gfl/g3d_light.c
    ../src/gfl/procsys.c ../src/gfl/fade.c ../src/gfl/gx_layers.c ../src/ov162/boot_screens.c
    ../src/gfl/tcb.c ../src/gfl/tcbl.c
    ../src/system/wipe.c ../src/system/wipe_sub.c ../src/system/wipe_wnd.c
    ../src/ov264/cdemo_main.c ../src/ov264/cdemo_script.c ../src/ov264/cdemo_control.c
    ../src/ov162/title.c ../src/gfl/clact.c ../src/gfl/clact_itcm.c ../src/system/ica_anime.c
    ../src/ov162/startmenu.c ../src/ov162/game_start.c ../src/ov162/delete_save.c ../src/ov253/mictest.c
    ../src/system/version.c
    ../src/gfl/vman.c ../src/gfl/net_handle.c
    ../src/system/snd_strm.c ../src/system/pm_voice.c ../src/system/perapvoice.c
    ../lib/nnsys/src/snd/snd_arc.c
    ../src/gfl/savedata.c ../src/gfl/backup_system.c ../src/gfl/backup_card.c
    ../src/gfl/gf_standard.c ../lib/nitro/src/math/math_crc.c
    sdk/cp.c sdk/memory.c sdk/gx.c sdk/bg.c sdk/obj.c sdk/fs.c sdk/g2d_resources.c sdk/display.c sdk/card.c sdk/power.c sdk/mic.c
    sdk/rtc.c sdk/wireless.c sdk/math.c)
target_link_libraries(bw2_g3d PUBLIC SDL3::SDL3)
include(game_sources.cmake)
if(NOT WIN32)
    target_link_libraries(bw2_g3d PUBLIC m)
endif()
target_include_directories(bw2_g3d PUBLIC include)
# The recovered SDK retains its original compiler extensions (anonymous unions and zero-length arrays).
target_include_directories(bw2_g3d SYSTEM PUBLIC
    ../lib/nitro/include ../lib/twl/include ../lib/nnsys/include ../lib/dwc/include ../lib/nitrowifi/include
    ../lib/spl/include ../lib/dsprot/include ../lib/irc/include)
target_compile_definitions(bw2_g3d PUBLIC POKEBW2_PC=1 BLACK2=1)
# Native resource allocation uses the documented fixes without changing the ROM build.
set_source_files_properties(../src/gfl/g3d_system.c ../src/gfl/arc_util.c ../src/ov012/field_actor.c
    ../src/ov284/evolution_demo.c
    ../src/ov165/plist_sys.c
    ../src/ov036/scrcmd_field_actor.c ../src/ov055/wbt_party.c ../src/ov209/musical_mcss.c
    ../src/ov148/encounter_effect_capture.c ../src/system/screentex.c
    ../src/ov168/btlv_mcss.c
    ../src/ov167/btlv_scu.c ../src/ov168/btlv_effvm.c
    ../src/ov209/sta_acting.c
    ../src/ov055/wbt_tool.c
    ../src/ov086/dowsing.c
    ../src/ov082/union_subdisp.c
    ../src/ov104/field_gimmick_gate.c
    ../src/ov036/field_map_chunk_rd.c
    ../src/system/fest_mission_beacon.c
    ../src/ov012/eventdata_system.c
    PROPERTIES COMPILE_DEFINITIONS BUGFIX=1)
set_target_properties(bw2_g3d PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
if(MSVC)
    target_compile_options(bw2_g3d PRIVATE /W3 /WX)
else()
    target_compile_options(bw2_g3d PRIVATE -Wall -Werror -ffunction-sections -fdata-sections)
endif()

function(pc_dead_strip target)
    if(APPLE)
        target_link_options(${target} PRIVATE -Wl,-dead_strip)
    elseif(NOT MSVC)
        target_link_options(${target} PRIVATE -Wl,--gc-sections)
    endif()
endfunction()

add_library(pc_layout STATIC layout.c)
include(sequence.cmake)
target_include_directories(pc_layout PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
add_executable(pokebw2-pc main.c video.c gx_video.c boot.c dex_ui.c dex_media.c dex_skin.c ui_draw.c ui_theme.c ui_widgets.c ui_frame.c ui_components.c ui_input.c ui_layout.c ui_skin.c ui_assets.c party_ui.c party_view.c menu_ui.c menu_apps.c menu_bag.c menu_summary.c menu_mail.c menu_phrase.c menu_map.c bag_view.c trainer_ui.c save_ui.c settings.c settings_ui.c battle_ui.c battle_menu.c audio.c save.c timing.c
    ${CMAKE_CURRENT_BINARY_DIR}/skin_vert.h ${CMAKE_CURRENT_BINARY_DIR}/skin_frag.h
    ${CMAKE_CURRENT_BINARY_DIR}/gx_vert.h ${CMAKE_CURRENT_BINARY_DIR}/gx_frag.h)
target_include_directories(pokebw2-pc PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
foreach(asset header center information-card description-card stats footer row selection tabs top-junction header-corner filter-row form-controls)
    configure_file(assets/dex/${asset}.svg assets/dex/${asset}.svg COPYONLY)
endforeach()
foreach(asset header row heading gender hp stats moves)
    configure_file(assets/party/${asset}.svg assets/party/${asset}.svg COPYONLY)
endforeach()
foreach(asset settings settings-selection profile footer bag summary mail trainer signature-pad save save-progress tab row light-row button marking trainer-badge greeting-row)
    configure_file(assets/menu/${asset}.svg assets/menu/${asset}.svg COPYONLY)
endforeach()
foreach(asset panel enemy-hp player-hp move action icons cursor notice keys)
    configure_file(assets/battle/${asset}.svg assets/battle/${asset}.svg COPYONLY)
endforeach()
target_link_libraries(pokebw2-pc PRIVATE pc_layout bw2_g3d pc_sequence SDL3::SDL3 Freetype::Freetype PkgConfig::RSVG)
pc_dead_strip(pokebw2-pc)

function(pc_warnings target)
    set_target_properties(${target} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /WX)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()
endfunction()
pc_warnings(pc_layout)
pc_warnings(pokebw2-pc)

include(CTest)
option(PC_GPU_TESTS "Run tests that require a display and Vulkan device" OFF)
set(PC_GAME_DATA "" CACHE PATH "Extracted Black 2 files for original-resource rendering tests")
if(BUILD_TESTING)
    add_executable(pc-sort-test tests/sort_test.c)
    target_link_libraries(pc-sort-test PRIVATE bw2_g3d)
    pc_warnings(pc-sort-test)
    add_test(NAME pc-sdk-sort COMMAND pc-sort-test)
    set_tests_properties(pc-sdk-sort PROPERTIES LABELS "cpu")
    add_executable(pc-timing-test tests/timing_test.c timing.c)
    target_include_directories(pc-timing-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    pc_warnings(pc-timing-test)
    add_test(NAME pc-fixed-timing COMMAND pc-timing-test)
    add_executable(pc-layout-test tests/layout_test.c)
    target_link_libraries(pc-layout-test PRIVATE pc_layout)
    pc_warnings(pc-layout-test)
    add_test(NAME pc-layout COMMAND pc-layout-test)
    add_executable(pc-abi-test tests/abi_test.c)
    target_link_libraries(pc-abi-test PRIVATE bw2_g3d)
    pc_warnings(pc-abi-test)
    add_test(NAME pc-64bit-abi COMMAND pc-abi-test)
    add_executable(pc-renderer-test tests/renderer_test.c)
    target_link_libraries(pc-renderer-test PRIVATE bw2_g3d)
    pc_dead_strip(pc-renderer-test)
    pc_warnings(pc-renderer-test)
    add_test(NAME pc-original-renderer COMMAND pc-renderer-test)
    add_executable(pc-ui-controls-test tests/ui_controls_test.c ui_layout.c)
    target_include_directories(pc-ui-controls-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    pc_warnings(pc-ui-controls-test)
    add_test(NAME pc-ui-controls COMMAND pc-ui-controls-test)
    add_executable(pc-settings-test tests/settings_test.c settings.c)
    target_link_libraries(pc-settings-test PRIVATE bw2_g3d pc_layout)
    pc_warnings(pc-settings-test)
    add_test(NAME pc-settings COMMAND pc-settings-test)
    set_tests_properties(pc-settings PROPERTIES LABELS "cpu")
    add_executable(pc-ui-input-test tests/ui_input_test.c ui_input.c settings.c)
    target_link_libraries(pc-ui-input-test PRIVATE bw2_g3d pc_layout)
    pc_dead_strip(pc-settings-test)
    pc_dead_strip(pc-ui-input-test)
    pc_warnings(pc-ui-input-test)
    add_test(NAME pc-ui-input COMMAND pc-ui-input-test)
    add_executable(pc-host-cleanup-test tests/host_cleanup_test.c)
    target_link_libraries(pc-host-cleanup-test PRIVATE bw2_g3d)
    pc_dead_strip(pc-host-cleanup-test)
    pc_warnings(pc-host-cleanup-test)
    add_test(NAME pc-host-cleanup COMMAND pc-host-cleanup-test "${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt")
    set_tests_properties(pc-host-cleanup PROPERTIES LABELS "cpu")
    set_tests_properties(pc-ui-input PROPERTIES LABELS "cpu")
    add_executable(pc-ui-text-test tests/ui_text_test.c ui_draw.c ui_theme.c ui_widgets.c)
    target_link_libraries(pc-ui-text-test PRIVATE bw2_g3d Freetype::Freetype)
    pc_dead_strip(pc-ui-text-test)
    pc_warnings(pc-ui-text-test)
    add_test(NAME pc-ui-text COMMAND pc-ui-text-test "${CMAKE_CURRENT_SOURCE_DIR}/")
    set_tests_properties(pc-ui-controls pc-ui-text PROPERTIES LABELS "cpu")
    add_executable(pc-native-transform-test tests/native_transform_test.c)
    target_link_libraries(pc-native-transform-test PRIVATE bw2_g3d)
    pc_dead_strip(pc-native-transform-test)
    pc_warnings(pc-native-transform-test)
    add_test(NAME pc-native-transform COMMAND pc-native-transform-test)
    set_tests_properties(pc-native-transform PROPERTIES LABELS "cpu")
    add_executable(pc-backup-test tests/backup_test.c)
    target_link_libraries(pc-backup-test PRIVATE bw2_g3d)
    pc_dead_strip(pc-backup-test)
    pc_warnings(pc-backup-test)
    add_test(NAME pc-backup COMMAND pc-backup-test)
    add_test(NAME pc-help COMMAND pokebw2-pc --help)
    add_test(NAME pc-invalid-frames COMMAND pokebw2-pc --frames -1)
    set_tests_properties(pc-invalid-frames PROPERTIES WILL_FAIL TRUE)
    set_tests_properties(pc-fixed-timing pc-layout pc-64bit-abi pc-original-renderer pc-backup pc-help pc-invalid-frames
        PROPERTIES LABELS "cpu")
    if(PC_GPU_TESTS)
        add_test(NAME pc-vulkan COMMAND pokebw2-pc --frames 12 --resize-test)
        set_tests_properties(pc-vulkan PROPERTIES TIMEOUT 30)
        add_test(NAME pc-model-renderer COMMAND pokebw2-pc
            --model ${CMAKE_CURRENT_SOURCE_DIR}/../res/graphics/psel/34.nsbmd --frames 12)
        add_test(NAME pc-animated-model-renderer COMMAND pokebw2-pc
            --model ${CMAKE_CURRENT_SOURCE_DIR}/../res/graphics/psel/34.nsbmd
            --animation ${CMAKE_CURRENT_SOURCE_DIR}/../res/graphics/psel/35.nsbca --frames 12)
        set_tests_properties(pc-model-renderer pc-animated-model-renderer PROPERTIES TIMEOUT 30)
        set_tests_properties(pc-vulkan pc-model-renderer pc-animated-model-renderer
            PROPERTIES LABELS "graphics" RESOURCE_LOCK pc_display)
        if(PC_GAME_DATA)
            if(NOT EXISTS "${PC_GAME_DATA}/a/1/6/8")
                message(FATAL_ERROR "PC_GAME_DATA must point to the extracted files directory")
            endif()
            add_test(NAME pc-intro-renderer COMMAND pokebw2-pc --data-root ${PC_GAME_DATA}
                --frames 60 --resize-test)
            set_tests_properties(pc-intro-renderer PROPERTIES TIMEOUT 60)
            add_test(NAME pc-title-sprites COMMAND pokebw2-pc --title-sprite --data-root ${PC_GAME_DATA}
                --frames 60)
            add_test(NAME pc-title-process COMMAND pokebw2-pc --title --data-root ${PC_GAME_DATA}
                --frames 120)
            set_tests_properties(pc-title-sprites pc-title-process PROPERTIES TIMEOUT 90)
            add_test(NAME pc-main-menu COMMAND pokebw2-pc --menu --data-root ${PC_GAME_DATA} --frames 120)
            set_tests_properties(pc-main-menu PROPERTIES TIMEOUT 90
                ENVIRONMENT "POKEBW2_SAVE_FILE=${CMAKE_CURRENT_BINARY_DIR}/menu-test.sav;SDL_AUDIODRIVER=dummy")
            add_test(NAME pc-new-game-intro COMMAND pokebw2-pc --new-game --data-root ${PC_GAME_DATA} --frames 300)
            set_tests_properties(pc-new-game-intro PROPERTIES TIMEOUT 90
                ENVIRONMENT "POKEBW2_SAVE_FILE=${CMAKE_CURRENT_BINARY_DIR}/intro-test.sav;SDL_AUDIODRIVER=dummy"
                PASS_REGULAR_EXPRESSION "Rendered sequence audio; peak")
            add_test(NAME pc-entry-movies COMMAND pokebw2-pc --boot --data-root ${PC_GAME_DATA} --frames 4000)
            set_tests_properties(pc-entry-movies PROPERTIES TIMEOUT 180
                ENVIRONMENT "POKEBW2_SAVE_FILE=${CMAKE_CURRENT_BINARY_DIR}/movies-test.sav;SDL_AUDIODRIVER=dummy"
                PASS_REGULAR_EXPRESSION "Starting original application TITLE_PROC_FUNCTIONS")
            set_tests_properties(pc-intro-renderer pc-title-sprites pc-title-process pc-main-menu
                pc-new-game-intro pc-entry-movies PROPERTIES LABELS "graphics" RESOURCE_LOCK pc_display)
        endif()
    endif()
endif()
