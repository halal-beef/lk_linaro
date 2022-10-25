/*
 * Copyright (C) 2020 Google LLC.
 * Copyright (C) 2021 Samsung Electronics Co. LTD
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <stdbool.h>
#include <compiler.h>

#define KSYM_NAME_LEN        192
#define BITS_PER_LONG        64

/* Chosen so that structs with an unsigned long line up. */
#define MAX_PARAM_PREFIX_LEN (64 - sizeof(uint64_t))
#define MODULE_NAME_LEN      MAX_PARAM_PREFIX_LEN
#define KSYM_SYMBOL_LEN      (sizeof("%s+%#lx/%#lx [%s]") + (KSYM_NAME_LEN - 1) + \
                              2*(BITS_PER_LONG*3/10) + (MODULE_NAME_LEN - 1) + 1)
#define GENMASK_ULL(h, l)    (((~0ULL) - (1ULL << (l)) + 1) & \
                             (~0ULL >> (BITS_PER_LONG - 1 - (h))))

#define VA_BITS              39
#define PAGE_OFFSET          ((uint64_t)(0xffffffffffffffff) - \
                              ((uint64_t)(1) << (VA_BITS - 1)) + 1)
#define EL1_THREAD_SIZE      16384

#define BOOT_DEBUG_MAGIC     0xCCEEDDFF

#define NEW_UTS_LEN          64
#define BUILD_INFO_LEN       256

#define KALLSYMS_LOOKUP_NAME_TIMEOUT   (10000)
#define FIND_MODULE_TIMEOUT            (1000)
#define FIND_KALLSYMS_TIMEOUT          (1000)

typedef struct {
	/* For kallsyms */
	uint8_t  enabled_all;
	uint8_t  enabled_base_relative;
	uint8_t  enabled_absolute_percpu;
	uint8_t  enabled_cfi_clang;
	uint32_t num_syms;
	uint16_t name_len;
	uint16_t bit_per_long;
	uint16_t module_name_len;
	uint16_t symbol_len;
	uint64_t _addresses_pa;
	uint64_t _relative_pa;
	uint64_t _stext_pa;
	uint64_t _etext_pa;
	uint64_t _sinittext_pa;
	uint64_t _einittext_pa;
	uint64_t _end_pa;
	uint64_t _offsets_pa;
	uint64_t _names_pa;
	uint64_t _token_table_pa;
	uint64_t _token_index_pa;
	uint64_t _markers_pa;

	/* For frame pointer */
	uint32_t thread_size;

	/* For virt_to_phys */
	uint64_t swapper_pg_dir_pa;

	/* For linux banner */
	uint8_t uts_release[NEW_UTS_LEN];

	/* Info of running build */
	char build_info[BUILD_INFO_LEN];

	/* For module kallsyms */
	u32 enabled_modules_tree_lookup;
	u32 mod_core_layout_offset;
	u32 mod_init_layout_offset;
	u32 mod_kallsyms_offset;
	u64 module_start_va;
	u64 module_end_va;
} __PACKED kernel_info_t;

typedef struct {
	uint32_t magic_number;
	uint32_t combined_checksum;
	kernel_info_t info;
} __PACKED kernel_all_info_t;

typedef struct {
	/* For linux banner */
	uint8_t uts_release[NEW_UTS_LEN];
} __PACKED vendor_kernel_info_t;

typedef struct {
	uint32_t magic_number;
	uint32_t combined_checksum;
	vendor_kernel_info_t info;
} __PACKED vendor_kernel_all_info_t;

typedef struct {
	uint8_t  dtype;
	uint64_t value;
	uint64_t output_address;
	uint64_t next_level_base_addr_upper;
} fl_desc;

bool is_dram_bound(uintptr_t addr, size_t len);
uintptr_t mmu_virt_to_phys(uintptr_t va);
uintptr_t phys_to_lk_virt(uintptr_t pa);
uintptr_t phys_to_lk_virt_with_align(uintptr_t pa, uint32_t align);
uintptr_t virt_to_lk_virt(uintptr_t va, size_t len);
uintptr_t virt_to_lk_virt_with_align(uintptr_t va, size_t len, uint32_t align);
kernel_info_t *linux_debug_get_kernel_info(void);
