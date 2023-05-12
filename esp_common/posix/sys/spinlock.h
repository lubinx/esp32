#ifdef __XTENSA__
    #include <xtensa/spinlock.h>
#elif __riscv
    #include <risv/spinlock.h>
#endif
