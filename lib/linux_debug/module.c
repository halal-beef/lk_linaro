/*
 * Copyright (C) 2021 Google LLC.
 * Copyright (C) 2021 Samsung Electronics Co. LTD
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#include <debug.h>
#include <stdio.h>
#include <string.h>
#include <platform/sfr.h>
#include <platform.h>

#include "linux_debug_internal.h"
#include "kallsyms.h"
#include "module.h"

#define container_of(ptr, type, member) ({                         \
        const typeof(((type *)0)->member) *__mptr = (ptr);         \
        (type *)((uint8_t *)__mptr - offsetof(type, member) );})   \

static kernel_info_t *info = NULL;

struct list_head {
	struct list_head *next, *prev;
};

/* Refer from linux/include/linux/module.h */
enum module_state {
	MODULE_STATE_LIVE,     /* Normal state. */
	MODULE_STATE_COMING,   /* Full formed, running module_init. */
	MODULE_STATE_GOING,    /* Going away. */
	MODULE_STATE_UNFORMED, /* Still setting it up. */
};

struct module_layout {
	/* The actual code + data. */
	void *base;
	/* Total size. */
	uint32_t size;
	/* The size of the executable code.  */
	uint32_t text_size;
	/* Size of RO section of the module (text+rodata) */
	uint32_t ro_size;
	/* Size of RO after init section */
	uint32_t ro_after_init_size;

	/* Unused */
	/* struct mod_tree_node mtn; */
};

/* special section indexes */
#define SHN_UNDEF    0

typedef struct elf64_sym {
	uint32_t    st_name;    /* Symbol name, index in string tbl */
	uint8_t     st_info;    /* Type and binding attributes */
	uint8_t     st_other;   /* No defined meaning, 0 */
	uint16_t    st_shndx;   /* Associated section index */
	uint64_t    st_value;   /* Value of the symbol */
	uint64_t    st_size;    /* Associated symbol size */
} Elf64_Sym;

#define Elf_Sym    Elf64_Sym

struct mod_kallsyms {
	Elf_Sym *symtab;
	uint32_t num_symtab;
	char *strtab;
	char *typetab;
};

struct module {
	enum module_state state;

	/* Member of list of modules */
	struct list_head list;

	/* Unique handle for this module */
	char name[MODULE_NAME_LEN];

	/* Just list 1st 3 members, other are not listed and replaced by info->mod_XXX_offset */
};

/* Refer from linux/kernel/module.c */
static bool within_module_core(uint64_t addr, const struct module *mod_vma)
{
	uintptr_t core_layout_vma = (uintptr_t)((uint8_t *)mod_vma + info->mod_core_layout_offset);
	struct module_layout *core_layout =
		(struct module_layout *)virt_to_lk_virt_with_align(core_layout_vma,
				sizeof(struct module_layout), 8);

	if (!core_layout)
		return false;

	return (uint64_t)core_layout->base <= addr &&
		   addr < (uint64_t)core_layout->base + core_layout->size;
}

static bool within_module_init(uint64_t addr, const struct module *mod_vma)
{
	uintptr_t init_layout_vma = (uintptr_t)((uint8_t *)mod_vma + info->mod_init_layout_offset);
	struct module_layout *init_layout =
		(struct module_layout *)virt_to_lk_virt_with_align(init_layout_vma,
				sizeof(struct module_layout), 8);

	if (!init_layout)
		return false;

	return (uint64_t)init_layout->base <= addr &&
		   addr < (uint64_t)init_layout->base + init_layout->size;
}

static bool within_module(uint64_t addr, const struct module *mod_vma)
{
	return within_module_init(addr, mod_vma) || within_module_core(addr, mod_vma);
}

static bool is_module_bound_valid(void)
{
	if (!info->module_start_va || !info->module_end_va ||
		info->module_start_va >= info->module_end_va)
		return false;

	return true;
}

static bool is_module_addr_in_bound(uintptr_t addr)
{
	return addr >= info->module_start_va && addr < info->module_end_va;
}

static struct module *mod_find(uint64_t addr)
{
	static bool one_time;
	static uintptr_t *modules_phys;

	if (is_module_bound_valid()) {
		if (!is_module_addr_in_bound(addr))
			return NULL;
	}

	if (!one_time) {
		modules_phys = (uintptr_t *)kallsyms_lookup_name("modules");
		one_time = true;
	}

