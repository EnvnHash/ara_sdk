set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
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
	set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-deprecated-declarations -Wno-unused-function")
endif()

# gcc debug flags
if (UNIX AND NOT ANDROID AND NOT APPLE)
	set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0" CACHE STRING "" FORCE)
	#set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-Wno-class-conversion")
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

	#include_directories(/opt/local/include/libomp)

	#try to build universal binary. on an intel mac x86_64 should be supported, on an m1 both should be supported
	set(CMAKE_REQUIRED_LINK_OPTIONS "-arch x86_64")
	check_c_compiler_flag("-arch x86_64" x86_64Supported)
	#message("x86_64Supported=${x86_64Supported}")

	set(CMAKE_REQUIRED_LINK_OPTIONS "-arch arm64")
	check_c_compiler_flag("-arch arm64" arm64Supported)
	#message("arm64Supported=${arm64Supported}")

	if (${arm64Supported})
		SET(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)
	else()
		SET(CMAKE_OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architectures for Mac OS X" FORCE)
	endif()
endif()
