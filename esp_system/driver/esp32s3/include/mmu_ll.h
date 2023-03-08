/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// The LL layer for MMU register operations

#pragma once

#include "soc/extmem_reg.h"
#include "soc/ext_mem_defs.h"


typedef enum {
    MMU_MEM_CAP_EXEC  = BIT(0),
    MMU_MEM_CAP_READ  = BIT(1),
    MMU_MEM_CAP_WRITE = BIT(2),
    MMU_MEM_CAP_32BIT = BIT(3),
    MMU_MEM_CAP_8BIT  = BIT(4),
} mmu_mem_caps_t;

/**
 * MMU Page size
 */
typedef enum {
    MMU_PAGE_8KB = 0x2000,
    MMU_PAGE_16KB = 0x4000,
    MMU_PAGE_32KB = 0x8000,
    MMU_PAGE_64KB = 0x10000,
} mmu_page_size_t;

/**
 * MMU virtual address type
 */
typedef enum {
    MMU_VADDR_DATA,
    MMU_VADDR_INSTRUCTION,
} mmu_vaddr_t;

/**
 * External physical memory
 */
typedef enum {
    MMU_TARGET_FLASH0 = BIT(0),
    MMU_TARGET_PSRAM0 = BIT(1),
} mmu_target_t;

/**
 * MMU table id
 */
typedef enum {
    MMU_TABLE_CORE0,
    MMU_TABLE_CORE1,
} mmu_table_id_t;


#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert MMU virtual address to linear address
 *
 * @param vaddr  virtual address
 *
 * @return linear address
 */
static inline uint32_t mmu_ll_vaddr_to_laddr(uint32_t vaddr)
{
    return vaddr & SOC_MMU_LINEAR_ADDR_MASK;
}

/**
 * Convert MMU linear address to virtual address
 *
 * @param laddr       linear address
 * @param vaddr_type  virtual address type, could be instruction type or data type. See `mmu_vaddr_t`
 *
 * @return virtual address
 */
static inline uint32_t mmu_ll_laddr_to_vaddr(uint32_t laddr, mmu_vaddr_t vaddr_type)
{
    uint32_t vaddr_base = 0;
    if (vaddr_type == MMU_VADDR_DATA) {
        vaddr_base = SOC_MMU_DBUS_VADDR_BASE;
    } else {
        vaddr_base = SOC_MMU_IBUS_VADDR_BASE;
    }

    return vaddr_base | laddr;
}

/**
 * Get MMU page size
 *
 * @return MMU page size code
 */
__attribute__((always_inline))
static inline mmu_page_size_t mmu_ll_get_page_size(void)
{
    return MMU_PAGE_64KB;
}

/**
 * Check if the external memory vaddr region is valid
 *
 * @param vaddr_start start of the virtual address
 * @param len         length, in bytes
 *
 * @return
 *         True for valid
 */
__attribute__((always_inline))
static inline bool mmu_ll_check_valid_ext_vaddr_region(uint32_t vaddr_start, uint32_t len)
{
    uint32_t vaddr_end = vaddr_start + len - 1;
    return (ADDRESS_IN_IRAM0_CACHE(vaddr_start) && ADDRESS_IN_IRAM0_CACHE(vaddr_end)) || (ADDRESS_IN_DRAM0_CACHE(vaddr_start) && ADDRESS_IN_DRAM0_CACHE(vaddr_end));
}

/**
 * Check if the paddr region is valid
 *
 * @param paddr_start start of the physical address
 * @param len         length, in bytes
 *
 * @return
 *         True for valid
 */
static inline bool mmu_ll_check_valid_paddr_region(uint32_t paddr_start, uint32_t len)
{
    return (paddr_start < (mmu_ll_get_page_size() * MMU_MAX_PADDR_PAGE_NUM)) &&
           (len < (mmu_ll_get_page_size() * MMU_MAX_PADDR_PAGE_NUM)) &&
           ((paddr_start + len - 1) < (mmu_ll_get_page_size() * MMU_MAX_PADDR_PAGE_NUM));
}

/**
 * To get the MMU table entry id to be mapped
 *
 * @param vaddr   virtual address to be mapped
 *
 * @return
 *         MMU table entry id
 */
__attribute__((always_inline))
static inline uint32_t mmu_ll_get_entry_id(uint32_t vaddr)
{
    return ((vaddr & MMU_VADDR_MASK) >> 16);
}

/**
 * Format the paddr to be mappable
 *
 * @param paddr   physical address to be mapped
 * @param target  paddr memory target, not used
 *
 * @return
 *         mmu_val - paddr in MMU table supported format
 */
__attribute__((always_inline))
static inline uint32_t mmu_ll_format_paddr(uint32_t paddr, mmu_target_t target)
{
    (void)target;
    return paddr >> 16;
}