	if (!is_dram_bound((uintptr_t)modules_phys, sizeof(struct list_head)))
		return NULL;

	struct list_head *modules = (struct list_head *)
					phys_to_lk_virt_with_align((uintptr_t)modules_phys, 4);
	if (!modules)
		return NULL;

	struct list_head *list_vma = modules->next;
	struct module *mod = NULL;

	lk_time_t to = current_time() + FIND_MODULE_TIMEOUT;

	do {
		struct module *mod_vma = container_of(list_vma, struct module, list);

		if (current_time() >= to)
			return NULL;

		if (within_module(addr, mod_vma))
			return mod_vma;

		struct list_head *pos = (struct list_head *)
						virt_to_lk_virt_with_align((uintptr_t)list_vma,
								sizeof(struct list_head), 8);
		if (!pos)
			return NULL;

		mod = (struct module *)virt_to_lk_virt_with_align((uintptr_t)mod_vma,
				sizeof(struct module), 8);
		if (!mod)
			return NULL;

		list_vma = pos->next;
	} while (&mod->list != modules);

	return NULL;
}

/*
 * __module_address - get the module which contains an address.
 * @addr: the address.
 */
static struct module *__module_address(uint64_t addr)
{
	struct module *mod_vma = mod_find(addr);

	if (mod_vma) {
		if (!within_module(addr, mod_vma))
			return NULL;

		struct module *mod =
			(struct module *)virt_to_lk_virt_with_align((uintptr_t)mod_vma,
					sizeof(struct module), 8);
		if (!mod || mod->state == MODULE_STATE_UNFORMED)
			return NULL;
	}

	return mod_vma;
}

/*
 * This ignores the intensely annoying "mapping symbols" found
 * in ARM ELF files: $a, $t and $d.
 */
static int is_arm_mapping_symbol(const char *str)
{
	if (str[0] == '.' && str[1] == 'L')
		return true;

	return str[0] == '$' && strchr("axtd", str[1])
		   && (str[2] == '\0' || str[2] == '.');
}

static const char *
kallsyms_symbol_name(uintptr_t kallsyms_vaddr, uintptr_t symtab_vaddr)
{
	uintptr_t vaddr = kallsyms_vaddr + offsetof(struct mod_kallsyms, strtab);
	uintptr_t *strtab_p = (uintptr_t *)virt_to_lk_virt_with_align(vaddr, sizeof(uintptr_t), 4);

	vaddr = symtab_vaddr + offsetof(Elf_Sym, st_name);
	uint32_t *st_name = (uint32_t *)virt_to_lk_virt_with_align(vaddr, sizeof(uint32_t), 4);

	if (!strtab_p || !st_name)
		return NULL;

	const char *symbol_name =
		(const char *)virt_to_lk_virt((uintptr_t)(*strtab_p + *st_name), KSYM_SYMBOL_LEN);

	if (!symbol_name)
		return NULL;

	return symbol_name;
}

static uint64_t kallsyms_symbol_value(uintptr_t vaddr)
{
	uint64_t *value =
		(uint64_t *)virt_to_lk_virt_with_align(vaddr + offsetof(Elf_Sym, st_value),
				sizeof(uint64_t), 4);

	if (!value)
		return 0;

	return *value;
}

static uint16_t kallsyms_symbol_shndx(uintptr_t vaddr)
{
	uint16_t *shndx =
		(uint16_t *)virt_to_lk_virt_with_align(vaddr + offsetof(Elf_Sym, st_shndx),
				sizeof(uint16_t), 2);

	if (!shndx)
		return 0;
	return *shndx;
}

/*
 * Given a module and address, find the corresponding symbol and return its name
 * while providing its size and offset if needed.
 */
