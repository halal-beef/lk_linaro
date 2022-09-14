/* Copyright (c) 2020 Samsung Electronics Co, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Copyright@ Samsung Electronics Co. LTD
 * Manseok Kim <manseoks.kim@samsung.com>
 *
 * Alternatively, Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <reg.h>
#include <stdio.h>
#include <platform/sfr.h>
#include <platform/gpio.h>

void display_te_init(void)
{
	struct exynos_gpio_bank *bank = (struct exynos_gpio_bank *)EXYNOS3830_GPA4CON;

	/* Set LCD_TE_HW (GPA4_1) to Pull-down enable, TE is active high */
	exynos_gpio_set_pull(bank, 1, GPIO_PULL_DOWN);

	/* Set LCD_TE_HW (GPA4_1) to DISP_TES0 */
	exynos_gpio_cfg_pin(bank, 1, GPIO_IRQ);
}

void display_panel_init(void)
{
	struct exynos_gpio_bank *bank = (struct exynos_gpio_bank *)EXYNOS3830_GPG2CON;

	/*
	 * !!! Do not use printf() !!!
	 */

	/* Set MLCD_RSTB (GPG2_5) to Pull-up enable, Reset is active low */
	exynos_gpio_set_pull(bank, 5, GPIO_PULL_UP);

	/* Set MLCD_RSTB (GPG2_5) to Output */
	exynos_gpio_cfg_pin(bank, 5, GPIO_OUTPUT);

	/* Set MLCD_RSTB (GPG2_5) to DRV 4X */
	exynos_gpio_set_drv(bank, 5, GPIO_DRV_4X);

}

void display_panel_reset(void)
{
	struct exynos_gpio_bank *bank = (struct exynos_gpio_bank *)EXYNOS3830_GPG2CON;

	/* Set MLCD_RSTB (GPG2_5) to low */
	exynos_gpio_set_value(bank, 5, 0);
}

void display_panel_release(void)
{
	struct exynos_gpio_bank *bank = (struct exynos_gpio_bank *)EXYNOS3830_GPG2CON;

	/* Set MLCD_RSTB (GPG2_5) to high */
	exynos_gpio_set_value(bank, 5, 1);
}

void display_panel_power(void)
{
	struct exynos_gpio_bank *bank = (struct exynos_gpio_bank *)EXYNOS3830_GPG1CON;

	/* Set VDD_DISP_1P6_EN (GPG1_1) to Output */
	exynos_gpio_cfg_pin(bank, 1, GPIO_OUTPUT);

	exynos_gpio_set_value(bank, 1, 1);

	/* TBD
	 * ***************************************************
		ldr	x0, =EXYNOS3830_GPG2CON
		ldr	w1, [x0]
		bic	w1, w1, #(0xF << 24)
		orr	w1, w1, #(0x1 << 24)
		str	w1, [x0]

		ldr	x0, =EXYNOS3830_GPG2DAT
		ldr	w1, [x0]
		bic	w1, w1, #(0x1 << 6)
		orr	w1, w1, #(0x1 << 6)
		str	w1, [x0]
	 * ***************************************************
		1. GPG2_6 to Output
		2. GPG2_6 is hight
	 * ***************************************************
	bank = (struct exynos_gpio_bank *)EXYNOS3830_GPG2CON;

	exynos_gpio_cfg_pin(bank, 6, GPIO_OUTPUT);

	exynos_gpio_set_value(bank, 6, 1);
	 */
}

void display_panel_detect_init(void)
{
}

u32 display_panel_connect_check(void)
{
	u32 ret = 0;

	return ret;	/* 0: connected */
}
