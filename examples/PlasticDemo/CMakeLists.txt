find_package(glfw3 CONFIG QUIET)
find_package(Threads REQUIRED)

if(TARGET glfw)
    set(PTGL_GLFW_TARGET glfw)
elseif(TARGET glfw3::glfw)
    set(PTGL_GLFW_TARGET glfw3::glfw)
else()
    find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h)
    find_library(GLFW_LIBRARY NAMES glfw3 glfw)
    if(NOT GLFW_INCLUDE_DIR OR NOT GLFW_LIBRARY)
        message(FATAL_ERROR "Install GLFW or set GLFW_INCLUDE_DIR and GLFW_LIBRARY")
    endif()
    add_library(ptgl_glfw UNKNOWN IMPORTED)
    set_target_properties(ptgl_glfw PROPERTIES
        IMPORTED_LOCATION "${GLFW_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${GLFW_INCLUDE_DIR}")
    set(PTGL_GLFW_TARGET ptgl_glfw)
endif()

add_executable(PlasticDemo main.cpp)
target_link_libraries(PlasticDemo PRIVATE ptgl ${PTGL_GLFW_TARGET} Threads::Threads)
# Use the existing desktop compatibility path in GLFWGraphicsDriver.
target_compile_definitions(PlasticDemo PRIVATE PTGL_DISABLE_GLES)
if(MSVC)
    target_compile_options(PlasticDemo PRIVATE /utf-8)
endif()
