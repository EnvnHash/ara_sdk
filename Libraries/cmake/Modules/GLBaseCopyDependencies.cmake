#copy GLBase to executable dir
if (WIN32)
    if(${CMAKE_BUILD_TYPE} MATCHES Debug)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/GLFW/bin/${LIB_ARCH_PATH}/glfw3.dll ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/GLEW/bin/${LIB_ARCH_PATH}/glew32.dll ${CMAKE_CURRENT_BINARY_DIR})
    else ()
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/GLFW/bin/${LIB_ARCH_PATH}/glfw3.dll ${CMAKE_CURRENT_BINARY_DIR}
                COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/GLEW/bin/${LIB_ARCH_PATH}/glew32.dll ${CMAKE_CURRENT_BINARY_DIR})
    endif ()
    if(ARA_USE_FREEIMAGE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/Freeimage/bin/${LIB_ARCH_PATH}/FreeImage.dll ${CMAKE_CURRENT_BINARY_DIR})
    endif()

    if(ARA_USE_ASSIMP)
        if(${CMAKE_BUILD_TYPE} MATCHES Debug)
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/assimp/bin/${LIB_ARCH_PATH}/assimp${ARCH_POSTFIX}d.dll ${CMAKE_CURRENT_BINARY_DIR}
                )
        else()
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/assimp/bin/${LIB_ARCH_PATH}/assimp${ARCH_POSTFIX}.dll ${CMAKE_CURRENT_BINARY_DIR}
                )
        endif()
    endif()
endif(WIN32)