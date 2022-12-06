set(component_path "${IDF_PATH}/components/bootloader_support")

idf_component_register(
    INCLUDE_DIRS
        "${component_path}/include"
        "${component_path}/bootloader_flash/include"
        "${component_path}/private_include"
    PRIV_INCLUDE_DIRS
        "${IDF_PATH}/components/esp_partition/include"
    SRCS
        "${component_path}/src/bootloader_common.c"
        "${component_path}/src/bootloader_common_loader.c"
        "${component_path}/src/bootloader_clock_init.c"
        "${component_path}/bootloader_flash/src/bootloader_flash.c"
        "${component_path}/src/bootloader_mem.c"
        "${component_path}/src/bootloader_random.c"
        "${component_path}/src/bootloader_random_${IDF_TARGET}.c"
        "${component_path}/src/bootloader_utility.c"
        "${component_path}/src/bootloader_efuse.c"
        # "${component_path}/src/esp_image_format.c"   # no load any image
        "${component_path}/src/flash_encrypt.c"
        # "${component_path}/src/secure_boot.c"
        # "${component_path}/src/flash_partitions.c"
        "${component_path}/bootloader_flash/src/flash_qio_mode.c"
        "${component_path}/bootloader_flash/src/bootloader_flash_config_${IDF_TARGET}.c"
    # BOOTLOADER_BUILD
        # "${component_path}/src/bootloader_init.c"
        # "${component_path}/src/bootloader_clock_loader.c"
        # "${component_path}/src/bootloader_console.c"
        # "${component_path}/src/bootloader_console_loader.c"
        # "${component_path}/src/bootloader_panic.c"
        # "${component_path}/src/${IDF_TARGET}/bootloader_sha.c"
        # "${component_path}/src/${IDF_TARGET}/bootloader_soc.c"
        # "${component_path}/src/${IDF_TARGET}/bootloader_${IDF_TARGET}.c"
    REQUIRES
        "soc"
    PRIV_REQUIRES
        "hal"
        "efuse"
)


