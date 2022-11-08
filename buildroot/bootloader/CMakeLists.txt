idf_component_register(SRCS "entry.c"
    REQUIRES bootloader_support)

idf_build_get_property(IDF_PATH, IDF_PATH)
idf_build_get_property(IDF_TARGET IDF_TARGET)

set(scripts "${IDF_PATH}/components/bootloader/subproject/main/ld/${IDF_TARGET}/bootloader.ld")
list(APPEND scripts "${IDF_PATH}/components/bootloader/subproject/main/ld/${IDF_TARGET}/bootloader.rom.ld")

target_linker_script(${COMPONENT_LIB} INTERFACE "${scripts}")
