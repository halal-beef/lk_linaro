/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

#include <reg.h>
#include <stdlib.h>
#include <libfdt.h>
#include <stdio.h>
#include <part.h>
#include <lib/fdtapi.h>
#include <lib/font_display.h>
#include <lib/console.h>
#include <dev/boot.h>
#include <dev/debug/dss.h>
#include <platform/sfr.h>
#include <platform/sizes.h>
#include <platform/dfd.h>
#include <lib/linux_debug.h>
#include <platform/mmu/barrier.h>
#include <target/bootinfo.h>

#define DSS_RESERVE_PATH	"/reserved-memory/debug_snapshot"

extern int load_boot_images(void);

struct reserve_mem {
	unsigned long long paddr;
	unsigned long long size;
};

typedef struct {
	uint64_t r[31]; // X0-X30
	uint64_t sp;
	uint64_t pc;
	uint64_t pstate;
	uint64_t pcsr;
	uint64_t power_state;
} pt_regs_t;

#define UNWIND_DEPTH	6
#define SIZE_2GB	0x80000000ULL

static int dss_display_backtrace(int panic_cpu) {

	for (int cpu = 0; cpu < NR_CPUS; ++cpu) {
		pt_regs_t *regs = (pt_regs_t *)((void *)(CONFIG_RAMDUMP_COREREG) + 0x200 * cpu);

		/* skip restoring callstack of panic cpu */
		if (panic_cpu == cpu)
			continue;

		printf("Backtrace Core %d", cpu);
		linux_debug_unwind_backtrace(regs->r[29], regs->pc, regs->sp, UNWIND_DEPTH);
		printf("\n");
	}

	return 0;
}

static linux_mem_region_t dss_memory_regions[] = {
	{ DRAM_BASE, SIZE_2GB },
	{ DRAM_BASE2, 0 },
};

static linux_mem_map_t dss_linux_mem_map = {
	.regions = (const linux_mem_region_t *)dss_memory_regions,
	.count = ARRAY_SIZE(dss_memory_regions),
};

static void reset_mem_map(u64 dram_size) {
	if (dram_size <= SIZE_2GB) {
		dss_memory_regions[0].size = (size_t)dram_size;
		dss_linux_mem_map.count = 1;
	} else {
		dss_memory_regions[1].size = (size_t)(dram_size - SIZE_2GB);
	}
}


int is_async_rst(unsigned int rst_stat)
{
	return rst_stat & (WARM_RESET | LITTLE_WDT_RESET | APM_WDT_RESET);
}

int is_abnormal_rst(unsigned int rst_stat)
{
	return is_ramdump_mode() || is_async_rst(rst_stat);
}

static int dss_get_panic_cpu(void)
{
	int panic_cpu = -1;
	unsigned int magic = readl((CONFIG_RAMDUMP_BACKTRACE_MAGIC));

	if (magic == 0x0DB90DB9)
		panic_cpu = (int)readl((CONFIG_RAMDUMP_BACKTRACE_CPU));

	return panic_cpu;
}

static void clear_backtrace_magic(void)
{
	writel(0, (CONFIG_RAMDUMP_BACKTRACE_MAGIC));
}

static void dss_print_and_clear_saved_backtrace(void)
{
	unsigned int magic = readl((CONFIG_RAMDUMP_BACKTRACE_MAGIC));

	if (magic == 0x0DB90DB9) {
		char *str;
		u64 size;

		clear_backtrace_magic();

		printf("Kernel panic or exception happened\n");
		str = (char *)(readq((CONFIG_RAMDUMP_BACKTRACE_PADDR)));
		size = (u64)readq((CONFIG_RAMDUMP_BACKTRACE_SIZE));
		str[size - 1] = 0;
		printf("%s", str);
	}
}

