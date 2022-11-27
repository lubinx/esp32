idf_component_register()

idf_build_get_property(python PYTHON)

# TODO: remove this if block when esp32h4 beta1 is no longer supported and we have h4 IDF_TARGET in esptool
if(IDF_TARGET STREQUAL "esp32h4")
    if(CONFIG_IDF_TARGET_ESP32H4_BETA_VERSION_1)
        set(IDF_TARGET esp32h2beta1)
    elseif(CONFIG_IDF_TARGET_ESP32H4_BETA_VERSION_2)
        set(IDF_TARGET esp32h2beta2)
    endif()
endif()

set(ESPTOOLPY ${python} "${CMAKE_CURRENT_LIST_DIR}/esptool.py" --chip ${IDF_TARGET})

# We still set "--min-rev" to keep the app compatible with older booloaders where this field is controlled.
if(CONFIG_IDF_TARGET_ESP32)
    # for this chip min_rev is major revision
    math(EXPR min_rev "${CONFIG_ESP_REV_MIN_FULL} / 100")
endif()
if(CONFIG_IDF_TARGET_ESP32C3)
    # for this chip min_rev is minor revision
    math(EXPR min_rev "${CONFIG_ESP_REV_MIN_FULL} % 100")
endif()

if(min_rev)
    list(APPEND esptool_elf2image_args --min-rev ${min_rev})
endif()
list(APPEND esptool_elf2image_args --min-rev-full ${CONFIG_ESP_REV_MIN_FULL})
list(APPEND esptool_elf2image_args --max-rev-full ${CONFIG_ESP_REV_MAX_FULL})


idf_build_get_property(elf EXECUTABLE GENERATOR_EXPRESSION)

add_custom_command(OUTPUT "dummy_output"
    COMMAND ${ESPTOOLPY} elf2image ${esptool_elf2image_args}
        -o "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin" "${CMAKE_BINARY_DIR}/${elf}"
    DEPENDS ${elf}
    VERBATIM
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating binary image from built executable"
    )
add_custom_target("generate_binary" ALL DEPENDS "dummy_output")
