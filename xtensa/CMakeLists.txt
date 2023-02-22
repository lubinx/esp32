idf_component_register(
    SRCS
        # "eri.c"
        "xt_trax.c"
        "xtensa_intr.c"
        "xtensa_intr_asm.S"
    INCLUDE_DIRS include
        "${IDF_TARGET}/include"
    LDFRAGMENTS
        # "linker.lf"
)

target_link_libraries(${COMPONENT_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/${target}/libxt_hal.a")
