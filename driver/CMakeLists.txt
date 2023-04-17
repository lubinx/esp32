list(APPEND includes
    "include"
)

list(APPEND priv_includes
)

list(APPEND srcs
    "common_gpio.c"
    "common_pwm.c"
    "common_rtc.c"
    "common_timer.c"
)

macro(__chip_linker_script)
    # override by ${IDF_TARGET}/.cmake
endmacro()

include("${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/.cmake")

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
)

__chip_linker_script()
