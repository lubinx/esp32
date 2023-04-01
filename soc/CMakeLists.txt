idf_component_register(
    INCLUDE_DIRS
        "${IDF_TARGET}"
    SRCS
)
target_linker_script(${COMPONENT_LIB} INTERFACE "${IDF_TARGET}/peripherals.ld")