list(APPEND srcs
    "log.c"
    # esp_common
    "${IDF_PATH}/components/esp_common/src/esp_err_to_name.c"
)


# TODO: remove these!!! esp_app_format & app_update
list(APPEND app_srcs
    "${IDF_PATH}/components/app_update/esp_ota_ops.c"
    "${IDF_PATH}/components/app_update/esp_ota_app_desc.c"
    "${IDF_PATH}/components/esp_app_format/esp_app_desc.c"
)
list(APPEND srcs ${app_srcs})

set_source_files_properties(${app_srcs} PROPERTIES COMPILE_OPTIONS
    "-DPROJECT_VER=\"${PROJECT_VERSION}\";-DPROJECT_NAME=\"${PROJECT_NAME}\"")
#########################################################

idf_component_register(
    INCLUDE_DIRS
        "include/posix"
        "include/esp32"
        "${IDF_PATH}/components/soc/${IDF_TARGET}/include" #  fix: soc => ${IDF_TARGET} subdir using relative paths
        "${IDF_PATH}/components/esp_app_format/include"
        "${IDF_PATH}/components/app_update/include"
        "${IDF_PATH}/components/bootloader_support/include"
        "${IDF_PATH}/components/bootloader_support/bootloader_flash/include"
    PRIV_INCLUDE_DIRS
        "${IDF_PATH}/components/esp_partition/include"
    SRCS
        ${srcs}
    LDFRAGMENTS
        "${IDF_PATH}/components/esp_common/common.lf"
        "${IDF_PATH}/components/esp_common/soc.lf"
)
