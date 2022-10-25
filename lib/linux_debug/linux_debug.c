/*
 * Copyright (C) 2020 Google LLC.
 * Copyright (C) 2021 Samsung Electronics Co. LTD
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <err.h>
#include <debug.h>
#include <arch/ops.h>
#include <lib/linux_debug.h>
#include <platform/sfr.h>
#include <string.h>

#include "linux_debug_internal.h"
#include "kallsyms.h"

#define DESCRIPTOR_INVALID      0x0
#define DESCRIPTOR_BLOCK        0x1
#define DESCRIPTOR_TABLE        0x3
#define TL_DESCRIPTOR_RESERVED  0x1
#define TL_DESCRIPTOR_PAGE      0x3

static kernel_all_info_t *all_info;
static kernel_info_t *info;
static vendor_kernel_all_info_t *vendor_all_info;
static vendor_kernel_info_t *vendor_info;
static fl_desc g_desc;
static const linux_mem_map_t *memory_map;

bool is_dram_bound(uintptr_t addr, size_t len)
{
	uint64_t input_end_addr = addr + len - 1;

	if ((len == 0) || (input_end_addr < addr))
		return false;

	for (size_t i = 0; i < memory_map->count; ++i) {
		const linux_mem_region_t *mem = &memory_map->regions[i];

		if (mem->addr <= addr && input_end_addr < mem->addr + mem->size)
			return true;
	}

	return false;
}

static fl_desc *do_level_lookup(uint64_t table_base_address, uint16_t table_index,
								uint8_t input_addr_split)
{
	uint8_t n = input_addr_split;
	uint64_t base = (table_base_address & GENMASK_ULL(47, n));
	uint16_t offset = ((table_index << 3) & GENMASK_ULL(n-1, 3));
	uint64_t phys_addr = base | offset;
	fl_desc *desc = &g_desc;

	if (!is_dram_bound(phys_addr, sizeof(uint64_t))) {
		printf("Invalid table_base addr: 0x%llx\n", table_base_address);
		return NULL;
	}

	desc->value = *(uint64_t *)(phys_addr);
	desc->dtype = (desc->value & GENMASK_ULL(1, 0)) >> 0;

	return &g_desc;
}

static fl_desc *do_fl_sl_level_lookup(uint64_t table_base_address, uint16_t table_index,
						  uint8_t input_addr_split, uint8_t block_split)
{
	fl_desc *desc = do_level_lookup(table_base_address, table_index, input_addr_split);

	if (!desc)
		return NULL;

	if (desc->dtype == DESCRIPTOR_BLOCK) {
		desc->output_address = (desc->value & GENMASK_ULL(47, block_split)) >> block_split;
		desc->next_level_base_addr_upper = 0;
	} else if (desc->dtype == DESCRIPTOR_TABLE) {
		desc->output_address = 0;
		desc->next_level_base_addr_upper = (desc->value & GENMASK_ULL(47, 12)) >> 12;
	} else
		return NULL;

	return desc;
}

static fl_desc *do_sl_level_lookup(uint64_t table_base_address, uint64_t table_index)
{
	return do_fl_sl_level_lookup(table_base_address, table_index, 12, 21);
}

static fl_desc *do_tl_level_lookup(uint64_t table_base_address, uint64_t table_index)
{
	fl_desc *desc = do_level_lookup(table_base_address, table_index, 12);

	if (!desc || (desc->dtype != TL_DESCRIPTOR_PAGE))
		return NULL;

	desc->output_address = (desc->value & GENMASK_ULL(47, 12)) >> 12;
	desc->next_level_base_addr_upper = 0;

	return desc;
}

static uint64_t block_or_page_desc_2_phys(fl_desc *desc, uint64_t page_addr, uint16_t n)
{
	return (desc->output_address << n) | ((page_addr & GENMASK_ULL(n - 1, 0)) >> 0);
}

static uint64_t fl_block_desc_2_phys(fl_desc *desc, uint64_t page_addr)
{
	/* Block descriptor to physical address. */
	return block_or_page_desc_2_phys(desc, page_addr, 30);
}

static uint64_t sl_block_desc_2_phys(fl_desc *desc, uint64_t page_addr)
{
	/* Block descriptor to physical address. */
	return block_or_page_desc_2_phys(desc, page_addr, 21);
}