void dss_kinfo_init(uint level) {
	unsigned int rst_stat = readl((EXYNOS_POWER_RST_STAT));

	if (check_dram_init_flag() && is_abnormal_rst(rst_stat)) {
		u64 dram_size = *(u64 *)(BL_SYS_INFO_DRAM_SIZE);

		reset_mem_map(dram_size);
		linux_debug_init((CONFIG_RAMDUMP_DEBUG_KINFO_BASE), 0x1000,
				(const linux_mem_map_t *)&dss_linux_mem_map);
	}
}
LK_INIT_HOOK(dss_kinfo_init, dss_kinfo_init, DSS_KINFO_INIT_LEVEL);

#ifndef DSS_SKIP_PRINT_BACKTRACE
void dss_print_backtrace(uint level) {
	unsigned int rst_stat = readl((EXYNOS_POWER_RST_STAT));

	if (check_dram_init_flag() && is_abnormal_rst(rst_stat)) {
		int panic_cpu;

		panic_cpu = dss_get_panic_cpu();
		dss_print_and_clear_saved_backtrace();

		if (linux_debug_is_valid()) {
			printf("aosp version: %s\n", linux_debug_get_version());
			printf("vendor version: %s\n", linux_debug_get_vendor_version());
			printf("build info: %s\n", linux_debug_get_build_info());
			if (is_async_rst(rst_stat))
				dss_display_backtrace(panic_cpu);
		}
	}
}
LK_INIT_HOOK(dss_print_backtrace, dss_print_backtrace, DSS_PRINT_BACKTRACE_INIT_LEVEL);
#endif /* DSS_SKIP_PRINT_BACKTRACE */
struct dss_item {
	char name[16];
	struct reserve_mem rmem;
};

struct dss_bl {
	unsigned int magic;
	unsigned int item_count;
	struct dss_item item[24];
	unsigned int checksum;
};

struct dss_bl static_dss_bl = {
	.item_count = 10,
	.item = {
		{"header",		{0, 0}},
		{"log_kernel",		{0, 0}},
		{"log_platform",	{0, 0}},
		{"log_sfr",		{0, 0}},
		{"log_s2d",		{0, 0}},
		{"log_arrdumpreset",	{0, 0}},
		{"log_arrdumppanic",	{0, 0}},
		{"log_dbgc",		{0, 0}},
		{"log_kevents",		{0, 0}},
		{"log_fatal",		{0, 0}},
	},
};

struct dss_bl *dss_bl_p = &static_dss_bl;

void dss_boot_cnt(void)
{
	unsigned int reg;

	reg = readl(CONFIG_RAMDUMP_BL_BOOT_CNT_MAGIC);
	if (reg == RAMDUMP_BOOT_CNT_MAGIC) {
		reg = readl(CONFIG_RAMDUMP_BL_BOOT_CNT);
		reg += 1;
		writel(reg , CONFIG_RAMDUMP_BL_BOOT_CNT);
	} else {
		reg = 1;
		writel(reg, CONFIG_RAMDUMP_BL_BOOT_CNT);
		writel(RAMDUMP_BOOT_CNT_MAGIC, CONFIG_RAMDUMP_BL_BOOT_CNT_MAGIC);
	}

	printf("Bootloader Booting SEQ #%u\n", reg);
}

