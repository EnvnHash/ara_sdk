if (UNIX AND NOT APPLE AND NOT ANDROID)
	find_package(PkgConfig REQUIRED)
	pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
	if (GTK3_FOUND)
		include_directories(${GTK3_INCLUDE_DIRS})
		link_directories(${GTK3_LIBRARY_DIRS})
		add_definitions(${GTK3_CFLAGS_OTHER})
		target_link_libraries (${PROJECT_NAME} ${GTK3_LIBRARIES})
	endif (GTK3_FOUND)
endif()
