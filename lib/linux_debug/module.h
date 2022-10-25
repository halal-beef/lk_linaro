/*
 * Copyright (C) 2021 Google LLC.
 * Copyright (C) 2021 Samsung Electronics Co. LTD
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

#include <stdint.h>

/* For kallsyms to ask for address resolution.  namebuf should be at
 * least KSYM_NAME_LEN long: a pointer to namebuf is returned if
 * found, otherwise NULL. */
const char *module_address_lookup(uint64_t addr,
				  uint64_t *symbolsize,
				  uint64_t *offset,
				  char **modname,
				  char *namebuf);
