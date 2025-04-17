project(Assets)
include(CMakeRC)

file(GLOB_RECURSE RESOURCES ${ARA_SDK_SOURCE_DIR}/Assets/resdata/* )
cmrc_add_resource_library(resources ALIAS ara::rc NAMESPACE ara ${RESOURCES})
