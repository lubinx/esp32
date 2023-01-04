#pragma once

#include <sys/features.h>
#include <sys/cdefs.h>

#ifndef __CONCAT
    #define __CONCAT(x, y)          x##y
#endif
#ifndef __STRING
    #define __STRING(x)             #x
#endif

#define __VA_SELECT(NAME, ARG_CNT)  __CONCAT(NAME##_, ARG_CNT)
#define __VA_SIZE(...)              __VA_COUNT( __VA_ARGS__, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#define __VA_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, _9, __VA_COUNT, ...) __VA_COUNT
#define __VA_OVERLOAD(NAME, ...)    __VA_SELECT(NAME, __VA_SIZE(__VA_ARGS__))(__VA_ARGS__)

#define ARG_UNUSED(...)             __VA_OVERLOAD(__ARG_UNUSED, __VA_ARGS__)
#define __ARG_UNUSED_0
#define __ARG_UNUSED_1(p1)          \
    ((void)p1)
#define __ARG_UNUSED_2(p1, p2)      \
    ((void)p1, (void)p2)
#define __ARG_UNUSED_3(p1, p2, p3)  \
    ((void)p1, (void)p2, (void)p3)
#define __ARG_UNUSED_4(p1, p2, p3, p4)  \
    ((void)p1, (void)p2, (void)p3, (void)p4)
#define __ARG_UNUSED_5(p1, p2, p3, p4, p5)  \
    ((void)p1, (void)p2, (void)p3, (void)p4, (void)p5)
#define __ARG_UNUSED_6(p1, p2, p3, p4, p5, p6)  \
    ((void)p1, (void)p2, (void)p3, (void)p4, (void)p5, (void)p6)
#define __ARG_UNUSED_7(p1, p2, p3, p4, p5, p6, p7)  \
    ((void)p1, (void)p2, (void)p3, (void)p4, (void)p5, (void)p6, (void)p7)
#define __ARG_UNUSED_8(p1, p2, p3, p4, p5, p6, p7, p8)  \
    ((void)p1, (void)p2, (void)p3, (void)p4, (void)p5, (void)p6, (void)p7, (void)p8)
#define __ARG_UNUSED_9(p1, p2, p3, p4, p5, p6, p7, p8, p9)  \
    ((void)p1, (void)p2, (void)p3, (void)p4, (void)p5, (void)p6, (void)p7, (void)p8, (void)p9)