static uint64_t tl_page_desc_2_phys(fl_desc *desc, uint64_t page_addr)
{
	/* Page descriptor to physical address. */
	return block_or_page_desc_2_phys(desc, page_addr, 12);
}

static uint64_t page_table_walk(uint64_t page_addr)
{
	fl_desc *desc;
	uint16_t fl_index = (page_addr & GENMASK_ULL(38, 30)) >> 30;
	uint16_t sl_index = (page_addr & GENMASK_ULL(29, 21)) >> 21;
	uint16_t tl_index = (page_addr & GENMASK_ULL(20, 12)) >> 12;

	desc = do_fl_sl_level_lookup(info->swapper_pg_dir_pa, fl_index, 12, 30);
	if (!desc)
		return 0;

	if (desc->dtype == DESCRIPTOR_BLOCK)
		return fl_block_desc_2_phys(desc, page_addr);

	desc = do_sl_level_lookup((desc->next_level_base_addr_upper) << 12, sl_index);
	if (!desc)
		return 0;

	if (desc->dtype == DESCRIPTOR_BLOCK)
		return sl_block_desc_2_phys(desc, page_addr);

	desc = do_tl_level_lookup((desc->next_level_base_addr_upper) << 12, tl_index);
	if (!desc)
		return 0;

	return tl_page_desc_2_phys(desc, page_addr);
}

uintptr_t mmu_virt_to_phys(uintptr_t va)
{
	uintptr_t page_addr = (va >> 12) << 12;
	uintptr_t page_offset = va & 0xFFF;
	uintptr_t phys_addr;

	if (info) {
		phys_addr = page_table_walk(page_addr);
		if (phys_addr)
			return phys_addr + page_offset;
	}

	return 0;
}

uintptr_t phys_to_lk_virt(uintptr_t pa)
{
	return pa ? (pa) : 0;
}

uintptr_t phys_to_lk_virt_align_4(uintptr_t pa)
{
	if (!pa || (pa & 0x3))
		return 0;
	else
		return (pa);
}

uintptr_t phys_to_lk_virt_with_align(uintptr_t pa, uint32_t align)
{
	if (!pa || (pa % align))
		return 0;
	else
		return (pa);
}

uintptr_t virt_to_lk_virt(uintptr_t va, size_t len)
{
	uintptr_t pa = mmu_virt_to_phys(va);

	if (!is_dram_bound(pa, len))
		return 0;

	return phys_to_lk_virt(pa);
}

uintptr_t virt_to_lk_virt_align_4(uintptr_t va, size_t len)
{
	uintptr_t pa = mmu_virt_to_phys(va);

	if (!is_dram_bound(pa, len) || (va & 0x3))
		return 0;

	return phys_to_lk_virt(pa);
}

uintptr_t virt_to_lk_virt_align_2(uintptr_t va, size_t len)
{
	uintptr_t pa = mmu_virt_to_phys(va);

	if (!is_dram_bound(pa, len) || (va & 0x1))
		return 0;

	return phys_to_lk_virt(pa);
}

uintptr_t virt_to_lk_virt_with_align(uintptr_t va, size_t len, uint32_t align)
{
	uintptr_t pa = mmu_virt_to_phys(va);

	if (!is_dram_bound(pa, len) || (pa % align))
		return 0;

	return phys_to_lk_virt(pa);
}

void linux_debug_unwind_backtrace(unsigned long fp, unsigned long pc, unsigned long sp_el1,
								  unsigned int depth)
{

	unsigned long mask = EL1_THREAD_SIZE - 1;
	unsigned long low = sp_el1;

	char *pc_symbol = print_symbol(pc);

	printf("\n%s\n", pc_symbol);

	unsigned int i = 0;

	do {
		unsigned long high = (low + mask) & (~mask);

		if ((fp < low) || (fp > high) || (fp & 0xf))
			break;

		low = fp + 0x10;
		unsigned long pc_p = mmu_virt_to_phys(fp + 0x8);
		unsigned long fp_p = mmu_virt_to_phys(fp);

		if (!is_dram_bound(fp_p, sizeof(uint64_t)))
			break;

		fp = *(uint64_t *)(fp_p);

		if (!is_dram_bound(pc_p, sizeof(uint64_t)))
			break;

		pc = *(uint64_t *)(pc_p);
		pc_symbol = print_symbol(pc);
		printf("%s\n", pc_symbol);
		i++;
	} while (i < depth);
}

const char *linux_debug_get_version(void)
{
	return info ? (char *)info->uts_release : "N/A";
}

