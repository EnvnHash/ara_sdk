#copy GLBase to executable dir
if (WIN32)
    if(ARA_USE_FREEIMAGE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/Freeimage/bin/${LIB_ARCH_PATH}/FreeImage.dll ${CMAKE_CURRENT_BINARY_DIR})
    endif()
endif()