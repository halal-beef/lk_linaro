/*
 * Copyright (C) 2020 Google LLC.
 * Copyright (C) 2021 Samsung Electronics Co. LTD
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <platform.h>
#include <lib/linux_debug.h>
#include <platform/sfr.h>
#include <sys/types.h>

#include "linux_debug_internal.h"
#include "kallsyms.h"
#include "module.h"

#define UNKNOWN_SYMBOL  "UNKNOWN+0x0"

static char kallsyms_buffer[KSYM_SYMBOL_LEN];
static kernel_info_t *info = NULL;

static bool is_kernel_inittext(uintptr_t addr)
{
	return (addr >= info->_sinittext_pa && addr <= info->_einittext_pa);
}

static bool is_kernel_text(uintptr_t addr)
{
	return ((addr >= info->_stext_pa && addr <= info->_etext_pa));
}

static bool is_kernel(uintptr_t addr)
{
	return (addr >= info->_stext_pa && addr <= info->_end_pa);
}

static bool is_ksym_addr(uintptr_t addr)
{
	if (info->enabled_all)
		return is_kernel(addr);

	return is_kernel_text(addr) || is_kernel_inittext(addr);
}

/*
 * Expand a compressed symbol data into the resulting uncompressed string,
 * if uncompressed string is too long (>= maxlen), it will be truncated,
 * given the offset to where the symbol is in the compressed stream.
 */
static int64_t kallsyms_expand_symbol(uint32_t off, char *result, size_t maxlen)
{
	uint8_t *kallsyms_names = (uint8_t *)phys_to_lk_virt(info->_names_pa);
	uint8_t *kallsyms_token_table = (uint8_t *)phys_to_lk_virt(info->_token_table_pa);
	uint16_t *kallsyms_token_index = (uint16_t *)phys_to_lk_virt(info->_token_index_pa);
	int len, skipped_first = 0;
	const uint8_t *tptr, *data;

	/* Get the compressed symbol length from the first symbol byte. */
	data = &kallsyms_names[off];
	len = *data;
	data++;

	if (len >= (int)maxlen)
		return -1;

	/*
	 * Update the offset to return the offset for the next symbol on
	 * the compressed stream.
	 */
	off += len + 1;

	/*
	 * For every byte on the compressed symbol data, copy the table
	 * entry for that byte.
	 */
	while (len) {
		tptr = &kallsyms_token_table[kallsyms_token_index[(uint8_t)*data]];
		data++;
		len--;

		while (*tptr) {
			if (skipped_first) {
				if (maxlen <= 1)
					goto tail;
				*result = *tptr;
				result++;
				maxlen--;
			} else
				skipped_first = 1;

			tptr++;
		}
	}

tail:
	if (maxlen)
		*result = '\0';

	/* Return to offset to the next symbol. */
	return off;
}

/*
 * Find the offset on the compressed stream given and index in the
 * kallsyms array.
 */
static uint32_t get_symbol_offset(uintptr_t pos)
{
	uint8_t *kallsyms_names = (uint8_t *)phys_to_lk_virt(info->_names_pa);
	uint64_t *kallsyms_markers_64;
	uint32_t *kallsyms_markers_32;
	const uint8_t *name;
	uint32_t i;

	/*
	 * Use the closest marker we have. We have markers every 256 positions,
	 * so that should be close enough.
	 */
	// [TODO] Remove judgment if kernel 5.4 or later are launched
	if (!strncmp(linux_debug_get_version(), "4.19", 4)) { // kernel 4.19
		kallsyms_markers_64 = (uint64_t *)phys_to_lk_virt(info->_markers_pa);
		name = &kallsyms_names[kallsyms_markers_64[pos >> 8]];
	} else { // kernel 5.4 or later
		kallsyms_markers_32 = (uint32_t *)phys_to_lk_virt(info->_markers_pa);
		name = &kallsyms_names[kallsyms_markers_32[pos >> 8]];
	}

	/*
	 * Sequentially scan all the symbols up to the point we're searching
	 * for. Every symbol is stored in a [<len>][<len> bytes of data] format,
	 * so we just need to add the len to the current pointer nn..for every
	 * symbol we wish to skip.
	 */
	for (i = 0; i < (pos & 0xFF); i++)
		name = name + (*name) + 1;

	return name - kallsyms_names;
}

static uint64_t kallsyms_sym_address(uint64_t idx)
{
	uint64_t *kallsyms_addresses = (uint64_t *)phys_to_lk_virt(info->_addresses_pa);
	uint64_t kallsyms_relative_base = info->_relative_pa;
	int *kallsyms_offsets = (int *)phys_to_lk_virt(info->_offsets_pa);

	if (!info->enabled_base_relative)
		return (uint64_t)mmu_virt_to_phys(kallsyms_addresses[idx]);

	/* values are unsigned offsets if --absolute-percpu is not in effect */
	if (!info->enabled_absolute_percpu)
		return kallsyms_relative_base + (uint64_t)kallsyms_offsets[idx];

	/* ...otherwise, positive offsets are absolute values */
	if (kallsyms_offsets[idx] >= 0)
		return (uint64_t)mmu_virt_to_phys(kallsyms_offsets[idx]);

	/* ...and negative offsets are relative to kallsyms_relative_base - 1 */
	return kallsyms_relative_base - 1 - kallsyms_offsets[idx];
}

