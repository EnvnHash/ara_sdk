if(WIN32)
    if (ARA_USE_CURL)
        set(UTILITIES_LIBS wsock32 ws2_32 user32 iphlpapi Strmiids.lib Dwmapi.lib winmm.lib ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/curl/lib/libcurl.dll.a)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/curl/bin/libcurl-x64.dll ${CMAKE_CURRENT_BINARY_DIR})
    else()
        set(UTILITIES_LIBS wsock32 ws2_32 user32 iphlpapi Strmiids.lib Dwmapi.lib winmm.lib )
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
    target_link_libraries(${PROJECT_NAME} "-framework CoreFoundation")
    target_link_libraries(${PROJECT_NAME} "-framework CoreGraphics")
endif()

target_link_libraries (${PROJECT_NAME} ${UTILITIES_LIBS})
