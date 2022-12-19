idf_component_register(
    INCLUDE_DIRS
        "include"
        "include/posix"
        "include/bootloader_support"
        "include/dummy"
        #  NOTE fix: soc => ${IDF_TARGET} subdir using relative paths
        "${IDF_PATH}/components/soc/${IDF_TARGET}/include"
    PRIV_INCLUDE_DIRS
    SRCS
        "log.c"
        "${IDF_PATH}/components/esp_common/src/esp_err_to_name.c"
        #  NOTE: bootloader_support of anykind should not exists after system bootup
        "bootloader_support_shim.c"
    REQUIRES
        "efuse"
    LDFRAGMENTS
        "${IDF_PATH}/components/esp_common/common.lf"
        "${IDF_PATH}/components/esp_common/soc.lf"
)
