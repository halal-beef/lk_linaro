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
#include <platform/environment.h>
#include <platform/wdt_recovery.h>
#include <platform/sfr.h>
#include <platform/charger.h>
#include <platform/fastboot.h>
#include <platform/dfd.h>
#include <dev/boot.h>
#include <platform/gpio.h>
#include <platform/pmic_s2mpu09.h>

int cmd_boot(int argc, const cmd_args *argv);
int ab_update_slot_info_bootloader(void);
extern unsigned int uart_log_mode;

static void exynos_boot_task(const struct app_descriptor *app, void *args)
{
	unsigned int rst_stat = readl(EXYNOS9610_POWER_RST_STAT);
	int cpu, chk_smpl;
	struct exynos_gpio_bank *bank = (struct exynos_gpio_bank *)EXYNOS9610_GPA1CON;
	int fb_mode_failed = 0;
	unsigned int *env_val;
	struct pit_entry *ptn_env;
	unsigned int soc, mif, apm;

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
		ptn_env = pit_get_part_info("env");
		env_val = memalign(0x1000, pit_get_length(ptn_env));
		pit_access(ptn_env, PIT_OP_LOAD, (u64)env_val, 0);
		if(env_val[ENV_ID_FB_MODE] == FB_MODE_FLAG) {
			printf("Fastboot is not completed on a prior booting.\n");
			printf("Entering fastboot.\n");
			print_lcd_update(FONT_RED, FONT_BLACK, "Fastboot is not completed on a prior booting.");
			print_lcd_update(FONT_RED, FONT_BLACK, "Entering fastboot.");
			fb_mode_failed = 1;
		}
		free(env_val);
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

	if (!is_first_boot()) {
		printf("Entering fastboot: not first_boot\n");
		goto fastboot;
	} else if (rst_stat & (WARM_RESET | LITTLE_WDT_RESET | BIG_WDT_RESET)) {
		printf("Entering fastboot: Abnormal RST_STAT: 0x%x\n", rst_stat);
		goto fastboot;
	} else if ((readl(CONFIG_RAMDUMP_SCRATCH) == CONFIG_RAMDUMP_MODE) && get_charger_mode() == 0) {
		printf("Entering fastboot: Ramdump_Scratch & Charger\n");
		goto fastboot;
	} else if (fb_mode_failed == 1) {
		printf("Entering fastboot: fastboot_reg | fb_mode\n");
		goto fastboot;
	} else
		goto reboot;

fastboot:
	uart_log_mode = 1;
	dfd_run_dump_gpr();
	do_fastboot(0, 0);
	return;

reboot:
	/* Turn on dumpEN for DumpGPR */
	dfd_set_dump_gpr(CACHE_RESET_EN | DUMPGPR_EN);
	cmd_boot(0, 0);
	return;
}

APP_START(exynos_boot)
	.entry = exynos_boot_task,
	.flags = 0,
APP_END
