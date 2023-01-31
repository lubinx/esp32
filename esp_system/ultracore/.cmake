list(APPEND includes
    "${CMAKE_CURRENT_LIST_DIR}/include"
)

list(APPEND srcs
    "${CMAKE_CURRENT_LIST_DIR}/glist.c"
    "${CMAKE_CURRENT_LIST_DIR}/kernel.c"
    "${CMAKE_CURRENT_LIST_DIR}/atomic.c"
    "${CMAKE_CURRENT_LIST_DIR}/filesystem.c"
    "${CMAKE_CURRENT_LIST_DIR}/io.c"
)
