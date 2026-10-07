cmake_minimum_required(VERSION 3.13)
project(ptgl C CXX)

option(BUILD_SHARED_LIBS "Build shared library" ON)
option(PTGL_BUILD_PLASTIC_DEMO "Build the plastic/CAD/legacy rendering comparison" OFF)
option(PTGL_BUILD_EMSCRIPTEN_DEMO "Build the simple browser sample" OFF)
option(PTGL_BUILD_THREADED_DEMO "Build the multiple-producer drawing sample" OFF)
option(PTGL_ENABLE_WEB_THREADS "Enable Emscripten pthreads (requires COOP/COEP headers)" OFF)
set(EIGEN_DIR /usr/include/eigen3 CACHE STRING "The directory of the Eigen library")

if(NOT EMSCRIPTEN)
    find_package(Threads REQUIRED)
    find_package(OpenGL REQUIRED)
    find_package(GLEW REQUIRED)
endif()

file(GLOB_RECURSE PTGL_LIB_INC RELATIVE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/ptgl/*.h)
file(GLOB_RECURSE PTGL_LIB_SRC ${PROJECT_SOURCE_DIR}/ptgl/*.cpp)

# thirdparty
set(PTGL_THIRDPARTY_SRC
    ${PROJECT_SOURCE_DIR}/thirdparty/Core/nanovg/src/nanovg.c
    ${PROJECT_SOURCE_DIR}/thirdparty/Loader/STL/stl_parser/parse_stl.cpp
)

if(EMSCRIPTEN)
    set(PTGL_LIBRARY_TYPE STATIC)
endif()
add_library(ptgl ${PTGL_LIBRARY_TYPE}
    ${PTGL_LIB_SRC}
    ${PTGL_THIRDPARTY_SRC}
)

target_compile_features(ptgl PUBLIC cxx_std_17)
target_include_directories(ptgl PUBLIC ${EIGEN_DIR} ${PROJECT_SOURCE_DIR})
if(EMSCRIPTEN)
    # JS exceptions preserve stack objects when execute()/glutMainLoop yields
    # via Emscripten's simulated infinite loop; Wasm exceptions unwind them.
    target_compile_options(ptgl PUBLIC -fexceptions)
    target_link_libraries(ptgl PUBLIC GLEW)
    target_link_options(ptgl PUBLIC -fexceptions -sDISABLE_EXCEPTION_CATCHING=0
        -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2
        -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=1048576)
    if(PTGL_ENABLE_WEB_THREADS)
        target_compile_options(ptgl PUBLIC -pthread)
        target_link_options(ptgl PUBLIC -pthread -sPTHREAD_POOL_SIZE=4)
    endif()
else()
    target_link_libraries(ptgl PUBLIC OpenGL::GL GLEW::GLEW Threads::Threads)
endif()
if(WIN32)
    target_link_libraries(ptgl PUBLIC imm32)
endif()

if(MSVC)
    target_compile_definitions(ptgl PUBLIC _USE_MATH_DEFINES NOMINMAX)
    target_compile_definitions(ptgl PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(ptgl PRIVATE /W3 /utf-8)
    set_target_properties(ptgl PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
else()
    target_compile_options(ptgl PRIVATE -Wall)
endif()

# install
install(TARGETS ptgl ARCHIVE DESTINATION lib LIBRARY DESTINATION lib RUNTIME DESTINATION bin)

foreach(file ${PTGL_LIB_INC})
    get_filename_component(dir ${file} DIRECTORY)
    install(FILES ${file} DESTINATION include/${dir})
endforeach()

if(PTGL_BUILD_PLASTIC_DEMO)
    add_subdirectory(examples/PlasticDemo)
endif()

if(PTGL_BUILD_EMSCRIPTEN_DEMO)
    add_subdirectory(examples/SimpleEmscriptenDemo)
endif()

if(PTGL_BUILD_THREADED_DEMO)
    add_subdirectory(examples/ThreadedDemo)
endif()



