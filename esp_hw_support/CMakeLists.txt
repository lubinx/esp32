list(APPEND srcs
    # "esp_clk.c"
    # "clk_ctrl_os.c"
    # "mac_addr.c"
    # "periph_ctrl.c"
    # "esp_memory_utils.c"
    "rtc_module.c"
    "regi2c_ctrl.c"
)

idf_component_register(SRCS ${srcs}
    INCLUDE_DIRS
        "include"
        "include/soc"
        "include/soc/${target}"
    PRIV_INCLUDE_DIRS
        "include/esp_private"
    REQUIRES
        "soc"
)

add_subdirectory(port/${IDF_TARGET})
