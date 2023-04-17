idf_component_register(
    SRCS
        "xt_trax.c"
        "xtensa_intr.c"
        "xtensa_intr_asm.S"
    INCLUDE_DIRS
        "include"
        "${IDF_TARGET}/include"
)

target_link_libraries(${COMPONENT_LIB} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/${IDF_TARGET}/libxt_hal.a")
