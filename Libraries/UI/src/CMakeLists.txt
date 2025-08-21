cmake_minimum_required(VERSION 3.10)

project(UI)

include_directories(${ara_sdk_INCLUDE_DIRS})

file(GLOB_RECURSE UI_SRC "*.cpp")

add_library(${PROJECT_NAME} OBJECT ${UI_SRC})
add_library(ara::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
