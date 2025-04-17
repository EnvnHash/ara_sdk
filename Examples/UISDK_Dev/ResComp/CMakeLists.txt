cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH ${ARA_SDK_SOURCE_DIR}/Libraries/cmake/Modules)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
CMAKE_POLICY(SET CMP0071 NEW)
set(CMAKE_CXX_STANDARD 17)

include(GeneralSettings)
include(VisualStudioSettings)
 
project(ResComp)

include_directories(
	${CMAKE_SOURCE_DIR}/Libraries/Utilities/src
	${CMAKE_SOURCE_DIR}/Libraries/third_party/nameof
)

set(SOURCES main.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

include(GLBaseDependencies)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
	if(WIN32)
		target_link_libraries(${PROJECT_NAME} GLBase Utilities user32 iphlpapi Strmiids.lib Dwmapi.lib)
	else()
		target_link_libraries(${PROJECT_NAME} GLBase Utilities)
	endif()
else()
	if(WIN32)
		target_link_libraries(${PROJECT_NAME} GLBase Utilities user32 iphlpapi Strmiids.lib Dwmapi.lib msvcrt.lib)
	else()
		target_link_libraries(${PROJECT_NAME} GLBase Utilities)
	endif()
endif()

INSTALL(TARGETS ${PROJECT_NAME}
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR} COMPONENT ${PROJECT_NAME} OPTIONAL)