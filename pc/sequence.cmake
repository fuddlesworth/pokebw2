# Reuse the reference port's C sequence interpreter and envelope engine.
# Only these three translation units are built; its SDL2/C++ simulator is not used.
include(FetchContent)
FetchContent_Declare(pc_libntr
    GIT_REPOSITORY https://github.com/cybervisi0n/libntr.git
    GIT_TAG 1555d911ac80c0190ac1c321f99f1323de8b3c09
    PATCH_COMMAND ${CMAKE_COMMAND} -DPC_LIBNTR_SOURCE_DIR=<SOURCE_DIR>
        -P ${CMAKE_CURRENT_LIST_DIR}/patches/apply-libntr-patch.cmake
    SOURCE_SUBDIR pc-unused-build)
FetchContent_MakeAvailable(pc_libntr)
add_library(pc_sequence_core STATIC
    ${pc_libntr_SOURCE_DIR}/libraries/sim7/src/snd/snd_seq.c
    ${pc_libntr_SOURCE_DIR}/libraries/sim7/src/snd/snd_exchannel.c
    ${pc_libntr_SOURCE_DIR}/libraries/snd/src/snd_util.c)
set_target_properties(pc_sequence_core PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
target_include_directories(pc_sequence_core SYSTEM PUBLIC ${pc_libntr_SOURCE_DIR}/include)
target_compile_definitions(pc_sequence_core PRIVATE SDK_FROM_TOOL SDK_PORT SDK_BUILD_LINUX __stdcall=)
add_library(pc_sequence STATIC sequence.c)
set_target_properties(pc_sequence PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
target_compile_definitions(pc_sequence PRIVATE SDK_FROM_TOOL SDK_PORT SDK_BUILD_LINUX __stdcall=)
target_link_libraries(pc_sequence PRIVATE pc_sequence_core SDL3::SDL3)
if(NOT MSVC)
    target_compile_options(pc_sequence PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
