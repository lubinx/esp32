list(APPEND includes
    "${CMAKE_CURRENT_LIST_DIR}/include"      # common/include
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/include"
)

list(APPEND srcs
    "${CMAKE_CURRENT_LIST_DIR}/esp_cpu.c"
    "${CMAKE_CURRENT_LIST_DIR}/esp_intr.c"
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/cache_err_int.c"
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/clk_tree.c"
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/io_mux.c"
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/uart.c"
    "${CMAKE_CURRENT_LIST_DIR}/${IDF_TARGET}/true-rng.c"
)
