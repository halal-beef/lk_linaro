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
#include "uart_simple.h"
#include <dev/ufs.h>
#include <dev/boot.h>
#include <dev/rpmb.h>
#include <pit.h>
#include <lib/sysparam.h>
#include <lib/console.h>
#include <dev/interrupt/arm_gic.h>
#include <dev/timer/arm_generic.h>
#include <platform/interrupts.h>
#include <platform/sfr.h>
#include <platform/smc.h>
#include <platform/speedy.h>
#include <platform/pmic_s2mpu09.h>
#include <platform/if_pmic_s2mu004.h>
#include <platform/tmu.h>
#include <platform/dfd.h>
#include <platform/ldfw.h>
#include <platform/gpio.h>
#include <platform/bl_sys_info.h>
#include <platform/dram_training.h>
#include <platform/mmu/mmu_func.h>
#include <platform/fastboot.h>

#include <lib/font_display.h>
#include <lib/logo_display.h>
#include <part_gpt.h>
#include <target/dpu_config.h>
#include <stdio.h>

#define ARCH_TIMER_IRQ		30

void speedy_gpio_init(void);
void xbootldo_gpio_init(void);
void fg_init_s2mu004(void);

unsigned int s5p_chip_id[4] = {0x0, 0x0, 0x0, 0x0};
unsigned int charger_mode = 0;
unsigned int board_id = 0;
unsigned int board_rev = 0;
unsigned int dram_info[24] = {0, 0, 0, 0};
unsigned long long dram_size_info = 0;
unsigned int secure_os_loaded = 0;

unsigned int get_charger_mode(void)
{
	return charger_mode;
}

static void read_chip_id(void)
{
	s5p_chip_id[0] = readl(EXYNOS9610_PRO_ID + CHIPID0_OFFSET);
	s5p_chip_id[1] = readl(EXYNOS9610_PRO_ID + CHIPID1_OFFSET) & 0xFFFF;
}

static void read_dram_info(void)
{
	char type[16];
	char rank_num[20];
	char manufacturer[20];
	unsigned int M5 = 0, M6 = 0, M7 = 0, M8 = 0;
	unsigned int tmp = 0;

	printf("%s %d\n", __func__, __LINE__);
	/* 1. Type */
	dram_info[0] = readl(DRAM_INFO);
	tmp = dram_info[0] & 0xF;
	printf("%s %d\n", __func__, __LINE__);

	switch (tmp) {
	case 0x0:
		strcpy(type, "LPDDR4");
		break;
	case 0x2:
		strcpy(type, "LPDDR4X");
		break;
	default:
		printf("Type None!\n");
	}

	printf("%s %d\n", __func__, __LINE__);
	/* 2. rank_num */
	dram_info[0] = readl(DRAM_INFO);
	tmp = (dram_info[0] >> 4) & 0xF;

	printf("%s %d\n", __func__, __LINE__);
	switch (tmp) {
	case 0x0:
		strcpy(rank_num, "1RANK");
		break;
	case 0x3:
		strcpy(rank_num, "2RANK");
		break;
	default:
		printf("Rank_num None!\n");
	}

	printf("%s %d\n", __func__, __LINE__);
	/* 3. manufacturer */
	dram_info[0] = readl(DRAM_INFO);
	tmp = (dram_info[0] >> 8) & 0xFF;
	M5 = tmp;

	printf("%s %d\n", __func__, __LINE__);
	switch (tmp) {
	case 0x01:
		strcpy(manufacturer, "Samsung");
		break;
	case 0x06:
		strcpy(manufacturer, "SK hynix");
		break;
	case 0xFF:
		strcpy(manufacturer, "Micron");
		break;
	default:
		printf("Manufacturer None!\n");
	}

	printf("%s %d\n", __func__, __LINE__);
	dram_info[1] = readl(DRAM_INFO + 0x4);
	dram_info[2] = readl(DRAM_SIZE_INFO);
	dram_info[3] = readl(DRAM_SIZE_INFO + 0x4);
	dram_size_info |= (unsigned long long)(dram_info[2]);
	dram_size_info |= (unsigned long long)(dram_info[3]) << 32;
	/* Set to GB */
	dram_size_info = dram_size_info / 1024 / 1024 / 1024;

	M6 = dram_info[1] & 0xFF;
	M7 = (dram_info[1] >> 8) & 0xFF;
	M8 = (dram_info[0] & 0x3) | (((dram_info[0] >> 20) & 0xF) << 2) | ((dram_info[0]  >> 16 & 0x3) << 6);

#ifdef CONFIG_EXYNOS_BOOTLOADER_DISPLAY
	print_lcd(FONT_WHITE, FONT_BLACK, "DRAM %lu GB %s %s %s M5=0x%02x M6=0x%02x M7=0x%02x M8=0x%02x",
		dram_size_info,	type, rank_num, manufacturer,
		M5, M6, M7, M8);
#endif
}

