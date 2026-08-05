find_package(X11)

add_library(platform OBJECT src/platform/x11.c)
target_include_directories(platform PRIVATE ${X11_INCLUDE_DIR})
target_link_libraries(platform PUBLIC ${X11_LIBRARIES})

link_to_doom(platform)
