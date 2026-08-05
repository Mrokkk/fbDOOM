cmake_minimum_required(VERSION 3.14)

project(doom C)

include(GNUInstallDirs)
include(cmake/build_types.cmake)

option(DEBUG_INFO  "Enable debug info"             ${DEF_DEBUG_INFO})
option(OPTIMIZE    "Enable compiler optimizations" ${DEF_OPTIMIZE})
option(LTO         "Enable Link Time Optimization" ${DEF_LTO})
option(SANITIZE    "Enable ASan and UBsan"         OFF)
option(PROFILE     "Enable profiling"              OFF)

if(NOT BACKEND)
    set(BACKEND "Unix" CACHE STRING "Used platform")
endif()
set(DOOM_DATA_DIR "${CMAKE_INSTALL_FULL_DATADIR}/doom" CACHE STRING "DOOM data dir used by game")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)

add_definitions(
    -DDOOM_DATA_DIR=\"${DOOM_DATA_DIR}\"
)

set(CMAKE_C_STANDARD 90)
add_compile_options(-pipe -Wall -Wextra -fdata-sections -ffunction-sections -Wno-unused-parameter)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_link_options(-Wl,--gc-sections)
elseif(CMAKE_C_COMPILER_ID STREQUAL "TinyCC")
    add_link_options(-Wl,-lm)
endif()

if(DEBUG_INFO)
    add_compile_options(-ggdb3)
endif()

if(PROFILE)
    add_compile_options(-fno-omit-frame-pointer)
endif()

if(SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-Wl,-lasan -Wl,-lubsan)
endif()

if(OPTIMIZE)
    add_compile_options(-O3)
endif()

if(LTO)
    add_compile_options(-flto=auto)
endif()

include_directories(src)

add_executable(${PROJECT_NAME}

    src/am_map.c
    src/d_event.c
    src/d_items.c
    src/d_iwad.c
    src/d_loop.c
    src/d_main.c
    src/d_mode.c
    src/d_net.c
    src/doomdef.c
    src/doomstat.c
    src/dstrings.c
    src/dummy.c
    src/f_finale.c
    src/f_wipe.c
    src/g_game.c
    src/hu_lib.c
    src/hu_stuff.c
    src/i_cdmus.c
    src/i_endoom.c
    src/i_input.c
    src/i_joystick.c
    src/i_main.c
    src/i_sound.c
    src/i_system.c
    src/i_timer.c
    src/i_video.c
    src/info.c
    src/m_argv.c
    src/m_bbox.c
    src/m_cheat.c
    src/m_config.c
    src/m_controls.c
    src/m_fixed.c
    src/m_menu.c
    src/m_misc.c
    src/m_random.c
    src/memio.c
    src/p_ceilng.c
    src/p_doors.c
    src/p_enemy.c
    src/p_floor.c
    src/p_inter.c
    src/p_lights.c
    src/p_map.c
    src/p_maputl.c
    src/p_mobj.c
    src/p_plats.c
    src/p_pspr.c
    src/p_saveg.c
    src/p_setup.c
    src/p_sight.c
    src/p_spec.c
    src/p_switch.c
    src/p_telept.c
    src/p_tick.c
    src/p_user.c
    src/r_bsp.c
    src/r_data.c
    src/r_draw.c
    src/r_main.c
    src/r_plane.c
    src/r_segs.c
    src/r_sky.c
    src/r_things.c
    src/s_sound.c
    src/sha1.c
    src/sounds.c
    src/st_lib.c
    src/st_stuff.c
    src/statdump.c
    src/tables.c
    src/v_video.c
    src/w_checksum.c
    src/w_file.c
    src/w_file_posix.c
    src/w_file_stdc_unbuffered.c
    src/w_main.c
    src/w_wad.c
    src/wi_stuff.c
    src/z_zone.c

)

set(SOUND_AVAILABLE OFF CACHE INTERNAL "")

function(link_to_doom target)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${target})
endfunction()

function(enable_sound)
    set(SOUND_AVAILABLE ON CACHE INTERNAL "")
    add_compile_definitions(-DFEATURE_SOUND=1)
    target_sources(${PROJECT_NAME} PRIVATE src/mus2mid/mus2mid.c)

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_SOURCE_DIR}/soundfont/gzdoom.sf2
            ${CMAKE_BINARY_DIR}/gzdoom.sf2
    )

    install(FILES "${CMAKE_SOURCE_DIR}/soundfont/gzdoom.sf2" DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/doom)
endfunction()

include(cmake/platform_${BACKEND}.cmake)

message(STATUS "Build type:                    ${CMAKE_BUILD_TYPE}")
message(STATUS "Enable debug info:             ${DEBUG_INFO}")
message(STATUS "Enable compiler optimizations: ${OPTIMIZE}")
message(STATUS "Enable Link Time Optimization: ${LTO}")
message(STATUS "Enable ASan and UBsan:         ${SANITIZE}")
message(STATUS "Enable profiling:              ${PROFILE}")
message(STATUS "Platform:                      ${BACKEND}")
message(STATUS "Sound support available:       ${SOUND_AVAILABLE}")
message(STATUS "Install binary dir:            ${CMAKE_INSTALL_FULL_BINDIR}")
message(STATUS "Install data dir:              ${CMAKE_INSTALL_FULL_DATADIR}")
message(STATUS "DOOM data dir:                 ${DOOM_DATA_DIR}")

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
install(FILES "${CMAKE_SOURCE_DIR}/default.cfg" DESTINATION ${CMAKE_INSTALL_FULL_DATADIR}/doom)
