cmake_minimum_required(VERSION 3.14)

project(fbDOOM C)

option(LTO         "Enable Link Time Optimization" OFF)
option(OPTIMIZE    "Enable compiler optimizations" ON)
option(SANITIZE    "Enable ASan and UBsan"         OFF)
option(PROFILE     "Enable profiling"              OFF)

set(BACKEND "Unix" CACHE STRING "Used platform")

message(STATUS "Enable Link Time Optimization: ${LTO}")
message(STATUS "Enable compiler optimizations: ${OPTIMIZE}")
message(STATUS "Enable ASan and UBsan:         ${SANITIZE}")
message(STATUS "Enable profiling:              ${PROFILE}")
message(STATUS "Platform:                      ${BACKEND}")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_BINARY_DIR}/compile_commands.json
        ${CMAKE_SOURCE_DIR}/compile_commands.json
)

set(CMAKE_C_STANDARD 90)
add_compile_options(-pipe -ggdb3 -Wall -Wextra -fdata-sections -ffunction-sections -Wno-unused-parameter)
add_link_options(-Wl,--gc-sections)

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

add_executable(doom

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

if(BACKEND STREQUAL "X11")

    find_package(X11)

    add_library(platform_x11 OBJECT src/platform/x11.c)
    target_include_directories(platform_x11 PRIVATE ${X11_INCLUDE_DIR})
    target_link_libraries(platform_x11 PUBLIC ${X11_LIBRARIES})

    target_link_libraries(doom PRIVATE platform_x11)

elseif(BACKEND STREQUAL "Raylib")

    include(FindPkgConfig)
    pkg_check_modules(RAYLIB raylib)

    add_library(platform_raylib OBJECT src/platform/raylib.c)
    target_compile_options(platform_raylib PRIVATE -std=c99)

    if(USE_OWN_RAYLIB OR NOT RAYLIB_FOUND)
        include(FetchContent)
        set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

        FetchContent_Declare(raylib
            SYSTEM
            GIT_REPOSITORY https://github.com/raysan5/raylib
            GIT_TAG dbc56a87da87d973a9c5baa4e7438a9d20121d28 # tag: 6.0
        )
        FetchContent_MakeAvailable(raylib)

        target_compile_definitions(raylib PUBLIC -DSUPPORT_SCREEN_CAPTURE=0)

        target_compile_options(raylib PRIVATE -O3 -ggdb3)
        target_compile_definitions(raylib PRIVATE
            -DSUPPORT_MODULE_RSHAPES=0
            -DSUPPORT_MODULE_RMODELS=0
            -DSUPPORT_CAMERA_SYSTEM=0
            -DSUPPORT_GESTURES_SYSTEM=0
            -DSUPPORT_MOUSE_GESTURES=0
            -DSUPPORT_MESH_GENERATION=0
            -DSUPPORT_FILEFORMAT_OBJ=0
            -DSUPPORT_FILEFORMAT_MTL=0
            -DSUPPORT_FILEFORMAT_IQM=0
            -DSUPPORT_FILEFORMAT_GLTF=0
            -DSUPPORT_FILEFORMAT_VOX=0
            -DSUPPORT_FILEFORMAT_M3D=0
        )

        target_link_libraries(platform_raylib PRIVATE raylib)
    else()
        target_link_libraries(platform_raylib PRIVATE ${RAYLIB_LIBRARIES})
        target_link_directories(platform_raylib PRIVATE ${RAYLIB_LIBRARY_DIRS})
        target_include_directories(platform_raylib PRIVATE ${RAYLIB_INCLUDE_DIRS})
    endif()

    target_link_libraries(doom PRIVATE platform_raylib)

elseif(BACKEND STREQUAL "Unix")

    add_library(platform_unix OBJECT src/platform/unix.c)
    target_link_libraries(doom PRIVATE platform_unix)

else()
    message(FATAL_ERROR "Unrecognized platform: ${BACKEND}")
endif()

install(TARGETS doom)
