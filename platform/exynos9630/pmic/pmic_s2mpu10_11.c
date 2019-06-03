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
#include <dev/lk_acpm_ipc.h>

void pmic_enable_manual_reset (void)
{
}

void pmic_init (void)
{
	unsigned char reg;

	/* Enable LCD power */
	reg = 0xF0;
	i3c_write(0, S2MPU10_PM_ADDR, S2MPU10_PM_LDO21_CTRL, reg);

	reg = 0xEC;
	i3c_write(0, S2MPU10_PM_ADDR, S2MPU10_PM_LDO22_CTRL, reg);

	/* Enable TSP power */
	reg = 0xE8;
	i3c_write(0, S2MPU10_PM_ADDR, S2MPU10_PM_LDO23_CTRL, reg);
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
	unsigned char read_ldo21_ctrl, read_ldo22_ctrl, read_ldo23_ctrl;

	i3c_read(0, S2MPU10_PM_ADDR, S2MPU10_PM_LDO21_CTRL, &read_ldo21_ctrl);
	i3c_read(0, S2MPU10_PM_ADDR, S2MPU10_PM_LDO22_CTRL, &read_ldo22_ctrl);
	i3c_read(0, S2MPU10_PM_ADDR, S2MPU10_PM_LDO23_CTRL, &read_ldo23_ctrl);

	printf("S2MPU10_PM_LDO21M_CTRL: 0x%x\n", read_ldo21_ctrl);
	printf("S2MPU10_PM_LDO22M_CTRL: 0x%x\n", read_ldo22_ctrl);
	printf("S2MPU10_PM_LDO23M_CTRL: 0x%x\n", read_ldo23_ctrl);
}

int chk_smpl_wtsr_s2mpu10(void)
{
	return 0;
}
