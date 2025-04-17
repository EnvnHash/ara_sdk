cmake_minimum_required(VERSION 3.10)

project(Libraries)

add_subdirectory(src)

#unit test
if (NOT ANDROID)
    include(FetchGoogleTest) # fetch google test from github if necessary
    if (NOT BUILD_UNVUE_OSX_BUNDLE)
        add_subdirectory(UnitTests)
        add_dependencies(GLBase_UnitTests GLBase Utilities gmock_main gtest_main)
    endif ()
endif ()
