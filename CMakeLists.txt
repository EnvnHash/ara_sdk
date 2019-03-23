# CMakeLists files in this project can
# refer to the root source directory of the project as ${HELLO_SOURCE_DIR} and
# to the root binary directory of the project as ${HELLO_BINARY_DIR}.
cmake_minimum_required (VERSION 2.8.11)
project (TAV)

option (WITH_AUDIO "Build Tav Audio Module" ON)
option (WITH_TRACKING "Build Tav Tracking Module" ON)
option (WITH_VIDEO "Build Tav Video Module" ON)
option (WITH_SN_AUDIO "Build basic audio SceneNodes" OFF)
option (WITH_SN_COMPLEX "Build complex (project) SceneNodes" OFF)
option (WITH_SN_TESTING "Build testing SceneNodes" ON)
option (WITH_SN_TRACKING "Build basic tracking SceneNodes" OFF)
option (WITH_SN_VIDEO "Build basic video SceneNodes" OFF)


# Recurse into the "Hello" and "Demo" subdirectories. This does not actually
# cause another cmake executable to run. The same process will walk through
# the project's entire directory structure.
add_subdirectory (tav_core)
if (WITH_AUDIO)
  add_subdirectory (tav_audio)
endif(WITH_AUDIO)
if (WITH_VIDEO)
  add_subdirectory (tav_video)
endif(WITH_VIDEO)
if (WITH_TRACKING)
  add_subdirectory (tav_tracking)
endif(WITH_TRACKING)
if(WITH_SN_AUDIO)
  add_subdirectory (tav_sn_audio)
endif(WITH_SN_AUDIO)
if(WITH_SN_COMPLEX)
  add_subdirectory (tav_sn_complex)
endif(WITH_SN_COMPLEX)
if(WITH_SN_TESTING)
  add_subdirectory (tav_sn_testing)
endif(WITH_SN_TESTING)
if(WITH_SN_TRACKING)
  add_subdirectory (tav_sn_tracking)
endif(WITH_SN_TRACKING)
if(WITH_SN_VIDEO)
  add_subdirectory (tav_sn_video)
endif(WITH_SN_VIDEO)


add_subdirectory (tav_app)

