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
        "esp_platform.c"
        "glist.c"
        "log.c"
        "ubsan.c"
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
        # "posix/realpath.c"
        # only #defines in this file, it could never link
        #   TODO: assume this file will implement the <sys/atomic.h> at the global scope
        #   so it keep here to further examination
        # "posix/stdatomic.c"
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
