set(srcs
    "src/phy_init.c"
    # "src/phy_override.c"
    # "src/lib_printf.c"
)

if(CONFIG_SOC_BT_SUPPORTED OR CONFIG_SOC_IEEE802154_SUPPORTED OR CONFIG_SOC_IEEE802154_BLE_ONLY)
    list(APPEND srcs "src/btbb_init.c")
endif()

if(CONFIG_ESP_PHY_ENABLE_CERT_TEST)
    list(APPEND srcs "src/phy_callback.c")
endif()

idf_build_get_property(build_dir BUILD_DIR)

if(CONFIG_SOC_WIFI_SUPPORTED)
    if(CONFIG_ESP_PHY_MULTIPLE_INIT_DATA_BIN)
        if(NOT EXISTS "${build_dir}/phy_multiple_init_data.bin")
            file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/${IDF_TARGET}/phy_multiple_init_data.bin DESTINATION "${build_dir}")
        endif()
    endif()
    if(CONFIG_ESP_PHY_MULTIPLE_INIT_DATA_BIN_EMBED)
        set(embed_files "${build_dir}/phy_multiple_init_data.bin")
    endif()
endif()

# [refactor-todo]: requires "driver" component for periph_ctrl header file
idf_component_register(
    SRCS
        "${srcs}"
    INCLUDE_DIRS
        "include"
        "${IDF_TARGET}/include"
    # PRIV_REQUIRES nvs_flash driver efuse esp_timer esp_wifi
    REQUIRES
        "driver"
    LDFRAGMENTS
        "linker.lf"
    EMBED_FILES
        ${embed_files}
)
target_link_directories(${COMPONENT_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lib/${IDF_TARGET}")

# Override functions in PHY lib with the functions in 'phy_override.c'
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u include_esp_phy_override")
target_link_libraries(${COMPONENT_LIB} PUBLIC phy)

if(CONFIG_SOC_BT_SUPPORTED OR CONFIG_SOC_IEEE802154_SUPPORTED)
    target_link_libraries(${COMPONENT_LIB} PUBLIC
        "btbb"      # libbtbb.a
    )
    target_link_libraries(${COMPONENT_LIB} INTERFACE libphy.a libbtbb.a)
endif()

# if(CONFIG_ESP_PHY_INIT_DATA_IN_PARTITION)
#     idf_component_get_property(esp_common_dir esp_common COMPONENT_DIR)
#     partition_table_get_partition_info(phy_partition_offset "--partition-type data --partition-subtype phy" "offset")

#     if(CONFIG_ESP_PHY_MULTIPLE_INIT_DATA_BIN)
#         set(phy_init_data_bin "${build_dir}/phy_multiple_init_data.bin")
#         if(CONFIG_ESP_PHY_MULTIPLE_INIT_DATA_BIN_EMBED)
#             set(COMPONENT_EMBED_FILES "${build_dir}/phy_multiple_init_data.bin")
#         endif()
#     else()
#         set(phy_init_data_bin "${build_dir}/phy_init_data.bin")

#         # To get the phy_init_data.bin file, compile phy_init_data.h as a C file and then objcopy
#         # the object file to a raw binary
#         idf_build_get_property(config_dir CONFIG_DIR)
#         add_custom_command(
#             OUTPUT ${phy_init_data_bin}
#             DEPENDS ${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/include/phy_init_data.h
#             COMMAND ${CMAKE_C_COMPILER} -x c -c
#             -I ${esp_common_dir}/include -I ${CMAKE_CURRENT_LIST_DIR}/include -I ${config_dir}
#             -o phy_init_data.obj
#             ${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/include/phy_init_data.h
#             COMMAND ${CMAKE_OBJCOPY} -O binary phy_init_data.obj ${phy_init_data_bin}
#             )
#         add_custom_target(phy_init_data ALL DEPENDS ${phy_init_data_bin})
#         add_dependencies(flash phy_init_data)

#         idf_component_get_property(main_args esptool_py FLASH_ARGS)
#         idf_component_get_property(sub_args esptool_py FLASH_SUB_ARGS)
#     endif()

#     set(phy_name "phy")

#     esptool_py_flash_target(${phy_name}-flash "${main_args}" "${sub_args}")
#     esptool_py_flash_target_image(${phy_name}-flash ${phy_name} "${phy_partition_offset}" "${phy_init_data_bin}")
#     esptool_py_flash_target_image(flash ${phy_name} "${phy_partition_offset}" "${phy_init_data_bin}")
# endif()
