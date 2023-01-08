list(APPEND srcs
    "${CMAKE_CURRENT_LIST_DIR}/_init.c"
    "${CMAKE_CURRENT_LIST_DIR}/esp_time_impl.c"
    "${CMAKE_CURRENT_LIST_DIR}/random.c"
    "${CMAKE_CURRENT_LIST_DIR}/heap.c"          # TODO: retarget instead of reimplement malloc()/free()..etc.
    "${CMAKE_CURRENT_LIST_DIR}/locks.c"
    # "${CMAKE_CURRENT_LIST_DIR}/poll.c"
    "${CMAKE_CURRENT_LIST_DIR}/pthread.c"
    "${CMAKE_CURRENT_LIST_DIR}/pthread_local_storage.c"
    "${CMAKE_CURRENT_LIST_DIR}/sched.c"
    "${CMAKE_CURRENT_LIST_DIR}/signal.c"
    "${CMAKE_CURRENT_LIST_DIR}/strerror.c"
    "${CMAKE_CURRENT_LIST_DIR}/time.c"
    "${CMAKE_CURRENT_LIST_DIR}/unistd.c"
)

list(APPEND includes
    "${CMAKE_CURRENT_LIST_DIR}/include"
)

set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/heap.c" PROPERTIES COMPILE_FLAGS -fno-builtin)
set_source_files_properties("${CMAKE_CURRENT_LIST_DIR}/strerror.c" PROPERTIES COMPILE_FLAGS -fno-builtin)