static void dss_get_items(void)
{
	u32 rst_stat = readl(EXYNOS_POWER_RST_STAT);
	char path[64];
	u32 ret[8];
	u32 i;

	if (!(rst_stat & PIN_RESET) && (readl(CONFIG_RAMDUMP_DSS_ITEM_INFO) == 0x1234ABCD)) {
		dss_bl_p = (struct dss_bl *)CONFIG_RAMDUMP_DSS_ITEM_INFO;
	} else {
		load_boot_images();
		fdt_dtb = (struct fdt_header *)DT_BASE;

		for (i = 0; i < static_dss_bl.item_count; i++) {
			if (!strlen(static_dss_bl.item[i].name))
				continue;

			memset(path, 0, 64);
			memset(ret, 0, 8 * sizeof(u32));
			strncpy(path, DSS_RESERVE_PATH, sizeof(DSS_RESERVE_PATH));
			strncat(path, "/", 1);
			strncat(path, static_dss_bl.item[i].name,
			        strlen(static_dss_bl.item[i].name));

			if (!get_fdt_val(path, "reg", (char *)ret)) {
				static_dss_bl.item[i].rmem.paddr |= be32_to_cpu(ret[1]);
				static_dss_bl.item[i].rmem.size = be32_to_cpu(ret[2]);
			} else {
				static_dss_bl.item[i].rmem.size = 0;
			}
		}

		dss_bl_p = &static_dss_bl;
	}

	printf("debug-snapshot kernel physical memory layout:(MAGIC:0x%lx)\n",
	       (unsigned long)readl(CONFIG_RAMDUMP_DSS_ITEM_INFO));

	for (i = 0; i < dss_bl_p->item_count; i++) {
		if (dss_bl_p->item[i].rmem.size) {
			printf("%-15s: phys:0x%llx / size:0x%llx\n",
			       dss_bl_p->item[i].name,
			       dss_bl_p->item[i].rmem.paddr,
			       dss_bl_p->item[i].rmem.size);
		}
	}
}

unsigned long dss_get_item_count(void)
{
	return dss_bl_p->item_count;
}

struct dss_item *dss_get_item(const char *name)
{
	unsigned int i;

	for (i = 0; i < dss_bl_p->item_count; i++) {
		if (!strncmp(dss_bl_p->item[i].name, name, strlen(dss_bl_p->item[i].name))
						&& dss_bl_p->item[i].rmem.size)
			return &dss_bl_p->item[i];
	}

	return NULL;
}

unsigned long dss_get_item_paddr(const char *name)
{
	unsigned int i;

	for (i = 0; i < dss_bl_p->item_count; i++) {
		if (!strncmp(dss_bl_p->item[i].name, name, strlen(name))
						&& dss_bl_p->item[i].rmem.size)
			return dss_bl_p->item[i].rmem.paddr;
	}

	return 0;
}

unsigned long dss_get_item_size(const char *name)
{
	unsigned int i;

	for (i = 0; i < dss_bl_p->item_count; i++) {
		if (!strncmp(dss_bl_p->item[i].name, name, strlen(name))
						&& dss_bl_p->item[i].rmem.size)
			return dss_bl_p->item[i].rmem.size;
	}

	return 0;
}

void dss_fdt_init(void)
{
	fdt_dpm = (struct fdt_header *)CONFIG_RAMDUMP_DPM_BASE;
	dss_get_items();
}

int dss_getvar_item(const char *name, char *response)
{
	char log_name[32] = { 0, };
	struct dss_item *item;

	if (!strcmp(name, "dramsize")) {
		u64 dram_size = *(u64 *)BL_SYS_INFO_DRAM_SIZE;

		dram_size /= SZ_1M;
		sprintf(response, "%lluMB", dram_size);
		return 0;
	}

	if (!strcmp(name, "header")) {
		item = dss_get_item("header");
		if (!item)
			return -1;

		sprintf(response, "%llX, %llX, %llX", item->rmem.paddr, item->rmem.size,
		        item->rmem.paddr + item->rmem.size - 1);

		return 0;
	}

	snprintf(log_name, sizeof(log_name) - 1, "log_%s", name);
	item = dss_get_item(log_name);
	if (!item)
		return -1;

	sprintf(response, "%llX, %llX, %llX", item->rmem.paddr, item->rmem.size,
	        item->rmem.paddr + item->rmem.size - 1);
	return 0;
}

int dss_getvar_item_with_index(unsigned int index, char *response)
{
	struct dss_item *item;

	if (index >= dss_bl_p->item_count)
		return -1;

	item = &dss_bl_p->item[index];
	if (item->rmem.size) {
		sprintf(response, "%s: %llX, %llX, %llX", item->name, item->rmem.paddr, item->rmem.size,
			item->rmem.paddr + item->rmem.size - 1);
	}

	return 0;
}
