cmake_minimum_required(VERSION 3.10)
project(Utilities_UnitTests VERSION 0.1 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)

include(GeneralSettings)
include(VisualStudioSettings)

include_directories(
    ${CMAKE_SOURCE_DIR}/Libraries/Utilities/src
)

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
    ##UdpSignalTest.cpp
    TableTest.cpp
)

add_executable(${PROJECT_NAME} ${UTILITIES_UT_SRC})

include(Gtk3Include)
include(ResCompInclude)
include(UtilitiesDepInclude)
include(UtilitiesDependencies)
include(UnitTestDependencies)
set(TARGET_LIBS ${ARA_SDK_TARGET_LINK_LIBS})

# automatically scan for gtest macros in target
gtest_add_tests(TARGET ${PROJECT_NAME}
        TEST_SUFFIX .noArgs
        TEST_LIST noArgsTests
)

list(APPEND TARGET_LIBS gmock_main gtest_main Utilities)
if (NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
    list(APPEND TARGET_LIBS resources)
endif ()

target_link_libraries(${PROJECT_NAME} ${TARGET_LIBS})

INSTALL(TARGETS ${PROJECT_NAME}
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR} COMPONENT ${PROJECT_NAME} OPTIONAL)