static void load_secure_payload(void)
{
	unsigned long ret = 0;
	unsigned int boot_dev = 0;
	unsigned int dfd_en = readl(EXYNOS9610_POWER_RESET_SEQUENCER_CONFIGURATION);
	unsigned int rst_stat = readl(EXYNOS9610_POWER_RST_STAT);

	if (*(unsigned int *)DRAM_BASE != 0xabcdef) {
		printf("Running on DRAM by TRACE32: skip load_secure_payload()\n");
	} else {
		if (is_first_boot()) {
			boot_dev = readl(EXYNOS9610_POWER_INFORM3);

			/*
			 * In case WARM Reset/Watchdog Reset and DumpGPR is enabled,
			 * Secure payload doesn't have to be loaded.
			 */
			if (!((rst_stat & (WARM_RESET | LITTLE_WDT_RESET)) &&
				dfd_en & EXYNOS9610_EDPCSR_DUMP_EN)) {
				ret = load_sp_image(boot_dev);
				if (ret)
					/*
					 * 0xFEED0002 : Signature check fail
					 * 0xFEED0020 : Anti-rollback check fail
					 */
					printf("Fail to load Secure Payload!! [ret = 0x%lX]\n", ret);
				else {
					printf("Secure Payload is loaded successfully!\n");
					secure_os_loaded = 1;
				}
			}

			/*
			 * If boot device is eMMC, emmc_endbootop() should be
			 * implemented because secure payload is the last image
			 * in boot partition.
			 */
			if (boot_dev == BOOT_EMMC)
				emmc_endbootop();
		} else {
			/* second_boot = 1; */
		}
	}
}

static int check_charger_connect(void)
{
	unsigned char read_pwronsrc = 0;
	unsigned int rst_stat = readl(EXYNOS9610_POWER_RST_STAT);

	if (rst_stat == PIN_RESET) {
		speedy_init();
		speedy_read(S2MPU09_PM_ADDR, S2MPU09_PM_PWRONSRC, &read_pwronsrc);

		/* Check USB or TA connected and PWRONSRC(USB)  */
		if(read_pwronsrc & ACOK)
			charger_mode = 1;
		else
			charger_mode = 0;
	} else {
		charger_mode = 0;
	}

	return 0;
}

#ifdef CONFIG_EXYNOS_BOOTLOADER_DISPLAY
extern int display_drv_init(void);
void display_panel_init(void);

static void initialize_fbs(void)
{
	memset((void *)CONFIG_DISPLAY_LOGO_BASE_ADDRESS, 0, LCD_WIDTH * LCD_HEIGHT * 4);
	memset((void *)CONFIG_DISPLAY_FONT_BASE_ADDRESS, 0, LCD_WIDTH * LCD_HEIGHT * 4);
}
#endif

void arm_generic_timer_disable(void)
{
	mask_interrupt(ARCH_TIMER_IRQ);
}

