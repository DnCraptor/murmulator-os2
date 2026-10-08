# MOS standalone relocatable application, modelled on apps/cmd.
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(PICO_BOARD pico CACHE STRING "Board type")
cmake_minimum_required(VERSION 3.22)
include(${CMAKE_CURRENT_LIST_DIR}/../../pico_sdk_import.cmake)
project(tcc C CXX ASM)
pico_sdk_init()
set(CMAKE_C_STANDARD 11)
set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/../../bin/${CMAKE_BUILD_TYPE}")
set(COMPILED_DIR "${CMAKE_SOURCE_DIR}/../../apps/compiled")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../libs/runtime" "${CMAKE_CURRENT_BINARY_DIR}/runtime")
# ONE_SOURCE: tcc.c includes libtcc.c and tcctools.c; libtcc.c includes backend.
add_executable(tcc tcc.c mos_time.c)
target_include_directories(tcc PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../api/libc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../api/posix
    ${CMAKE_CURRENT_SOURCE_DIR}/../../freertos_config
    ${CMAKE_CURRENT_SOURCE_DIR}/../../api/FreeRTOS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../api
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_definitions(tcc PRIVATE NO_SEMAPHORE_H=1)
target_link_libraries(tcc PRIVATE murmulator_runtime)
target_compile_options(tcc PRIVATE -fno-jump-tables -Wno-error -Wno-unused-function)
target_link_options(tcc PRIVATE -Xlinker --print-memory-usage -r)
pico_set_program_name(tcc "Murmulator OS Applications: tcc")
pico_set_program_version(tcc "bootstrap")
pico_enable_stdio_uart(tcc 0)
pico_enable_stdio_usb(tcc 0)
# Only compiled binaries go into apps/compiled. Headers are installed from
# their original source paths by tools/install_tcc.py and install.json.
add_custom_target(tcc_binaries ALL
    COMMAND ${CMAKE_COMMAND} "-DNM=${CMAKE_NM}" "-DELF=$<TARGET_FILE:tcc>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/check_undefined.cmake"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${COMPILED_DIR}/bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:tcc> "${COMPILED_DIR}/bin/tcc"
    COMMAND ${CMAKE_COMMAND} -E rm -f "${COMPILED_DIR}/tcc"
    DEPENDS tcc
    VERBATIM
)

