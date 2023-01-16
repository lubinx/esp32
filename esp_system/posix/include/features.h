#pragma once

#include <sys/features.h>
#include <sys/cdefs.h>
    #ifndef STRINGIFY
        #define STRINGIFY(val)              #val
    #endif

    // REVIEW: restrict keyword not defined?
    #define restrict

    #define ARG_UNUSED(...)             __ARG_UNUSED_TEARDOWN(__VA_ARGS__,  \
            __ARG_UNUSED_15, __ARG_UNUSED_14, __ARG_UNUSED_13, __ARG_UNUSED_12, __ARG_UNUSED_11,    \
            __ARG_UNUSED_10, __ARG_UNUSED_9, __ARG_UNUSED_8, __ARG_UNUSED_7, __ARG_UNUSED_6,    \
            __ARG_UNUSED_5, __ARG_UNUSED_4, __ARG_UNUSED_3, __ARG_UNUSED_2, __ARG_UNUSED_1)(__VA_ARGS__)
        // teardown
        #define __ARG_UNUSED_TEARDOWN(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, TEARDOWN, ...) TEARDOWN
        // ARG_UNUSED(1..9)
        #define __ARG_UNUSED_1(p1)      \
            ((void)p1)
        #define __ARG_UNUSED_2(p1, p2)  \
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
        #define __ARG_UNUSED_10(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10) \
            ((void)p1, (void)p2, (void)p3, (void)p4, (void)p5, (void)p6, (void)p7, (void)p8, (void)p9, (void)p10)
        #define __ARG_UNUSED_11(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)    \
            ((void)p1, (void)p2, (void)p3, (void)p4, (void)p5, (void)p6, (void)p7, (void)p8, (void)p9, (void)p10, (void)p11)
        #define __ARG_UNUSED_12(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12)   \
            ((void)p1, (void)p2, (void)p3, (void)p4, (void)p5, (void)p6, (void)p7, (void)p8, (void)p9, (void)p10, (void)p11, (void)p12)
        #define __ARG_UNUSED_13(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)  \
            ((void)p1, (void)p2, (void)p3, (void)p4, (void)p5, (void)p6, (void)p7, (void)p8, (void)p9, (void)p10, (void)p11, (void)p12, (void)p13)
        #define __ARG_UNUSED_14(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14) \
            ((void)p1, (void)p2, (void)p3, (void)p4, (void)p5, (void)p6, (void)p7, (void)p8, (void)p9, (void)p10, (void)p11, (void)p12, (void)p13, (void)p14)
        #define __ARG_UNUSED_15(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15)    \
            ((void)p1, (void)p2, (void)p3, (void)p4, (void)p5, (void)p6, (void)p7, (void)p8, (void)p9, (void)p10, (void)p11, (void)p12, (void)p13, (void)p14, (void)p15)

    #ifndef MAX
    #define MAX(...)                    __MAX_TEARDOWN(__VA_ARGS__, \
           __MAX_15, __MAX_14, __MAX_13, __MAX_12, __MAX_11,    \
           __MAX_10, __MAX_9, __MAX_8, __MAX_7, __MAX_6,    \
           __MAX_5, __MAX_4, __MAX_3, __MAX_2, __MAX_1)(__VA_ARGS__)
        // teardown
        #define __MAX_TEARDOWN(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, TEARDOWN, ...) TEARDOWN
        // MAX(1..9)
        #define __MAX_1(p1)             \
            (p1)
        #define __MAX_2(p1, p2)         \
            ((p1) > (p2) ? (p1) : (p2))
        #define __MAX_3(p1, p2, p3)     \
            __MAX_2(__MAX_2(p1, p2), p3)
        #define __MAX_4(p1, p2, p3, p4) \
            __MAX_2(__MAX_3(p1, p2, p3), p4)
        #define __MAX_5(p1, p2, p3, p4, p5) \
            __MAX_2(__MAX_4(p1, p2, p3, p4), p5)
        #define __MAX_6(p1, p2, p3, p4, p5, p6) \
            __MAX_2(__MAX_5(p1, p2, p3, p4, p5), p6)
        #define __MAX_7(p1, p2, p3, p4, p5, p6, p7) \
            __MAX_2(__MAX_6(p1, p2, p3, p4, p5, p6), p7)
        #define __MAX_8(p1, p2, p3, p4, p5, p6, p7, p8) \
            __MAX_2(__MAX_7(p1, p2, p3, p4, p5, p6, p7), p8)
        #define __MAX_9(p1, p2, p3, p4, p5, p6, p7, p8, p9) \
            __MAX_2(__MAX_8(p1, p2, p3, p4, p5, p6, p7, p8), p9)
        #define __MAX_10(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)   \
            __MAX_2(__MAX_9(p1, p2, p3, p4, p5, p6, p7, p8, p9), p10)
        #define __MAX_11(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)  \
            __MAX_2(__MAX_10(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10), p11)
        #define __MAX_12(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12) \
            __MAX_2(__MAX_11(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11), p12)
        #define __MAX_13(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)    \
            __MAX_2(__MAX_12(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12), p13)
        #define __MAX_14(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14)   \
            __MAX_2(__MAX_13(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13), p14)
        #define __MAX_15(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15)   \
            __MAX_2(__MAX_14(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14), p15)
    #endif

    #ifndef MIN
    #define MIN(...)                    __MIN_TEARDOWN(__VA_ARGS__, \
           __MIN_15, __MIN_14, __MIN_13, __MIN_12, __MIN_11,    \
           __MIN_10, __MIN_9, __MIN_8, __MIN_7, __MIN_6,    \
           __MIN_5, __MIN_4, __MIN_3, __MIN_2, __MIN_1)(__VA_ARGS__)
        // teardown
        #define __MIN_TEARDOWN(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, TEARDOWN, ...) TEARDOWN
        // MIN(1..9)
        #define __MIN_1(p1)             \
            (p1)
        #define __MIN_2(p1, p2)         \
            ((p1) < (p2) ? (p1) : (p2))
        #define __MIN_3(p1, p2, p3)     \
            __MIN_2(__MIN_2(p1, p2), p3)
        #define __MIN_4(p1, p2, p3, p4) \
            __MIN_2(__MIN_3(p1, p2, p3), p4)
        #define __MIN_5(p1, p2, p3, p4, p5) \
            __MIN_2(__MIN_4(p1, p2, p3, p4), p5)
        #define __MIN_6(p1, p2, p3, p4, p5, p6) \
            __MIN_2(__MIN_5(p1, p2, p3, p4, p5), p6)
        #define __MIN_7(p1, p2, p3, p4, p5, p6, p7) \
            __MIN_2(__MIN_6(p1, p2, p3, p4, p5, p6), p7)
        #define __MIN_8(p1, p2, p3, p4, p5, p6, p7, p8) \
            __MIN_2(__MIN_7(p1, p2, p3, p4, p5, p6, p7), p8)
        #define __MIN_9(p1, p2, p3, p4, p5, p6, p7, p8, p9) \
            __MIN_2(__MIN_8(p1, p2, p3, p4, p5, p6, p7, p8), p9)
        #define __MIN_10(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10)   \
            __MIN_2(__MIN_9(p1, p2, p3, p4, p5, p6, p7, p8, p9), p10)
        #define __MIN_11(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11)  \
            __MIN_2(__MIN_10(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10), p11)
        #define __MIN_12(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12) \
            __MIN_2(__MIN_11(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11), p12)
        #define __MIN_13(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13)    \
            __MIN_2(__MIN_12(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12), p13)
        #define __MIN_14(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14)   \
            __MIN_2(__MIN_13(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13), p14)
        #define __MIN_15(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14, p15)   \
            __MIN_2(__MIN_14(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11, p12, p13, p14), p15)
    #endif

    #ifndef ABS
        #define ABS(n)                  ((n) > 0 ? (n) : (-n))
    #endif

/* helper to get array element count */
    #ifndef lengthof
        #define lengthof(ARY)           (sizeof(ARY) / sizeof(ARY[0]))
    #endif

/* helper to get struct member's offset */
    #ifndef offsetof
        #define offsetof(STRUCT_TYPE, MEMBER)   \
            ((uintptr_t)&((STRUCT_TYPE *)0x0)->MEMBER)
    #endif

/* helper to get struct member's sizeof */
    #define sizeof_member(STRUCT_TYPE, MEMBER)  \
        sizeof(((STRUCT_TYPE *)0)->MEMBER)
