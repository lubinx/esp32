list(APPEND includes
    "include"
)

list(APPEND priv_includes
)

list(APPEND srcs
)

idf_component_register(
    INCLUDE_DIRS
        ${includes}
    PRIV_INCLUDE_DIRS
        ${priv_includes}
    SRCS
        ${srcs}
    REQUIRES
        "esp_common"
)
