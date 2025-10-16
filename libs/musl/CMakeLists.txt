add_library(murmulator_stdlib INTERFACE)

file(GLOB_RECURSE MURMULATOR_STDLIB_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/*.c
    ${CMAKE_CURRENT_LIST_DIR}/src/*.s
    ${CMAKE_CURRENT_LIST_DIR}/src/*.S
)

target_sources(murmulator_stdlib INTERFACE
    ${MURMULATOR_STDLIB_SOURCES}
)

target_link_libraries(murmulator_stdlib INTERFACE)

target_include_directories(murmulator_stdlib INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)
