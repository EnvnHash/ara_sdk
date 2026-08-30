cmake_minimum_required(VERSION 3.10)
project(Utilities_UnitTests VERSION 1.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)
include(UnitTestDependencies)

# add test source files
file(GLOB UTILITIES_UT_SRC 
    AnimValTest.cpp
    AssetLoaderTest.cpp
    CommonTest.cpp
    ConditionalTest.cpp
    CycleBufferTest.cpp
    ItemTest.cpp
    ListPropertyTest.cpp
    NodeTest.cpp
    PoissonTest.cpp
    PropertyTest.cpp
    RwBinFileTest.cpp
    StringUtilsTest.cpp
    TableTest.cpp
    TypeDeductionTest.cpp
    FreeImageHandlerTest.cpp
)

add_executable(${PROJECT_NAME} ${UTILITIES_UT_SRC})

# automatically scan for gtest macros in target
gtest_add_tests(TARGET ${PROJECT_NAME}
        TEST_SUFFIX .noArgs
        TEST_LIST noArgsTests
)

set(TARGET_LIBS ${ara_sdk_LIBRARIES} gmock_main gtest_main Utilities)

if (NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
    list(APPEND TARGET_LIBS resources)
endif ()

target_link_libraries(${PROJECT_NAME} ${TARGET_LIBS})

include(ResCompInclude)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}
)

FOREACH(binary ${ara_sdk_BINARIES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${binary} ${CMAKE_CURRENT_BINARY_DIR})
ENDFOREACH()