cmake_minimum_required(VERSION 3.10)

project(UI)

file(GLOB_RECURSE UI_SRC "*.cpp")

# This script generates UINodeFactory.h by including all headers in INPUT_DIR and registering classes.
#file(GLOB_RECURSE HEADER_SUB_FILES "${CMAKE_CURRENT_SOURCE_DIR}/UIElements/*/*.h")
file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/UIElements/*.h")
list(APPEND HEADER_FILES ${HEADER_SUB_FILES})

set(OUTPUT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/UINodeFactory.h")
set(UINODE_FACTORY_CONTENT "#pragma once
")


foreach(HEADER_FILE ${HEADER_FILES})
    string(FIND ${HEADER_FILE} "UIElements/" SUB_PATH_POS)
    string(LENGTH ${HEADER_FILE} header_path_length)
    string(SUBSTRING ${HEADER_FILE} ${SUB_PATH_POS} ${header_path_length} HEADER_REL_PATH)
    list(APPEND UINODE_FACTORY_CONTENT "#include <${HEADER_REL_PATH}>
")
endforeach()

list(APPEND UINODE_FACTORY_CONTENT "
namespace ara {
static void registerDefaultUITypes() {
")

set(EXCLUDE_CLASSES
    LabelPars
    ListItem
    ListBase
    List
    PList
    Scene3D
    UINodeGeom
    UINodeHID
    UINodeStyle
    UINodePars
    UITableParameters
)

foreach(headerFile ${HEADER_FILES})
    file(READ ${headerFile} headerText)

    string(REPLACE "\n" ";" line_list ${headerText})
    set(CURRENT_NAMESPACE "")
    foreach(line IN LISTS line_list)
        if (NOT line STREQUAL "")
            string(REGEX MATCH "namespace.[(A-z)|(a-z)|(0-9)]*" nmsp ${line})

            if (NOT nmsp STREQUAL "")
                string(LENGTH ${nmsp} var_length)
                string(SUBSTRING ${nmsp} 10 ${var_length} CURRENT_NAMESPACE)
            endif ()

            string(REGEX MATCH "class.[(A-z)|(a-z)|(0-9)]*" class_name ${line})
            string(REGEX MATCH "enum*" contains_enum ${line})
            if (NOT class_name STREQUAL "" AND NOT class_name STREQUAL "class T"  AND NOT class_name STREQUAL "classes")
                string(LENGTH ${line} line_length)
                string(LENGTH ${class_name} class_length)
                if (NOT line_length EQUAL class_length AND contains_enum STREQUAL "")
                    string(REPLACE "class " "" class_name "${class_name}")
                    list(FIND EXCLUDE_CLASSES ${class_name} EXCLUDE_CLASS_INDEX)
                    if (EXCLUDE_CLASS_INDEX EQUAL -1)
                        string(APPEND UINODE_FACTORY_CONTENT "  Node::m_factory.registerClass(getTypeName<${class_name}>(), [&] { return std::make_shared<${class_name}>()\; })\;
")
                    endif()
                endif ()
            endif ()
        endif()
    endforeach()
endforeach()

list(APPEND UINODE_FACTORY_CONTENT "
}
}
")

file(WRITE "${OUTPUT_FILE}" ${UINODE_FACTORY_CONTENT})


add_library(${PROJECT_NAME} OBJECT ${UI_SRC})
add_library(ara::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
target_compile_definitions(${PROJECT_NAME} PUBLIC ${ara_sdk_COMPILE_DEFINITIONS})
target_include_directories(${PROJECT_NAME} PUBLIC ${ara_sdk_INCLUDE_DIRS})