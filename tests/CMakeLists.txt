project(unittest C)

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
)
