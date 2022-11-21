idf_component_register()

# NOTE: default to 0x8000
idf_build_set_property(COMPILE_OPTIONS "-DCONFIG_PARTITION_TABLE_OFFSET=0x8000" APPEND)
