if (ARA_USE_OPENCV)
    if (WIN32)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/Libraries/third_party/opencv/x64/vc16/bin/opencv_videoio_ffmpeg4100_64.dll ${CMAKE_CURRENT_BINARY_DIR})
        if (${CMAKE_BUILD_TYPE} MATCHES Debug)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/Libraries/third_party/opencv/x64/vc16/bin/opencv_world4100d.dll ${CMAKE_CURRENT_BINARY_DIR})
        else ()
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/Libraries/third_party/opencv/x64/vc16/bin/opencv_world4100.dll ${CMAKE_CURRENT_BINARY_DIR})
        endif ()
    else ()
        find_package(OpenCV REQUIRED PATHS "..")
        if (OPENCV_FOUND)
            include_directories( ${OpenCV_INCLUDE_DIRS} )
        endif ()
    endif()
endif()