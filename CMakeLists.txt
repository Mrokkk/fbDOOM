cmake_minimum_required(VERSION 3.10)
project(fbDOOM C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS "-pipe -ggdb3 -O3 -Wall -Wextra")

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
    src/i_input_tty.c
    src/i_joystick.c
    src/i_main.c
    src/i_scale.c
    src/i_sound.c
    src/i_system.c
    src/i_timer.c
    src/i_video_fbdev.c
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
    src/w_file_stdc_unbuffered.c
    src/w_main.c
    src/w_wad.c
    src/wi_stuff.c
    src/z_zone.c

)

install(TARGETS doom)