static const char *find_kallsyms_symbol(struct module *mod_vma,
					uint64_t addr,
					uint64_t *size,
					uint64_t *offset)
{
	/* "struct mod_kallsyms *kallsyms;" defined in "struct module" */
	uintptr_t vaddr = (uintptr_t)((uint8_t *)mod_vma + info->mod_kallsyms_offset);
	uintptr_t *kallsyms_p =
			(uintptr_t *)virt_to_lk_virt_with_align(vaddr, sizeof(uintptr_t), 4);

	if (!kallsyms_p)
		return NULL;

	vaddr = *kallsyms_p;
	struct mod_kallsyms *kallsyms = (struct mod_kallsyms *)
				virt_to_lk_virt_with_align(vaddr, sizeof(struct mod_kallsyms), 4);

	if (!kallsyms)
		return NULL;

	/* "struct Elf64_Sym *symtab;" defined in "struct mod_kallsyms" */
	vaddr = *kallsyms_p + offsetof(struct mod_kallsyms, symtab);
	uintptr_t *symtab_p = (uintptr_t *)virt_to_lk_virt_with_align(vaddr, sizeof(uintptr_t), 4);

	if (!symtab_p)
		return NULL;

	/* "uint32_t num_symtab;" defined in "struct mod_kallsyms" */
	vaddr = *kallsyms_p + offsetof(struct mod_kallsyms, num_symtab);
	uint32_t *num_symtab = (uint32_t *)virt_to_lk_virt_with_align(vaddr, sizeof(uint32_t), 4);

	if (!num_symtab)
		return NULL;

	/* "struct struct module_layout core_layout;" defined in "struct module" */
	vaddr = (uintptr_t)((uint8_t *)mod_vma + info->mod_core_layout_offset);
	struct module_layout *core_layout = (struct module_layout *)
				virt_to_lk_virt_with_align(vaddr, sizeof(struct module_layout), 4);

	if (!core_layout)
		return NULL;

	/* "struct struct module_layout init_layout;" defined in "struct module" */
	vaddr = (uintptr_t)((uint8_t *)mod_vma + info->mod_init_layout_offset);
	struct module_layout *init_layout = (struct module_layout *)
				virt_to_lk_virt_with_align(vaddr, sizeof(struct module_layout), 4);

	if (!init_layout)
		return NULL;

	/* At worse, next value is at end of module */
	uint64_t nextval;

	if (within_module_init(addr, mod_vma))
		nextval = (uint64_t)init_layout->base + init_layout->text_size;
	else
		nextval = (uint64_t)core_layout->base + core_layout->text_size;

	uint32_t best = 0;
	uint64_t bestval = kallsyms_symbol_value(*symtab_p + best * sizeof(Elf_Sym));

	lk_time_t to = current_time() + FIND_KALLSYMS_TIMEOUT;

	/* Scan for closest preceding symbol, and next symbol. (ELF starts real symbols at 1). */
	for (uint32_t i = 1; i < *num_symtab; i++) {
		vaddr = *symtab_p + i * sizeof(Elf_Sym);
		uint64_t thisval = kallsyms_symbol_value(vaddr);
		const char *symbol_name;

		if (current_time() >= to)
			return NULL;

		if (kallsyms_symbol_shndx(vaddr) == SHN_UNDEF)
			continue;

		/* We ignore unnamed symbols: they're uninformative and inserted at a whim. */
		symbol_name = kallsyms_symbol_name(*kallsyms_p, vaddr);
		if (!symbol_name || *symbol_name == '\0' || is_arm_mapping_symbol(symbol_name))
			continue;

		if (thisval <= addr && thisval > bestval) {
			best = i;
			bestval = thisval;
		}

		if (thisval > addr && thisval < nextval)
			nextval = thisval;
	}

	if (!best)
		return NULL;

	if (size)
		*size = nextval - bestval;

	if (offset)
		*offset = addr - bestval;

	vaddr = *symtab_p + best * sizeof(Elf_Sym);

	return kallsyms_symbol_name(*kallsyms_p, vaddr);
}

/* For kallsyms to ask for address resolution.  NULL means not found. */
const char *module_address_lookup(uint64_t addr,
				  uint64_t *size,
				  uint64_t *offset,
				  char **modname,
				  char *namebuf)
{
	const char *ret = NULL;

	if (!info)
		info = linux_debug_get_kernel_info();

	/* consider ARM64_PTR_AUTH */
	addr |= 0xFFFFFF8000000000;

	if (info) {
		struct module *mod_vma, *mod = NULL;

		mod_vma = __module_address(addr);
		if (mod_vma) {
			if (modname) {
				mod = (struct module *)
					virt_to_lk_virt_with_align((uintptr_t)mod_vma,
								sizeof(struct module), 4);
				*modname = mod->name;
			}

			ret = find_kallsyms_symbol(mod_vma, addr, size, offset);
		}

		/* Make a copy in here where it's safe */
		if (ret) {
			strncpy(namebuf, ret, KSYM_NAME_LEN - 1);
			ret = namebuf;
		}
	}

	return ret;
}
