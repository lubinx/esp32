list(APPEND srcs
    "rtc_module.c"
)

idf_component_register(SRCS ${srcs}
    INCLUDE_DIRS
        "include/soc"
    PRIV_INCLUDE_DIRS
        "include"
        "include/esp_private"
        "include/soc/${target}"
    REQUIRES
        "soc"
)

add_subdirectory(port/${IDF_TARGET})
