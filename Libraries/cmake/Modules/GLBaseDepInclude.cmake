set(GLSG_LIB_DIR ${ARA_SDK_SOURCE_DIR}/Libraries/third_party)

include_directories(
	${ARA_SDK_SOURCE_DIR}/Libraries/GLBase/src
	${ARA_SDK_SOURCE_DIR}/Libraries/SceneGraph/src
	${ARA_SDK_SOURCE_DIR}/Libraries/third_party/
)

if (ARA_USE_GLBASE)
	add_compile_definitions(ARA_USE_GLBASE)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(ARCH_POSTFIX "")
	set(LIB_ARCH_PATH "Win32")
else()
	set(ARCH_POSTFIX 64)
	set(LIB_ARCH_PATH "x64")
endif()

#include 3rd party libraries
if(NOT WIN32 AND NOT ANDROID)
	set(CMAKE_THREAD_LIBS_INIT "-lpthread")
	set(CMAKE_HAVE_THREADS_LIBRARY 1)
	#set(CMAKE_USE_WIN32_THREADS_INIT 0)
	set(CMAKE_USE_PTHREADS_INIT 1)
	set(THREADS_PREFER_PTHREAD_FLAG ON)

	if (ARA_USE_ASSIMP)
		find_package (assimp REQUIRED)
	endif()
	if (ARA_USE_FREEIMAGE)
		find_package (FreeImage REQUIRED)
	endif()
	if (ARA_USE_GLFW)
		find_package (GLFW REQUIRED)
	endif()
	find_package (GLEW REQUIRED)
	find_package (GLM REQUIRED)
	if (NOT ARA_USE_GLES31)
		find_package (OpenGL REQUIRED)
	endif()
	find_package (PugiXML REQUIRED)
	find_package (PkgConfig REQUIRED)
	if (NOT APPLE)
		pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
	endif()
endif()

include(Gtk3Include)

#Assimp
if (ARA_USE_ASSIMP)
	add_compile_definitions(ARA_USE_ASSIMP)
	if(WIN32)
		include_directories(${GLSG_LIB_DIR}/assimp/include)
	elseif(ANDROID)
		include_directories(${GLSG_LIB_DIR}/assimp/Android/include)
	else()
		if (ASSIMP_FOUND)
			include_directories(${ASSIMP_INCLUDE_DIRS})
		endif (ASSIMP_FOUND)
	endif()
endif()

# Freeimage
if (ARA_USE_FREEIMAGE)
	if(WIN32)
		include_directories(${GLSG_LIB_DIR}/FreeImage/include)
	elseif(ANDROID)
		include_directories(${GLSG_LIB_DIR}/FreeImage/Android/include)
	else()
		if (FREEIMAGE_FOUND)
			include_directories(${FREEIMAGE_INCLUDE_DIRS})
		endif (FREEIMAGE_FOUND)
	endif()
endif()

# GLEW
if(WIN32)
	include_directories(${GLSG_LIB_DIR}/GLEW/include/GL)
elseif(ANDROID)
	include_directories(${GLSG_LIB_DIR}/GLEW/include/GL)
else()
	if (GLEW_FOUND)
  		include_directories(${GLEW_INCLUDE_DIRS})
	else()
		message(ERROR "glew not found")
	endif ()

endif(WIN32)

#GLFW
if (WIN32 AND ARA_USE_GLFW)
	include_directories(${GLSG_LIB_DIR}/GLFW/include/)
endif()

#GLM (header only)
include_directories(${GLSG_LIB_DIR}/glm/include)

# OpenGL
if (OpenGL_FOUND AND NOT ARA_USE_GLES31)
  include_directories(${OPENGL_INCLUDE_DIRS})
endif()

#openmp
if(APPLE)
	if(CMAKE_C_COMPILER_ID MATCHES "Clang")
		set(OpenMP_C "${CMAKE_C_COMPILER}")
		set(OpenMP_C_FLAGS "-fopenmp=libomp -Wno-unused-command-line-argument")
		set(OpenMP_C_LIB_NAMES "libomp" "libgomp" "libiomp5")
		set(OpenMP_libomp_LIBRARY ${OpenMP_C_LIB_NAMES})
		set(OpenMP_libgomp_LIBRARY ${OpenMP_C_LIB_NAMES})
		set(OpenMP_libiomp5_LIBRARY ${OpenMP_C_LIB_NAMES})
	endif()
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		set(OpenMP_CXX "${CMAKE_CXX_COMPILER}")
		set(OpenMP_CXX_FLAGS "-fopenmp=libomp -Wno-unused-command-line-argument")
		set(OpenMP_CXX_LIB_NAMES "libomp" "libgomp" "libiomp5")
		set(OpenMP_libomp_LIBRARY ${OpenMP_CXX_LIB_NAMES})
		set(OpenMP_libgomp_LIBRARY ${OpenMP_CXX_LIB_NAMES})
		set(OpenMP_libiomp5_LIBRARY ${OpenMP_CXX_LIB_NAMES})
	endif()
	include_directories(/opt/local/include/libomp)
elseif(NOT WIN32)
	#message(STATUS "--- looking for openmp")
	#set(LIBS OpenMP::OpenMP_CXX)
	#find_package(OpenMP)

	#if(OpenMP_FOUND AND NOT ANDROID)
	#	message(STATUS "found OPENMP!!!")
	#	include_directories(${OpenMP_CXX_INCLUDE_DIR})
	#	set(LIBS OpenMP::OpenMP_CXX)
	#endif()
endif()

include(Gtk3Include)
include(PugiXMLInclude)

unset(GLSG_LIB_DIR)
