cmake_minimum_required(VERSION 2.8)
project(ptgl C CXX)

option(BUILD_SHARED_LIBS "Build shared library" ON)
set(EIGEN_DIR /usr/include/eigen3 CACHE STRING "The directory of the Eigen library")

add_definitions("-Wall -std=c++1z")

include_directories(${EIGEN_DIR})
include_directories(${PROJECT_SOURCE_DIR})

file(GLOB_RECURSE ptglLibInc RELATIVE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/ptgl/*.h)
file(GLOB_RECURSE ptglLibSrc ${PROJECT_SOURCE_DIR}/ptgl/*.cpp)

# thirdparty
set(thirdpartySrc
    ${PROJECT_SOURCE_DIR}/thirdparty/Core/nanovg/src/nanovg.c
    ${PROJECT_SOURCE_DIR}/thirdparty/Loader/STL/stl_parser/parse_stl.cpp
)

add_library(ptgl
    ${ptglLibSrc}
    ${thirdpartySrc}
)

# install
install(TARGETS ptgl ARCHIVE DESTINATION lib LIBRARY DESTINATION lib)

foreach(file ${ptglLibInc})
    get_filename_component(dir ${file} DIRECTORY)
    install(FILES ${file} DESTINATION include/${dir})
endforeach()



