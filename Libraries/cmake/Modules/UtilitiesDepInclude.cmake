append_unique(ara_sdk_INCLUDE_DIRS
	${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/glm/include
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/nameof
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/magic_enum
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/pugixml
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/easywsclient
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/threadpool
)

if (ARA_USE_CLIP)
	append_unique(ara_sdk_INCLUDE_DIRS ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/clip)
endif()

if (ARA_USE_CURL)
	append_unique(ara_sdk_INCLUDE_DIRS ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/curl/include)
endif()

if(NOT WIN32 AND NOT ANDROID)
	if (ARA_USE_FREEIMAGE)
		find_package(FreeImage REQUIRED)
	endif()
endif()

# Freeimage
if (ARA_USE_FREEIMAGE)
	if(WIN32)
		append_unique(ara_sdk_INCLUDE_DIRS ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/FreeImage/include)
	elseif(ANDROID)
		append_unique(ara_sdk_INCLUDE_DIRS ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/FreeImage/Android/include)
	else()
		if (FREEIMAGE_FOUND)
			append_unique(ara_sdk_INCLUDE_DIRS ${FREEIMAGE_INCLUDE_DIRS})
		endif (FREEIMAGE_FOUND)
	endif()
endif()
