if (WIN32)
    set(THIRD_PARTY_LIB_DIR ${ARA_SDK_SOURCE_DIR}/Libraries/third_party)
    if (ARA_USE_FFMPEG)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/ffmpeg/bin/${LIB_ARCH_PATH}/avcodec-59.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/ffmpeg/bin/${LIB_ARCH_PATH}/avdevice-59.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/ffmpeg/bin/${LIB_ARCH_PATH}/avfilter-8.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/ffmpeg/bin/${LIB_ARCH_PATH}/avformat-59.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/ffmpeg/bin/${LIB_ARCH_PATH}/avutil-57.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/ffmpeg/bin/${LIB_ARCH_PATH}/postproc-56.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/ffmpeg/bin/${LIB_ARCH_PATH}/swresample-4.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/ffmpeg/bin/${LIB_ARCH_PATH}/swscale-6.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>)
    endif()

    if (ARA_USE_NDI)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/NDI/Bin/${LIB_ARCH_PATH}/Processing.NDI.Lib.${LIB_ARCH_PATH}.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
       )
    endif()

    if (ARA_USE_REALSENSE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/intel_realsense/bin/${LIB_ARCH_PATH}/realsense2.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endif ()
    
    if (ARA_USE_OPENAL)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/OpenAL/bin/${LIB_ARCH_PATH}/OpenAL32.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    endif()
    if (ARA_USE_PORTAUDIO AND WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/portaudio/bin/${LIB_ARCH_PATH}/portaudio_${LIB_ARCH_PATH}.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
    endif()

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/libyuv/bin/${LIB_ARCH_PATH}/libyuv.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )

    if (ARA_USE_OBS AND WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/libobs-d3d11.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/libobs-opengl.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/libobs-winrt.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/obsglad.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/obs.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/w32-pthreads.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/zlib.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/libcurl.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/libmbedcrypto.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/libogg-0.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/libopus-0.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/libsrt.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/libvorbis-0.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/libvorbisenc-2.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/libvorbisfile-3.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/libvpx-1.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/libx264-161.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E copy ${THIRD_PARTY_LIB_DIR}/obs/bin/${LIB_ARCH_PATH}/lua51.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/obs-plugins
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/obs/obs-plugins $<TARGET_FILE_DIR:${PROJECT_NAME}>/obs-plugins
            COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:${PROJECT_NAME}>/obs_data
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/obs/data $<TARGET_FILE_DIR:${PROJECT_NAME}>/obs_data
            )
    endif()

    if (ARA_USE_OPENCV AND WIN32)
        if (WIN32)
            include_directories(${ARA_SDK_SOURCE_DIR}/Libraries/third_party/opencv/include)
            #target_link_libraries (${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/opencv/lib/${LIB_ARCH_PATH}/opencv_world460.lib)
            #add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            #   COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/Libraries/third_party/opencv/bin/${LIB_ARCH_PATH}/opencv_world460.dll ${CMAKE_CURRENT_BINARY_DIR})
        else ()
            find_package(OpenCV REQUIRED PATHS "..")
            if (OPENCV_FOUND)
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy C:\\Program\ Files\\OpenCV\\${LIB_ARCH_PATH}\\vc15\\bin\\opencv_world454.dll ${CMAKE_CURRENT_BINARY_DIR})
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy C:\\Program\ Files\\OpenCV\\${LIB_ARCH_PATH}\\vc15\\bin\\opencv_world454d.dll ${CMAKE_CURRENT_BINARY_DIR})
            else()
                message(ERROR "OpenCV not found")
            endif()
        endif()
    endif()
endif()