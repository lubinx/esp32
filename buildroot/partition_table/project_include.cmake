set(PARTITION_TABLE_OFFSET ${CONFIG_PARTITION_TABLE_OFFSET})

function(partition_table_add_check_size_target target_name)
    add_custom_target(${target_name})
endfunction()

function(partition_table_add_check_bootloader_size_target target_name)
    add_custom_target(${target_name})
endfunction()
