cmake_minimum_required(VERSION 3.13)
project(SimpleEmscriptenDemo CXX C)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "Configure this sample with emcmake cmake")
endif()

# Keep standalone builds working, while sharing the root library configuration.
if(NOT TARGET ptgl)
    set(PTGL_BUILD_EMSCRIPTEN_DEMO OFF CACHE BOOL "Build the simple browser sample" FORCE)
    add_subdirectory(../.. ptgl)
endif()

add_executable(SimpleEmscriptenDemo main.cpp)
target_link_libraries(SimpleEmscriptenDemo PRIVATE ptgl glut)
set_target_properties(SimpleEmscriptenDemo PROPERTIES SUFFIX ".html")
