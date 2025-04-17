include_directories(
	${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/nameof
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/magic_enum
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/pugixml
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/easywsclient
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/threadpool
)

if(ARA_USE_CMRC AND NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
	include_directories(${ARA_SDK_SOURCE_DIR}/Libraries/third_party/cmrc/include)
endif()

if (ARA_USE_CURL)
	include_directories(${ARA_SDK_SOURCE_DIR}/Libraries/third_party/curl/include)
endif()

if (ARA_USE_LIBRTMP)
	if (WITH_RTMPS)
		find_package(MbedTLS REQUIRED)
		find_package(ZLIB REQUIRED)
		add_definitions(-DCRYPTO -DUSE_MBEDTLS)
		include_directories(${MBEDTLS_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS})
	else()
		add_definitions(-DNO_CRYPTO)
	endif()
endif()

