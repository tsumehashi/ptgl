cmake_minimum_required(VERSION 2.8)
set(PROJ_NAME SimpleDemo)
project(${PROJ_NAME} CXX)

set(EIGEN_DIR /usr/include/eigen3 CACHE STRING "The directory of the Eigen library")
set(PTGL_INC_DIR /usr/local/include CACHE STRING "The directory of the ptgl headers")
set(PTGL_LIB_DIR /usr/local/lib CACHE STRING "The directory of the ptgl lib")

add_definitions("-Wall -std=c++1z -pthread")

include_directories(
    ${EIGEN_DIR}
    ${PTGL_INC_DIR}
)

link_directories(
    ${PTGL_LIB_DIR}
)

add_executable(${PROJ_NAME}
    main.cpp
)

target_link_libraries(${PROJ_NAME}
    ptgl
    GL
    glfw
    GLEW
    pthread
)

