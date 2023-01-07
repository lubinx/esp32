list(APPEND srcs
    "${CMAKE_CURRENT_LIST_DIR}/panic_arch.c"
    "${CMAKE_CURRENT_LIST_DIR}/panic_handler_asm.S"
    "${CMAKE_CURRENT_LIST_DIR}/expression_with_stack.c"
    "${CMAKE_CURRENT_LIST_DIR}/expression_with_stack_asm.S"
    "${CMAKE_CURRENT_LIST_DIR}/debug_helpers.c"
    "${CMAKE_CURRENT_LIST_DIR}/debug_helpers_asm.S"
    "${CMAKE_CURRENT_LIST_DIR}/debug_stubs.c"
    "${CMAKE_CURRENT_LIST_DIR}/trax.c"
)
