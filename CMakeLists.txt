cmake_minimum_required(VERSION 3.4)
project(qcom-tee C)

option(BUILD_UNITTEST "Build unittest" FALSE)

include(GNUInstallDirs)

add_compile_options(
	-Wall -Wextra -Werror -Wshadow -Wcast-align
	-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations
	-Wformat-security -Wbad-function-cast -Wmissing-noreturn
	-Wdeprecated -Wswitch-default -fPIC
)

add_subdirectory(libqcomtee)

if(BUILD_UNITTEST)
	add_subdirectory(tests)
endif()
