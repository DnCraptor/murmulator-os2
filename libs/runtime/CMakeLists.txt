add_library(murmulator_runtime INTERFACE)

target_sources(murmulator_runtime INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}/__aeabi.c
)

target_link_libraries(murmulator_runtime INTERFACE)

target_include_directories(murmulator_runtime INTERFACE
  ${CMAKE_CURRENT_LIST_DIR}
)
