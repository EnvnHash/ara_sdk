# FFMpeg
#include(VideoInputDepInclude)

set(THIRD_PARTY ${ARA_SDK_SOURCE_DIR}/Libraries/third_party)
include_directories(${ARA_SDK_SOURCE_DIR}/Libraries/VideoInput/src)

if (ARA_USE_FFMPEG)
    if(ANDROID)
        if (${CMAKE_ANDROID_ARCH_ABI} STREQUAL "armeabi-v7a")
            target_link_libraries(${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/ffmpeg/Android/${CMAKE_ANDROID_ARCH_ABI}/libavcodec_neon.so
                    ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/ffmpeg/Android/${CMAKE_ANDROID_ARCH_ABI}/libavdevice_neon.so
                    ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/ffmpeg/Android/${CMAKE_ANDROID_ARCH_ABI}/libavfilter_neon.so
                    ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/ffmpeg/Android/${CMAKE_ANDROID_ARCH_ABI}/libavformat_neon.so
                    ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/ffmpeg/Android/${CMAKE_ANDROID_ARCH_ABI}/libavutil_neon.so
                    ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/ffmpeg/Android/${CMAKE_ANDROID_ARCH_ABI}/libswresample_neon.so
                    ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/ffmpeg/Android/${CMAKE_ANDROID_ARCH_ABI}/libswscale_neon.so)
        else()
            target_link_libraries(${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/ffmpeg/Android/${CMAKE_ANDROID_ARCH_ABI}/libavcodec.so
                ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/ffmpeg/Android/${CMAKE_ANDROID_ARCH_ABI}/libavdevice.so
                ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/ffmpeg/Android/${CMAKE_ANDROID_ARCH_ABI}/libavfilter.so
                ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/ffmpeg/Android/${CMAKE_ANDROID_ARCH_ABI}/libavformat.so
                ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/ffmpeg/Android/${CMAKE_ANDROID_ARCH_ABI}/libavutil.so
                ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/ffmpeg/Android/${CMAKE_ANDROID_ARCH_ABI}/libswresample.so
                ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/ffmpeg/Android/${CMAKE_ANDROID_ARCH_ABI}/libswscale.so)
        endif()
    elseif(WIN32)
        target_link_libraries (${PROJECT_NAME}
                ${THIRD_PARTY}/ffmpeg/lib/${LIB_ARCH_PATH}/avcodec.lib
                ${THIRD_PARTY}/ffmpeg/lib/${LIB_ARCH_PATH}/avdevice.lib
                ${THIRD_PARTY}/ffmpeg/lib/${LIB_ARCH_PATH}/avfilter.lib
                ${THIRD_PARTY}/ffmpeg/lib/${LIB_ARCH_PATH}/avformat.lib
                ${THIRD_PARTY}/ffmpeg/lib/${LIB_ARCH_PATH}/avutil.lib
                ${THIRD_PARTY}/ffmpeg/lib/${LIB_ARCH_PATH}/postproc.lib
                ${THIRD_PARTY}/ffmpeg/lib/${LIB_ARCH_PATH}/swresample.lib
                ${THIRD_PARTY}/ffmpeg/lib/${LIB_ARCH_PATH}/swscale.lib)
    else()
        find_package (FFMpeg REQUIRED)
        if (FFMPEG_FOUND)
            target_link_libraries (${PROJECT_NAME} ${FFMPEG_LIBRARIES} ${LIB_LINK_OPT})
        endif ()
    endif()
endif()

#NDI
if (NOT COMPILE_SCENEGRAPH_LIB AND ARA_USE_NDI)
    if (WIN32)
        target_link_libraries(${PROJECT_NAME}
            ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/NDI/lib/${LIB_ARCH_PATH}/Processing.NDI.Lib.${LIB_ARCH_PATH}.lib
            )
    elseif(APPLE)
        #target_link_libraries(${PROJECT_NAME}
         #   ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/NDI/OsX/lib/${LIB_ARCH_PATH}/libndi.4.dylib )
    elseif(UNIX AND NOT ANDROID)
        target_link_libraries(${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/NDI/linux/lib/x86_64-linux-gnu/libndi.so)
    elseif(ANDROID)
        target_link_libraries(${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/NDI/Android/${CMAKE_ANDROID_ARCH_ABI}/libndi.so)
    endif()
endif()

#OpenAL
if (NOT COMPILE_SCENEGRAPH_LIB AND ARA_USE_OPENAL)
    if (WIN32)
        target_link_libraries(${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/OpenAL/lib/x64/OpenAL32.lib)
    elseif(APPLE)
        find_package (OpenAL REQUIRED)
        if (OPENAL_FOUND)
            target_link_libraries(${PROJECT_NAME} ${OPENAL_LIBRARY})
        endif()
    elseif(ANDROID)
        target_link_libraries(${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/OpenAL/Android/${CMAKE_ANDROID_ARCH_ABI}/libopenal.so)
    elseif(UNIX)
        target_link_libraries(${PROJECT_NAME} openal)
    endif()
endif()

#portaudio
if (ARA_USE_PORTAUDIO)
    if (WIN32)
        target_link_libraries(${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/portaudio/lib/${LIB_ARCH_PATH}/portaudio_${LIB_ARCH_PATH}.lib)
    #elseif(APPLE)
    #    target_link_libraries(${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/portaudio/bin/OsX/libportaudio.dylib)
    elseif(ANDROID)
        target_link_libraries(${PROJECT_NAME} OpenSLES ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/portaudio/Android/${CMAKE_ANDROID_ARCH_ABI}/libportaudio.so)
    elseif(UNIX AND NOT ANDROID)
        find_package(Portaudio)
        target_link_libraries(${PROJECT_NAME} ${PORTAUDIO_LIBRARIES})
    endif()
endif()

#realsense
if(ARA_USE_REALSENSE)
    if (WIN32)
        target_link_libraries(${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/intel_realsense/lib/${LIB_ARCH_PATH}/realsense2.lib)
    else()
        find_package (Realsense2 REQUIRED)
        if (REALSENSE2_FOUND)
            target_link_libraries(${PROJECT_NAME} ${REALSENSE2_LIBRARIES})
        endif()
    endif()
endif()

#google libyuv
if (WIN32)
    target_link_libraries(${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/libyuv/lib/${LIB_ARCH_PATH}/yuv.lib)
elseif(UNIX AND NOT ANDROID)
    target_link_libraries(${PROJECT_NAME} yuv)
endif()

# libobs
if (WIN32 AND ARA_USE_OBS)
    target_link_libraries(${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/obs/lib/${LIB_ARCH_PATH}/obs.lib Winmm.lib)
endif()

# opencv
if (WIN32 AND ARA_USE_OPENCV)
    find_package(OpenCV REQUIRED)
    include_directories(${OpenCV_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})
    #include_directories(${ARA_SDK_SOURCE_DIR}/Libraries/third_party/opencv/include)
endif()
