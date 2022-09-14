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
#include <platform/mmu/barrier.h>

#define DSS_RESERVE_PATH	"/reserved-memory/debug_snapshot"

extern int load_boot_images(void);

struct reserve_mem {
	unsigned long long paddr;
	unsigned long long size;
};

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
