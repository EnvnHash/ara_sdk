cmake_minimum_required(VERSION 3.10)

project(UI)

file(GLOB_RECURSE UI_SRC "*.cpp")

add_library(${PROJECT_NAME} OBJECT ${UI_SRC})
add_library(ara::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${ara_sdk_COMPILE_DEFINITIONS})
target_include_directories(${PROJECT_NAME} PUBLIC ${ara_sdk_INCLUDE_DIRS})