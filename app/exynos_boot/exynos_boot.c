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

int cmd_boot(int argc, const cmd_args *argv);
int ab_update_slot_info_bootloader(void);

static void exynos_boot_task(const struct app_descriptor *app, void *args)
{
	unsigned int rst_stat = readl(EXYNOS9610_POWER_RST_STAT);
	int cpu;
	struct exynos_gpio_bank *bank = (struct exynos_gpio_bank *)EXYNOS9610_GPA1CON;
	int fb_mode_failed = 0;
	unsigned int *env_val;
	struct pit_entry *ptn_env;

	printf("RST_STAT: 0x%x\n", rst_stat);

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

	if (!is_first_boot() || (rst_stat & (WARM_RESET | LITTLE_WDT_RESET | BIG_WDT_RESET)) ||
		((readl(CONFIG_RAMDUMP_SCRATCH) == CONFIG_RAMDUMP_MODE) && get_charger_mode() == 0) ||
		(fb_mode_failed == 1)) {
		dfd_run_dump_gpr();
		do_fastboot(0, 0);
	} else {
		/* Turn on dumpEN for DumpGPR */
		dfd_set_dump_gpr(CACHE_RESET_EN | DUMPGPR_EN);

		cmd_boot(0, 0);
	}

	return;
}

APP_START(exynos_boot)
	.entry = exynos_boot_task,
	.flags = 0,
APP_END
