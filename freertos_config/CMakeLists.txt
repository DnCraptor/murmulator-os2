# freertos_config/CMakeLists.txt
add_library(freertos_config INTERFACE)

target_include_directories(freertos_config SYSTEM
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR} # Папка, где лежит FreeRTOSConfig.h
)

target_compile_definitions(freertos_config
    INTERFACE
        projCOVERAGE_TEST=0
)
