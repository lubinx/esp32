list(APPEND includes
    "${CMAKE_CURRENT_LIST_DIR}/include"
)

list(APPEND srcs
    "${CMAKE_CURRENT_LIST_DIR}/clk.c"
    "${CMAKE_CURRENT_LIST_DIR}/cache_err_int.c"
    "${CMAKE_CURRENT_LIST_DIR}/io_mux.c"
    "${CMAKE_CURRENT_LIST_DIR}/systimer.c"
    "${CMAKE_CURRENT_LIST_DIR}/uart.c"
)
