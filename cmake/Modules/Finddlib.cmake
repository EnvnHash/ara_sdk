# - Find Dlib
# Find the native Dlib includes and library
#
#  DLIB_INCLUDE_DIR - where to find zlib.h, etc.
#  DLIB_LIBRARIES   - List of libraries when using zlib.
#  DLIB_FOUND       - True if zlib found.

IF (NOT DLIB_DIR)
  FIND_PATH (DLIB_DIR DLIBconfig.cmake
    $ENV{DLIB_DIR}
    DOC "The build directory, containing Dlibconfig.cmake")
ENDIF (NOT DLIB_DIR)

IF (DLIB_DIR)
  IF (EXISTS (${DLIB_DIR}/Dlibconfig.cmake))
    INCLUDE (${DLIB_DIR}/Dlibconfig.cmake)
  ENDIF (EXISTS (${DLIB_DIR}/Dlibconfig.cmake))
ENDIF (DLIB_DIR)

IF (DLIB_INCLUDE_DIR)
  # Already in cache, be silent
  SET (Dlib_FIND_QUIETLY TRUE)
ENDIF (DLIB_INCLUDE_DIR)

FIND_PATH(DLIB_INCLUDE_DIR "dlib/algs.h")

SET (DLIB_NAMES dlib)
FIND_LIBRARY (DLIB_LIBRARY NAMES ${DLIB_NAMES})

# handle the QUIETLY and REQUIRED arguments and set DLIB_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE (FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS (DLIB DEFAULT_MSG 
  DLIB_LIBRARY 
  DLIB_INCLUDE_DIR)

IF(DLIB_FOUND)
  SET (DLIB_LIBRARIES ${DLIB_LIBRARY})
ELSE (DLIB_FOUND)
  SET (DLIB_LIBRARIES)
ENDIF (DLIB_FOUND)

MARK_AS_ADVANCED (DLIB_LIBRARY)
