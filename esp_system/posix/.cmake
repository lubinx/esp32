list(APPEND srcs
    "${CMAKE_CURRENT_LIST_DIR}/_retarget.c"
    "${CMAKE_CURRENT_LIST_DIR}/_rtos_freertos_impl.c"
    "${CMAKE_CURRENT_LIST_DIR}/_rtos_kernel.c"
    "${CMAKE_CURRENT_LIST_DIR}/fdio.c"
    "${CMAKE_CURRENT_LIST_DIR}/filesystem.c"
    "${CMAKE_CURRENT_LIST_DIR}/mqueue.c"
    "${CMAKE_CURRENT_LIST_DIR}/random.c"
    "${CMAKE_CURRENT_LIST_DIR}/pthread.c"
    "${CMAKE_CURRENT_LIST_DIR}/sched.c"
    "${CMAKE_CURRENT_LIST_DIR}/time.c"
)

set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/_retarget_init.c" PROPERTIES COMPILE_FLAGS -fno-builtin)
set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/heap.c" PROPERTIES COMPILE_FLAGS -fno-builtin)
