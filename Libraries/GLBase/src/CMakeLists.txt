cmake_minimum_required(VERSION 3.10)
project(GLBase VERSION 1.0 LANGUAGES CXX)

if (NOT COMPILE_SCENEGRAPH_LIB)
    add_compile_definitions(WGLWINDOW)
endif ()

include_directories(
    ${ara_sdk_INCLUDE_DIRS}
)

file(GLOB_RECURSE GLB_SRC *.cpp)

add_library(${PROJECT_NAME} OBJECT ${GLB_SRC} ${GLB_HEADERS})
add_library(ara::${PROJECT_NAME} ALIAS ${PROJECT_NAME})