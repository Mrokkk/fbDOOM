find_package(SDL2 REQUIRED)
find_package(SDL2_mixer)

add_library(platform OBJECT src/platform/sdl.c)

if(SDL2_mixer_FOUND)
    target_sources(platform PRIVATE src/platform/sdl_sound.c)
    target_link_libraries(platform PRIVATE SDL2 SDL2_mixer)
    enable_sound()
endif()

link_to_doom(platform)
