/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#define BIT63                           (0x80000000ULL << 32)
#define BIT62                           (0x40000000ULL << 32)
#define BIT61                           (0x20000000ULL << 32)
#define BIT60                           (0x10000000ULL << 32)
#define BIT59                           (0x08000000ULL << 32)
#define BIT58                           (0x04000000ULL << 32)
#define BIT57                           (0x02000000ULL << 32)
#define BIT56                           (0x01000000ULL << 32)
#define BIT55                           (0x00800000ULL << 32)
#define BIT54                           (0x00400000ULL << 32)
#define BIT53                           (0x00200000ULL << 32)
#define BIT52                           (0x00100000ULL << 32)
#define BIT51                           (0x00080000ULL << 32)
#define BIT50                           (0x00040000ULL << 32)
#define BIT49                           (0x00020000ULL << 32)
#define BIT48                           (0x00010000ULL << 32)
#define BIT47                           (0x00008000ULL << 32)
#define BIT46                           (0x00004000ULL << 32)
#define BIT45                           (0x00002000ULL << 32)
#define BIT44                           (0x00001000ULL << 32)
#define BIT43                           (0x00000800ULL << 32)
#define BIT42                           (0x00000400ULL << 32)
#define BIT41                           (0x00000200ULL << 32)
#define BIT40                           (0x00000100ULL << 32)
#define BIT39                           (0x00000080ULL << 32)
#define BIT38                           (0x00000040ULL << 32)
#define BIT37                           (0x00000020ULL << 32)
#define BIT36                           (0x00000010ULL << 32)
#define BIT35                           (0x00000008ULL << 32)
#define BIT34                           (0x00000004ULL << 32)
#define BIT33                           (0x00000002ULL << 32)
#define BIT32                           (0x00000001ULL << 32)

#define BIT31                           (0x80000000U)
#define BIT30                           (0x40000000U)
#define BIT29                           (0x20000000U)
#define BIT28                           (0x10000000U)
#define BIT27                           (0x08000000U)
#define BIT26                           (0x04000000U)
#define BIT25                           (0x02000000U)
#define BIT24                           (0x01000000U)
#define BIT23                           (0x00800000U)
#define BIT22                           (0x00400000U)
#define BIT21                           (0x00200000U)
#define BIT20                           (0x00100000U)
#define BIT19                           (0x00080000U)
#define BIT18                           (0x00040000U)
#define BIT17                           (0x00020000U)
#define BIT16                           (0x00010000U)
#define BIT15                           (0x00008000U)
#define BIT14                           (0x00004000U)
#define BIT13                           (0x00002000U)
#define BIT12                           (0x00001000U)
#define BIT11                           (0x00000800U)
#define BIT10                           (0x00000400U)
#define BIT9                            (0x00000200U)
#define BIT8                            (0x00000100U)
#define BIT7                            (0x00000080U)
#define BIT6                            (0x00000040U)
#define BIT5                            (0x00000020U)
#define BIT4                            (0x00000010U)
#define BIT3                            (0x00000008U)
#define BIT2                            (0x00000004U)
#define BIT1                            (0x00000002U)
#define BIT0                            (0x00000001U)


#ifndef __ASSEMBLER__
    #ifndef BIT
        #define BIT(nr)                 (1UL << (nr))
    #endif
    #ifndef BIT64
        #define BIT64(nr)               (1ULL << (nr))
    #endif
#else
    #ifndef BIT
        #define BIT(nr)                 (1 << (nr))
    #endif
#endif
