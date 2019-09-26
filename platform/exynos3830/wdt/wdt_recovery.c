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

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

static unsigned int watchdog_count;

void wdt_stop(void)
{
	unsigned long wtcon, pmu_reg;

	pmu_reg = readl(EXYNOS3830_POWER_CLUSTER0_NONCPU_INT_EN);
	pmu_reg &= ~(0x1 << EXYNOS3830_WDT_INT_EN);
	writel(pmu_reg, EXYNOS3830_POWER_CLUSTER0_NONCPU_INT_EN);
	printf("Watchdog INT_EN disable!\n");

	pmu_reg = readl(EXYNOS3830_POWER_CLUSTER0_NONCPU_OUT);
	pmu_reg &= ~(0x1 << EXYNOS3830_WDT_CNT_EN);
	writel(pmu_reg, EXYNOS3830_POWER_CLUSTER0_NONCPU_OUT);
	printf("Watchdog CNT_EN disable!\n");


	wtcon = readl(EXYNOS3830_WDT_WTCON);
	wtcon &= ~(EXYNOS3830_WDT_WTCON_ENABLE | EXYNOS3830_WDT_WTCON_RSTEN);
	writel(wtcon, EXYNOS3830_WDT_WTCON);

	wtcon = readl(EXYNOS3830_WDT_WTCON);
	printf("Watchdog cluster 0 stop done, WTCON = %lx\n", wtcon);
	writel(1, EXYNOS3830_WDT_WTCLRINT);

}

void wdt_start(unsigned int timeout)
{
	unsigned int count = timeout * (EXYNOS3830_WDT_FREQ / EXYNOS3830_WDT_INIT_PRESCALER);
	unsigned int divisor = 1;
	unsigned long wtcon, pmu_reg;

	printf("watchdog cl0 start, count = %x, timeout = %d\n", count, timeout);

	if (count >= 0x10000) {
		divisor = DIV_ROUND_UP(count, 0xffff);

		if (divisor > 0x100)
			divisor = 0x100;
	}

	printf("timeout=%d, divisor=%d, count=%d (%08x)\n",
		timeout, divisor, count, DIV_ROUND_UP(count, divisor));

	count = DIV_ROUND_UP(count, divisor);
	watchdog_count = count;

	/* update the pre-scaler */
	wtcon = readl(EXYNOS3830_WDT_WTCON);
	wtcon &= ~EXYNOS3830_WDT_PRESCALE_MASK;
	wtcon |= EXYNOS3830_WDT_PRESCALE(divisor - 1);

	writel(count, EXYNOS3830_WDT_WTDAT);
	writel(wtcon, EXYNOS3830_WDT_WTCON);

	pmu_reg = readl(EXYNOS3830_POWER_CLUSTER0_NONCPU_INT_EN);
	pmu_reg |= (0x1 << EXYNOS3830_WDT_INT_EN);
	writel(pmu_reg, EXYNOS3830_POWER_CLUSTER0_NONCPU_INT_EN);
	printf("Watchdog INT_EN enable!\n");

	pmu_reg = readl(EXYNOS3830_POWER_CLUSTER0_NONCPU_OUT);
	pmu_reg |= (0x1 << EXYNOS3830_WDT_CNT_EN);
	writel(pmu_reg, EXYNOS3830_POWER_CLUSTER0_NONCPU_OUT);
	printf("Watchdog CNT_EN enable!\n");

	wtcon = readl(EXYNOS3830_WDT_WTCON);
	wtcon |= EXYNOS3830_WDT_WTCON_ENABLE | EXYNOS3830_WDT_WTCON_DIV128;

	wtcon &= ~EXYNOS3830_WDT_WTCON_INTEN;
	wtcon |= EXYNOS3830_WDT_WTCON_RSTEN;

	writel(count, EXYNOS3830_WDT_WTDAT);
	writel(count, EXYNOS3830_WDT_WTCNT);
	writel(wtcon, EXYNOS3830_WDT_WTCON);

	wtcon = readl(EXYNOS3830_WDT_WTCON);
	printf("Watchdog cluster 0 start, WTCON = %lx\n", wtcon);

}

void wdt_keepalive(void)
{
	writel(watchdog_count, EXYNOS3830_WDT_WTCNT);
}

void clear_wdt_recovery_settings(void)
{
	unsigned int reg;

	wdt_stop();

	printf("Clear bootloader booting start flag\n");
	reg = readl(EXYNOS3830_POWER_DREX_CALIBRATION7);
	reg &= ~0xF;
	writel(reg, EXYNOS3830_POWER_DREX_CALIBRATION7);
}

void force_wdt_recovery(void)
{
	unsigned int reg;

	printf("Set bootloader booting retry count to 1\nto enter USB booting after WDT reset\n");
	reg = readl(EXYNOS3830_POWER_DREX_CALIBRATION7);
	reg &= ~0xF;
	reg |= 0x1;
	writel(reg, EXYNOS3830_POWER_DREX_CALIBRATION7);

	printf("Disable DUMP_EN\n");
	reg = readl(EXYNOS3830_POWER_RESET_SEQUENCER_CONFIGURATION);
	reg &= ~(EXYNOS3830_EDPCSR_DUMP_EN);
	writel(reg, EXYNOS3830_POWER_RESET_SEQUENCER_CONFIGURATION);

	wdt_start(1);

	do {
		asm volatile("wfi");
	} while(1);
}
