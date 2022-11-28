set(component_path "${IDF_PATH}/components/esp_hw_support")

list(APPEND includes
    "${component_path}/include/"
    "${component_path}/include/soc"
    "${component_path}/include/soc/${IDF_TARGET}"
)
list(APPEND priv_includes
    "${component_path}/port/include"
    "${component_path}/include/esp_private"
)

list(APPEND srcs
    "${component_path}/cpu.c"
    "${component_path}/esp_memory_utils.c"
    # BOOTLOADER_BUILD
    "${component_path}/esp_clk.c"
    "${component_path}/clk_ctrl_os.c"
    "${component_path}/hw_random.c"
    "${component_path}/intr_alloc.c"
    "${component_path}/mac_addr.c"
    "${component_path}/periph_ctrl.c"
    "${component_path}/rtc_module.c"
    "${component_path}/sleep_modes.c"
    "${component_path}/sleep_gpio.c"
    "${component_path}/sleep_mac_bb.c"
    "${component_path}/regi2c_ctrl.c"
    "${component_path}/adc_share_hw_ctrl.c"
)

if(NOT CONFIG_IDF_TARGET_ESP32 AND NOT CONFIG_IDF_TARGET_ESP32S2)
    list(APPEND srcs
        "${component_path}/sleep_retention.c"
    )
endif()

if(CONFIG_IDF_TARGET_ESP32 OR CONFIG_IDF_TARGET_ESP32S2)
    list(APPEND srcs
        "${component_path}/rtc_wdt.c"
    )
endif()

if(CONFIG_SOC_GDMA_SUPPORTED)
    list(APPEND srcs
        "${component_path}/dma/gdma.c"
        "${component_path}/dma/async_memcpy_impl_gdma.c"
    )
endif()

if(CONFIG_SOC_CP_DMA_SUPPORTED)
    list(APPEND srcs
        "${component_path}/dma/async_memcpy_impl_cp_dma.c"
    )
endif()

if(CONFIG_SOC_GDMA_SUPPORTED OR CONFIG_SOC_CP_DMA_SUPPORTED)
    list(APPEND srcs
        "${component_path}/dma/esp_async_memcpy.c"
    )
endif()

if(CONFIG_SOC_GDMA_SUPPORT_ETM)
    list(APPEND srcs "${component_path}/dma/gdma_etm.c")
endif()

if(CONFIG_SOC_SYSTIMER_SUPPORTED)
    list(APPEND srcs
        "${component_path}/port/${IDF_TARGET}/systimer.c"
    )
endif()

if(CONFIG_SOC_HMAC_SUPPORTED)
    list(APPEND srcs
        "${component_path}/esp_hmac.c"
    )
endif()

if(CONFIG_SOC_DIG_SIGN_SUPPORTED)
    list(APPEND srcs "${component_path}/esp_ds.c")
endif()


# ESP32C6-TODO
# if(NOT CONFIG_IDF_TARGET_ESP32C6)
#     list(APPEND srcs
#         "${component_path}/adc_share_hw_ctrl.c" # TODO: IDF-5312
#     )
# endif()

# MERGE: add_subdirectory("${component_path}/port/${IDF_TARGET}" "${build_dir}/${prefix}/${component_name}/port")
list(APPEND srcs
    "${component_path}/port/${IDF_TARGET}/rtc_clk.c"
    "${component_path}/port/${IDF_TARGET}/rtc_clk_init.c"
    "${component_path}/port/${IDF_TARGET}/rtc_init.c"
    "${component_path}/port/${IDF_TARGET}/rtc_pm.c"
    "${component_path}/port/${IDF_TARGET}/rtc_sleep.c"
    "${component_path}/port/${IDF_TARGET}/rtc_time.c"
    "${component_path}/port/${IDF_TARGET}/chip_info.c"
    # BOOTLOADER_BUILD
    "${component_path}/port/${IDF_TARGET}/esp_crypto_lock.c"
    "${component_path}/port/${IDF_TARGET}/sar_periph_ctrl.c"
)
if(CONFIG_ESP_SYSTEM_MEMPROT_FEATURE)
    list(APPEND srcs
        "${component_path}/port/${IDF_TARGET}/esp_memprot.c"
        "${component_path}/port/esp_memprot_conv.c")
endif()

idf_component_register(
    INCLUDE_DIRS ${includes} PRIV_INCLUDE_DIRS ${priv_includes}
        SRCS ${srcs}
    REQUIRES
        "soc"
    PRIV_REQUIRES
        "bootloader_support"
        "efuse"
        "spi_flash"
        # BOOTLOADER_BUILD
        "driver"
        "esp_timer"
    # LDFRAGMENTS linker.lf
)

if(CONFIG_SPIRAM)
    idf_component_optional_requires(PRIVATE esp_psram)
endif()
