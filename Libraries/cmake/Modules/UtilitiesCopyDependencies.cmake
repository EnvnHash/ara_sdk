#copy GLBase to executable dir
if (WIN32 AND ARA_USE_FREEIMAGE)
    append_unique(ara_sdk_BINARIES ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/Freeimage/bin/${LIB_ARCH_PATH}/FreeImage.dll)
endif()