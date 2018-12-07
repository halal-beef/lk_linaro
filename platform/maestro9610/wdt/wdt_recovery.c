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
#include <platform/sfr.h>

void clear_wdt_recovery_settings(void)
{
	unsigned int reg;

	printf("Mask WDT reset request\n");
	reg = readl(EXYNOS9610_POWER_MASK_WDT_RESET_REQUEST);
	reg |= (0x1 << 23);
	writel(reg, EXYNOS9610_POWER_MASK_WDT_RESET_REQUEST);

	printf("Clear bootloader booting start flag\n");
	reg = readl(EXYNOS9610_POWER_DREX_CALIBRATION7);
	reg &= ~0xF;
	writel(reg, EXYNOS9610_POWER_DREX_CALIBRATION7);
}
