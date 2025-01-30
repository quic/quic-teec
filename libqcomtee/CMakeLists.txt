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

# ''Source files''.

set(SRC
        src/qcomtee_object.c
        src/objects/credentials_obj.c
)

add_library(${PROJECT_NAME} ${SRC})

# ''Library pkg-config file and version''.

set(pc_req_private qcbor)
set(libqcomteetgt qcomtee)

configure_file(qcomtee.pc.in qcomtee.pc @ONLY)

set_target_properties(${PROJECT_NAME} PROPERTIES
	VERSION ${PROJECT_VERSION}
	SOVERSION ${PROJECT_VERSION_MAJOR}
)

# ''Headers and dependencies''.

target_include_directories(${PROJECT_NAME}
	PUBLIC include
	PRIVATE src
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE ${QCBOR_LIBRARIES}
)

# ''Install targets''.

install(TARGETS ${PROJECT_NAME}
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(DIRECTORY include
	DESTINATION ${CMAKE_INSTALL_PREFIX}
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/qcomtee.pc"
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
