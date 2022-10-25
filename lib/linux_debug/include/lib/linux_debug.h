/*
 * Copyright (C) 2020 Google LLC.
 * Copyright (C) 2021 Samsung Electronics Co. LTD
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <err.h>
#include <stdint.h>

typedef struct {
	uintptr_t addr;
	size_t size;
} linux_mem_region_t;

typedef struct {
	const linux_mem_region_t *regions;
	size_t count;
} linux_mem_map_t;

void linux_debug_unwind_backtrace(unsigned long fp, unsigned long pc, unsigned long sp_el1,
								  unsigned int depth);
const char *linux_debug_print_symbol(uintptr_t va);
const char *linux_debug_get_version(void);
const char *linux_debug_get_vendor_version(void);
const char *linux_debug_get_build_info(void);
bool linux_debug_is_valid(void);
bool linux_debug_is_vendor_valid(void);
void linux_debug_clear_kernel_info(void);
void linux_debug_init(uintptr_t addr, size_t size, const linux_mem_map_t *mem_map);
