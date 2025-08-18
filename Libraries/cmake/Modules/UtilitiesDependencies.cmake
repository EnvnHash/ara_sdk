include(${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules/AraSdkMacros.cmake)

if(WIN32)
    if (ARA_USE_CURL)
        set(UTILITIES_LIBS wsock32 ws2_32 user32 iphlpapi Strmiids.lib winmm.lib ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/curl/lib/libcurl.dll.a)
        append_unique(ara_sdk_BINARIES ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/curl/bin/libcurl-x64.dll)
    else()
        set(UTILITIES_LIBS wsock32 ws2_32 user32 iphlpapi Strmiids.lib winmm.lib )
    endif()
elseif(ANDROID AND ARA_USE_CURL)
    set(UTILITIES_LIBS
        ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/openssl/Android/${CMAKE_ANDROID_ARCH_ABI}/libssl.so
        ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/openssl/Android/${CMAKE_ANDROID_ARCH_ABI}/libcrypto.so
        ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/curl/Android/${CMAKE_ANDROID_ARCH_ABI}/libcurl.so)
elseif(ARA_USE_CURL)
    find_package (CURL REQUIRED)
    set(UTILITIES_LIBS curl)
endif()

if (APPLE)
    #target_link_libraries(${PROJECT_NAME}
    list(APPEND ARA_SDK_TARGET_LINK_OPTS
        "-framework CoreFoundation")

    #target_link_libraries(${PROJECT_NAME}
    list(APPEND ARA_SDK_TARGET_LINK_OPTS
        "-framework CoreGraphics")
endif()

#include 3rd party libraries
if(NOT WIN32 AND NOT ANDROID)
    if(ARA_USE_FREEIMAGE AND NOT FREEIMAGE_FOUND)
        find_package (FreeImage REQUIRED)
    endif()
endif()

# Freeimage
if (ARA_USE_FREEIMAGE)
    if(WIN32)
        list(APPEND UTILITIES_LIBS ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/Freeimage/lib/${LIB_ARCH_PATH}/FreeImage.lib)
        append_unique(ara_sdk_BINARIES ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/Freeimage/bin/${LIB_ARCH_PATH}/FreeImage.dll)
    elseif(ANDROID)
        list(APPEND UTILITIES_LIBS ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/FreeImage/Android/${CMAKE_ANDROID_ARCH_ABI}/libfreeimage.so
            ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/FreeImage/Android/${CMAKE_ANDROID_ARCH_ABI}/libpng16.so
        )
    else()
        if (FREEIMAGE_FOUND)
            list(APPEND UTILITIES_LIBS ${FREEIMAGE_LIBRARIES})
        endif()
    endif()
endif()

append_unique(ara_sdk_LIBRARIES ${UTILITIES_LIBS})
