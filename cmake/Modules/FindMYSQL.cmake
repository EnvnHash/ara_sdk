# - Find mysqlclient
# Find the native MySQL includes and library
#
#  MYSQL_INCLUDE_DIR - where to find mysql.h, etc.
#  MYSQL_LIBRARIES   - List of libraries when using MySQL.
#  MYSQL_FOUND       - True if MySQL found.
#
# Based on: http://www.itk.org/Wiki/CMakeUserFindMySQL


#-------------- FIND MYSQL_INCLUDE_DIR ------------------
FIND_PATH(MYSQL_INCLUDE_DIR mysql.h
  /usr/include/mysql
  /usr/local/include/mysql
  /opt/mysql/mysql/include
  /opt/mysql/mysql/include/mysql
  /opt/mysql/include
  /opt/local/include/mysql5
  /usr/local/mysql/include
  /usr/local/mysql/include/mysql
  $ENV{ProgramFiles}/MySQL/*/include
  $ENV{SystemDrive}/MySQL/*/include)

#----------------- FIND MYSQL_LIB_DIR -------------------
IF (WIN32)
  # Set lib path suffixes
  # dist = for mysql binary distributions
  # build = for custom built tree
  IF (CMAKE_BUILD_TYPE STREQUAL Debug)
    SET(libsuffixDist debug)
    SET(libsuffixBuild Debug)
  ELSE (CMAKE_BUILD_TYPE STREQUAL Debug)
    SET(libsuffixDist opt)
    SET(libsuffixBuild Release)
    ADD_DEFINITIONS(-DDBUG_OFF)
  ENDIF (CMAKE_BUILD_TYPE STREQUAL Debug)

  FIND_LIBRARY(MYSQL_LIB NAMES mysqlclient
    PATHS
    $ENV{MYSQL_DIR}/lib/${libsuffixDist}
    $ENV{MYSQL_DIR}/libmysql
    $ENV{MYSQL_DIR}/libmysql/${libsuffixBuild}
    $ENV{MYSQL_DIR}/client/${libsuffixBuild}
    $ENV{MYSQL_DIR}/libmysql/${libsuffixBuild}
    $ENV{ProgramFiles}/MySQL/*/lib/${libsuffixDist}
    $ENV{SystemDrive}/MySQL/*/lib/${libsuffixDist})
ELSE (WIN32)
  FIND_LIBRARY(MYSQL_LIB NAMES mysqlclient_r mysqlclient
    PATHS
    /usr/lib/mysql
    /usr/lib/x86_64-linux-gnu
    /usr/local/lib/mysql
    /usr/local/mysql/lib
    /usr/local/mysql/lib/mysql
    /opt/local/mysql5/lib
    /opt/local/lib/mysql5/mysql
    /opt/mysql/mysql/lib/mysql
    /opt/mysql/lib/mysql)
ENDIF (WIN32)

IF(MYSQL_LIB)
  GET_FILENAME_COMPONENT(MYSQL_LIB_DIR ${MYSQL_LIB} PATH)
ENDIF(MYSQL_LIB)

IF (MYSQL_INCLUDE_DIR AND MYSQL_LIB_DIR)
  SET(MYSQL_FOUND TRUE)

  INCLUDE_DIRECTORIES(${MYSQL_INCLUDE_DIR})
  LINK_DIRECTORIES(${MYSQL_LIB_DIR})

  FIND_LIBRARY(MYSQL_ZLIB zlib PATHS ${MYSQL_LIB_DIR})
  FIND_LIBRARY(MYSQL_TAOCRYPT taocrypt PATHS ${MYSQL_LIB_DIR})
  IF (MYSQL_LIB)
    SET(MYSQL_CLIENT_LIBS ${MYSQL_LIB})
  ELSE()
    SET(MYSQL_CLIENT_LIBS mysqlclient_r)
  ENDIF()
  IF (MYSQL_ZLIB)
    SET(MYSQL_CLIENT_LIBS ${MYSQL_CLIENT_LIBS} zlib)
  ENDIF (MYSQL_ZLIB)
  IF (MYSQL_TAOCRYPT)
    SET(MYSQL_CLIENT_LIBS ${MYSQL_CLIENT_LIBS} taocrypt)
  ENDIF (MYSQL_TAOCRYPT)
  # Added needed mysqlclient dependencies on Windows
  IF (WIN32)
    SET(MYSQL_CLIENT_LIBS ${MYSQL_CLIENT_LIBS} ws2_32)
  ENDIF (WIN32)

  MESSAGE(STATUS "MySQL Include dir: ${MYSQL_INCLUDE_DIR}  library dir: ${MYSQL_LIB_DIR}")
  MESSAGE(STATUS "MySQL client libraries: ${MYSQL_CLIENT_LIBS}")
ELSEIF (MySQL_FIND_REQUIRED)
  MESSAGE(FATAL_ERROR "Cannot find MySQL. Include dir: ${MYSQL_INCLUDE_DIR}  library dir: ${MYSQL_LIB_DIR}")
ENDIF (MYSQL_INCLUDE_DIR AND MYSQL_LIB_DIR)
