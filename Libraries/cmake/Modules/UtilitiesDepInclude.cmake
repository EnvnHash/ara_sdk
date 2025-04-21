include_directories(
	${ARA_SDK_SOURCE_DIR}/Libraries/Utilities/src
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/nameof
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/magic_enum
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/pugixml
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/easywsclient
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/threadpool
)

if (ARA_USE_CURL)
	include_directories(${ARA_SDK_SOURCE_DIR}/Libraries/third_party/curl/include)
endif()
