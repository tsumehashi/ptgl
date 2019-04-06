cmake_minimum_required(VERSION 2.8)
set(PROJ_NAME SimpleEmscriptenDemo)
project(${PROJ_NAME} CXX C)

set(EIGEN_DIR /usr/include/eigen3 CACHE STRING "The directory of the Eigen library")
set(PTGL_INC_DIR ../.. CACHE STRING "The directory of the ptgl headers")
set(PTGL_SRC_DIR ../.. CACHE STRING "The directory of the ptgl sources")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++1z")

file(GLOB_RECURSE ptglLibSrc ${PTGL_SRC_DIR}/ptgl/*.cpp)

# thirdparty
set(thirdpartySrc
    ${PTGL_SRC_DIR}/thirdparty/Core/nanovg/src/nanovg.c
    ${PTGL_SRC_DIR}/thirdparty/Loader/STL/stl_parser/parse_stl.cpp
)

include_directories(
    ${EIGEN_DIR}
    ${PTGL_INC_DIR}
)

add_executable(${PROJ_NAME}
    main.cpp
    ${ptglLibSrc}
    ${thirdpartySrc}
)

set(CMAKE_EXECUTABLE_SUFFIX ".html")