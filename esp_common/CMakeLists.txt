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
        "log.c"
        "${IDF_PATH}/components/esp_common/src/esp_err_to_name.c"
    #  NOTE: bootloader_support of anykind should not exists after system bootup
        "bootloader_support_shim.c"
    # retarget newlib
        "newlib_retarget/_init.c"
        "newlib_retarget/_exit.c"
        "newlib_retarget/err.c"
        "newlib_retarget/random.c"
        "newlib_retarget/heap.c"
        "newlib_retarget/os_sleeps.c"
        "newlib_retarget/esp_time_impl.c"
        "newlib_retarget/locks.c"
        # "newlib_retarget/poll.c"
        "newlib_retarget/pthread.c"
        "newlib_retarget/time.c"
        "newlib_retarget/sched.c"
        # "newlib_retarget/realpath.c"
        # only #defines in this file, it could never link
        #   TODO: assume this file will implement the <sys/atomic.h> at the global scope
        #   so it keep here to further examination
        # "newlib_retarget/stdatomic.c"
    REQUIRES
        "efuse"
    PRIV_REQUIRES
        "heap"
    LDFRAGMENTS
        "${IDF_PATH}/components/esp_common/common.lf"
        "${IDF_PATH}/components/esp_common/soc.lf"
)