void platform_early_init(void)
{
	unsigned int rst_stat = readl(EXYNOS9610_POWER_RST_STAT);
	unsigned int dfd_en = readl(EXYNOS9610_POWER_RESET_SEQUENCER_CONFIGURATION);
	struct exynos_gpio_bank *bank = (struct exynos_gpio_bank *)EXYNOS9610_GPA1CON;

	if (!((rst_stat & (WARM_RESET | LITTLE_WDT_RESET)) &&
			dfd_en & EXYNOS9610_EDPCSR_DUMP_EN)) {
		invalidate_dcache_all();
		cpu_common_init();
		clean_invalidate_dcache_all();
	}

	read_chip_id();

	speedy_gpio_init();
	xbootldo_gpio_init();
	/* Volume up set Input & Pull up */
	exynos_gpio_set_pull(bank, 5, GPIO_PULL_UP);
	exynos_gpio_cfg_pin(bank, 5, GPIO_INPUT);
	/* Volume down set Input & Pull up */
	exynos_gpio_set_pull(bank, 6, GPIO_PULL_UP);
	exynos_gpio_cfg_pin(bank, 6, GPIO_INPUT);
#ifdef CONFIG_EXYNOS_BOOTLOADER_DISPLAY
	display_panel_init();
	initialize_fbs();
#endif
	set_first_boot_device_info();

	if (get_current_boot_device() == BOOT_USB ||
		rst_stat & (WARM_RESET | LITTLE_WDT_RESET)) {
		uart_log_mode = 1;
	}
	uart_test_function();
	printf("LK build date: %s, time: %s\n", __DATE__, __TIME__);

	arm_gic_init();
	writel(1 << 8, EXYNOS9610_MCT_G_TCON);
	arm_generic_timer_init(ARCH_TIMER_IRQ, 26000000);
}

extern bool is_xct_boot(void);
extern int init_fastboot_variables(void);

void platform_init(void)
{
	u32 ret = 0;
	u32 temp = 0;
	struct bl_sys_info *bl_sys = (struct bl_sys_info *)BL_SYS_INFO;

	printf("bootloader partition start block(512 Byte): %d\n",
			bl_sys->bl1_info.epbl_start * (UFS_BSIZE / MMC_BSIZE));

	pmic_init();
	fg_init_s2mu004();
	check_charger_connect();
	read_pmic_info_s2mpu09();

	load_secure_payload();

	ufs_init(2);
	ufs_set_configuration_descriptor();
	pit_init();

	if (get_current_boot_device() != BOOT_USB &&
		*(unsigned int *)DRAM_BASE == 0xabcdef &&
		secure_os_loaded == 1) {
		unsigned int env_val = 0;

		if (sysparam_read("uart_log_enable", &env_val, sizeof(env_val)) > 0) {
			if (env_val == UART_LOG_MODE_FLAG)
				uart_log_mode = 1;
			else
				uart_log_mode = 0;
		} else
			uart_log_mode = 0;
	}

#ifdef CONFIG_EXYNOS_BOOTLOADER_DISPLAY
	/* If the display_drv_init function is not called before,
	 * you must use the print_lcd function.
	 */
	print_lcd(FONT_RED, FONT_BLACK, "LK Display is enabled!");
	ret = display_drv_init();
	if (ret == 0 && is_first_boot() && *(unsigned int *)DRAM_BASE == 0xabcdef)
		show_boot_logo();

	/* If the display_drv_init function is called,
	 * you must use the print_lcd_update function.
	 */
	//print_lcd_update(FONT_BLUE, FONT_BLACK, "LK display is enabled!");
#endif
	read_dram_info();

	tmu_initialize();
	read_temperature(TZ_G3D, &temp, PRINT);
	read_temperature(TZ_LIT, &temp, PRINT);
	read_temperature(TZ_BIG, &temp, PRINT);
	display_trip_info();
	dfd_display_reboot_reason();
	if ((get_current_boot_device() != BOOT_USB) &&
		*(unsigned int *)DRAM_BASE == 0xabcdef)
		init_fastboot_variables();
	if (is_xct_boot())
		return;

	if (is_first_boot() && *(unsigned int *)DRAM_BASE == 0xabcdef)
		debug_snapshot_fdt_init();

	if (secure_os_loaded == 1) {
		write_dram_training_data();

		if (!init_keystorage())
			printf("keystorage: init done successfully.\n");
		else
			printf("keystorage: init failed.\n");

		if (!init_ldfws()) {
			printf("ldfw: init done successfully.\n");
		} else {
			printf("ldfw: init failed.\n");
		}

		rpmb_key_programming();
		rpmb_load_boot_table();
	}
}
