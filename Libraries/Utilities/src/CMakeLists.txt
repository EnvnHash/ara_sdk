cmake_minimum_required(VERSION 3.10)
project(Utilities VERSION 0.1 LANGUAGES CXX)

if (COMPILE_SCENEGRAPH_LIB)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif ()

include(VisualStudioSettings)
include(GeneralSettings)
include(CMakeRC)
include(UtilitiesDepInclude)

file(GLOB COMMON_SRC
        DataModel/Project.cpp
        DataModel/Workspace.cpp
        DataModel/Item.cpp
        DataModel/Node.cpp
        DataModel/NodeColor.cpp
        DataModel/NodeImage.cpp
        Network/RestClient.cpp
        Network/TCPCmdListener.cpp
        Network/TCPLineCmdRequest.cpp
        Network/UDPReceiver.cpp
        Network/UDPReceiver.cpp
        Network/UDPSender.cpp
        Network/TCPListener.cpp
        Network/MicroHttpConnection.cpp
        Network/MicroHttpContent.cpp
        Network/MicroHttpContentRaw.cpp
        Network/MicroHttpServer.cpp
        Network/WebSocketClient.cpp
        ThreadedTasks/Cycler.cpp
        AnimVal.cpp
        Align.cpp
        AssetLoader.cpp
        RwBinFile.cpp
        string_utils.cpp
        Table.cpp
        MappedFile.cpp
        ${ARA_SDK_SOURCE_DIR}/Libraries/third_party/easywsclient/easywsclient.cpp
)

if (ANDROID)
    file(GLOB ANDROID_UTIL_SRC
            Android/cardboard/Cardboard.cpp
            Android/cardboard/DeviceParams.cpp
            Android/cardboard/jni_utils.cpp
            Android/cardboard/cardboard_v1.cpp
            Android/cardboard/DistortionMesh.cpp
            Android/cardboard/ScreenParams.cpp
            Android/cardboard/LensDistortion.cpp
            Android/cardboard/PolynomialRadialDistortion.cpp
    )
    list(APPEND COMMON_SRC ${ANDROID_UTIL_SRC})
endif ()

if (WIN32)
    file(GLOB UTILITIES_SRC ${COMMON_SRC}
            OSFileDialog.cpp
            Windows/Registry.cpp
            Network/WinSockWrapper.cpp
            Windows/OSDisplayInfo.cpp
    )
elseif (APPLE)
    file(GLOB UTILITIES_SRC ${COMMON_SRC} OSFileDialog.cpp)
elseif (ANDROID)
    file(GLOB UTILITIES_SRC ${COMMON_SRC})
else ()
    file(GLOB UTILITIES_SRC ${COMMON_SRC} OSFileDialog.cpp src/X11/*.cpp)
endif ()

#header files this is just to have them m_show up in the visual studio project
if (WIN32)
    add_compile_definitions(DLL_EXPORT UNICODE _UNICODE)
endif ()

add_library(${PROJECT_NAME} OBJECT ${UTILITIES_SRC})
add_library(ara::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

include(Gtk3Include)
include(PugiXMLInclude)