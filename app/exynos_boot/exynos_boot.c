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
#include <platform/fg_s2mu004.h>
#include <platform/tmu.h>
#include <platform/xct.h>
#include <dev/boot.h>
#include <platform/gpio.h>
#include <platform/pmic_s2mpu09.h>
#include <platform/gpio.h>
#include <platform/debug-store-ramdump.h>
#include <lib/font_display.h>
#include <lib/logo_display.h>
#include "almighty.h"
#include "drex_v3_3.h"
#include "mct.h"

int cmd_boot(int argc, const cmd_args *argv);
int ab_update_slot_info_bootloader(void);
static void do_memtester(unsigned int loop);
extern unsigned int uart_log_mode;

static void exynos_boot_task(const struct app_descriptor *app, void *args)
{
	unsigned int rst_stat = readl(EXYNOS9610_POWER_RST_STAT);
	int cpu, chk_smpl;
	struct exynos_gpio_bank *bank = (struct exynos_gpio_bank *)EXYNOS9610_GPA1CON;
	int fb_mode_failed = 0;
	unsigned int soc, mif, apm;
	int vol_up_val;
	int vol_down_val;
	struct pit_entry *ptn;

	printf("RST_STAT: 0x%x\n", rst_stat);

	soc = readl(EXYNOS9610_POWER_BASE + 0x0f00);
	mif = readl(EXYNOS9610_POWER_BASE + 0x0f04);
	apm = readl(EXYNOS9610_POWER_BASE + 0x0f40);

	printf("PMUDBG_CENTRAL_SEQ_*: apsoc 0x%x mif 0x%x apm 0x%x\n", soc, mif, apm);
	printf("PMUDBG_CL0_*: ");
	for (cpu = LITTLE_CORE_START; cpu <= LITTLE_CORE_LAST; cpu++)
		printf("cpu%d: 0x%x ", cpu, dfd_get_pmudbg_stat(cpu));
	printf("\n");
	printf("PMUDBG_CL1_*: ");
	for (cpu = BIG_CORE_START; cpu <= BIG_CORE_LAST; cpu++)
		printf("cpu%d: 0x%x ", cpu-BIG_CORE_START, dfd_get_pmudbg_stat(cpu));
	printf("\n");

	if (*(unsigned int *)DRAM_BASE != 0xabcdef) {
		printf("Running on DRAM by TRACE32: skip auto booting\n");
		do_fastboot(0, 0);
		return;
	}

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

	dfd_display_reboot_reason();
	dfd_display_core_stat();
	/* Volume up set Input & Pull up */
	exynos_gpio_set_pull(bank, 5, GPIO_PULL_UP);
	exynos_gpio_cfg_pin(bank, 5, GPIO_INPUT);
	/* Volume down set Input & Pull up */
	exynos_gpio_set_pull(bank, 6, GPIO_PULL_UP);
	exynos_gpio_cfg_pin(bank, 6, GPIO_INPUT);

	clear_wdt_recovery_settings();
	if (is_first_boot())
		ab_update_slot_info_bootloader();

	/* check SMPL & WTSR with S2MPU09 */
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

	if (!is_first_boot()) {
		printf("Entering fastboot: not first_boot\n");
		goto fastboot;
	} else if (rst_stat & (WARM_RESET | LITTLE_WDT_RESET | BIG_WDT_RESET)) {
		printf("Entering fastboot: Abnormal RST_STAT: 0x%x\n", rst_stat);
		dfd_run_dump_gpr();
		goto fastboot;
	} else if ((readl(CONFIG_RAMDUMP_SCRATCH) == CONFIG_RAMDUMP_MODE) && get_charger_mode() == 0) {
		printf("Entering fastboot: Ramdump_Scratch & Charger\n");
		goto fastboot;
	} else if (fb_mode_failed == 1) {
		printf("Entering fastboot: fastboot_reg | fb_mode\n");
		goto fastboot;
	} else
		goto reboot;

reboot:
	if (is_xct_boot()) {
		cpu = cmd_xct(0, 0);
		printf("Entering fastboot: xct boot fail code - %d\n", cpu);
		do_fastboot(0, 0);
		return;
	}
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

#ifdef RAMDUMP_MODE_OFF
	dfd_set_dump_gpr(0);
	set_debug_level("low");
#else
	dfd_set_dump_gpr(CACHE_RESET_EN | DUMPGPR_EN);
	set_debug_level("mid");
#endif
	set_debug_level_by_env();

	cmd_boot(0, 0);
	return;

fastboot:
	uart_log_mode = 1;
#ifdef RAMDUMP_MODE_OFF
	cmd_boot(0, 0);
#else
	debug_store_ramdump();
	do_fastboot(0, 0);
#endif
	return;
}

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
	int iter = 0, dram_freq;

	dram_freq = almighty_get_dram_freq();
	print_lcd_update(FONT_WHITE, FONT_BLACK, "DRAM Test will start at %dMHz\n", dram_freq);
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

APP_START(exynos_boot)
	.entry = exynos_boot_task,
	.flags = 0,
APP_END
