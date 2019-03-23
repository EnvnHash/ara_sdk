#  UCDN_FOUND - system has ucdn
#  UCDN_INCLUDE_DIRS - the ucdn include directory
#  UCDN_LIBRARY - Link these to use ucdn
#  UCDN_LIBRARIES



find_path(UCDN_INCLUDE_DIRS NAMES ucdn.h
	HINTS
	/usr/local/include/
	/usr/include
)
 
find_library(UCDN_LIBRARY NAMES ucdn)

if(UCDN_INCLUDE_DIRS AND UCDN_LIBRARY)
  set(UCDN_FOUND TRUE)
endif()

if(UCDN_LIBRARY)
    set(UCDN_LIBRARY ${UCDN_LIBRARY})
endif()

if (UCDN_FOUND)
  mark_as_advanced(UCDN_INCLUDE_DIRS UCDN_LIBRARY UCDN_LIBRARIES)
endif()
