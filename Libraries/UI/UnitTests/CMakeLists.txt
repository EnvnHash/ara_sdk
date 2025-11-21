cmake_minimum_required(VERSION 3.10)
project(UI_UnitTests VERSION 1.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)
include(UnitTestDependencies)
include_directories(${ara_sdk_INCLUDE_DIRS})

# add test source files
file(GLOB UI_UT_SRC
    ReloadGLContextTest.cpp
    AlignTest.cpp
    ButtonTests.cpp
    ComboBoxTest.cpp
    CarrouselTest.cpp
    HidTest.cpp
    ImageTest.cpp
    ScrollViewTest.cpp
    SerializeBasicTest.cpp
    SerializeImageTest.cpp
    SerializeTestCreateAndRemove.cpp
    SetXSetY.cpp
    TabTest.cpp
    ZoomViewTest.cpp
    UIMatTests/GetPixelPosition_Absolute.cpp
    UIMatTests/GetPixelPosition_ParentRelative.cpp
)

add_executable(${PROJECT_NAME} ${UI_UT_SRC})

# automatically scan for gtest macros in target
gtest_add_tests(TARGET      ${PROJECT_NAME}
                TEST_SUFFIX .noArgs
                TEST_LIST   noArgsTests
)

set(TARGET_LIBS ${ara_sdk_LIBRARIES} gmock_main gtest_main GLBase SceneGraph UI Utilities)

if(NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
    list(APPEND TARGET_LIBS resources)
endif()

target_link_libraries(${PROJECT_NAME} ${TARGET_LIBS})

include(ResCompInclude)

# copy GLBase to executable dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}
    )

FOREACH(binary ${ara_sdk_BINARIES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${binary} ${CMAKE_CURRENT_BINARY_DIR})
ENDFOREACH()

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} COMPONENT ${PROJECT_NAME} OPTIONAL)