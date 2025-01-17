/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

#include <debug.h>
#include <reg.h>
#include <app.h>
#include <part.h>
#include <stdlib.h>
#include <lib/console.h>
#include <lib/font_display.h>
#include <platform/mmu/mmu_func.h>
#include <lib/sysparam.h>
#include <platform/wdt_recovery.h>
#include <platform/pmic_s2mpu12.h>
#include <platform/sfr.h>
#include <platform/charger.h>
#include <platform/fastboot.h>
#include <platform/dfd.h>
#include <platform/xct.h>
#include <dev/boot.h>
#include <dev/usb/gadget.h>
#include <platform/gpio.h>
#include <platform/gpio.h>
#include <platform/dss_store_ramdump.h>
#include <platform/smc.h>
#include <lib/font_display.h>
#include <lib/logo_display.h>
#include "almighty.h"
#include "drex_v3_3.h"
#include "mct.h"
#include "recovery.h"

#define CONFIG_PIT_IMAMGE_BASE 0x80100000
#define CONFIG_FWBL1_IMAMGE_BASE 0x80200000
#define CONFIG_EPBL_IMAMGE_BASE 0x80300000
#define CONFIG_BL2_IMAMGE_BASE 0x80400000
#define CONFIG_LK_IMAMGE_BASE 0x80500000
#define CONFIG_EL3_MON_IMAMGE_BASE 0x80700000
#define CONFIG_BOOT_IMAMGE_BASE 0x80800000
#define CONFIG_DTBO_IMAMGE_BASE 0x83000000
#define CONFIG_RAMDISK_IMAMGE_BASE 0x83100000

int cmd_boot(int argc, const cmd_args *argv);
int ab_update_slot_info_bootloader(void);
static void do_memtester(unsigned int loop);
extern unsigned int uart_log_mode;
extern unsigned int board_rev;

