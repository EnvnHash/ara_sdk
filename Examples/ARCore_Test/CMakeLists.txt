cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GeneralSettings)
include(VisualStudioSettings)
include(VideoInputDepInclude)

project(ARCore_Test)

file(GLOB UNV_SRC *.cpp)

set(SOURCES ${UNV_SRC})

if (ANDROID)
	add_library(${PROJECT_NAME} STATIC ${SOURCES})
else()
	if(${CMAKE_BUILD_TYPE} MATCHES Debug)
		add_executable(${PROJECT_NAME} ${SOURCES} ${WIN_INSTALLER_RES})
	else()
		if(WIN32)
			add_executable(${PROJECT_NAME} ${SOURCES} ${WIN_INSTALLER_RES})
			#add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${WIN_INSTALLER_RES})
		elseif(APPLE)
			add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${SOURCES})
		else()
			add_executable(${PROJECT_NAME} ${SOURCES})
		endif()
	endif()
endif()


set(TARGET_LIBS GLBase GLSceneGraph VideoInput Utilities)
if(NOT ${CMAKE_BUILD_TYPE} MATCHES Debug OR ANDROID)
	list(APPEND TARGET_LIBS resources)
endif()

target_link_libraries (${PROJECT_NAME} ${TARGET_LIBS})

include(GLBaseDependencies)
include(GLBaseCopyDependencies)
include(VideoInputCopyDependencies)
include(VideoInputDependencies)
include(Gtk3Include)
include(ResCompInclude)
include(PugiXMLInclude)
include(UtilitiesDependencies)

#gen_android_proj(${PROJECT_NAME} 1)
