idf_component_register()
idf_build_get_property(python PYTHON)

set(ESPTOOLPY ${python} "${CMAKE_CURRENT_LIST_DIR}/esptool.py" --chip ${IDF_TARGET})

if(CONFIG_SPI_FLASH_HPM_ENABLE)
# When set flash frequency to 120M, must keep 1st bootloader work under ``DOUT`` mode
# because on some flash chips, 120M will modify the status register,
# which will make ROM won't work.
# This change intends to be for esptool only and the bootloader should keep use
# ``DOUT`` mode.
    set(ESPFLASHMODE "dout")
    message("Note: HPM is enabled for the flash, force the ROM bootloader into DOUT mode for stable boot on")
else()
    set(ESPFLASHMODE ${CONFIG_ESPTOOLPY_FLASHMODE})
endif()

set(esptool_elf2image_args
    --flash_mode ${ESPFLASHMODE}
    --flash_freq ${CONFIG_ESPTOOLPY_FLASHFREQ}
    --flash_size ${CONFIG_ESPTOOLPY_FLASHSIZE}
    --dont-append-digest
)

add_custom_command(OUTPUT "elf2image_project"
    COMMAND ${ESPTOOLPY} elf2image ${esptool_elf2image_args}
        -o "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin" "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.elf"
    DEPENDS
        "${CMAKE_PROJECT_NAME}"
    VERBATIM
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating binary image from executable"
)
add_custom_command(OUTPUT "elf2image_bootloader"
    COMMAND ${ESPTOOLPY} elf2image ${esptool_elf2image_args}
        -o "${CMAKE_BINARY_DIR}/bootloader.bin" "${CMAKE_BINARY_DIR}/bootloader/bootloader.elf"
    DEPENDS
        "bootloader"
    VERBATIM
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating binary image from bootloader"
)
add_custom_command(OUTPUT "size_utils"
    COMMAND "${_CMAKE_TOOLCHAIN_PREFIX}size" -A "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.elf"
    DEPENDS
    VERBATIM
    COMMENT "${_CMAKE_TOOLCHAIN_PREFIX}size -A"
)

add_custom_target("generate_binary" ALL DEPENDS
    "elf2image_project" "elf2image_bootloader")

add_custom_target("size" ALL
    DEPENDS "${CMAKE_PROJECT_NAME}" "size_utils"
)
