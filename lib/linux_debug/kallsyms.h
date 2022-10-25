/*
 * Copyright (C) 2020 Google LLC.
 * Copyright (C) 2021 Samsung Electronics Co. LTD
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <stdint.h>

/* Lookup the address for this symbol. Returns 0 if not found. */
uint64_t kallsyms_lookup_name(const char *name);

/* Look up a kernel symbol and print it to the kernel messages. */
char *print_symbol(uintptr_t va);
