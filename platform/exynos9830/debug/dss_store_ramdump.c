/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

#include <string.h>
#include <stdlib.h>
#include <pit.h>
#include <lib/console.h>
#include <lib/font_display.h>
#include <lib/fastboot.h>
#include <platform/sizes.h>
#include <platform/dfd.h>
#include <platform/delay.h>
#include <platform/dss_store_ramdump.h>
#include <platform/gpio.h>
#include <platform/pmic_s2mps_19_22.h>
#include <platform/mmu/mmu_func.h>

#ifdef CONFIG_OFFLINE_RAMDUMP
static int g_is_enabled = 1;
#else
static int g_is_enabled = 0;
#endif

union store_ramdump_metadata metadata;

static void wfi(void)
{
	asm volatile("wfi");
}

static int debug_store_is_skip(void)
{
	struct exynos_gpio_bank *bank = (struct exynos_gpio_bank *)EXYNOS9830_GPA0CON;
	int gpio = 4;	/* Volume Down */
	int ret = 0;
	int cnt = 100;

	/* Volume down set Input & Pull up */
	exynos_gpio_set_pull(bank, gpio, GPIO_PULL_UP);
	exynos_gpio_cfg_pin(bank, gpio, GPIO_INPUT);

	print_lcd_update(FONT_GREEN, FONT_BLACK, "After 10 seconds, store ramdump");
	print_lcd_update(FONT_GREEN, FONT_BLACK, "press Vol-Down to skip store ramdump");

	do {
		u_delay(100000);
		if (!exynos_gpio_get_value(bank, gpio)) {
			ret = -1;
			break;
		}
	} while(--cnt);

	return ret;
}

int debug_store_ramdump(void)
{
	struct pit_entry *ptn;
	u64 dram_size;
	u64 dram_write_size;
	u64 dram_ptr;
	u32 reboot_reason;
	int ret = 0;

	if (!g_is_enabled) {
		printf("%s: Store Ramdump is off\n", __func__);
		goto store_out;
	}

	reboot_reason = readl(CONFIG_RAMDUMP_REASON);
	if (reboot_reason == RAMDUMP_SIGN_BL_REBOOT) {
		printf("%s: Bootloader Reboot, Skip saving remdump\n", __func__);

		/* reset reboot reason */
		writel(0, CONFIG_RAMDUMP_REASON);
		goto store_out;
	}

	if (debug_store_is_skip()) {
		print_lcd_update(FONT_GREEN, FONT_BLACK, "Skip Saving Ramdump");
		printf("%s: Volume down key is pressed!\n", __func__);
		goto store_out;
	}

	ptn = pit_get_part_info("ramdump");
	if (!ptn) {
		printf("%s: no ramdump partition\n", __func__);
		ret = -1;
		goto store_out;
	}

	dram_size = *(u64 *)BL_SYS_INFO_DRAM_SIZE;
	printf("%s: dram size is [0x%llx]\n", __func__, dram_size);

	ret = pit_entry_read(ptn, &metadata, METADATA_OFFSET, METADATA_SIZE);
	if (ret) {
		printf("%s: pit read fail(line:%u)\n", __func__, __LINE__);
		goto store_out;
	}

	if (metadata.data.magic == RAMDUMP_STORE_MAGIC)
		printf("%s: ramdump data will be overwritten[%x]\n", __func__, metadata.data.magic);

	print_lcd_update(FONT_GREEN, FONT_BLACK, "WAIT for storing ramdump...");

	if (dram_size > DRAM_WRITE_SIZE_DEFAULT) {
		dram_write_size = DRAM_WRITE_SIZE_DEFAULT;
		dram_ptr = DRAM_BASE;
		ret = pit_entry_write(ptn, (void *)dram_ptr, RAMDUMP_OFFSET, dram_write_size);
		if (ret) {
			printf("%s: pit write fail(line:%u)\n", __func__, __LINE__);
			goto store_out;
		}
#ifdef DRAM_BASE2
		dram_write_size = dram_size - DRAM_WRITE_SIZE_DEFAULT;
		dram_ptr = DRAM_BASE2;
		ret = pit_entry_write(ptn, (void *)dram_ptr,
				RAMDUMP_OFFSET + DRAM_WRITE_SIZE_DEFAULT, dram_write_size);
		if (ret) {
			printf("%s: pit write fail(line:%u)\n", __func__, __LINE__);
			goto store_out;
		}
#endif
	} else {
		dram_write_size = dram_size;
		dram_ptr = DRAM_BASE;
		ret = pit_entry_write(ptn, (void *)dram_ptr, RAMDUMP_OFFSET, dram_write_size);
		if (ret) {
			printf("%s: pit write fail(line:%u)\n", __func__, __LINE__);
			goto store_out;
		}
	}

	metadata.data.magic = RAMDUMP_STORE_MAGIC;
	metadata.data.flag_data_to_storage = 0xffffffff;
	metadata.data.dram_size = dram_size;
	metadata.data.dram_start_addr = DRAM_BASE;
	get_pmic_rtc_time(metadata.data.file_name);
	printf("%s: get_pmic_rtc_time = [%s]\n", __func__,
					metadata.data.file_name);

	ret = pit_entry_write(ptn, &metadata, METADATA_OFFSET, METADATA_SIZE);
	if (ret) {
		printf("%s: pit write fail(line:%u)\n", __func__, __LINE__);
		goto store_out;
	}

	print_lcd_update(FONT_GREEN, FONT_BLACK, "Finish storing ramdump!");
	printf("%s: Finish storing ramdump!", __func__);
	printf("%s: Wait for 1 second for storing ramdump data\n", __func__);
	u_delay(1000000);

#ifndef CONFIG_OFFLINE_RAMDUMP
	goto store_out;
#endif

	/* Reset device for normal booting */
	writel(0, CONFIG_RAMDUMP_SCRATCH);
	clean_invalidate_dcache_range(CONFIG_RAMDUMP_SCRATCH, CONFIG_RAMDUMP_SCRATCH + 64);

	writel(0x2, EXYNOS9830_POWER_SYSTEM_CONFIGURATION);

	/* Do not run this code */
	do {
		wfi();
	} while(1);

store_out:
	return ret;
}

