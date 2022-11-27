set(component_path "${IDF_PATH}/components/spi_flash")

list(APPEND srcs
    "${component_path}/flash_brownout_hook.c"
    "${component_path}/memspi_host_driver.c"
    "${component_path}/spi_flash_chip_drivers.c"
    "${component_path}/spi_flash_chip_generic.c"
    "${component_path}/spi_flash_chip_issi.c"
    "${component_path}/spi_flash_chip_mxic.c"
    "${component_path}/spi_flash_chip_gd.c"
    "${component_path}/spi_flash_chip_winbond.c"
    "${component_path}/spi_flash_chip_boya.c"
    "${component_path}/spi_flash_chip_mxic_opi.c"
    "${component_path}/spi_flash_chip_th.c"
)

# if(CONFIG_SOC_SPI_MEM_SUPPORT_OPI_MODE)
#     list(APPEND srcs
#         "${component_path}/${IDF_TARGET}/spi_flash_oct_flash_init.c"
#     )
# endif()

if(CONFIG_IDF_TARGET_ESP32S3)
    list(APPEND srcs
        "${component_path}/esp32s3/spi_timing_config.c"
        "${component_path}/spi_flash_timing_tuning.c"
        # "${component_path}/spi_flash_hpm_enable.c"
    )
endif()

list(APPEND cache_srcs
    "${component_path}/${IDF_TARGET}/flash_ops_${IDF_TARGET}.c"
    # "${component_path}/esp_flash_api.c"
    # "${component_path}/esp_flash_spi_init.c"
    # "${component_path}/spi_flash_os_func_app.c"
    "${component_path}/spi_flash_os_func_noos.c"
    "${component_path}/cache_utils.c"
    "${component_path}/flash_mmap.c"
    "${component_path}/flash_ops.c"
)
list(APPEND srcs ${cache_srcs})

list(APPEND includes
    "${component_path}/include"
)
list(APPEND priv_includes
    "${component_path}/include/spi_flash"
)

idf_component_register(
    INCLUDE_DIRS "${includes}" PRIV_INCLUDE_DIRS "${priv_includes}"
        SRCS "${srcs}"
    REQUIRES
        "hal"
    PRIV_REQUIRES
        "bootloader_support"
        "soc"
    # LDFRAGMENTS linker.lf
)

# Avoid cache miss by unexpected inlineing when built by -Os
set_source_files_properties(${cache_srcs} PROPERTIES COMPILE_FLAGS "-fno-inline-functions")

if(CMAKE_C_COMPILER_ID MATCHES "GNU")
    # These flags are GCC specific
    set_property(SOURCE ${cache_srcs} APPEND_STRING PROPERTY COMPILE_FLAGS
        " -fno-inline-small-functions -fno-inline-functions-called-once")
endif()

if(CONFIG_SPIRAM)
    # [refactor-todo]: requires "esp_psram" for few MMU usages in `flash_mmap.c`
    # will be replaced with MMU requirements
    idf_component_optional_requires(PRIVATE esp_psram)
endif()
