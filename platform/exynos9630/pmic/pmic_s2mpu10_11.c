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
#include <sys/types.h>
#include <platform/speedy.h>
#include <platform/delay.h>
#include <platform/pmic_s2mpu10_11.h>
#include <platform/if_pmic_s2mu107.h>
#include <dev/lk_acpm_ipc.h>
#include <platform/gpio.h>
#include <platform/exynos9630.h>

void pmic_enable_manual_reset (void)
{
}

void pmic_int_mask(unsigned int chan, unsigned int addr, unsigned int interrupt)
{
	unsigned char reg;
	i3c_write(chan, addr, interrupt, 0xFF);
	i3c_read(chan, addr, interrupt, &reg);
	printf("interrupt(0x%x) : 0x%x\n", interrupt, reg);
}

void pmic_init (void)
{
	unsigned char reg;
	struct exynos_gpio_bank *bank = (struct exynos_gpio_bank *)EXYNOS9630_GPP2CON;

	/* Disable manual reset */
	i3c_read(0, S2MPU10_PM_ADDR, S2MPU10_PM_CTRL1, &reg);
	reg &= ~MRSTB_EN;
	i3c_write(0, S2MPU10_PM_ADDR, S2MPU10_PM_CTRL1, reg);

	/* Enable warm reset */
	i3c_read(0, S2MPU10_PM_ADDR, S2MPU10_PM_CTRL3, &reg);
	reg |= WRSTEN;
	i3c_write(0, S2MPU10_PM_ADDR, S2MPU10_PM_CTRL3, reg);

	/* Enable LCD power */
	reg = 0xF0;
	i3c_write(0, S2MPU10_PM_ADDR, S2MPU10_PM_LDO21_CTRL, reg);

	reg = 0xEC;
	i3c_write(0, S2MPU10_PM_ADDR, S2MPU10_PM_LDO22_CTRL, reg);

	/* Enable TSP power */
	reg = 0xE8;
	i3c_write(0, S2MPU10_PM_ADDR, S2MPU10_PM_LDO23_CTRL, reg);

	/* ICEN enable for PB03 */
	exynos_gpio_set_pull(bank, 4, GPIO_PULL_NONE);
	exynos_gpio_cfg_pin(bank, 4, GPIO_OUTPUT);
	exynos_gpio_set_value(bank, 4, 1);

	/* Main/Slave PMIC interrupt blocking */
	pmic_int_mask(0, S2MPU10_PM_ADDR, S2MPU10_PM_INT1M);
	pmic_int_mask(0, S2MPU10_PM_ADDR, S2MPU10_PM_INT2M);
	pmic_int_mask(0, S2MPU10_PM_ADDR, S2MPU10_PM_INT3M);
	pmic_int_mask(0, S2MPU10_PM_ADDR, S2MPU10_PM_INT4M);
	pmic_int_mask(0, S2MPU10_PM_ADDR, S2MPU10_PM_INT5M);
	pmic_int_mask(0, S2MPU10_PM_ADDR, S2MPU10_PM_INT6M);

	pmic_int_mask(1, S2MPU11_PM_ADDR, S2MPU11_PM_INT1M);
	pmic_int_mask(1, S2MPU11_PM_ADDR, S2MPU11_PM_INT2M);
	pmic_int_mask(1, S2MPU11_PM_ADDR, S2MPU11_PM_INT3M);
}

void display_pmic_rtc_time(void)
{
}

int get_pmic_rtc_time(char *buf)
{
	return 0;
}

void read_pmic_info_s2mpu10 (void)
{
	unsigned char read_int1, read_int2;
	unsigned char read_ldo21_ctrl, read_ldo22_ctrl, read_ldo23_ctrl;
	unsigned char read_pwronsrc, read_wtsr_smpl;

	i3c_read(0, S2MPU10_PM_ADDR, S2MPU10_PM_INT1, &read_int1);
	i3c_read(0, S2MPU10_PM_ADDR, S2MPU10_PM_INT2, &read_int2);
	i3c_read(0, S2MPU10_PM_ADDR, S2MPU10_PM_PWRONSRC, &read_pwronsrc);
	i3c_read(0, S2MPU10_PM_ADDR, S2MPU10_PM_LDO21_CTRL, &read_ldo21_ctrl);
	i3c_read(0, S2MPU10_PM_ADDR, S2MPU10_PM_LDO22_CTRL, &read_ldo22_ctrl);
	i3c_read(0, S2MPU10_PM_ADDR, S2MPU10_PM_LDO23_CTRL, &read_ldo23_ctrl);

	/* read PMIC RTC */
	i3c_read(0, S2MPU10_RTC_ADDR, S2MPU10_RTC_WTSR_SMPL, &read_wtsr_smpl);

	printf("S2MPU10_PM_INT1: 0x%x\n", read_int1);
	printf("S2MPU10_PM_INT2: 0x%x\n", read_int2);
	printf("S2MPU10_PM_PWRONSRC: 0x%x\n", read_pwronsrc);
	printf("S2MPU10_PM_LDO21M_CTRL: 0x%x\n", read_ldo21_ctrl);
	printf("S2MPU10_PM_LDO22M_CTRL: 0x%x\n", read_ldo22_ctrl);
	printf("S2MPU10_PM_LDO23M_CTRL: 0x%x\n", read_ldo23_ctrl);
	printf("S2MPU10_RTC_WTSR_SMPL : 0x%x\n", read_wtsr_smpl);

	if ((read_pwronsrc & (1 << 7)) && (read_int2 & (1 << 5)) && !(read_int1 & (1 << 7)))
		/* WTSR detect condition - WTSR_ON && WTSR_INT && ! MRB_INT */
		printf("WTSR detected\n");
	else if ((read_pwronsrc & (1 << 6)) && (read_int2 & (1 << 3)) && (read_wtsr_smpl & (1 << 7)))
		/* SMPL detect condition - SMPL_ON && SMPL_INT && SMPL_EN */
		printf("SMPL detected\n");
}

int chk_smpl_wtsr_s2mpu10(void)
{
	return 0;
}
