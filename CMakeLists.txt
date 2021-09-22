cmake_minimum_required(VERSION 2.8)
project(ptgl C CXX)

option(BUILD_SHARED_LIBS "Build shared library" ON)
set(EIGEN_DIR /usr/include/eigen3 CACHE STRING "The directory of the Eigen library")

add_definitions("-O2 -Wall -std=c++17")

include_directories(${EIGEN_DIR})
include_directories(${PROJECT_SOURCE_DIR})

file(GLOB_RECURSE PTGL_LIB_INC RELATIVE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/ptgl/*.h)
file(GLOB_RECURSE PTGL_LIB_SRC ${PROJECT_SOURCE_DIR}/ptgl/*.cpp)

# thirdparty
set(PTGL_THIRDPARTY_SRC
    ${PROJECT_SOURCE_DIR}/thirdparty/Core/nanovg/src/nanovg.c
    ${PROJECT_SOURCE_DIR}/thirdparty/Loader/STL/stl_parser/parse_stl.cpp
)

add_library(ptgl
    ${PTGL_LIB_SRC}
    ${PTGL_THIRDPARTY_SRC}
)

# install
install(TARGETS ptgl ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)

foreach(file ${PTGL_LIB_INC})
    get_filename_component(dir ${file} DIRECTORY)
    install(FILES ${file} DESTINATION include/${dir})
endforeach()



