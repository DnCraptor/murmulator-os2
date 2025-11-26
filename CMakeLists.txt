# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 13_3_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2 CACHE STRING "Board type")
set(FREERTOS_KERNEL_PATH "../FreeRTOS-Kernel")
set(FREERTOS_PORT GCC_ARM_CM33_NTZ_NONSECURE CACHE STRING "FreeRTOS port")

cmake_minimum_required(VERSION 3.22)

option(VGA "Enable VGA" OFF)
option(TFT "Enable TFT display" OFF)
option(ILI9341 "Enable TFT ILI9341 display" OFF)
option(HDMI "Enable HDMI display" OFF)
option(TV "Enable TV composite output" OFF)
option(SOFTTV "Enable TV soft composite output" OFF)
option(HID "Enable HID devices (USB drive is unsupported in this mode)" OFF)

#set(HID ON)

set(MURM1x ON)
#set(MURM20 ON)
#set(ZERO2 ON)
#set(PICO_PC ON)
#set(PICO_DV ON)

#set(HID ON)
#set(VGA ON)
set(VGAHDMI ON)
#set(TFT ON)
#set(CPU_MHZ 252)
#set(CPU_MHZ 352)
#set(CPU_MHZ 366)
set(CPU_MHZ 378)
#set(CPU_MHZ 424)
set(FLASH_FREQ_MHZ "88")
set(PSRAM_FREQ_MHZ "88")

IF(ZERO2)
    # For Board RP2350-PiZero
    set(PICO_BOARD_HEADER_DIRS ${CMAKE_SOURCE_DIR}/src/boards)
    set(PICO_BOARD waveshare_rp2350_pizero)
    set(PICO_PLATFORM rp2350-arm-s)
    set(CPU_MHZ 252)
elseif(ZERO)
    set(PICO_BOARD pico)
    set(CPU_MHZ 252)
endif()

set(FLASH_SIZE 16384)
set(FIRMWARE_OFFSET 64)

if(NOT MOS_VERSION)
set(MOS_VERSION "2.2.1")
endif ()

include(pico_sdk_import.cmake)

project(murmulator-os C CXX ASM)
pico_sdk_init()

include(FreeRTOS_Kernel_import.cmake)

if(MURM20)
    SET(BUILD_NAME "m2p2-${PROJECT_NAME}")
elseif(PICO_PC)
    SET(BUILD_NAME "PCp2-${PROJECT_NAME}")
    set(HID ON)
elseif(PICO_DV)
    SET(BUILD_NAME "DVp2-${PROJECT_NAME}")
    set(HID ON)
elseif(ZERO2)
    SET(BUILD_NAME "z0p2-${PROJECT_NAME}")
    set(HID ON)
