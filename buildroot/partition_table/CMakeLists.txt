function(partition_table_add_check_size_target target_name)
    add_custom_target(${target_name})
endfunction()

function(partition_table_add_check_bootloader_size_target target_name)
    add_custom_target(${target_name})
endfunction()

function(partition_table_get_partition_info result get_part_info_args part_info)
endfunction()

idf_component_register()
