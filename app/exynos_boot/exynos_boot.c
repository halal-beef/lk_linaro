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
#include <pit.h>
#include <stdlib.h>
#include <lib/console.h>
#include <lib/font_display.h>
#include <platform/mmu/mmu_func.h>
#include <lib/sysparam.h>
#include <platform/wdt_recovery.h>
#include <platform/sfr.h>
#include <platform/charger.h>
#include <platform/fastboot.h>
#include <platform/dfd.h>
#include <platform/xct.h>
#include <dev/boot.h>
#include <dev/usb/gadget.h>
#include <platform/gpio.h>
#include <platform/gpio.h>
#include <platform/debug-store-ramdump.h>
#include <lib/font_display.h>
#include <lib/logo_display.h>
#include "almighty.h"
#include "drex_v3_3.h"
#include "mct.h"

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

static void exynos_boot_task(const struct app_descriptor *app, void *args)
{
	unsigned int rst_stat = readl(EXYNOS9630_POWER_RST_STAT);
	/* struct pit_entry *ptn; */
	int cpu;
	struct exynos_gpio_bank *bank = (struct exynos_gpio_bank *)EXYNOS9630_GPA1CON;
	int vol_up_val;
	int i;

	printf("RST_STAT: 0x%x\n", rst_stat);

	if (*(unsigned int *)DRAM_BASE != 0xabcdef) {
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

// have to modify
//		do_fastboot(0, 0);
		start_usb_gadget();
		return;
	}

	/* Volume up set Input & Pull up */
	exynos_gpio_set_pull(bank, 0, GPIO_PULL_NONE);
	exynos_gpio_cfg_pin(bank, 0, GPIO_INPUT);
	for (i = 0; i < 10; i++) {
		vol_up_val = exynos_gpio_get_value(bank, 0);
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

		printf("Fastboot is not completed on a prior booting.\n");
		printf("Entering fastboot.\n");
		if (sysparam_read("fb_mode_set", &env_val, sizeof(env_val)) > 0) {
			if (env_val == FB_MODE_FLAG) {
				print_lcd_update(FONT_RED, FONT_BLACK,
						"Fastboot is not completed on a prior booting.");
				print_lcd_update(FONT_RED, FONT_BLACK,
						"Entering fastboot.");
				fb_mode_failed = 1;
			}
		}
	}
*/

/*
	dfd_display_reboot_reason();
	dfd_display_core_stat();
*/

/*
	clear_wdt_recovery_settings();
	if (is_first_boot())
		ab_update_slot_info_bootloader();
*/
	/* check SMPL & WTSR with S2MPU09 */
/*
	chk_smpl = chk_smpl_wtsr_s2mpu09();
	if (chk_smpl)
		print_lcd_update(FONT_RED, FONT_BLACK, "WTSR or SMPL DETECTED");

#ifdef S2MPU09_PM_IGNORE_SMPL_DETECT
	if (chk_smpl == PMIC_DETECT_SMPL_IGNORE) {
		print_lcd_update(FONT_RED, FONT_BLACK, ",But Ignore SMPL DETECTION");
		writel(0, CONFIG_RAMDUMP_SCRATCH);
	}
#endif
#ifdef S2MPU09_PM_IGNORE_WTSR_DETECT
	if (chk_smpl == PMIC_DETECT_WTSR_IGNORE) {
		print_lcd_update(FONT_RED, FONT_BLACK, ",But Ignore WTSR DETECTION");
		writel(0, CONFIG_RAMDUMP_SCRATCH);
	}
#endif
*/

	if (!is_first_boot()) {
		printf("Entering fastboot: not first_boot\n");
		goto fastboot;
	} else if (rst_stat & (WARM_RESET | LITTLE_WDT_RESET | BIG_WDT_RESET)) {
		printf("Entering fastboot: Abnormal RST_STAT: 0x%x\n", rst_stat);
		dfd_run_post_processing();
		goto fastboot;
	} else if ((readl(CONFIG_RAMDUMP_SCRATCH) == CONFIG_RAMDUMP_MODE) && get_charger_mode() == 0) {
		printf("Entering fastboot: Ramdump_Scratch & Charger\n");
		goto fastboot;
/*
	} else if (fb_mode_failed == 1) {
		printf("Entering fastboot: fastboot_reg | fb_mode\n");
		goto fastboot;
*/
	} else
		goto reboot;

fastboot:
	uart_log_mode = 1;
	debug_store_ramdump();
	//do_fastboot(0, 0);
	start_usb_gadget();
	return;

fastboot_dump_gpr:
	uart_log_mode = 1;
	debug_store_ramdump();
	//do_fastboot(0, 0);
	start_usb_gadget();
	return;

reboot:
	if (is_xct_boot()) {
		cpu = cmd_xct(0, 0);
		printf("Entering fastboot: xct boot fail code - %d\n", cpu);
		start_usb_gadget();
		return;
	}

/*
	vol_up_val = exynos_gpio_get_value(bank, 5);
	vol_down_val = exynos_gpio_get_value(bank, 6);

	if ((vol_up_val == 0) && (vol_down_val == 0)) {
		do_memtester(0);

		ptn = pit_get_part_info("logbuf");
		if (ptn == 0) {
			printf("Partition 'logbuf' does not exist.\n");
			print_lcd_update(FONT_RED, FONT_BLACK, "Partition 'logbuf' does not exist.");
		} else {
			printf("Saving memory test logs to 'logbuf' partition.\n");
			print_lcd_update(FONT_GREEN, FONT_BLACK, "Saving memory test logs to 'logbuf' partition.");
			pit_access(ptn, PIT_OP_FLASH, (u64)CONFIG_RAMDUMP_LOGBUF, 0);
		}
	}
*/
	/* Turn on dumpEN for DumpGPR */
#ifdef CONFIG_RAMDUMP_GPR
	dfd_set_dump_gpr(CACHE_RESET_EN | DUMPGPR_EN);
#endif
	cmd_boot(0, 0);
	return;
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