/* Lookup the address for this symbol. Returns 0 if not found. */
uint64_t kallsyms_lookup_name(const char *name)
{
	char namebuf[KSYM_NAME_LEN];
	int64_t off = 0;
	lk_time_t to = current_time() + KALLSYMS_LOOKUP_NAME_TIMEOUT;

	for (uint64_t i = 0; i < info->num_syms; i++) {
		if (current_time() >= to) {
			printf("finding '%s' symbol timeout!\n", name);
			return 0;
		}
		off = kallsyms_expand_symbol(off, namebuf, ARRAY_SIZE(namebuf));

		if (off < 0)
			break;
		if (strcmp(namebuf, name) == 0)
			return kallsyms_sym_address(i);
	}

	return 0;
}

static uint64_t get_symbol_pos(uintptr_t addr, uint64_t *symbolsize, uint64_t *offset)
{
	uint64_t *kallsyms_addresses = (uint64_t *)info->_addresses_pa;
	int *kallsyms_offsets = (int *)phys_to_lk_virt(info->_offsets_pa);
	uint64_t symbol_start = 0, symbol_end = 0;
	uint64_t i, low, high, mid;

	/* This kernel should never had been booted. */
	if (!info->enabled_base_relative) {
		if (!kallsyms_addresses) {
			printf("%s: NULL kallsyms_addresses\n", __func__);
			return 0;
		}
	} else {
		if (!kallsyms_offsets) {
			printf("%s: NULL kallsyms_offsets\n", __func__);
			return 0;
		}
	}

	/* Do a binary search on the sorted kallsyms_addresses array. */
	low = 0;
	high = info->num_syms;

	while (high - low > 1) {
		mid = low + (high - low) / 2;
		if (kallsyms_sym_address(mid) <= addr)
			low = mid;
		else
			high = mid;
	}

	/*
	 * Search for the first aliased symbol. Aliased
	 * symbols are symbols with the same address.
	 */
	while (low && kallsyms_sym_address(low-1) == kallsyms_sym_address(low))
		--low;

	symbol_start = kallsyms_sym_address(low);

	/* Search for next non-aliased symbol. */
	for (i = low + 1; i < info->num_syms; i++) {
		if (kallsyms_sym_address(i) > symbol_start) {
			symbol_end = kallsyms_sym_address(i);
			break;
		}
	}

	/* If we found no next symbol, we use the end of the section. */
	if (!symbol_end) {
		if (is_kernel_inittext(addr))
			symbol_end = info->_einittext_pa;
		else if (info->enabled_all)
			symbol_end = info->_end_pa;
		else
			symbol_end = info->_etext_pa;
	}

	if (symbolsize)
		*symbolsize = symbol_end - symbol_start;

	if (offset)
		*offset = addr - symbol_start;

	return low;
}

/*
 * Lookup an address
 * - modname is set to NULL if it's in the kernel.
 * - We guarantee that the returned name is valid until we reschedule even if.
 *   It resides in a module.
 * - We also guarantee that modname will be valid until rescheduled.
 */
static const char *kallsyms_lookup(uintptr_t addr, uint64_t *symbolsize, uint64_t *offset,
								   char **modname, char *namebuf)
{
	namebuf[KSYM_NAME_LEN - 1] = 0;
	namebuf[0] = 0;
	uintptr_t addr_pa = mmu_virt_to_phys(addr);

	if (is_ksym_addr(addr_pa)) {
		uint32_t pos;
		int64_t off;

		pos = get_symbol_pos(addr_pa, symbolsize, offset);

		/* Grab name */
		off = kallsyms_expand_symbol(get_symbol_offset(pos), namebuf, KSYM_NAME_LEN);

		if (modname)
			*modname = NULL;

		if (off < 0)
			return NULL;

		return namebuf;
	}

	/* See if it's in a module. */
	const char *ret = module_address_lookup(addr, symbolsize, offset, modname, namebuf);

	return ret;
}

/* Look up a kernel symbol and return it in a text buffer. */
static int __sprint_symbol(char *buffer, uintptr_t address, int symbol_offset, int add_offset)
{
	char *modname;
	const char *name;
	uint64_t offset = 0, size = 0;
	int len;

	address += symbol_offset;
	name = kallsyms_lookup(address, &size, &offset, &modname, buffer);
	if (!name)
		return snprintf(buffer, KSYM_SYMBOL_LEN, UNKNOWN_SYMBOL);

	if (name != buffer)
		strcpy(buffer, name);

	len = strlen(buffer);
	offset -= symbol_offset;

	if (add_offset)
		len += snprintf(buffer + len, KSYM_SYMBOL_LEN - len, "+%#llx", offset);

	if (modname)
		len += snprintf(buffer + len, KSYM_SYMBOL_LEN - len, " [%s]", modname);

	return len;
}

/**
 * sprint_symbol - Look up a kernel symbol and return it in a text buffer
 * @buffer: buffer to be stored
 * @address: address to lookup
 *
 * This function looks up a kernel symbol with @address and stores its name,
 * offset, size and module name to @buffer if possible. If no symbol was found,
 * just saves its @address as is.
 *
 * This function returns the number of bytes stored in @buffer.
 */
static int sprint_symbol(char *buffer, uintptr_t address)
{
	return __sprint_symbol(buffer, address, 0, 1);
}

/* Look up a kernel symbol and print it to the kernel messages. */
char *print_symbol(uintptr_t va)
{
	uintptr_t pa;

	if (!info)
		info = linux_debug_get_kernel_info();

	pa = mmu_virt_to_phys(va);
	if (info && is_dram_bound(pa, sizeof(uint64_t)))
		sprint_symbol(kallsyms_buffer, va);
	else {
		kallsyms_buffer[0] = 0;
		snprintf(kallsyms_buffer, KSYM_SYMBOL_LEN, UNKNOWN_SYMBOL);
	}

	return kallsyms_buffer;
}
