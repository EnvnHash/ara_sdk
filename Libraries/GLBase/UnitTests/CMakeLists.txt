cmake_minimum_required(VERSION 3.10)
project(GLBase_UnitTests VERSION 1.0 LANGUAGES CXX)

set(CMAKE_MODULE_PATH ${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)

include_directories(${ara_sdk_INCLUDE_DIRS})
include(UnitTestDependencies)

# add test source files
file(GLOB GLBASE_UT_SRC
    TypoGlyphMapTest.cpp
    ContextSharing.cpp
    #ContextSharingFbo.cpp
    CreateFbo.cpp
    CreateGLCtx.cpp
    CreateWindow.cpp
    CtrlPointSerialization.cpp
    DrawQuad.cpp
    FreeImageTest.cpp
    MemLeakTests/DrawQuadMemLeak.cpp
    GLBaseCreationDestruction.cpp
    GLBaseStopStart.cpp
    GLBaseStopStartResources.cpp
    GLRendererTest.cpp
    LoadTextureMipMap.cpp
    LoadTexture.cpp
    NativeWindow.cpp
    PolygonTest.cpp
    PolygonSerializationHps.cpp
    PolygonSerialization.cpp
    ThreadedWindow.cpp
)

add_executable(${PROJECT_NAME} ${GLBASE_UT_SRC})

# automatically scan for gtest macros in target
gtest_add_tests(TARGET ${PROJECT_NAME}
        TEST_SUFFIX .noArgs
        TEST_LIST noArgsTests
)

set(TARGET_LIBS ${ara_sdk_LIBRARIES} gmock_main gtest_main GLBase Utilities)

if (NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
    list(APPEND TARGET_LIBS resources)
endif ()

target_link_libraries(${PROJECT_NAME} ${TARGET_LIBS})

include(ResCompInclude)

#copy GLBase to executable dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/data/FullHD_Pattern.png ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/data/typoGlyphTest.png ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/data/loadTexTest.png ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/data/freeimage_test_img.png ${CMAKE_CURRENT_BINARY_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/data/OpenSans-Light.ttf ${CMAKE_CURRENT_BINARY_DIR}
)

FOREACH(binary ${ara_sdk_BINARIES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${binary} ${CMAKE_CURRENT_BINARY_DIR})
ENDFOREACH()

INSTALL(TARGETS ${PROJECT_NAME}
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR} COMPONENT ${PROJECT_NAME} OPTIONAL)