static void exynos_boot_task(const struct app_descriptor *app, void *args)
{
	unsigned int rst_stat = readl(EXYNOS3830_POWER_RST_STAT);
	/* struct pit_entry *ptn; */
	int cpu;
	struct exynos_gpio_bank *bank = (struct exynos_gpio_bank *)EXYNOS3830_GPA0CON;
	int vol_up_val;
	int chk_wtsr_smpl;
	int i;
	int err;
	//void *part;

	print_lcd_update(FONT_WHITE, FONT_BLACK, "Board revision : 0x%X", board_rev);

	if (*(unsigned int *)BL2_TAG_ADDR != BL2_TAG) {
		printf("Running on DRAM by TRACE32: skip auto booting\n");

#if 0
		ptn = pit_get_part_info("pit");
		if (ptn == 0)
			printf("Partition 'pit' does not exist.\n");
		pit_update((void *)CONFIG_PIT_IMAMGE_BASE, 0);

		ptn = pit_get_part_info("fwbl1");
		if (ptn == 0)
			printf("Partition 'fwbl1' does not exist.\n");
		pit_access(ptn, PIT_OP_FLASH, (u64)CONFIG_FWBL1_IMAMGE_BASE, 0);

		ptn = pit_get_part_info("epbl");
		if (ptn == 0)
			printf("Partition 'epbl' does not exist.\n");
		pit_access(ptn, PIT_OP_FLASH, (u64)CONFIG_EPBL_IMAMGE_BASE, 0);

		ptn = pit_get_part_info("bl2");
		if (ptn == 0)
			printf("Partition 'bl2' does not exist.\n");
		pit_access(ptn, PIT_OP_FLASH, (u64)CONFIG_BL2_IMAMGE_BASE, 0);

		ptn = pit_get_part_info("bootloader");
		if (ptn == 0)
			printf("Partition 'bootloader' does not exist.\n");
		pit_access(ptn, PIT_OP_FLASH, (u64)CONFIG_LK_IMAMGE_BASE, 0);

		ptn = pit_get_part_info("el3_mon");
		if (ptn == 0)
			printf("Partition 'el3_mon' does not exist.\n");
		pit_access(ptn, PIT_OP_FLASH, (u64)CONFIG_EL3_MON_IMAMGE_BASE, 0);

		ptn = pit_get_part_info("boot");
		if (ptn == 0)
			printf("Partition 'boot' does not exist.\n");
		pit_access(ptn, PIT_OP_FLASH, (u64)CONFIG_BOOT_IMAMGE_BASE, 0);

		ptn = pit_get_part_info("dtbo");
		if (ptn == 0)
			printf("Partition 'dtbo' does not exist.\n");
		pit_access(ptn, PIT_OP_FLASH, (u64)CONFIG_DTBO_IMAMGE_BASE, 0);

		ptn = pit_get_part_info("ramdisk");
		if (ptn == 0)
			printf("Partition 'ramdisk' does not exist.\n");
		pit_access(ptn, PIT_OP_FLASH, (u64)CONFIG_RAMDISK_IMAMGE_BASE, 0);
#endif

		start_usb_gadget();
		return;
	}

	/* Volume up set Input & Pull up */
	exynos_gpio_set_pull(bank, 7, GPIO_PULL_NONE);
	exynos_gpio_cfg_pin(bank, 7, GPIO_INPUT);
	for (i = 0; i < 10; i++) {
		vol_up_val = exynos_gpio_get_value(bank, 7);
		printf("Volume up key: %d\n", vol_up_val);
	}
	if (vol_up_val == 0) {
		printf("Volume up key is pressed. Entering fastboot mode!\n");
		start_usb_gadget();
		return;
	}

/*

	if (is_first_boot()) {
		unsigned int env_val = 0;

		if (sysparam_read("fb_mode_set", &env_val, sizeof(env_val)) > 0) {
			if (env_val == FB_MODE_FLAG) {
				printf("Fastboot is not completed on a prior booting.\n");
				printf("Entering fastboot.\n");
				print_lcd_update(FONT_RED, FONT_BLACK,
						"Fastboot is not completed on a prior booting.");
				print_lcd_update(FONT_RED, FONT_BLACK,
						"Entering fastboot.");
				fb_mode_failed = 1;
			}
		}
	}
*/
#ifdef CONFIG_WDT_RECOVERY_USB_BOOT
	clear_wdt_recovery_settings();
#endif
/*
	if (is_first_boot())
		ab_update_slot_info_bootloader();
*/
	/* check SMPL & WTSR with S2MPU10 */
	chk_wtsr_smpl = chk_smpl_wtsr_s2mpu12();
	if (chk_wtsr_smpl == PMIC_DETECT_WTSR) {
		print_lcd_update(FONT_RED, FONT_BLACK, "WTSR DETECTED");
		printf("WTSR detected\n");
#ifdef S2MPU10_PM_IGNORE_WTSR_DETECT
		print_lcd_update(FONT_RED, FONT_BLACK, ",But Ignore WTSR DETECTION");
		printf(", but ignored by build config\n");
		writel(0, CONFIG_RAMDUMP_SCRATCH);
#endif
	} else if (chk_wtsr_smpl == PMIC_DETECT_SMPL) {
		print_lcd_update(FONT_RED, FONT_BLACK, "SMPL DETECTED");
		printf("SMPL detected\n");
#ifdef S2MPU10_PM_IGNORE_SMPL_DETECT
		print_lcd_update(FONT_RED, FONT_BLACK, ",But Ignore SMPL DETECTION");
		printf(", but ignored by build config\n");
		writel(0, CONFIG_RAMDUMP_SCRATCH);
#endif
	}

	/* Fastboot upload or download check */
	if (!is_first_boot()) {
		pmic_enable_manual_reset(PMIC_MRDT_3);
		printf("Entering fastboot: not first_boot\n");
		goto download;
	} else if (rst_stat & (WARM_RESET | LITTLE_WDT_RESET | BIG_WDT_RESET)) {
		printf("Entering fastboot: Abnormal RST_STAT: 0x%x\n", rst_stat);
		sdm_encrypt_secdram();
		dfd_set_dump_en_for_cacheop(0);
		goto fastboot;
	} else if (readl(EXYNOS3830_POWER_SYSIP_DAT0) == REBOOT_MODE_FASTBOOT) {
		printf("Entering fastboot: reboot bootloader command\n");
		writel(0, CONFIG_RAMDUMP_SCRATCH);
		writel(0, EXYNOS3830_POWER_SYSIP_DAT0);
		goto download;
	} else if ((readl(CONFIG_RAMDUMP_SCRATCH) == CONFIG_RAMDUMP_MODE) && get_charger_mode() == 0) {
		printf("Entering fastboot: Ramdump_Scratch & Charger\n");
		sdm_encrypt_secdram();
		goto fastboot;
/*
	} else if (fb_mode_failed == 1) {
		printf("Entering fastboot: fastboot_reg | fb_mode\n");
		goto fastboot;
*/
	} else if (is_xct_boot()) {
		cpu = cmd_xct(0, 0);
		printf("Entering fastboot: xct boot fail code - %d\n", cpu);
		goto download;
	} else {
		goto reboot;
	}

download:
	uart_log_mode = 1;
	start_usb_gadget();
	return;

reboot:
/*
	vol_up_val = exynos_gpio_get_value(bank, 5);
	vol_down_val = exynos_gpio_get_value(bank, 6);

	if ((vol_up_val == 0) && (vol_down_val == 0)) {
		do_memtester(0);

		part = part_get("logbuf");
		if (!part) {
			printf("Partition 'logbuf' does not exist.\n");
			print_lcd_update(FONT_RED, FONT_BLACK, "Partition 'logbuf' does not exist.");
		} else {
			printf("Saving memory test logs to 'logbuf' partition.\n");
			print_lcd_update(FONT_GREEN, FONT_BLACK, "Saving memory test logs to 'logbuf' partition.");
			part_write(part, (void *)CONFIG_RAMDUMP_LOGBUF);
		}
	}
*/
	/* Turn on dumpEN for DumpGPR */
#ifdef RAMDUMP_MODE_OFF
	dfd_set_dump_en_for_cacheop(0);
	set_debug_level("low");
#else
	dfd_set_dump_en_for_cacheop(1);
	set_debug_level("mid");
#endif
	set_debug_level_by_env();
	recovery_init();
	err = cmd_boot(0, 0);
	if (err) {
		start_usb_gadget();
		return;
	}

	return;

fastboot:
	uart_log_mode = 1;
#ifndef RAMDUMP_MODE_OFF
	debug_store_ramdump();
	start_usb_gadget();
	return;
#endif
}

