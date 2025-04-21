cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")

include(FetchContent)
include(GoogleTest)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.16.0
)

FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED)
    FetchContent_MakeAvailable(googletest)
endif()

include_directories(
        ${GTEST_INCLUDE_DIRS}
)

enable_testing()
