idf_component_register(
    SRCS
        "startup.c"
    REQUIRES
        "hal"
)

idf_build_get_property(IDF_TARGET IDF_TARGET)

list(APPEND scripts
    "ld/${IDF_TARGET}.ld"
    "${IDF_PATH}/components/bootloader/subproject/main/ld/${IDF_TARGET}/bootloader.rom.ld"
)
target_linker_script(${COMPONENT_LIB} INTERFACE "${scripts}")
