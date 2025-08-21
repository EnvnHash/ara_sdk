include(${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules/AraSdkMacros.cmake)

if(NOT WIN32 AND NOT ANDROID)
	set(CMAKE_THREAD_LIBS_INIT "-lpthread")
	set(CMAKE_HAVE_THREADS_LIBRARY 1)
	set(CMAKE_USE_WIN32_THREADS_INIT 0)
	set(CMAKE_USE_PTHREADS_INIT 1)
	set(THREADS_PREFER_PTHREAD_FLAG ON)

	if (ARA_USE_ASSIMP AND NOT ASSIMP_FOUND)
		find_package(assimp REQUIRED)
	endif()
	if(NOT GLFW_FOUND)
		find_package(GLFW REQUIRED)
	endif()
	if(NOT GLEW_FOUND)
		find_package(GLEW REQUIRED)
	endif()
	if(NOT GLM_FOUND)
		find_package(GLM REQUIRED)
	endif()
	if(NOT OpenGL_FOUND AND NOT ARA_USE_GLES31)
		find_package(OpenGL REQUIRED)
	endif()
	if(NOT PUGIXML_FOUND)
		find_package(PugiXML REQUIRED)
	endif()
	# find_package(OpenMP)
endif()

if (ARA_USE_ASSIMP)
	if(WIN32)
		append_unique(ara_sdk_LIBRARIES
			debug ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/assimp/lib/${LIB_ARCH_PATH}/assimp${ARCH_POSTFIX}d.lib
			optimized ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/assimp/lib/${LIB_ARCH_PATH}/Assimp${ARCH_POSTFIX}.lib
			)
	elseif(ANDROID)
		append_unique(ara_sdk_LIBRARIES
				${ARA_SDK_SOURCE_DIR}/Libraries/third_party/assimp/Android/${CMAKE_ANDROID_ARCH_ABI}/libassimp.so
				)
	else()
		if (ASSIMP_FOUND)
			append_unique(ara_sdk_LIBRARIES ${ASSIMP_LIBRARIES})
		endif()
	endif()
endif()

# GLEW
if(WIN32)
	append_unique(ara_sdk_LIBRARIES ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/GLEW/lib/${LIB_ARCH_PATH}/glew32.lib)
else()
	if (GLEW_FOUND)
		append_unique(ara_sdk_LIBRARIES GLEW)
	endif()
endif()

#GLFW
if (WIN32)
	if(${CMAKE_BUILD_TYPE} MATCHES Debug)
		append_unique(ara_sdk_LIBRARIES ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/GLFW/lib/${LIB_ARCH_PATH}/glfw3dll.lib)
	else ()
		append_unique(ara_sdk_LIBRARIES ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/GLFW/lib/${LIB_ARCH_PATH}/glfw3dll.lib)
	endif ()
elseif(APPLE)
	append_unique(ara_sdk_LIBRARIES ${GLFW_glfw_LIBRARY})
elseif(NOT ANDROID)
	append_unique(ara_sdk_LIBRARIES ${GLFW_LIBRARIES})
endif()


# OpenGL
if(WIN32)
	find_package (OpenGL REQUIRED)
endif()

if (OpenGL_FOUND)
	append_unique(ara_sdk_LIBRARIES GL)
endif ()

if (ANDROID)
	append_unique(ara_sdk_LIBRARIES EGL GLESv1_CM GLESv2 GLESv3)
endif()

if (NOT ANDROID AND ARA_USE_GLES31)
	append_unique(ara_sdk_LIBRARIES EGL GL)
endif()

#pthreads
if (NOT WIN32)
	append_unique(ara_sdk_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
endif()

if (WIN32)
	append_unique(ara_sdk_LIBRARIES Dwmapi.lib)
endif()
