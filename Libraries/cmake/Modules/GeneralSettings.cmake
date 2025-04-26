set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 20)
set(OpenGL_GL_PREFERENCE "LEGACY")
CMAKE_POLICY(SET CMP0071 NEW) 
set(CMAKE_MODULE_PATH ${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
	set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

if(UNIX)
	set(CUDA_DIR /usr/local/cuda)
endif()


if(WIN32)
	add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
	add_compile_definitions(UNICODE _UNICODE)
	set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	    set(ARCH_POSTFIX "")
		set(LIB_ARCH_PATH "Win32")
	else()
		set(ARCH_POSTFIX 64)
		set(LIB_ARCH_PATH "x64")
	endif()
endif(WIN32)

if (UNIX AND NOT ANDROID)
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-deprecated-declarations -Wno-unused-function -Wno-deprecated-volatile")
endif()

# gcc debug flags
if (UNIX AND NOT ANDROID AND NOT APPLE)
	string(APPEND CMAKE_C_FLAGS " -Wno-nonnull")
	string(APPEND CMAKE_CXX_FLAGS " -Wno-nonnull")

	set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0" CACHE STRING "" FORCE)
	# supress deprecated warnings
	add_definitions(${GXX_COVERAGE_COMPILE_FLAGS})

	MACRO(HEADER_DIRECTORIES return_list ending)
		FILE(GLOB_RECURSE new_list ${ending})
		SET(dir_list "")
		FOREACH(file_path ${new_list})
			GET_FILENAME_COMPONENT(dir_path ${file_path} PATH)
			file(RELATIVE_PATH rel_dir_path ${CMAKE_CURRENT_SOURCE_DIR} ${dir_path})
			SET(dir_list ${dir_list} ${rel_dir_path})
		ENDFOREACH()
		LIST(REMOVE_DUPLICATES dir_list)
		SET(${return_list} ${dir_list})
	ENDMACRO()
endif()

# osx platform selection
if (APPLE)
	include(CheckCCompilerFlag)

	set(SUPPORTED_ARCHITECTURES "")

	execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine OUTPUT_VARIABLE MACHINE)

	#try to build universal binary. on an intel mac x86_64 should be supported, on an m1 both should be supported
	if(${MACHINE} MATCHES "arm64-*" OR ${MACHINE} MATCHES "aarch64-*")
		list(APPEND SUPPORTED_ARCHITECTURES "arm64")
		message(STATUS "arm64 is supported")
	endif ()

	if(${MACHINE} MATCHES "x86_64")
		list(APPEND SUPPORTED_ARCHITECTURES "x86_64")
		message(STATUS "x86_64 is supported")
	endif ()
	#check_c_compiler_flag("-arch arm64" arm64Supported)
	#if (${x86_64Supported})
#		list(APPEND SUPPORTED_ARCHITECTURES ";arm64")
#		message(STATUS "arm 64 is supported")
	#endif ()

	SET(CMAKE_OSX_ARCHITECTURES ${SUPPORTED_ARCHITECTURES} CACHE STRING "Build architectures for Mac OS X" FORCE)
endif()

# virtual filesystem
if (ARA_USE_CMRC AND NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
	include_directories(${ARA_SDK_SOURCE_DIR}/Libraries/third_party/cmrc/include)
endif ()
