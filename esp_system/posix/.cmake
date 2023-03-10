list(APPEND srcs
    "${CMAKE_CURRENT_LIST_DIR}/_esp_freertos_impl.c"
    "${CMAKE_CURRENT_LIST_DIR}/_retarget_init.c"
    "${CMAKE_CURRENT_LIST_DIR}/_retarget_lock.c"
    "${CMAKE_CURRENT_LIST_DIR}/random.c"
    "${CMAKE_CURRENT_LIST_DIR}/pthread.c"
    "${CMAKE_CURRENT_LIST_DIR}/pthread_local_storage.c"
    "${CMAKE_CURRENT_LIST_DIR}/pthread_spinlock.c"
    "${CMAKE_CURRENT_LIST_DIR}/sched.c"
    "${CMAKE_CURRENT_LIST_DIR}/time.c"
)

set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/_retarget_init.c" PROPERTIES COMPILE_FLAGS -fno-builtin)
set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/heap.c" PROPERTIES COMPILE_FLAGS -fno-builtin)