#if 0
static void print_status(int iter)
{
	int vbat;
	unsigned int cpu_temp;
	int i, j;
	/* Get status */
	vbat = s2mu004_get_avgvbat();
	read_temperature(TZ_LIT, &cpu_temp, NO_PRINT);

	/* print cpu temp and vbat gauge */
	printf("[%d] CPU : %d, BATT : %d DRAM: ", 0, cpu_temp, vbat);
	print_lcd_update(FONT_WHITE, FONT_BLACK, "[%d] CPU: %d, BATT: %d \n", iter, cpu_temp, vbat);
	clean_invalidate_dcache_all();
	for (i = 0; i < MC_CH_ALL; i++) {
		for (j = 1; j < MC_RANK_ALL; j++) {
			printf("[CH%d.CS%d].MR4 = %d\n", i, j - 1, mc_driver.command.mode_read(i, j, 4));
		}
	}
	printf("\n");

}

static void do_memtester(unsigned int loop)
{
	int iter = 0;
#ifdef CONFIG_DISPLAY_DRAWFONT
	int dram_freq;

	dram_freq = almighty_get_dram_freq();
	print_lcd_update(FONT_WHITE, FONT_BLACK, "DRAM Test will start at %dMHz\n", dram_freq);
#endif
	cpu_common_init();
	print_lcd_update(FONT_WHITE, FONT_BLACK, "Cache is enabled.\n");
	clean_invalidate_dcache_all();

	do {
		mct.init();

		if (almighty_pattern_test(1)) {	/* '-1' is all pattern(8ea), '0 ~ 7' is fixed pattern */
			printf("test fail\n");
			print_lcd_update(FONT_RED, FONT_BLACK, "DRAM test failed\n");
			break;
		}

		print_status(++iter);
		mct.deinit();
	} while (loop-- > 0);

	print_lcd_update(FONT_BLUE, FONT_BLACK, "DRAM test passed.\n");

	/* After the test */
	clean_invalidate_dcache_all();
	disable_mmu_dcache();
}
#endif

APP_START(exynos_boot)
	.entry = exynos_boot_task,
	.flags = 0,
APP_END
