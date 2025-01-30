project(unittest C)

# ''Packages''.

find_package(Threads REQUIRED)
if(NOT THREADS_FOUND)
	message(FATAL_ERROR "Threads not found")
endif()

# ''Source files''.

set(SRC
	common.c
	diagnostics.c
	ta_load.c
	main.c
)

# ''Built binary''.

add_executable(${PROJECT_NAME} ${SRC})

# ''Headers and dependencies''.

target_include_directories(${PROJECT_NAME}
	PRIVATE src
	PRIVATE ../libqcomtee/src
)

target_link_libraries(${PROJECT_NAME}
	PRIVATE libqcomtee
	PRIVATE ${CMAKE_THREAD_LIBS_INIT}
)
