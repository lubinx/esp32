idf_component_register(
    SRCS
        "err.c"
        "log.c"
    INCLUDE_DIRS
        "include"
        "posix"     # posix includes should be globaly visiable
    LDFRAGMENTS
        "soc.lf"
        "common.lf"
)
