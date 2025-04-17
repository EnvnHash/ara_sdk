#include(GLBaseDepInclude)

#include 3rd party libraries
if(NOT WIN32 AND NOT ANDROID)
	set(CMAKE_THREAD_LIBS_INIT "-lpthread")
	set(CMAKE_HAVE_THREADS_LIBRARY 1)
	set(CMAKE_USE_WIN32_THREADS_INIT 0)
	set(CMAKE_USE_PTHREADS_INIT 1)
	set(THREADS_PREFER_PTHREAD_FLAG ON)

	if (ARA_USE_ASSIMP AND NOT ASSIMP_FOUND)
		find_package (assimp REQUIRED)
	endif()
	if(ARA_USE_FREEIMAGE AND NOT FREEIMAGE_FOUND)
		find_package (FreeImage REQUIRED)
	endif()
	if(NOT GLFW_FOUND)
		find_package (GLFW REQUIRED)
	endif()
	if(NOT GLEW_FOUND)
		find_package (GLEW REQUIRED)
	endif()
	if(NOT GLM_FOUND)
		find_package (GLM REQUIRED)
	endif()
	if(NOT OpenGL_FOUND AND NOT ARA_USE_GLES31)
		find_package (OpenGL REQUIRED)
	endif()
	if(NOT PUGIXML_FOUND)
		find_package (PugiXML REQUIRED)
	endif()
	# find_package(OpenMP)
endif()

if (ARA_USE_ASSIMP)
	if(WIN32)
		target_link_libraries(${PROJECT_NAME}
			debug ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/assimp/lib/${LIB_ARCH_PATH}/assimp${ARCH_POSTFIX}d.lib
			optimized ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/assimp/lib/${LIB_ARCH_PATH}/Assimp${ARCH_POSTFIX}.lib
			)
	elseif(ANDROID)
		target_link_libraries(${PROJECT_NAME}
				${ARA_SDK_SOURCE_DIR}/Libraries/third_party/assimp/Android/${CMAKE_ANDROID_ARCH_ABI}/libassimp.so
				)
	else()
		if (ASSIMP_FOUND)
			target_link_libraries (${PROJECT_NAME} ${ASSIMP_LIBRARIES})
		endif()
	endif()
endif()

# Freeimage
if (ARA_USE_FREEIMAGE)
	if(WIN32)
		target_link_libraries (${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/Freeimage/lib/${LIB_ARCH_PATH}/FreeImage.lib)
	elseif(ANDROID)
		target_link_libraries (${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/FreeImage/Android/${CMAKE_ANDROID_ARCH_ABI}/libfreeimage.so
				${ARA_SDK_SOURCE_DIR}/Libraries/third_party/FreeImage/Android/${CMAKE_ANDROID_ARCH_ABI}/libpng16.so
				)
	else()
		if (FREEIMAGE_FOUND)
			target_link_libraries (${PROJECT_NAME} ${FREEIMAGE_LIBRARIES} ${LIB_LINK_OPT})
		endif()
	endif()
endif()

# GLEW
if(WIN32)
	target_link_libraries (${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/GLEW/lib/${LIB_ARCH_PATH}/glew32.lib)
else()
	if (GLEW_FOUND)
		target_link_libraries (${PROJECT_NAME} GLEW::GLEW ${LIB_LINK_OPT})
	endif()
endif()

#GLFW
if (WIN32)
	if(${CMAKE_BUILD_TYPE} MATCHES Debug)
		target_link_libraries (${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/GLFW/lib/${LIB_ARCH_PATH}/glfw3dll.lib)
	else ()
		target_link_libraries (${PROJECT_NAME} ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/GLFW/lib/${LIB_ARCH_PATH}/glfw3dll.lib)
	endif ()
elseif(APPLE)
	target_link_libraries (${PROJECT_NAME} ${GLFW_glfw_LIBRARY} ${LIB_LINK_OPT})
elseif(NOT ANDROID)
	target_link_libraries (${PROJECT_NAME} ${GLFW_LIBRARIES} ${LIB_LINK_OPT})
endif()


# OpenGL
if(WIN32)
	find_package (OpenGL REQUIRED)
endif()
if (OpenGL_FOUND)
  target_link_libraries (${PROJECT_NAME} OpenGL::GL ${LIB_LINK_OPT})
endif ()
if (ANDROID)
	target_link_libraries (${PROJECT_NAME} EGL GLESv1_CM GLESv2 GLESv3)
endif()

if (NOT ANDROID AND ARA_USE_GLES31)
	target_link_libraries (${PROJECT_NAME} EGL GL)
endif()

#pthreads
if (NOT WIN32)
	target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})
endif()

unset(GLSG_LIB_DIR)
