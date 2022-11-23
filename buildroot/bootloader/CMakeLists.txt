idf_component_register(
    SRCS
        "startup.c"
    REQUIRES
        bootloader_support
)

idf_build_get_property(IDF_TARGET IDF_TARGET)

list(APPEND scripts
    "ld/${IDF_TARGET}/bootloader.ld"
    "ld/${IDF_TARGET}/bootloader.rom.ld"
)
target_linker_script(${COMPONENT_LIB} INTERFACE "${scripts}")
