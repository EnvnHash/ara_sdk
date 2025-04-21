cmake_minimum_required(VERSION 3.10)

project(Libraries)

add_subdirectory(src)

if (NOT ANDROID)
    add_subdirectory(UnitTests)
endif()
