set(idf_path "${IDF_PATH}/components/bootloader_support")

list(APPEND include_dirs
    "${idf_path}/include"
    "${idf_path}/bootloader_flash/include"
    "${idf_path}/private_include"
)
list(APPEND priv_include_dirs
    "${IDF_PATH}/components/esp_app_format/include" # only need "esp_app_format" headers to compile, no link anything
    # "${idf_path}/private_include"
)

list(APPEND srcs
    "${idf_path}/src/bootloader_common.c"
    "${idf_path}/src/bootloader_common_loader.c"
    "${idf_path}/src/bootloader_clock_init.c"
    "${idf_path}/bootloader_flash/src/bootloader_flash.c"
    "${idf_path}/src/bootloader_mem.c"
    "${idf_path}/src/bootloader_random.c"
    "${idf_path}/src/bootloader_random_${IDF_TARGET}.c"
    "${idf_path}/src/bootloader_utility.c"
    "${idf_path}/src/bootloader_efuse.c"
    # "${idf_path}/src/esp_image_format.c"   # no load any image
    # "${idf_path}/src/flash_encrypt.c"
    # "${idf_path}/src/secure_boot.c"
    # "${idf_path}/src/flash_partitions.c"
    "${idf_path}/bootloader_flash/src/flash_qio_mode.c"
    "${idf_path}/bootloader_flash/src/bootloader_flash_config_${IDF_TARGET}.c"
)

# BOOTLOADER_BUILD
    list(APPEND srcs
        "${idf_path}/src/bootloader_init.c"
        "${idf_path}/src/bootloader_clock_loader.c"
        "${idf_path}/src/bootloader_console.c"
        "${idf_path}/src/bootloader_console_loader.c"
        # "${idf_path}/src/bootloader_panic.c"
        # "${idf_path}/src/${IDF_TARGET}/bootloader_sha.c"
        "${idf_path}/src/${IDF_TARGET}/bootloader_soc.c"
        "${idf_path}/src/${IDF_TARGET}/bootloader_${IDF_TARGET}.c"
    )
# else
    # this somehow use mbedtls
    # list(APPEND srcs
    #     "${idf_path}/src/idf/bootloader_sha.c"
    # )

list(APPEND requires
    "soc"
)
list(APPEND priv_requires
    "spi_flash"
    # "hal"
    # "efuse"
    # "partition_table"
    # "micro-ecc"
    # "mbedtls"
)

# if(BOOTLOADER_BUILD)
#     if(CONFIG_SECURE_FLASH_ENC_ENABLED)
#         list(APPEND srcs
#             "${idf_path}/src/flash_encryption/flash_encrypt.c"
#             "${idf_path}/src/${IDF_TARGET}/flash_encryption_secure_features.c"
#         )
#     endif()

#     if(CONFIG_SECURE_SIGNED_ON_BOOT)
#         if(CONFIG_SECURE_SIGNED_APPS_ECDSA_SCHEME)
#             list(APPEND srcs
#                 "${idf_path}/src/secure_boot_v1/secure_boot_signatures_bootloader.c"
#             )
#         endif()
#         if(CONFIG_SECURE_BOOT_V1_ENABLED)
#             list(APPEND srcs
#                 "${idf_path}/src/secure_boot_v1/secure_boot.c"
#                 "${idf_path}/src/${IDF_TARGET}/secure_boot_secure_features.c"
#             )
#         endif()

#         if(CONFIG_SECURE_BOOT_V2_ENABLED)
#             list(APPEND srcs
#                 "${idf_path}/src/secure_boot_v2/secure_boot_signatures_bootloader.c"
#                 "${idf_path}/src/secure_boot_v2/secure_boot.c"
#                 "${idf_path}/src/${IDF_TARGET}/secure_boot_secure_features.c"
#             )
#         endif()
#     endif()
# else()
#     if(CONFIG_SECURE_SIGNED_ON_UPDATE)
#         if(CONFIG_SECURE_SIGNED_APPS_ECDSA_SCHEME)
#             list(APPEND srcs
#                 "${idf_path}/src/secure_boot_v1/secure_boot_signatures_app.c"
#             )
#         endif()

