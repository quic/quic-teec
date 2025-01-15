project(libqcomtee
	VERSION 0.1.0
	LANGUAGES C
)

# ''Packages''.

# Append modules directory.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(QCBOR_DIR_HINT "" CACHE PATH "Hint path for QCBOR directory")

find_package(QCBOR REQUIRED)
if(NOT QCBOR_FOUND)
	message(FATAL_ERROR "QCBOR not found")
endif()

include_directories(${QCBOR_INCLUDE_DIRS})

# ''Configurations''.

option(CFG_USE_SYSLOG "Use syslog" FALSE)

# ''Source files''.

set(SRC
        src/qcomtee_object.c
        src/objects/credentials_obj.c
)

add_library(qcomtee ${SRC})

# ''Library pkg-config file and version''.

set(libqcomteetgt qcomtee)

configure_file(qcomtee.pc.in qcomtee.pc @ONLY)

set_target_properties(qcomtee PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
)

# ''Compile flags''.

if(CFG_USE_SYSLOG)
        target_compile_definitions(qcomtee PRIVATE -DOE)
endif()

# ''Headers and dependencies''.

target_include_directories(qcomtee
	PUBLIC include
	PRIVATE src
)

target_link_libraries(qcomtee
	PRIVATE ${QCBOR_LIBRARIES}
)

# ''Install targets''.

install(TARGETS qcomtee
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(DIRECTORY include
	DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qcomtee.pc"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