const char *linux_debug_get_vendor_version(void)
{
	return vendor_info ? (char *)vendor_info->uts_release : "N/A";
}

const char *linux_debug_get_build_info(void)
{
	return info ? (char *)info->build_info : "N/A";
}

bool linux_debug_is_valid(void)
{
	return !!info;
}

bool linux_debug_is_vendor_valid(void)
{
	return !!vendor_info;
}

const char *linux_debug_print_symbol(uintptr_t va)
{
	return print_symbol(va);
}

kernel_info_t *linux_debug_get_kernel_info(void)
{
	return info;
}

void linux_debug_clear_kernel_info(void)
{
	if (all_info) {
		memset(all_info, 0, sizeof(*all_info));
		arch_clean_cache_range((addr_t)all_info, sizeof(*all_info));
	}

	if (vendor_all_info) {
		memset(vendor_all_info, 0, sizeof(*vendor_all_info));
		arch_clean_cache_range((addr_t)vendor_all_info, sizeof(*vendor_all_info));
	}
}

status_t all_info_update(uintptr_t addr, size_t size)
{
	all_info = (kernel_all_info_t *)addr;
	if (all_info == NULL)
		return ERR_INVALID_ARGS;

	if (sizeof(*all_info) > size) {
		printf("Error: all_info size must be less than 0x%08zx but 0x%08zx\n",
			   size, sizeof(*all_info));
		return ERR_INVALID_ARGS;
	}

	if (all_info->magic_number != BOOT_DEBUG_MAGIC) {
		printf("Error: magic_number must be 0x%08x but 0x%08x\n",
			   BOOT_DEBUG_MAGIC, all_info->magic_number);
		return ERR_NOT_VALID;
	}

	uint32_t kernel_info_combined_checksum = 0;

	for (uint32_t index = 0; index < sizeof(kernel_info_t)/sizeof(uint32_t); index++) {
		uint32_t *checksum_info = (uint32_t *)&all_info->info;

		kernel_info_combined_checksum ^= checksum_info[index];
	}

	if (all_info->combined_checksum != kernel_info_combined_checksum) {
		printf("Error: combined_checksum must be 0x%08x but 0x%08x\n",
			   all_info->combined_checksum, kernel_info_combined_checksum);
		return ERR_CHECKSUM_FAIL;
	}

	all_info->combined_checksum = kernel_info_combined_checksum ^ -1U;
	info = &all_info->info;

	return 0;
}

status_t vendor_all_info_update(uintptr_t addr, size_t size)
{
	vendor_all_info = (vendor_kernel_all_info_t *)addr;
	if (vendor_all_info == NULL)
		return ERR_INVALID_ARGS;

	if (sizeof(*vendor_all_info) > size) {
		printf("Error: vendor_all_info size must be less than 0x%08zx but 0x%08zx\n",
			   size, sizeof(*vendor_all_info));
		return ERR_INVALID_ARGS;
	}

	if (vendor_all_info->magic_number != BOOT_DEBUG_MAGIC) {
		printf("Error: vendor magic_number must be 0x%08x but 0x%08x\n",
			   BOOT_DEBUG_MAGIC, vendor_all_info->magic_number);
		return ERR_NOT_VALID;
	}

	uint32_t vendor_kernel_info_combined_checksum = 0;

	for (uint32_t index = 0; index < sizeof(vendor_kernel_info_t)/sizeof(uint32_t); index++) {
		uint32_t *checksum_info = (uint32_t *)&vendor_all_info->info;

		vendor_kernel_info_combined_checksum ^= checksum_info[index];
	}

	if (vendor_all_info->combined_checksum != vendor_kernel_info_combined_checksum) {
		printf("Error: vendor combined_checksum must be 0x%08x but 0x%08x\n",
			   vendor_all_info->combined_checksum, vendor_kernel_info_combined_checksum);
		return ERR_CHECKSUM_FAIL;
	}

	vendor_all_info->combined_checksum = vendor_kernel_info_combined_checksum & -1U;
	vendor_info = &vendor_all_info->info;

	return 0;
}

void linux_debug_init(uintptr_t addr, size_t size, const linux_mem_map_t *mem_map)
{
	uintptr_t offset = addr + size / 2;

	all_info_update(addr, offset - addr);
	vendor_all_info_update(offset, size - offset);

	memory_map = mem_map;
}
