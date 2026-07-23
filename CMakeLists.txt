cmake_minimum_required(VERSION 3.10)
project(fbDOOM C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_FLAGS "-pipe -ggdb3 -O3 -Wall -Wextra")

add_executable(doom

    fbdoom/am_map.c
    fbdoom/d_event.c
    fbdoom/d_items.c
    fbdoom/d_iwad.c
    fbdoom/d_loop.c
    fbdoom/d_main.c
    fbdoom/d_mode.c
    fbdoom/d_net.c
    fbdoom/doomdef.c
    fbdoom/doomstat.c
    fbdoom/dstrings.c
    fbdoom/dummy.c
    fbdoom/f_finale.c
    fbdoom/f_wipe.c
    fbdoom/g_game.c
    fbdoom/hu_lib.c
    fbdoom/hu_stuff.c
    fbdoom/i_cdmus.c
    fbdoom/i_endoom.c
    fbdoom/i_input_tty.c
    fbdoom/i_joystick.c
    fbdoom/i_main.c
    fbdoom/i_scale.c
    fbdoom/i_sound.c
    fbdoom/i_system.c
    fbdoom/i_timer.c
    fbdoom/i_video_fbdev.c
    fbdoom/info.c
    fbdoom/m_argv.c
    fbdoom/m_bbox.c
    fbdoom/m_cheat.c
    fbdoom/m_config.c
    fbdoom/m_controls.c
    fbdoom/m_fixed.c
    fbdoom/m_menu.c
    fbdoom/m_misc.c
    fbdoom/m_random.c
    fbdoom/memio.c
    fbdoom/p_ceilng.c
    fbdoom/p_doors.c
    fbdoom/p_enemy.c
    fbdoom/p_floor.c
    fbdoom/p_inter.c
    fbdoom/p_lights.c
    fbdoom/p_map.c
    fbdoom/p_maputl.c
    fbdoom/p_mobj.c
    fbdoom/p_plats.c
    fbdoom/p_pspr.c
    fbdoom/p_saveg.c
    fbdoom/p_setup.c
    fbdoom/p_sight.c
    fbdoom/p_spec.c
    fbdoom/p_switch.c
    fbdoom/p_telept.c
    fbdoom/p_tick.c
    fbdoom/p_user.c
    fbdoom/r_bsp.c
    fbdoom/r_data.c
    fbdoom/r_draw.c
    fbdoom/r_main.c
    fbdoom/r_plane.c
    fbdoom/r_segs.c
    fbdoom/r_sky.c
    fbdoom/r_things.c
    fbdoom/s_sound.c
    fbdoom/sha1.c
    fbdoom/sounds.c
    fbdoom/st_lib.c
    fbdoom/st_stuff.c
    fbdoom/statdump.c
    fbdoom/tables.c
    fbdoom/v_video.c
    fbdoom/w_checksum.c
    fbdoom/w_file.c
    fbdoom/w_file_stdc_unbuffered.c
    fbdoom/w_main.c
    fbdoom/w_wad.c
    fbdoom/wi_stuff.c
    fbdoom/z_zone.c

)

install(TARGETS doom)
