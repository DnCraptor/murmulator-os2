cmake_minimum_required(VERSION 3.22)

# Can be configured directly, or included by an application build.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(PICO_BOARD pico CACHE STRING "Board type")
    include("${CMAKE_CURRENT_LIST_DIR}/../../pico_sdk_import.cmake")
    project(mos_runtime C CXX ASM)
    pico_sdk_init()
endif()

add_library(murmulator_runtime INTERFACE)

target_sources(murmulator_runtime INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/__aeabi.c
  ${CMAKE_CURRENT_LIST_DIR}/ftime.c
  ${CMAKE_CURRENT_LIST_DIR}/setjmp.S
  ${CMAKE_CURRENT_LIST_DIR}/strto.c
  ${CMAKE_CURRENT_LIST_DIR}/strtod.c
  ${CMAKE_CURRENT_LIST_DIR}/ldexp.c
  ${CMAKE_CURRENT_LIST_DIR}/time.c
  ${CMAKE_CURRENT_LIST_DIR}/gettimeofday.c
  ${CMAKE_CURRENT_LIST_DIR}/localtime.c
)

target_link_libraries(murmulator_runtime INTERFACE)

target_include_directories(murmulator_runtime INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)

# The archive and its publication belong to runtime, not to any application.
get_filename_component(MOS_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_library(mos_runtime_archive STATIC)
target_link_libraries(mos_runtime_archive PRIVATE murmulator_runtime)
target_include_directories(mos_runtime_archive PRIVATE
    "${MOS_ROOT}/api/libc"
    "${MOS_ROOT}/api/posix"
    "${MOS_ROOT}/freertos_config"
    "${MOS_ROOT}/api/FreeRTOS"
    "${MOS_ROOT}/api"
)
target_compile_features(mos_runtime_archive PRIVATE c_std_11)
target_compile_options(mos_runtime_archive PRIVATE -fno-jump-tables)
set_target_properties(mos_runtime_archive PROPERTIES
    OUTPUT_NAME mos
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib"
)
add_custom_target(runtime_binaries ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${MOS_ROOT}/apps/compiled/lib"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:mos_runtime_archive> "${MOS_ROOT}/apps/compiled/lib/libmos.a"
    DEPENDS mos_runtime_archive
    VERBATIM
)
