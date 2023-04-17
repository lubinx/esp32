set (esp_rom_path "${IDF_PATH}/components/esp_rom/")

list(APPEND includes
    "${CMAKE_CURRENT_LIST_DIR}/include"
    # esp_rom
    "${esp_rom_path}/include"
    "${esp_rom_path}/esp32s3"         # for esp_rom_caps.h
)

list(APPEND srcs
    "${CMAKE_CURRENT_LIST_DIR}/clk-tree.c"
    "${CMAKE_CURRENT_LIST_DIR}/gpio.c"
    "${CMAKE_CURRENT_LIST_DIR}/i2c.c"
    "${CMAKE_CURRENT_LIST_DIR}/rtc.c"
    "${CMAKE_CURRENT_LIST_DIR}/true-rng.c"
    "${CMAKE_CURRENT_LIST_DIR}/uart.c"
)

# overrides
macro(__chip_linker_script)
    target_linker_script(${COMPONENT_LIB} INTERFACE "${esp_rom_path}/esp32s3/ld/esp32s3.rom.ld")
    target_linker_script(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/esp32s3/ld/errata.ld")

    # generate link scripts
    idf_build_get_property(sdkconfig_header SDKCONFIG_HEADER)
    idf_build_get_property(config_dir CONFIG_DIR)

    set(ld_input "${CMAKE_CURRENT_LIST_DIR}/esp32s3/ld/memory.ld.in")
    set(ld_output "${config_dir}/esp32s3_memory.ld")

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

    target_linker_script(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/esp32s3/ld/sections.ld"
        PROCESS "${config_dir}/esp32s3_sections.ld"
    )
endmacro()
