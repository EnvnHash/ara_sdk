#copy GLBase to executable dir
if (WIN32)
    if(${CMAKE_BUILD_TYPE} MATCHES Debug)
        append_unique(ara_sdk_BINARIES ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/GLFW/bin/${LIB_ARCH_PATH}/glfw3.dll
                                       ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/GLEW/bin/${LIB_ARCH_PATH}/glew32.dll)
    else ()
        append_unique(ara_sdk_BINARIES ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/GLFW/bin/${LIB_ARCH_PATH}/glfw3.dll
                                       ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/GLEW/bin/${LIB_ARCH_PATH}/glew32.dll)
    endif ()

    if(ARA_USE_ASSIMP)
        if(${CMAKE_BUILD_TYPE} MATCHES Debug)
            append_unique(ara_sdk_BINARIES ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/assimp/bin/${LIB_ARCH_PATH}/assimp${ARCH_POSTFIX}d.dll)
        else()
            append_unique(ara_sdk_BINARIES ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/assimp/bin/${LIB_ARCH_PATH}/assimp${ARCH_POSTFIX}.dll)
        endif()
    endif()
endif()