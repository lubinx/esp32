list(APPEND srcs
    "rtc_module.c"
)

idf_component_register(SRCS ${srcs}
    INCLUDE_DIRS
        "include/soc"
    PRIV_INCLUDE_DIRS
        "include"
    REQUIRES
        "soc"
)
