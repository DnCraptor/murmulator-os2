add_library(murmulator_stdlib INTERFACE)

target_sources(murmulator_stdlib INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/libc.c
	${CMAKE_CURRENT_LIST_DIR}/stdio/printf.c
	${CMAKE_CURRENT_LIST_DIR}/stdio/putchar.c
	${CMAKE_CURRENT_LIST_DIR}/stdio/puts.c
	${CMAKE_CURRENT_LIST_DIR}/stdio/vfprintf.c
	${CMAKE_CURRENT_LIST_DIR}/stdio/vprintf.c
	${CMAKE_CURRENT_LIST_DIR}/string/strlen.c
)

target_link_libraries(murmulator_stdlib INTERFACE)

target_include_directories(murmulator_stdlib INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)
