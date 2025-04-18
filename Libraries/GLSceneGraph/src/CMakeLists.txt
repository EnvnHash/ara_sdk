cmake_minimum_required(VERSION 3.10)

if (COMPILE_GLSCENEGRAPH_LIB)
    if (${CMAKE_BUILD_TYPE} MATCHES Debug)
        project(GLSceneGraphd VERSION 0.1 LANGUAGES CXX)
    else ()
        project(GLSceneGraph VERSION 0.1 LANGUAGES CXX)
    endif ()
    add_compile_definitions(GLSG_EXPORT GLBASE_EXPORT)
else ()
    project(GLSceneGraph VERSION 0.1 LANGUAGES CXX)
endif ()

#include(VisualStudioSettings)
include(GeneralSettings)
include(AraConfigure)

if (WIN32)
    add_compile_definitions(UNICODE _UNICODE)
endif (WIN32)

include_directories(
        ${ARA_SDK_SOURCE_DIR}/Libraries/third_party
        ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src
        ${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src
        ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/nameof
)

file(GLOB GLSG_SRC "*.cpp")
#file(GLOB GLSG_NVENC_SRC "third_party/nvidia/*.cpp")
file(GLOB_RECURSE GLSG_ASSIMP_SRC "Assimp/*.cpp")
file(GLOB_RECURSE GLSG_CAMERA_SRC "CameraSets/*.cpp")
file(GLOB_RECURSE GLSG_COMMON_SRC "glsg_common/*.cpp")
file(GLOB_RECURSE GLSG_GLUTILS_SRC "GLUtils/*.cpp")
file(GLOB_RECURSE GLSG_LIGHTS_SRC "Lights/*.cpp")
file(GLOB_RECURSE GLSG_SCENENODES_SRC "SceneNodes/*.cpp")
file(GLOB_RECURSE GLSG_SHADERS_SRC "Shaders/*.cpp")
file(GLOB_RECURSE GLSG_UI_SRC "UI/*.cpp")
#header files this is just to have them m_show up in the visual studio project
file(GLOB_RECURSE GLSG_HEADERS "src/*.h")
if (ANDROID)
    file(GLOB_RECURSE GLSG_ANDROID_SRC "Android/*.cpp")
endif ()

set(GLSG_ALL_SRC
        ${GLSG_SRC}
        #${GLSG_NVENC_SRC}
        ${GLSG_ASSIMP_SRC}
        ${GLSG_CAMERA_SRC}
        ${GLSG_COMMON_SRC}
        ${GLSG_GLUTILS_SRC}
        ${GLSG_LIGHTS_SRC}
        ${GLSG_SCENENODES_SRC}
        ${GLSG_SHADERS_SRC}
        ${GLSG_UI_SRC}
        ${GLSG_ANDROID_SRC}
        ${GLSG_HEADERS} CACHE INTERNAL "glsg_src_all")

if (COMPILE_GLSCENEGRAPH_LIB AND NOT ANDROID)
    if (COMPILE_GLSCENEGRAPH_LIB_STATIC)
        add_library(${PROJECT_NAME} STATIC ${GLSG_ALL_SRC} $<TARGET_OBJECTS:Utilities> $<TARGET_OBJECTS:GLBase>)
        if (ARA_USE_CURL)
            target_link_libraries(${PROJECT_NAME} Utilities GLBase VideoInput Dwmapi.lib libcurl.a)
        else ()
            target_link_libraries(${PROJECT_NAME} VideoInput Dwmapi.lib OpenGL::GL)
        endif ()
    else ()
        add_library(${PROJECT_NAME} SHARED ${GLSG_ALL_SRC} $<TARGET_OBJECTS:GLBase> $<TARGET_OBJECTS:Utilities>)
        #target_link_libraries(${PROJECT_NAME} Utilities GLBase)
        if (WIN32)
            if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
                target_link_libraries(${PROJECT_NAME} ws2_32.lib)
            else ()
                target_link_libraries(${PROJECT_NAME} legacy_stdio_definitions.lib)
            endif ()
        endif ()
    endif ()
elseif (ANDROID)
    add_compile_definitions(ARA_USE_GLES31)
    add_library(${PROJECT_NAME} OBJECT ${GLSG_ALL_SRC})
else ()
    add_compile_definitions(COMPILE_GLSCENEGRAPH_AS_OBJECT)
    add_library(${PROJECT_NAME} OBJECT ${GLSG_ALL_SRC})
endif ()

if (ARA_USE_CMRC AND NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
    include_directories(${ARA_SDK_SOURCE_DIR}/Libraries/third_party/cmrc/include)
endif ()

if (ANDROID)
    target_include_directories(${PROJECT_NAME} PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue)
endif ()

include(GLBaseDepInclude)
if (COMPILE_GLSCENEGRAPH_LIB)
    include(GLBaseDependencies)

    set(TP_DIR ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/)
    set(DST_DIR ${CMAKE_CURRENT_BINARY_DIR}/UISDK/)

    file(GLOB UTILITY_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src/*.h)
    file(GLOB_RECURSE UTILITY_DM_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src/DataModel/*.h)
    file(GLOB_RECURSE UTILITY_NET_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src/Network/*.h)
    file(GLOB_RECURSE UTILITY_THREAD_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src/ThreadedTasks/*.h)
    file(GLOB_RECURSE UTILITY_WINDOWS_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src/Windows/*.h)
    file(GLOB_RECURSE UTILITY_X11_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src/X11/*.h)

    file(GLOB VIDEOINPUT_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/VideoInput/src/*.h)
    file(GLOB_RECURSE VIDEOINPUT_DS_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/VideoInput/src/DirectShow/*.h)
    file(GLOB_RECURSE VIDEOINPUT_FFMPEG_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/VideoInput/src/FFMpeg/*.h)
    file(GLOB_RECURSE VIDEOINPUT_INTELREALSENSE_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/VideoInput/src/IntelRealsense/*.h)
    file(GLOB_RECURSE VIDEOINPUT_NDI_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/VideoInput/src/NDI/*.h)
    file(GLOB_RECURSE VIDEOINPUT_PORTAUDIO_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/VideoInput/src/Portaudio/*.h)

    file(GLOB GLBASE_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src/*.h)
    file(GLOB_RECURSE GLBASE_ASSIMP_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src/Assimp/*.h)
    file(GLOB_RECURSE GLBASE_GEOPRIM_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src/GeoPrimitives/*.h)
    file(GLOB_RECURSE GLBASE_GLBCOMM_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src/glb_common/*.h)
    file(GLOB_RECURSE GLBASE_MESH_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src/Meshes/*.h)
    file(GLOB GLBASE_RES_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src/Res/*.h)
    file(GLOB GLBASE_SHDR_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src/Shaders/*.h)
    file(GLOB GLBASE_SHDR_UTIL_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src/Shaders/ShaderUtils/*.h)
    file(GLOB_RECURSE GLBASE_UTIL_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src/Utils/*.h)
    file(GLOB_RECURSE GLBASE_WMMANAG_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src/WindowManagement/*.h)

    file(GLOB GLSCENEGRAPH_ASSIMP_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLSceneGraph/src/Assimp/*.h)
    file(GLOB GLSCENEGRAPH_CAMSET_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLSceneGraph/src/CameraSets/*.h)
    file(GLOB GLSCENEGRAPH_COMMON_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLSceneGraph/src/glsg_common/*.h)
    file(GLOB GLSCENEGRAPH_UTILS_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLSceneGraph/src/GLUtils/*.h)
    file(GLOB GLSCENEGRAPH_LIGHTS_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLSceneGraph/src/Lights/*.h)
    file(GLOB GLSCENEGRAPH_SCNNODES_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLSceneGraph/src/SceneNodes/*.h)
    file(GLOB GLSCENEGRAPH_SHADERS_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLSceneGraph/src/Shaders/*.h)
    file(GLOB GLSCENEGRAPH_SHADERS_PROP_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLSceneGraph/src/Shaders/ShaderProperties/*.h)
    file(GLOB GLSCENEGRAPH_SHADERS_PROTO_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLSceneGraph/src/Shaders/ShaderPrototype/*.h)
    file(GLOB GLSCENEGRAPH_SHADERS_MAP_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLSceneGraph/src/Shaders/ShadowMap/*.h)
    file(GLOB GLSCENEGRAPH_UI_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLSceneGraph/src/UI/*.h)
    file(GLOB GLSCENEGRAPH_UI_DIAG_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLSceneGraph/src/UI/Dialoges/*.h)
    file(GLOB GLSCENEGRAPH_UI_ITEME_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLSceneGraph/src/UI/ItemEditors/*.h)
    file(GLOB GLSCENEGRAPH_HEADERS ${ARA_SDK_SOURCE_DIR}/Libraries/GLSceneGraph/src/*.h)

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/bin
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/bin/x64
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/bin/x86
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/lib
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/lib/x64
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/lib/x86
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLBase)

    # includes
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLBase
            COMMAND ${CMAKE_COMMAND} -E copy ${GLBASE_HEADERS} ${DST_DIR}/include/GLBase
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLBase/Assimp
            COMMAND ${CMAKE_COMMAND} -E copy ${GLBASE_ASSIMP_HEADERS} ${DST_DIR}/include/GLBase/Assimp
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLBase/GeoPrimitives
            COMMAND ${CMAKE_COMMAND} -E copy ${GLBASE_GEOPRIM_HEADERS} ${DST_DIR}/include/GLBase/GeoPrimitives
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLBase/glb_common
            COMMAND ${CMAKE_COMMAND} -E copy ${GLBASE_GLBCOMM_HEADERS} ${DST_DIR}/include/GLBase/glb_common
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLBase/Meshes
            COMMAND ${CMAKE_COMMAND} -E copy ${GLBASE_MESH_HEADERS} ${DST_DIR}/include/GLBase/Meshes
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLBase/Res
            COMMAND ${CMAKE_COMMAND} -E copy ${GLBASE_RES_HEADERS} ${DST_DIR}/include/GLBase/Res
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLBase/Shaders
            COMMAND ${CMAKE_COMMAND} -E copy ${GLBASE_SHDR_HEADERS} ${DST_DIR}/include/GLBase/Shaders
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLBase/Shaders/ShaderUtils
            COMMAND ${CMAKE_COMMAND} -E copy ${GLBASE_SHDR_UTIL_HEADERS} ${DST_DIR}/include/GLBase/Shaders/ShaderUtils
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLBase/Utils
            COMMAND ${CMAKE_COMMAND} -E copy ${GLBASE_UTIL_HEADERS} ${DST_DIR}/include/GLBase/Utils
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLBase/WindowManagement
            COMMAND ${CMAKE_COMMAND} -E copy ${GLBASE_WMMANAG_HEADERS} ${DST_DIR}/include/GLBase/WindowManagement
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLSceneGraph
            COMMAND ${CMAKE_COMMAND} -E copy ${GLSCENEGRAPH_HEADERS} ${DST_DIR}/include/GLSceneGraph
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLSceneGraph/Assimp
            COMMAND ${CMAKE_COMMAND} -E copy ${GLSCENEGRAPH_ASSIMP_HEADERS} ${DST_DIR}/include/GLSceneGraph/Assimp
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLSceneGraph/CameraSets
            COMMAND ${CMAKE_COMMAND} -E copy ${GLSCENEGRAPH_CAMSET_HEADERS} ${DST_DIR}/include/GLSceneGraph/CameraSets
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLSceneGraph/glsg_common
            COMMAND ${CMAKE_COMMAND} -E copy ${GLSCENEGRAPH_COMMON_HEADERS} ${DST_DIR}/include/GLSceneGraph/glsg_common
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLSceneGraph/GLUtils
            COMMAND ${CMAKE_COMMAND} -E copy ${GLSCENEGRAPH_UTILS_HEADERS} ${DST_DIR}/include/GLSceneGraph/GLUtils
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLSceneGraph/Lights
            COMMAND ${CMAKE_COMMAND} -E copy ${GLSCENEGRAPH_LIGHTS_HEADERS} ${DST_DIR}/include/GLSceneGraph/Lights
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLSceneGraph/SceneNodes
            COMMAND ${CMAKE_COMMAND} -E copy ${GLSCENEGRAPH_SCNNODES_HEADERS} ${DST_DIR}/include/GLSceneGraph/SceneNodes
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLSceneGraph/Shaders
            #COMMAND ${CMAKE_COMMAND} -E copy ${GLSCENEGRAPH_SHADERS_HEADERS} ${DST_DIR}/include/GLSceneGraph/Shaders # is empty
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLSceneGraph/Shaders/ShaderProperties
            COMMAND ${CMAKE_COMMAND} -E copy ${GLSCENEGRAPH_SHADERS_PROP_HEADERS} ${DST_DIR}/include/GLSceneGraph/Shaders/ShaderProperties
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLSceneGraph/Shaders/ShaderPrototype
            COMMAND ${CMAKE_COMMAND} -E copy ${GLSCENEGRAPH_SHADERS_PROTO_HEADERS} ${DST_DIR}/include/GLSceneGraph/Shaders/ShaderPrototype
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLSceneGraph/Shaders/ShadowMap
            COMMAND ${CMAKE_COMMAND} -E copy ${GLSCENEGRAPH_SHADERS_MAP_HEADERS} ${DST_DIR}/include/GLSceneGraph/Shaders/ShadowMap
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLSceneGraph/UI/
            COMMAND ${CMAKE_COMMAND} -E copy ${GLSCENEGRAPH_UI_HEADERS} ${DST_DIR}/include/GLSceneGraph/UI
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLSceneGraph/UI/Dialoges
            COMMAND ${CMAKE_COMMAND} -E copy ${GLSCENEGRAPH_UI_DIAG_HEADERS} ${DST_DIR}/include/GLSceneGraph/UI/Dialoges
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/GLSceneGraph/UI/ItemEditors
            COMMAND ${CMAKE_COMMAND} -E copy ${GLSCENEGRAPH_UI_ITEME_HEADERS} ${DST_DIR}/include/GLSceneGraph/UI/ItemEditors

            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/Utilities
            COMMAND ${CMAKE_COMMAND} -E copy ${UTILITY_HEADERS} ${DST_DIR}/include/Utilities
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/Utilities/DataModel
            COMMAND ${CMAKE_COMMAND} -E copy ${UTILITY_DM_HEADERS} ${DST_DIR}/include/Utilities/DataModel
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/Utilities/Network
            COMMAND ${CMAKE_COMMAND} -E copy ${UTILITY_NET_HEADERS} ${DST_DIR}/include/Utilities/Network
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/Utilities/ThreadedTasks
            COMMAND ${CMAKE_COMMAND} -E copy ${UTILITY_THREAD_HEADERS} ${DST_DIR}/include/Utilities/ThreadedTasks
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/Utilities/Windows
            COMMAND ${CMAKE_COMMAND} -E copy ${UTILITY_WINDOWS_HEADERS} ${DST_DIR}/include/Utilities/Windows
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/Utilities/X11
            COMMAND ${CMAKE_COMMAND} -E copy ${UTILITY_X11_HEADERS} ${DST_DIR}/include/Utilities/X11

            #COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/VideoInput
            #COMMAND ${CMAKE_COMMAND} -E copy ${VIDEOINPUT_HEADERS} ${DST_DIR}/include/VideoInput
            #COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/VideoInput/DirectShow
            #COMMAND ${CMAKE_COMMAND} -E copy ${VIDEOINPUT_DS_HEADERS} ${DST_DIR}/include/VideoInput/DirectShow
            #COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/VideoInput/FFMpeg
            #COMMAND ${CMAKE_COMMAND} -E copy ${VIDEOINPUT_FFMPEG_HEADERS} ${DST_DIR}/include/VideoInput/FFMpeg
            #COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/VideoInput/IntelRealsense
            #COMMAND ${CMAKE_COMMAND} -E copy ${VIDEOINPUT_INTELREALSENSE_HEADERS} ${DST_DIR}/include/VideoInput/IntelRealsense
            #COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/VideoInput/NDI
            #COMMAND ${CMAKE_COMMAND} -E copy ${VIDEOINPUT_NDI_HEADERS} ${DST_DIR}/include/VideoInput/NDI
            #COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/include/VideoInput/Portaudio
            #COMMAND ${CMAKE_COMMAND} -E copy ${VIDEOINPUT_PORTAUDIO_HEADERS} ${DST_DIR}/include/VideoInput/Portaudio

            # dlls and libs
            COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}FreeImage/include/FreeImage.h ${DST_DIR}/include/
            COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}/nameof/nameof.hpp ${DST_DIR}/include/
            COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}/stb_truetype.h ${DST_DIR}/include/)

    if (WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}GLEW/bin/x64/glew32.dll ${DST_DIR}/bin/x64
                COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}GLEW/bin/Win32/glew32.dll ${DST_DIR}/bin/x86
                COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}GLFW/bin/x64/glfw3.dll ${DST_DIR}/bin/x64
                COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}GLFW/lib/x64/glfw3dll.lib ${DST_DIR}/lib/x64
        )
    endif ()

    if (WIN32 AND ARA_USE_ASSMIP)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}Assimp/include/assimp ${DST_DIR}/include/assimp
                COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}Assimp/lib/x64/assimp64.lib ${DST_DIR}/lib/x64
                COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}Assimp/bin/x64/Assimp64.dll ${DST_DIR}/bin/x64
                COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}Assimp/lib/Win32/assimp.lib ${DST_DIR}/bin/x86
                COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}Assimp/bin/Win32/assimp.dll ${DST_DIR}/lib/x86
        )
    endif ()

    if (WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}FreeImage/bin/x64/FreeImage.dll ${DST_DIR}/bin/x64
                COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}FreeImage/bin/Win32/FreeImage.dll ${DST_DIR}/bin/x86
                COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}FreeImage/lib/x64/FreeImage.lib ${DST_DIR}/lib/x64
                COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}FreeImage/lib/Win32/FreeImage.lib ${DST_DIR}/lib/x86
                COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}libyuv/lib/x64/yuv.lib ${DST_DIR}/lib/x64
                COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}libyuv/bin/x64/libyuv.dll ${DST_DIR}/bin/x64
                #COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}ffmpeg/lib/x64/avcodec.lib ${DST_DIR}/lib/x64
                #COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}ffmpeg/lib/x64/avcodec-58.dll ${DST_DIR}/bin/x64
                #COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}ffmpeg/lib/x64/avdevice.lib ${DST_DIR}/lib/x64
                #COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}ffmpeg/lib/x64/avdevice-58.dll ${DST_DIR}/bin/x64
                #COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}ffmpeg/lib/x64/avformat.lib ${DST_DIR}/lib/x64
                #COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}ffmpeg/lib/x64/avformat-58.dll ${DST_DIR}/bin/x64
                #COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}ffmpeg/lib/x64/avfilter.lib ${DST_DIR}/lib/x64
                #COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}ffmpeg/lib/x64/avfilter-7.dll ${DST_DIR}/bin/x64
                #COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}ffmpeg/lib/x64/avutil.lib ${DST_DIR}/lib/x64
                #COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}ffmpeg/lib/x64/avutil-56.dll ${DST_DIR}/bin/x64
                #COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}ffmpeg/lib/x64/postproc.lib ${DST_DIR}/lib/x64
                #COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}ffmpeg/lib/x64/swresample.lib ${DST_DIR}/lib/x64
                #COMMAND ${CMAKE_COMMAND} -E copy ${TP_DIR}ffmpeg/lib/x64/swscale.lib ${DST_DIR}/lib/x64
        )
    endif ()

    if (WIN32 AND COMPILE_GLSCENEGRAPH_LIB_STATIC)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.lib ${DST_DIR}/lib/x64)
    elseif (APPLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}.a ${DST_DIR}/lib)
    endif ()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/third_party/curl
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}intel_realsense ${DST_DIR}/third_party/intel_realsense
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}portaudio ${DST_DIR}/third_party/portaudio
            #COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}NDI ${DST_DIR}/third_party/NDI
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}ffmpeg ${DST_DIR}/third_party/ffmpeg
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}curl ${DST_DIR}/third_party/curl

            # header only libs
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}earcut ${DST_DIR}/include/earcut
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}pugixml ${DST_DIR}/include/pugixml
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}GLFW/include/GLFW ${DST_DIR}/include/GLFW
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}glm/include/glm ${DST_DIR}/include/glm
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}hps ${DST_DIR}/include/hps
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}GLEW/include/GL ${DST_DIR}/include/GL
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}json11 ${DST_DIR}/include/json11
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}easywsclient ${DST_DIR}/include/easywsclient
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}threadpool ${DST_DIR}/include/threadpool
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}ffmpeg/include ${DST_DIR}/include/ffmpeg/include
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${TP_DIR}libyuv/include ${DST_DIR}/include/libyuv/include
            # resdata
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/resdata
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/resdata/xml
            COMMAND ${CMAKE_COMMAND} -E make_directory ${DST_DIR}/resdata/models
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/ui-icons.png ${DST_DIR}/resdata
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/stage3d_res.txt ${DST_DIR}/resdata
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/add_icon.png ${DST_DIR}/resdata
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/view_icon.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/trash_icon.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/trash_icon_dark.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/icon_selected.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/ctrl_point_move_icon.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/ctrl_point_add_and_arrow_icon.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/ctrl_point_add_icon.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/ctrl_point_del_and_arrow_icon.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/ctrl_point_del_icon.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/interpol_mode.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/invert_icon.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/quad_preset.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/circle_preset.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/grid_icon.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/crosshair_icon.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/mouse_cursor_asc.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/mouse_cursor_desc.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/mouse_move_cursor.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/mouse_rotate_cursor.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/mouse_scaling_cursor.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Icons/icon-arrow-down.png ${DST_DIR}/resdata/Icons
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/models/camera.3ds ${DST_DIR}/resdata/models
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/models/camera_y_up.3ds ${DST_DIR}/resdata/models
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/xml/config.xml ${DST_DIR}/resdata/xml
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Assets/resdata/xml/masking_config.xml ${DST_DIR}/resdata/xml
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${ARA_SDK_SOURCE_DIR}/Assets/resdata/Fonts ${DST_DIR}/resdata/Fonts
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/create_symlink.bat ${DST_DIR}
    )

    if (NOT COMPILE_GLSCENEGRAPH_LIB_STATIC)
        if (WIN32)
            if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}.dll ${DST_DIR}/bin/x64
                )
            else ()
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.dll ${DST_DIR}/bin/x64
                )
            endif ()
        else (UNIX)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/Libraries/GLBase/src/libGLBase.so ${DST_DIR}/lib/x64
                    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/Libraries/Utilities/src/libUtilities.so ${DST_DIR}/lib/x64
                    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/lib${PROJECT_NAME}.so ${DST_DIR}/lib/x64
            )
        endif ()
    endif ()

    #file(TO_NATIVE_PATH ${ARA_SDK_SOURCE_DIR}/Examples/UI_Example/UISDK link)
    #file(TO_NATIVE_PATH ${DST_DIR} target)
    #execute_process(COMMAND cmd /c ${ARA_SDK_SOURCE_DIR}\\create_symlink.bat ${link} ${target})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${DST_DIR} ${ARA_SDK_SOURCE_DIR}/Examples/UI_Example/UISDK)

endif ()

#generate UINodeFact
set(GEN_UINODEFACTORY 0)
if (${GEN_UINODEFACTORY} EQUAL 1)
    message(------ gen uinodefact)
    MATH(EXPR GEN_ITEMFACTORY "${GEN_ITEMFACTORY}+1")

    set(outfile ${CMAKE_CURRENT_SOURCE_DIR}/src/UI/UINodeFact.h)
    file(REMOVE ${outfile})

    # generate the ItemFactoryBase class
    # get header files by searching all file for "public Item"
    file(GLOB_RECURSE itemHeaderFiles ${CMAKE_CURRENT_SOURCE_DIR}/src/UI/*.h)
    list(REMOVE_ITEM itemHeaderFiles
            ${CMAKE_CURRENT_SOURCE_DIR}/src/UI/UINodeFact.h
            ${CMAKE_CURRENT_SOURCE_DIR}/src/UI/ObjectInteraction.h
            ${CMAKE_CURRENT_SOURCE_DIR}/src/UI/UISharedRes.h)

    #message(${itemHeaderFiles})
    set(headerList)

    foreach (headerFile ${itemHeaderFiles})
        file(READ ${headerFile} headerText)
        string(REGEX MATCH "class.[(A-z)|(a-z)]*" cn ${headerText})
        string(LENGTH ${cn} var_length)
        string(SUBSTRING ${cn} 6 ${var_length} cnClean)
        list(APPEND headerList ${cnClean})
    endforeach ()

    set(outStr)
    list(APPEND outStr "#pragma once\n")
    list(APPEND outStr "#include <iostream>\n")
    list(APPEND outStr "#include <string>\n")
    foreach (class ${headerList})
        list(APPEND outStr "#include \"${class}.h\"\n")
    endforeach ()
    list(APPEND outStr "\nnamespace VIOSO::glsg {\n")
    list(APPEND outStr "class UINodeFact {\n")
    list(APPEND outStr "public:\n")
    list(APPEND outStr "\tstd::string getStyleDefaults(){\n\t\tstd::stringstream ss\;\n")

    list(APPEND outStr "\t\tss ")
    foreach (class ${headerList})
        list(APPEND outStr "\t\t<< ${class}::m_defaultStyle \n")
    endforeach ()
    list(APPEND outStr "\;")
    list(APPEND outStr "\t\treturn std::move(ss.str())\;\n")
    list(APPEND outStr "\t}\n\n")

    list(APPEND outStr "\tstd::unique_ptr<UINode> Create(const std::string &sClassName) {\n")

    SET(ITPOS 0)
    foreach (class ${headerList})
        list(APPEND outStr "\t\t")
        if (NOT ${ITPOS} EQUAL 0)
            list(APPEND outStr "else ")
        endif ()
        list(APPEND outStr "if (className == \"${class}\") { return std::make_unique<${class}>()\; }\n")
        MATH(EXPR ITPOS "${ITPOS}+1")
    endforeach ()

    list(APPEND outStr "\t\treturn std::make_unique<Item>()\;\n")

    list(APPEND outStr "\t}\n")
    list(APPEND outStr "};\n")
    list(APPEND outStr "}\n")

    FILE(WRITE ${outfile} ${outStr}) # Empty file

endif ()