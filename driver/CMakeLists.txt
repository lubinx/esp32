set (esp_rom_path "${IDF_PATH}/components/esp_rom/")

list(APPEND includes
    "include"      # common/include
    "${IDF_TARGET}/include"
    # esp_rom
    "${esp_rom_path}/include"
    "${esp_rom_path}/${IDF_TARGET}"         # for esp_rom_caps.h
)

list(APPEND srcs
    "${IDF_TARGET}/clk-tree.c"
    "${IDF_TARGET}/gpio.c"
    "${IDF_TARGET}/i2c.c"
    "${IDF_TARGET}/rtc.c"
    "${IDF_TARGET}/soc.c"
    "${IDF_TARGET}/true-rng.c"
    "${IDF_TARGET}/uart.c"
)

list(APPEND srcs
    "common_gpio.c"
    "common_pwm.c"
    "common_rtc.c"
    "common_timer.c"
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
        "soc"
)

target_linker_script(${COMPONENT_LIB} INTERFACE "${esp_rom_path}/esp32s3/ld/esp32s3.rom.ld")
target_linker_script(${COMPONENT_LIB} INTERFACE "${IDF_TARGET}/ld/errata.ld")


# generate link scripts
idf_build_get_property(sdkconfig_header SDKCONFIG_HEADER)
idf_build_get_property(config_dir CONFIG_DIR)

set(ld_input "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/ld/memory.ld.in")
set(ld_output "${config_dir}/${IDF_TARGET}_memory.ld")

add_custom_command(
    OUTPUT ${ld_output}
    COMMAND "${CMAKE_C_COMPILER}" -C -P -x c -E -o ${ld_output} -I ${config_dir}
            -I "${CMAKE_CURRENT_LIST_DIR}" ${ld_input}
    DEPENDS ${sdkconfig_header}
    VERBATIM
)
add_custom_target(memory_ld DEPENDS ${ld_output})
# add_dependencies(${COMPONENT_LIB} memory_ld)
target_linker_script(${COMPONENT_LIB} INTERFACE "${ld_output}")

target_linker_script(${COMPONENT_LIB} INTERFACE "${IDF_TARGET}/ld/sections.ld"
    PROCESS "${config_dir}/${IDF_TARGET}_sections.ld"
)
