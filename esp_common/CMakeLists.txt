idf_component_register(
    INCLUDE_DIRS
        "include"
        "include/posix"
        "include/bootloader_support"
        "include/dummy"
        #  NOTE fix: soc => ${IDF_TARGET} subdir using relative paths
        "${IDF_PATH}/components/soc/${IDF_TARGET}/include"
    PRIV_INCLUDE_DIRS
    SRCS
        "glist.c"
        "esp_crosscore_int.c"
        "esp_dbg_ubsan.c"       # debug for undefined behavior, but not sure if its works
        "esp_log.c"
        "esp_freertos_hooks.c"
        "esp_platform.c"
    # retarget newlib
        "posix/_redirect.c"
        "posix/random.c"
        "posix/signal.c"
        "posix/heap.c"
        "posix/unistd.c"
        "posix/esp_time_impl.c"
        "posix/locks.c"
        # "posix/poll.c"
        "posix/pthread.c"
        "posix/pthread_local_storage.c"
        "posix/time.c"
        "posix/sched.c"
    # compatiable
        "bootloader_support_shim.c"
    REQUIRES
        "efuse"
    PRIV_REQUIRES
        "heap"
    LDFRAGMENTS
        "${IDF_PATH}/components/esp_common/common.lf"
        "${IDF_PATH}/components/esp_common/soc.lf"
)

# Force linking UBSAN hooks. If UBSAN is not enabled, the hooks will ultimately be removed
# due to -ffunction-sections -Wl,--gc-sections options.
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u __ubsan_include")
