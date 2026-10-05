cmake_minimum_required(VERSION 3.10)
project(ptgl C CXX)

option(BUILD_SHARED_LIBS "Build shared library" ON)
option(PTGL_BUILD_PLASTIC_DEMO "Build the plastic/CAD/legacy rendering comparison" OFF)
set(EIGEN_DIR /usr/include/eigen3 CACHE STRING "The directory of the Eigen library")

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

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

target_compile_features(ptgl PUBLIC cxx_std_17)
target_include_directories(ptgl PUBLIC ${EIGEN_DIR} ${PROJECT_SOURCE_DIR})
target_link_libraries(ptgl PUBLIC OpenGL::GL GLEW::GLEW)

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