else()
    SET(BUILD_NAME "m1p2-${PROJECT_NAME}")
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(OUTPUT_DIR "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

add_subdirectory(freertos_config)
add_subdirectory(FreeRTOS-Kernel)

add_subdirectory(drivers/audio)
if (HID)
    add_subdirectory(drivers/ps2kbd)
    # INCLUDE FILES THAT SHOULD BE COMPILED:
    file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c" "drivers/usbhost/*.c" "drivers/usbhost/*.cpp")
else ()
    add_subdirectory(drivers/ps2)
    # INCLUDE FILES THAT SHOULD BE COMPILED:
    file(GLOB_RECURSE SRC "src/*.cpp" "src/*.c" "drivers/usb/*.c")
endif()
add_subdirectory(drivers/fatfs)
add_subdirectory(drivers/sdcard)
add_subdirectory(drivers/nespad)
add_subdirectory(drivers/psram)

add_subdirectory(drivers/vga-nextgen)
add_subdirectory(drivers/st7789)
add_subdirectory(drivers/hdmi)
add_subdirectory(drivers/tv)
add_subdirectory(drivers/tv-software)
add_subdirectory(drivers/graphics)

message(STATUS "Add source files:")
foreach (SRC_FILE IN LISTS SRC)
    message(STATUS "${SRC_FILE}")
endforeach ()
message(STATUS "")

add_executable(${PROJECT_NAME} ${SRC})
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/freertos_config
    ${CMAKE_CURRENT_SOURCE_DIR}/src/posix.1/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/musl/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/obsd/include
)
if (HID)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/usbhost
    )
endif()

configure_file(memmap.ld.in memmap.ld @ONLY)
pico_set_linker_script(${PROJECT_NAME} ${CMAKE_CURRENT_BINARY_DIR}/memmap.ld)

pico_set_program_name(${PROJECT_NAME} "Murmulator OS ${MOS_VERSION}")
pico_set_program_version(${PROJECT_NAME} MOS_VERSION)

target_link_libraries(${PROJECT_NAME} PRIVATE
        FreeRTOS-Kernel
        FreeRTOS-Kernel-Heap4

        psram
        audio
        sdcard
        fatfs
        nespad

        graphics

        pico_runtime
        pico_stdlib
        hardware_pio
        hardware_pwm

        pico_multicore
        hardware_flash

        tinyusb_board
)
if (HID)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HID=1)
    target_link_libraries(${PROJECT_NAME} PRIVATE ps2kbd tinyusb_host)
else ()
    target_link_libraries(${PROJECT_NAME} PRIVATE ps2 tinyusb_device)
endif()

if (HDMI)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEFAULT_VIDEO_DRIVER=HDMI_DRV)
elseif(VGA)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEFAULT_VIDEO_DRIVER=VGA_DRV)
elseif(VGAHDMI)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEFAULT_VIDEO_DRIVER=VGA_DRV)
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEFAULT_VIDEO_DRIVER=RGB_DRV)
endif()

#family_configure_device_example(${PROJECT_NAME} noos)
target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --print-memory-usage)
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/drivers/usb
)

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
target_compile_options(${PROJECT_NAME} PUBLIC
        -fno-builtin-printf
#        -Werror=implicit-function-declaration

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

#target_compile_definitions(${PROJECT_NAME} PRIVATE DEBUG_VGA=1)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    FLASH_FREQ_MHZ=${FLASH_FREQ_MHZ}
    PSRAM_FREQ_MHZ=${PSRAM_FREQ_MHZ}
)


if (MURM20)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PSRAM_NO_FUGE=1
        MURM2=1
        MOS_VERSION_STR="${MOS_VERSION}"

        # VGA 8 pins starts from pin:
        VGA_BASE_PIN=12

        # HDMI 8 pins starts from pin:
        HDMI_BASE_PIN=12

        # TFT
        TFT_CS_PIN=12
        TFT_RST_PIN=14
        TFT_LED_PIN=15
        TFT_DC_PIN=16
        TFT_DATA_PIN=18
        TFT_CLK_PIN=19

        # SDCARD
        SDCARD_PIN_SPI0_CS=5
        SDCARD_PIN_SPI0_SCK=6
        SDCARD_PIN_SPI0_MOSI=7
        SDCARD_PIN_SPI0_MISO=4

        # PS2 keyboard
        KBD_CLOCK_PIN=2
        KBD_DATA_PIN=3

        # NES Gamepad
        USE_NESPAD
        NES_GPIO_CLK=20
        NES_GPIO_LAT=21
        NES_GPIO_DATA=26

#        PSRAM
        # PSRAM_MUTEX=1
        PSRAM_SPINLOCK=1
        PSRAM_ASYNC=1

        PSRAM_PIN_CS=18
        PSRAM_PIN_SCK=19
        PSRAM_PIN_MOSI=20
        PSRAM_PIN_MISO=21

        WAV_IN_PIO=22
        
        #I2S_SOUND
        PWM_PIN0=10
        PWM_PIN1=11
        BEEPER_PIN=9

        M_API_VERSION=26
        MIN_API_VERSION=14

        # do not touch last 512 bytes in RAM on launch app, say, no more RAM
        RESERVED_RAM=512
        OVERCLOCKING=${CPU_MHZ}
        #DEBUG_APP_LOAD=1
        PICO_DEFAULT_LED_PIN=25
    )
elseif (ZERO2)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PSRAM_NO_FUGE=1
        ZERO2=1
        PICO_FLASH_SIZE_BYTES=16777216
        MOS_VERSION_STR="${MOS_VERSION}"

        # VGA 8 pins starts from pin:
        VGA_BASE_PIN=32

        # HDMI 8 pins starts from pin:
        HDMI_BASE_PIN=32

        # TFT
        TFT_CS_PIN=12
        TFT_RST_PIN=14
        TFT_LED_PIN=15
        TFT_DC_PIN=16
        TFT_DATA_PIN=18
        TFT_CLK_PIN=19

        # SDCARD
        SDCARD_SPI_BUS=spi1
        SDCARD_PIN_SPI0_SCK=30
        SDCARD_PIN_SPI0_MOSI=31
        SDCARD_PIN_SPI0_MISO=40
        SDCARD_PIN_SPI0_CS=43

        # PS2 keyboard
        KBD_CLOCK_PIN=2
        KBD_DATA_PIN=3

        # NES Gamepad
        USE_NESPAD
        NES_GPIO_CLK=20
        NES_GPIO_LAT=21
        NES_GPIO_DATA=26

        # PSRAM
        # PSRAM_MUTEX=1
        PSRAM_SPINLOCK=1
        PSRAM_ASYNC=1

        PSRAM_PIN_CS=2
        PSRAM_PIN_SCK=3
        PSRAM_PIN_MOSI=4
        PSRAM_PIN_MISO=5

        WAV_IN_PIO=22
    
        PWM_PIN0=10
        PWM_PIN1=11
        BEEPER_PIN=12

        M_API_VERSION=26
        MIN_API_VERSION=14

        # do not touch last 512 bytes in RAM on launch app, say, no more RAM
        RESERVED_RAM=512
        OVERCLOCKING=${CPU_MHZ}
        #DEBUG_APP_LOAD=1

        PICO_DEFAULT_LED_PIN=25
    )
elseif (PICO_PC)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PSRAM_NO_FUGE=1
        PICO_PC=1
        PICO_FLASH_SIZE_BYTES=16777216
        MOS_VERSION_STR="${MOS_VERSION}"

        # VGA 8 pins starts from pin:
        VGA_BASE_PIN=12

        # HDMI 8 pins starts from pin:
        HDMI_BASE_PIN=12

        # # TFT
        # TFT_CS_PIN=12
        # TFT_RST_PIN=14
        # TFT_LED_PIN=15
        # TFT_DC_PIN=16
        # TFT_DATA_PIN=18
        # TFT_CLK_PIN=19

        # SDCARD
        SDCARD_PIN_SPI0_MISO=4
        SDCARD_PIN_SPI0_SCK=6
        SDCARD_PIN_SPI0_MOSI=7
        SDCARD_PIN_SPI0_CS=22

        # PS2 keyboard
        KBD_CLOCK_PIN=0
        KBD_DATA_PIN=1

        # NES Gamepad
        USE_NESPAD
        NES_GPIO_CLK=5    # UXT1-10
        NES_GPIO_LAT=9    # UXT1-5
        NES_GPIO_DATA=20  # UXT1-3
        NES_GPIO_DATA2=21 # UXT1-4

        # PSRAM
        # PSRAM_MUTEX=1
        PSRAM_SPINLOCK=1
        PSRAM_ASYNC=1

        PSRAM_PIN_CS=3   # not implemented
        PSRAM_PIN_SCK=3  # not implemented
        PSRAM_PIN_MOSI=3 # not implemented
        PSRAM_PIN_MISO=3 # not implemented

        WAV_IN_PIO=14
        
        PWM_PIN0=27
        PWM_PIN1=28
        BEEPER_PIN=26

        M_API_VERSION=26
        MIN_API_VERSION=14

        # do not touch last 512 bytes in RAM on launch app, say, no more RAM
        RESERVED_RAM=512
        OVERCLOCKING=${CPU_MHZ}
        #DEBUG_APP_LOAD=1
        PICO_DEFAULT_LED_PIN=25
    )
elseif (PICO_DV)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PSRAM_NO_FUGE=1
        PICO_DV=1
        PICO_FLASH_SIZE_BYTES=16777216
        MOS_VERSION_STR="${MOS_VERSION}"

        # VGA 8 pins starts from pin:
        VGA_BASE_PIN=6

        # HDMI 8 pins starts from pin:
        HDMI_BASE_PIN=6

        # # TFT
        # TFT_CS_PIN=12
        # TFT_RST_PIN=14
        # TFT_LED_PIN=15
        # TFT_DC_PIN=16
        # TFT_DATA_PIN=18
        # TFT_CLK_PIN=19

        # SDCARD
        SDCARD_SPI_BUS=spi1
        SDCARD_PIN_SPI0_MISO=19
        SDCARD_PIN_SPI0_SCK=5
        SDCARD_PIN_SPI0_MOSI=18
        SDCARD_PIN_SPI0_CS=22

        SDCARD_PIO=pio1
        SDCARD_PIO_SM=0

        # PS2 keyboard
        KBD_CLOCK_PIN=2
        KBD_DATA_PIN=3

        # NES Gamepad
        # USE_NESPAD
        NES_GPIO_CLK=20
        NES_GPIO_LAT=21
        NES_GPIO_DATA=26

        # PSRAM
        # PSRAM_MUTEX=1
        PSRAM_SPINLOCK=1
        PSRAM_ASYNC=1

        PSRAM_PIN_CS=3   # not implemented
        PSRAM_PIN_SCK=3  # not implemented
        PSRAM_PIN_MOSI=3 # not implemented
        PSRAM_PIN_MISO=3 # not implemented

        WAV_IN_PIO=14
        
        PWM_PIN0=26
        PWM_PIN1=27
        BEEPER_PIN=28

        M_API_VERSION=26
        MIN_API_VERSION=14

        # do not touch last 512 bytes in RAM on launch app, say, no more RAM
        RESERVED_RAM=512
        OVERCLOCKING=${CPU_MHZ}
        #DEBUG_APP_LOAD=1
        PICO_DEFAULT_LED_PIN=25
    )
else ()
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        MOS_VERSION_STR="${MOS_VERSION}"

        # VGA 8 pins starts from pin:
        VGA_BASE_PIN=6

        # HDMI 8 pins starts from pin:
        HDMI_BASE_PIN=6

        # TFT
        TFT_CS_PIN=6
        TFT_RST_PIN=8
        TFT_LED_PIN=9
        TFT_DC_PIN=10
        TFT_DATA_PIN=12
        TFT_CLK_PIN=13

        # SDCARD
        SDCARD_PIN_SPI0_SCK=2
        SDCARD_PIN_SPI0_MOSI=3
        SDCARD_PIN_SPI0_MISO=4
        SDCARD_PIN_SPI0_CS=5

        # PS2 keyboard
        KBD_CLOCK_PIN=0
        KBD_DATA_PIN=1

        # NES Gamepad
        NES_GPIO_CLK=14
        NES_GPIO_DATA=16
        NES_GPIO_LAT=15

        PSRAM
        # PSRAM_MUTEX=1
        PSRAM_SPINLOCK=1
        PSRAM_ASYNC=1

        PSRAM_PIN_CS=18
        PSRAM_PIN_SCK=19
        PSRAM_PIN_MOSI=20
        PSRAM_PIN_MISO=21

        WAV_IN_PIO=22
        #I2S_SOUND
        PWM_PIN0=26
        PWM_PIN1=27
        BEEPER_PIN=28

        M_API_VERSION=26
        MIN_API_VERSION=14

        # do not touch last 512 bytes in RAM on launch app, say, no more RAM
        RESERVED_RAM=512
        OVERCLOCKING=${CPU_MHZ}
        #DEBUG_APP_LOAD=1
        PICO_DEFAULT_LED_PIN=25
    )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE FLASH_SIZE=${FLASH_SIZE})
target_compile_definitions(${PROJECT_NAME} PRIVATE FIRMWARE_OFFSET=${FIRMWARE_OFFSET})

IF (TFT)
    if (VGAHDMI)
        target_compile_definitions(${PROJECT_NAME} PRIVATE TFT)
        target_compile_definitions(${PROJECT_NAME} PRIVATE VGA)
        target_compile_definitions(${PROJECT_NAME} PRIVATE HDMI)
        target_link_libraries(${PROJECT_NAME} PRIVATE st7789)
        target_link_libraries(${PROJECT_NAME} PRIVATE vga-nextgen)
        target_link_libraries(${PROJECT_NAME} PRIVATE hdmi)
        target_compile_definitions(${PROJECT_NAME} PRIVATE TFT)
        target_compile_definitions(${PROJECT_NAME} PRIVATE ILI9341)
        SET(BUILD_NAME "${BUILD_NAME}-VGA-HDMI-ILI9341")
    else ()
        target_link_libraries(${PROJECT_NAME} PRIVATE st7789)
        target_compile_definitions(${PROJECT_NAME} PRIVATE TFT)
        SET(BUILD_NAME "${BUILD_NAME}-TFT")
        IF (ILI9341)
            SET(BUILD_NAME "${BUILD_NAME}-ILI9341")
            target_compile_definitions(${PROJECT_NAME} PRIVATE ILI9341)
        ELSE ()
            SET(BUILD_NAME "${BUILD_NAME}-ST7789")
        ENDIF ()
    endif()
ELSEIF (HDMI)
    target_link_libraries(${PROJECT_NAME} PRIVATE hdmi)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HDMI)
    SET(BUILD_NAME "${BUILD_NAME}-HDMI")
ELSEIF (TV)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TV)
    target_link_libraries(${PROJECT_NAME} PRIVATE tv)
    SET(BUILD_NAME "${BUILD_NAME}-TV")
ELSEIF(TV)
    target_compile_definitions(${PROJECT_NAME} PRIVATE TV)
    target_link_libraries(${PROJECT_NAME} PRIVATE tv)
    SET(BUILD_NAME "${BUILD_NAME}-TV")
ELSEIF(SOFTTV)
	target_compile_definitions(${PROJECT_NAME} PRIVATE SOFTTV)
	target_link_libraries(${PROJECT_NAME} PRIVATE tv-software)
	SET(BUILD_NAME "${BUILD_NAME}-TV-SOFT")
ELSEIF (VGA)
    target_compile_definitions(${PROJECT_NAME} PRIVATE VGA)
    target_link_libraries(${PROJECT_NAME} PRIVATE vga-nextgen)
    SET(BUILD_NAME "${BUILD_NAME}-VGA")
ELSE()
    target_compile_definitions(${PROJECT_NAME} PRIVATE VGA)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HDMI)
    target_link_libraries(${PROJECT_NAME} PRIVATE vga-nextgen)
    target_link_libraries(${PROJECT_NAME} PRIVATE hdmi)
    SET(BUILD_NAME "${BUILD_NAME}-VGA-HDMI")
ENDIF()

if (HID)
    SET(BUILD_NAME "${BUILD_NAME}-HID-${MOS_VERSION}-${CPU_MHZ}MHz")
else()
    SET(BUILD_NAME "${BUILD_NAME}-${MOS_VERSION}-${CPU_MHZ}MHz")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${BUILD_NAME}")

pico_enable_stdio_uart(${PROJECT_NAME} 0)
pico_enable_stdio_usb(${PROJECT_NAME} 0)

#pico_set_binary_type(${PROJECT_NAME} no_flash)
#pico_set_binary_type(${PROJECT_NAME} copy_to_ram)

pico_add_extra_outputs(${PROJECT_NAME})