int debug_store_ramdump_redirection(void *ptr)
{
	struct pit_entry *ptn;
	struct fastboot_ramdump_hdr *hdr = ptr;
	u64 storage_base;
	u64 dram_size;
	u64 possible_size = 0;
	u64 redirection_base = 0;
	int ret = 0;

	if (!g_is_enabled)
		goto redirection_out;

	ptn = pit_get_part_info("ramdump");
	if (!ptn) {
		printf("%s: no ramdump partition\n", __func__);
		printf("%s: current ram data will be extracted\n", __func__);
		goto redirection_out;
	}

	/* Set possible redirection base and dump size */
	dram_size = *(u64 *)BL_SYS_INFO_DRAM_SIZE;
	if (dram_size > DRAM_WRITE_SIZE_DEFAULT) {
#ifdef DRAM_BASE2
		redirection_base = DRAM_BASE2;
		possible_size = dram_size - DRAM_WRITE_SIZE_DEFAULT;
#endif
	} else {
		possible_size = dram_size / 2;
		redirection_base = DRAM_BASE;
		if ((DRAM_BASE + possible_size) >= CONFIG_LK_TEXT_BASE)
			possible_size = CONFIG_LK_TEXT_BASE - DRAM_BASE;
	}

	/* Check ramdump size */
	if (possible_size < (hdr->size + 1)) {
		print_lcd_update(FONT_RED, FONT_BLACK, "Invalid ramdump size!");
		printf("%s: Invalid dump size[0x%lx]\n", __func__, hdr->size);
		ret = -1;
		goto redirection_out;
	}

	/* Check valid data in storage */
	ret = pit_entry_read(ptn, &metadata, METADATA_OFFSET, METADATA_SIZE);
	if (ret) {
		printf("%s: pit read fail(line:%u)\n", __func__, __LINE__);
		ret = -1;
		goto redirection_out;
	}
	if (metadata.data.magic != RAMDUMP_STORE_MAGIC) {
		printf("%s: ramdump data doesn't exist[%x]\n", __func__, metadata.data.magic);
		printf("%s: current ram data will be extracted\n", __func__);
		goto redirection_out;
	}

	/* Set storage data base */
	if (hdr->base > 0xFFFFFFFFUL)
		storage_base = RAMDUMP_OFFSET + hdr->base - 0x800000000UL;
	else
		storage_base = RAMDUMP_OFFSET + hdr->base - 0x80000000UL;

	hdr->base = redirection_base;
	ret = pit_entry_read(ptn, (void *)hdr->base, storage_base, hdr->size + 1);
	if (ret)
		 printf("%s: pit read fail(line:%u)\n", __func__, __LINE__);

redirection_out:
	return ret;
}

int debug_store_ramdump_oem(const char *cmd)
{
	int ret = 0;

	if (!strcmp(cmd, "set")) {
		g_is_enabled = 1;
	} else if (!strcmp(cmd, "reset")) {
		g_is_enabled = 0;
	} else if (!strcmp(cmd, "clear")) {
		struct pit_entry *ptn;

		ptn = pit_get_part_info("ramdump");
		if (!ptn) {
			printf("%s: no ramdump partition\n", __func__);
			ret = -1;
			goto oem_out;
		}

		ret = pit_entry_read(ptn, &metadata, METADATA_OFFSET, METADATA_SIZE);
		if (ret) {
			printf("%s: read metadata block fail\n", __func__);
			goto oem_out;
		}

		if (metadata.data.magic == RAMDUMP_STORE_MAGIC) {
			printf("%s: Data will be erased!\n", __func__);
			metadata.data.magic = 0;
			ret = pit_entry_write(ptn, &metadata,
						METADATA_OFFSET, METADATA_SIZE);
			if (ret)
				printf("%s: write metadata block fail\n", __func__);
		} else {
			printf("%s: Data does not exist!\n", __func__);
		}
	} else if (!strcmp(cmd, "save")) {
		ret = debug_store_ramdump();
	} else {
		ret = -1;
	}

oem_out:
	return ret;
}

void debug_store_ramdump_getvar(const char *cmd, char *response)
{
	struct pit_entry *ptn;
	int ret;

	if (!g_is_enabled) {
		sprintf(response, "STORE RAMDUMP IS DISABLED");
		goto getvar_out;
	}

	ptn = pit_get_part_info("ramdump");
	if (!ptn) {
		printf("%s: no ramdump partition\n", __func__);
		sprintf(response, "PIT DOES NOT EXIST");
		goto getvar_out;
	}

	ret = pit_entry_read(ptn, &metadata, METADATA_OFFSET, METADATA_SIZE);
	if (ret) {
		sprintf(response, "PIT READ FAIL");
		goto getvar_out;
	}

	if (metadata.data.magic != RAMDUMP_STORE_MAGIC) {
		sprintf(response, "%s\n%s", "RAMDUMP STORE IS ENABLED",
						"DATA DOES NOT EXIST");
	} else {
		sprintf(response, "%s\n%s", "RAMDUMP STORE IS ENABLED",
							"DATA EXIST");
	}

getvar_out:
	return;
}