#         if(CONFIG_SECURE_SIGNED_APPS_RSA_SCHEME)
#             list(APPEND srcs
#                 "${idf_path}/src/secure_boot_v2/secure_boot_signatures_app.c"
#                 "${idf_path}/src/secure_boot_v2/secure_boot_rsa_signature.c"
#             )
#         endif()
#         if(CONFIG_SECURE_SIGNED_APPS_ECDSA_V2_SCHEME)
#             list(APPEND srcs
#                 "${idf_path}/src/secure_boot_v2/secure_boot_signatures_app.c"
#                 "${idf_path}/src/secure_boot_v2/secure_boot_ecdsa_signature.c"
#             )
#         endif()
#     endif()
# endif()

idf_component_register(
    INCLUDE_DIRS "${include_dirs}" PRIV_INCLUDE_DIRS "${priv_include_dirs}"
        SRCS "${srcs}"
    REQUIRES "${requires}"
    PRIV_REQUIRES "${priv_requires}"
)

# if(BOOTLOADER_BUILD)
#     target_link_libraries(${COMPONENT_LIB} INTERFACE "-u abort")
# endif()

# if(NOT BOOTLOADER_BUILD)
#     if(CONFIG_SECURE_SIGNED_ON_UPDATE)
#         if(CONFIG_SECURE_SIGNED_APPS_ECDSA_SCHEME OR CONFIG_SECURE_SIGNED_APPS_RSA_SCHEME OR
#            CONFIG_SECURE_SIGNED_APPS_ECDSA_V2_SCHEME)
#             target_link_libraries(${COMPONENT_LIB} PRIVATE idf::app_update)
#         endif()
#     endif()
# endif()

# if(CONFIG_SECURE_SIGNED_APPS AND (CONFIG_SECURE_BOOT_V1_ENABLED OR CONFIG_SECURE_SIGNED_APPS_ECDSA_SCHEME))
#     if(BOOTLOADER_BUILD)
#         # Whether CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES or not, we need verification key to embed
#         # in the library.
#         if(CONFIG_SECURE_BOOT_BUILD_SIGNED_BINARIES)
#             # We generate the key from the signing key. The signing key is passed from the main project.
#             get_filename_component(secure_boot_signing_key
#                 "${SECURE_BOOT_SIGNING_KEY}"
#                 ABSOLUTE BASE_DIR "${project_dir}")
#             get_filename_component(secure_boot_verification_key
#                 "signature_verification_key.bin"
#                 ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")
#             add_custom_command(OUTPUT "${secure_boot_verification_key}"
#                 COMMAND ${ESPSECUREPY}
#                 extract_public_key --keyfile "${secure_boot_signing_key}"
#                 "${secure_boot_verification_key}"
#                 DEPENDS ${secure_boot_signing_key}
#                 VERBATIM)
#         else()
#             # We expect to 'inherit' the verification key passed from main project.
#             get_filename_component(secure_boot_verification_key
#                 ${SECURE_BOOT_VERIFICATION_KEY}
#                 ABSOLUTE BASE_DIR "${project_dir}")
#         endif()
#     else()  # normal app build
#         idf_build_get_property(project_dir PROJECT_DIR)

#         if(CONFIG_SECURE_BOOT_VERIFICATION_KEY)
#             # verification-only build supplies verification key
#             set(secure_boot_verification_key ${CONFIG_SECURE_BOOT_VERIFICATION_KEY})
#             get_filename_component(secure_boot_verification_key
#                 ${secure_boot_verification_key}
#                 ABSOLUTE BASE_DIR "${project_dir}")
#         else()
#             # sign at build time, extracts key from signing key
#             set(secure_boot_verification_key "${CMAKE_BINARY_DIR}/signature_verification_key.bin")
#             get_filename_component(secure_boot_signing_key
#                 ${CONFIG_SECURE_BOOT_SIGNING_KEY}
#                 ABSOLUTE BASE_DIR "${project_dir}")

#             add_custom_command(OUTPUT "${secure_boot_verification_key}"
#                 COMMAND ${ESPSECUREPY}
#                 extract_public_key --keyfile "${secure_boot_signing_key}"
#                 "${secure_boot_verification_key}"
#                 WORKING_DIRECTORY ${project_dir}
#                 DEPENDS ${secure_boot_signing_key}
#                 VERBATIM)
#         endif()
#     endif()

#     # Embed the verification key in the binary (app & bootloader)
#     #
#     target_add_binary_data(${COMPONENT_LIB} "${secure_boot_verification_key}" "BINARY"
#         RENAME_TO signature_verification_key_bin)
#     set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
#         APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
#         "${secure_boot_verification_key}")
# endif()