/**
 * Write to the MMU table to map the virtual memory and the physical memory
 *
 * @param entry_id MMU entry ID
 * @param mmu_val  Value to be set into an MMU entry, for physical address
 * @param target   MMU target physical memory.
 */
__attribute__((always_inline))
static inline void mmu_ll_write_entry(uint32_t entry_id, uint32_t mmu_val, mmu_target_t target)
{
    assert(entry_id < MMU_ENTRY_NUM);

    uint32_t target_code = (target == MMU_TARGET_FLASH0) ? MMU_ACCESS_FLASH : MMU_ACCESS_SPIRAM;
    *(uint32_t *)(DR_REG_MMU_TABLE + entry_id * 4) = mmu_val | target_code | MMU_VALID;
}

/**
 * Read the raw value from MMU table
 *
 * @param entry_id MMU entry ID
 * @param mmu_val  Value to be read from MMU table
 */
__attribute__((always_inline))
static inline uint32_t mmu_ll_read_entry(uint32_t entry_id)
{
    assert(entry_id < MMU_ENTRY_NUM);

    return *(uint32_t *)(DR_REG_MMU_TABLE + entry_id * 4);
}

/**
 * Set MMU table entry as invalid
 *
 * @param entry_id MMU entry ID
 */
__attribute__((always_inline))
static inline void mmu_ll_set_entry_invalid(uint32_t entry_id)
{
    assert(entry_id < MMU_ENTRY_NUM);

    *(uint32_t *)(DR_REG_MMU_TABLE + entry_id * 4) = MMU_INVALID;
}

/**
 * Unmap all the items in the MMU table
 */
__attribute__((always_inline))
static inline void mmu_ll_unmap_all()
{
    for (int i = 0; i < MMU_ENTRY_NUM; i++) {
        mmu_ll_set_entry_invalid(i);
    }
}

/**
 * Check MMU table entry value is valid
 *
 * @param entry_id MMU entry ID
 *
 * @return         Ture for MMU entry is valid; False for invalid
 */
static inline bool mmu_ll_check_entry_valid(uint32_t entry_id)
{
    assert(entry_id < MMU_ENTRY_NUM);

    return (*(uint32_t *)(DR_REG_MMU_TABLE + entry_id * 4) & MMU_INVALID) ? false : true;
}

/**
 * Get the MMU table entry target
 *
 * @param entry_id MMU entry ID
 *
 * @return         Target, see `mmu_target_t`
 */
static inline mmu_target_t mmu_ll_get_entry_target(uint32_t entry_id)
{
    assert(entry_id < MMU_ENTRY_NUM);

    bool target_code = (*(uint32_t *)(DR_REG_MMU_TABLE + entry_id * 4)) & MMU_TYPE;
    return (target_code == MMU_ACCESS_FLASH) ? MMU_TARGET_FLASH0 : MMU_TARGET_PSRAM0;
}

/**
 * Convert MMU entry ID to paddr base
 *
 * @param entry_id MMU entry ID
 *
 * @return         paddr base
 */
static inline uint32_t mmu_ll_entry_id_to_paddr_base(uint32_t entry_id)
{
    assert(entry_id < MMU_ENTRY_NUM);

    return ((*(uint32_t *)(DR_REG_MMU_TABLE + entry_id * 4)) & MMU_VALID_VAL_MASK) << 16;
}

/**
 * Find the MMU table entry ID based on table map value
 * @note This function can only find the first match entry ID. However it is possible that a physical address
 *       is mapped to multiple virtual addresses
 *
 * @param mmu_val  map value to be read from MMU table standing for paddr
 * @param target   physical memory target, see `mmu_target_t`
 *
 * @return         MMU entry ID, -1 for invalid
 */
static inline int mmu_ll_find_entry_id_based_on_map_value(uint32_t mmu_val, mmu_target_t target)
{
    for (int i = 0; i < MMU_ENTRY_NUM; i++) {
        if (mmu_ll_check_entry_valid(i)) {
            if (mmu_ll_get_entry_target(i) == target) {
                if (((*(uint32_t *)(DR_REG_MMU_TABLE + i * 4)) & MMU_VALID_VAL_MASK) == mmu_val) {
                    return i;
                }
            }
        }
    }

    return -1;
}

/**
 * Convert MMU entry ID to vaddr base
 *
 * @param entry_id MMU entry ID
 * @param type     virtual address type, could be instruction type or data type. See `mmu_vaddr_t`
 */
static inline uint32_t mmu_ll_entry_id_to_vaddr_base(uint32_t entry_id, mmu_vaddr_t type)
{
    uint32_t laddr = entry_id << 16;

    return mmu_ll_laddr_to_vaddr(laddr, type);
}

#ifdef __cplusplus
}
#endif
