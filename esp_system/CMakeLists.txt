if ("xtensa" STREQUAL ${IDF_TARGET_ARCH})
    include("${CMAKE_CURRENT_LIST_DIR}/arch/xtensa/.cmake")
elseif("risv" STREQUAL ${IDF_TARGET_ARCH})
    include("${CMAKE_CURRENT_LIST_DIR}/arch/risv/.cmake")
else()
    message(FATAL_ERROR "unknown arch")
endif()

list(APPEND srcs
    "esp_crosscore_int.c"
    "esp_dbg_ubsan.c"                   # debug for undefined behavior, but not sure if its works
    "esp_startup.c"
    "panic.c"
    "panic_handler.c"
    "esp_intr.c"
)

list(APPEND includes
    "include"
)
list(APPEND priv_includes
)

# include("${CMAKE_CURRENT_LIST_DIR}/driver/.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/posix/.cmake")

idf_component_register(
    INCLUDE_DIRS
        ${includes}
    PRIV_INCLUDE_DIRS
        ${priv_includes}
    SRCS
        ${srcs}
    REQUIRES
        "esp_common"
        "soc"
        "driver"
        "heap"
    LDFRAGMENTS
        "memap.lf"
)
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u __ubsan_include")
