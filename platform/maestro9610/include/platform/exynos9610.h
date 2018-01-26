/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

#ifndef __EXYNOS9610_H__
#define __EXYNOS9610_H__

/* EXYNOS9610 PLL */
#define SHARED0_PLL_9610							1
#define SHARED1_PLL_9610							2

/* SFR base address. */
#define EXYNOS9610_PRO_ID								0x10000000
#define EXYNOS9610_GPIO_ALIVE_BASE 						0x11850000
#define EXYNOS9610_GPA1CON							(EXYNOS9610_GPIO_ALIVE_BASE + 0x0040)
#define EXYNOS9610_GPA2CON 							(EXYNOS9610_GPIO_ALIVE_BASE + 0x0060)
#define EXYNOS9610_GPQ0CON 							(EXYNOS9610_GPIO_ALIVE_BASE + 0x0080)
#define EXYNOS9610_GPQ0PUD 							(EXYNOS9610_GPIO_ALIVE_BASE + 0x0088)
#define EXYNOS9610_POWER_BASE							0x11860000
#define EXYNOS9610_SWRESET 							(EXYNOS9610_POWER_BASE + 0x0400)
#define EXYNOS9610_POWER_RST_STAT 						(EXYNOS9610_POWER_BASE + 0x0404)
#define EXYNOS9610_POWER_RESET_SEQUENCER_CONFIGURATION				(EXYNOS9610_POWER_BASE + 0x0500)
#define EXYNOS9610_POWER_INFORM3 						(EXYNOS9610_POWER_BASE + 0x080C)
#define EXYNOS9610_POWER_MIPI_PHY_M4S4_CONTROL					(EXYNOS9610_POWER_BASE + 0x070C)
#define EXYNOS9610_EDPCSR_DUMP_EN						(1 << 0)
#define EXYNOS9610_CMU_TOP_BASE							0x12100000
#define EXYNOS9610_PLL_CON0_PLL_SHARED0 					(EXYNOS9610_CMU_TOP_BASE + 0x0120)
#define EXYNOS9610_PLL_CON0_PLL_SHARED1 					(EXYNOS9610_CMU_TOP_BASE + 0x0140)
#define EXYNOS9610_MUX_CLKCMU_PERI_UART 					(EXYNOS9610_CMU_TOP_BASE + 0x1080)
#define EXYNOS9610_DIV_CLKCMU_PERI_UART 					(EXYNOS9610_CMU_TOP_BASE + 0x1888)
#define EXYNOS9610_GPIO_FSYS_BASE 						0x13490000
#define EXYNOS9610_GPIO_TOP_BASE 						0x139B0000
#define EXYNOS9610_GPF0CON 							(EXYNOS9610_GPIO_FSYS_BASE + 0x0000)
#define EXYNOS9610_GPC2CON 							(EXYNOS9610_GPIO_TOP_BASE + 0x00A0)
#define EXYNOS9610_GPC2DAT 							(EXYNOS9610_GPIO_TOP_BASE + 0x00A4)
#define EXYNOS9610_GPC2PUD 							(EXYNOS9610_GPIO_TOP_BASE + 0x00A8)
#define EXYNOS9610_GPG1CON 							(EXYNOS9610_GPIO_TOP_BASE + 0x00E0)
#define EXYNOS9610_GPG1DAT 							(EXYNOS9610_GPIO_TOP_BASE + 0x00E4)
#define EXYNOS9610_GPG1PUD 							(EXYNOS9610_GPIO_TOP_BASE + 0x00E8)
#define EXYNOS9610_TMU_TOP_BASE							0x10070000
#define EXYNOS9610_UART_BASE 							0x13820000
#define EXYNOS9610_PWMTIMER_BASE						0x13970000
#define EXYNOS9610_UFS_EMBEDDED_BASE						0x13520000
#define EXYNOS9610_UFSP_EMBEDDED_BASE						0x13530000
#define EXYNOS9610_SYSREG_DPU							0x14811000

#ifdef CONFIG_CPU_EXYNOS9610
#define CONFIG_RAMDUMP_OFFSET							(0x79000000)
#define CONFIG_RAMDUMP_LOG_OFFSET						(0x10000)
#endif

#endif	/* __EXYNOS9610_H__ */
