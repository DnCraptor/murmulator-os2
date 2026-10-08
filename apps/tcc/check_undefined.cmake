# Relocatable linking (-r) permits undefined symbols; the MOS loader does not.
if(NOT DEFINED NM OR NOT DEFINED ELF)
    message(FATAL_ERROR "TCC symbol check requires NM and ELF")
endif()
execute_process(
    COMMAND "${NM}" --undefined-only "${ELF}"
    RESULT_VARIABLE status
    OUTPUT_VARIABLE undefined_symbols
    ERROR_VARIABLE diagnostic
)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "Cannot inspect TCC ELF: ${diagnostic}")
endif()
string(STRIP "${undefined_symbols}" undefined_symbols)
if(NOT "${undefined_symbols}" STREQUAL "")
    message(FATAL_ERROR "TCC has unresolved symbols (MOS cannot load it):\n${undefined_symbols}")
endif()
