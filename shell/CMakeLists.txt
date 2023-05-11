list(APPEND includes
    "include"
)

list(APPEND priv_includes
)

list(APPEND srcs
    "cmdline.c"
    "ucsh.c"
    "impl/cat.c"
    "impl/chdir.c"
    "impl/dt.c"
    "impl/format.c"
    "impl/ls.c"
    "impl/mkdir.c"
    "impl/nvm.c"
    "impl/pwd.c"
    "impl/rmdir.c"
    "impl/rst.c"
    "impl/unlink.c"
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
