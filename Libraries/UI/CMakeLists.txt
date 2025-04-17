cmake_minimum_required(VERSION 3.10)

project(Libraries)

add_subdirectory(src)

if (NOT ANDROID)
    #include(FetchGoogleTest) # fetch google test from github if necessary
    add_subdirectory(UnitTests)
endif()
