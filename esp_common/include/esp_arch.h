#pragma once

#ifdef __XTENSA__
    #include "xt_utils.h"
    #include "xtensa/xtensa_api.h"
#elif __riscv
    #include "riscv/rv_utils.h"
#endif
