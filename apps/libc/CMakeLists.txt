# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 11_3_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.22)
include(pico_sdk_import.cmake)

project(libc C CXX ASM)
pico_sdk_init()

SET(BUILD_NAME "${PROJECT_NAME}")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/../../bin/${CMAKE_BUILD_TYPE}")
set(COMPILED_DIR "${CMAKE_SOURCE_DIR}/../../apps/compiled")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

# INCLUDE FILES THAT SHOULD BE COMPILED:
file(GLOB_RECURSE SRC "*.cpp" "*.c")

message(STATUS "Add source files:")
foreach (SRC_FILE IN LISTS SRC)
    message(STATUS "${SRC_FILE}")
endforeach ()
message(STATUS "")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../libs/musl" "${CMAKE_CURRENT_BINARY_DIR}/libc")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../libs/runtime" "${CMAKE_CURRENT_BINARY_DIR}/runtime")

add_executable(${PROJECT_NAME}
    libc.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
    # workarounds
    ${CMAKE_CURRENT_SOURCE_DIR}/../../api/wa
    # MUSL
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/musl/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/musl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/musl/arch/arm
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/musl/arch/generic
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libs/musl/src/internal
    # POSIX.1
    ${CMAKE_CURRENT_SOURCE_DIR}/../../api/posix
    # GCC toolchain
    ${CMAKE_CURRENT_SOURCE_DIR}/../../api/gcc/toolchain/14_2_Rel1/arm-none-eabi/include
    # for your program
    ${CMAKE_CURRENT_SOURCE_DIR}
)


target_link_libraries(${PROJECT_NAME} PRIVATE
    murmulator_stdlib
    murmulator_runtime
)

#family_configure_device_example(${PROJECT_NAME} noos)
# -fPIC -shared
target_link_options(${PROJECT_NAME} PRIVATE
    -Xlinker --print-memory-usage
    -r
    -Wl,--allow-multiple-definition
)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
target_compile_options(${PROJECT_NAME} PUBLIC
        -fno-jump-tables  # suppress __gnu_thumb1_case_uni symbol generation
        -Wno-error=suggest-attribute=format
        -Wno-error=cast-qual
        -Wno-error=unused-parameter
        -Wno-error=conversion
        -Wno-error=format=
        -Wno-error=sign-compare
        -Wno-error=missing-field-initializers
        -Wno-error=switch
        -Wno-error=implicit-fallthrough=
        -Wno-error=stringop-truncation
        -Wno-error=restrict
        -w
)
endif ()


#| Значение `_POSIX_C_SOURCE` | Версия стандарта | Основные возможности                                |
#| -------------------------- | ---------------- | --------------------------------------------------- |
#| `1`                        | POSIX.1-1990     | Базовые функции (`read`, `write`, `stat`, `fork` …) |
#| `199309L`                  | POSIX.1b-1993    | Реальное время, `sched_*`, `sem_*`                  |
#| `199506L`                  | POSIX.1c-1995    | Потоки (`pthread_*`)                                |
#| `200112L`                  | POSIX.1-2001     | Совместимость с SUSv3                               |
#| `200809L`                  | POSIX.1-2008     | Совместимость с SUSv4                               |
target_compile_definitions(${PROJECT_NAME} PRIVATE
    _POSIX_SOURCE
    _POSIX_C_SOURCE=1
)

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${BUILD_NAME}")

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rename
        $<TARGET_FILE:${PROJECT_NAME}>
        ${COMPILED_DIR}/${BUILD_NAME}
)
