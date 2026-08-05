include(FindPkgConfig)
pkg_check_modules(RAYLIB raylib)

add_library(platform OBJECT src/platform/raylib.c)

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

    target_link_libraries(platform PRIVATE raylib)
else()
    target_link_libraries(platform PRIVATE ${RAYLIB_LIBRARIES})
    target_link_directories(platform PRIVATE ${RAYLIB_LIBRARY_DIRS})
    target_include_directories(platform PRIVATE ${RAYLIB_INCLUDE_DIRS})
endif()

link_to_doom(platform)
