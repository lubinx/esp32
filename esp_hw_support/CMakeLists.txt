idf_build_get_property(target IDF_TARGET)

# On Linux, we only support a few features, hence this simple component registration
if(${target} STREQUAL "linux")
    idf_component_register(SRCS "port/linux/esp_random.c"
                                 "port/linux/chip_info.c"
                           INCLUDE_DIRS "include")
    return()
endif()

set(requires soc)
# only esp_hw_support/adc_share_hw_ctrl.c requires efuse component
set(priv_requires efuse)

set(srcs "esp_memory_utils.c")
list(APPEND srcs
    "esp_clk.c"
    # "clk_ctrl_os.c"
    "intr_alloc.c"
    "mac_addr.c"
    "periph_ctrl.c"
    "rtc_module.c"
    "sleep_mac_bb.c"
    "regi2c_ctrl.c"
)

# [refactor-todo]: requires "driver" for GPIO and RTC  (by sleep_gpio and sleep_modes)
list(APPEND priv_requires esp_timer)

idf_component_register(SRCS ${srcs}
    INCLUDE_DIRS
        "include"
        "include/soc"
        "include/soc/${target}"
    PRIV_INCLUDE_DIRS
        "include/esp_private"
    REQUIRES
        ${requires}
    PRIV_REQUIRES
        ${priv_requires}
    LDFRAGMENTS
        linker.lf
)

idf_build_get_property(target IDF_TARGET)
add_subdirectory(port/${target})
