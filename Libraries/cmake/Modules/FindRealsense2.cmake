if (UNIX OR APPLE)
	if (REALSENSE2_LIBRARIES AND REALSENSE2_INCLUDE_DIRS)
		# in cache already
		set(REALSENSE2_FOUND TRUE)
	else ()
		find_path(REALSENSE2_INCLUDE_DIR
				NAMES
				librealsense2/rs.h
				PATHS
				/usr/include
				/usr/local/include
				/opt/local/include
				/opt/homebrew/include
				/sw/include
				)

		find_library(REALSENSE2_LIBRARY
				NAMES
				realsense2
				PATHS
				/usr/lib
				/usr/local/lib
				/opt/local/lib
				/opt/homebrew/lib
				/sw/lib
				)

		set(REALSENSE2_INCLUDE_DIRS ${REALSENSE2_INCLUDE_DIR})
		set(REALSENSE2_LIBRARIES ${REALSENSE2_LIBRARY})

		if (REALSENSE2_INCLUDE_DIRS AND REALSENSE2_LIBRARIES)
			set(REALSENSE2_FOUND TRUE)
		endif()

		if (REALSENSE2_FOUND)
			if (NOT REALSENSE2_FIND_QUIETLY)
				#message(STATUS "Found librealsense2: ${REALSENSE2_LIBRARIES}")
			endif ()
		else ()
			if (REALSENSE2_FIND_REQUIRED)
				message(FATAL_ERROR "Could not find librealsense2")
			endif ()
		endif ()

	endif()
endif()