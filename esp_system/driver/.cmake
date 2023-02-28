set (esp_rom_path "${IDF_PATH}/components/esp_rom/")

list(APPEND includes
    "${CMAKE_CURRENT_LIST_DIR}/include"      # common/include
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/include"
    # esp_rom
    "${esp_rom_path}/include"
    # "${esp_rom_path}/include/${IDF_TARGET}"
    "${esp_rom_path}/${IDF_TARGET}"
)

list(APPEND srcs
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/soc.c"
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/systimer.c"
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/cache_err_int.c"
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/clk_tree.c"
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/uart.c"
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/true-rng.c"
)

if (CONFIG_IDF_TARGET_ESP32S3)
    list(APPEND errata_link_scripts
        "${esp_rom_path}/esp32s3/ld/esp32s3.rom.ld"
        "${esp_rom_path}/esp32s3/ld/esp32s3.rom.api.ld"
        # "${esp_rom_path}/esp32s3/ld/esp32s3.rom.libgcc.ld", link current version of gcc's libc
        "${CMAKE_CURRENT_LIST_DIR}/esp32s3/ld/errata.ld"
    )
endif()
