set(srcs
    "${CMAKE_CURRENT_LIST_DIR}/clk.c"
    "${CMAKE_CURRENT_LIST_DIR}/reset_reason.c"
    "${CMAKE_CURRENT_LIST_DIR}/cache_err_int.c"
)

target_sources(${COMPONENT_LIB} PRIVATE ${srcs})
