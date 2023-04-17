idf_component_register(
    INCLUDE_DIRS
        "${IDF_TARGET}/include"
    SRCS
        "${IDF_TARGET}/soc.c"
)
target_linker_script(${COMPONENT_LIB} INTERFACE "${IDF_TARGET}/peripherals.ld")
