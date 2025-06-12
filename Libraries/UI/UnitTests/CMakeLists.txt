cmake_minimum_required(VERSION 3.10)
project(UI_UnitTests VERSION 0.1 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)

include(GeneralSettings)
include(VisualStudioSettings)
include(UnitTestDependencies)
include(UtilitiesDepInclude)
include(ResCompInclude)

include_directories(
    ${CMAKE_SOURCE_DIR}/Libraries/UI/src
    ${CMAKE_SOURCE_DIR}/Libraries/Utilities/src
    ${CMAKE_SOURCE_DIR}/Libraries/third_party/nameof
)

# add test source files
file(GLOB UI_UT_SRC
    #ReloadGLContextTest.cpp
    #AlignTest.cpp
    ButtonTests.cpp
    #ComboBoxTest.cpp
    #HidTest.cpp
    #ImageTest.cpp
    #ScrollViewTest.cpp
    #SetXSetY.cpp
    #TabTest.cpp
    #UIMatTests/GetPixelPosition_Absolute.cpp
    #UIMatTests/GetPixelPosition_ParentRelative.cpp
)

add_executable(${PROJECT_NAME} ${UI_UT_SRC})

# automatically scan for gtest macros in target
gtest_add_tests(TARGET      ${PROJECT_NAME}
                TEST_SUFFIX .noArgs
                TEST_LIST   noArgsTests
)

set(TARGET_LIBS gmock_main gtest_main GLBase SceneGraph UI Utilities)
if(NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
    list(APPEND TARGET_LIBS resources)
endif()

if (UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} ${TARGET_LIBS} glfw assimp)
elseif(APPLE)
    target_link_libraries(${PROJECT_NAME} ${TARGET_LIBS})
else()
    target_link_libraries(${PROJECT_NAME} ${TARGET_LIBS} Dwmapi.lib)
endif()

include(GLBaseDepInclude)
include(GLBaseDependencies)
include(GLBaseCopyDependencies)
include(ResCompInclude)
include(UtilitiesDependencies)
include(UtilitiesDepInclude)

include(CreateSymLink)
create_symlink(${ARA_SDK_SOURCE_DIR}/Assets/resdata ${CMAKE_CURRENT_BINARY_DIR}/resdata)

# copy GLBase to executable dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/data ${CMAKE_CURRENT_BINARY_DIR}
    )

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_CURRENT_BINARY_DIR} COMPONENT ${PROJECT_NAME} OPTIONAL)