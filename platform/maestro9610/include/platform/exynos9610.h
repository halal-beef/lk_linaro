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
#define EXYNOS9610_MCT_BASE								0x10040000
#define EXYNOS9610_MCT_G_TCON								(EXYNOS9610_MCT_BASE + 0x0240)
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
#define EXYNOS9610_POWER_SYSIP_DAT0 						(EXYNOS9610_POWER_BASE + 0x0810)
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
#define EXYNOS9610_GPG1DRV 							(EXYNOS9610_GPIO_TOP_BASE + 0x00EC)
#define EXYNOS9610_GPG4CON							(EXYNOS9610_GPIO_TOP_BASE + 0x0140)
#define EXYNOS9610_GPG4DAT							(EXYNOS9610_GPIO_TOP_BASE + 0x0144)
#define EXYNOS9610_GPG4PUD							(EXYNOS9610_GPIO_TOP_BASE + 0x0148)
#define EXYNOS9610_TMU_TOP_BASE							0x10070000
#define EXYNOS9610_UART_BASE 							0x13820000
#define EXYNOS9610_PWMTIMER_BASE						0x13970000
#define EXYNOS9610_UFS_EMBEDDED_BASE						0x13520000
#define EXYNOS9610_UFSP_EMBEDDED_BASE						0x13530000
#define EXYNOS9610_SYSREG_DPU							0x14811000

/* CHIP ID */
#define CHIPID0_OFFSET								0x4
#define CHIPID1_OFFSET								0x8

#define BOOT_DEV_INFO								EXYNOS9610_POWER_INFORM3
#define BOOT_DEV								readl(EXYNOS9610_POWER_INFORM3)

#define DRAM_BASE								0x80000000
#define DRAM_BASE2								0x880000000

#define CFG_FASTBOOT_MMC_BUFFER						(0xC0000000)

/* iRAM information */
#define IRAM_BASE								(0x02020000)
#define IRAM_NS_BASE								(IRAM_BASE + 0x18000)
#define BL_SYS_INFO								(IRAM_NS_BASE + 0x800)
#define BL_SYS_INFO_DRAM_SIZE							(BL_SYS_INFO + 0x48)
#define CONFIG_IRAM_STACK							(IRAM_NS_BASE + 0x1000)

#define WARM_RESET								(1 << 28)
#define LITTLE_WDT_RESET							(1 << 23)
#define BIG_WDT_RESET								(1 << 24)
#define PIN_RESET								(1 << 16)
#define CONFIG_RAMDUMP_GPR
#define CONFIG_RAMDUMP_MODE          	0xD
#define CONFIG_RAMDUMP_OFFSET		(0x79000000)
#define CONFIG_RAMDUMP_LOG_OFFSET	(0x10000)
#define CONFIG_RAMDUMP_BASE		(DRAM_BASE + CONFIG_RAMDUMP_OFFSET)
#define CONFIG_RAMDUMP_LOGBUF           (CONFIG_RAMDUMP_BASE + CONFIG_RAMDUMP_LOG_OFFSET)
#define CONFIG_RAMDUMP_LOGSZ            0x200000
#define CONFIG_RAMDUMP_PANIC_LOGSZ      0x400

#define CONFIG_RAMDUMP_SCRATCH          (CONFIG_RAMDUMP_BASE + 0x100)
#define CONFIG_RAMDUMP_LASTBUF          (CONFIG_RAMDUMP_BASE + 0x200)
#define CONFIG_RAMDUMP_REASON           (CONFIG_RAMDUMP_BASE + 0x300)
#define CONFIG_RAMDUMP_DUMP_GPR_DEBUG   (CONFIG_RAMDUMP_BASE + 0x320)
#define CONFIG_RAMDUMP_DUMP_GPR_WAIT    (CONFIG_RAMDUMP_BASE + 0x380)
#define CONFIG_RAMDUMP_CORE_POWER_STAT  (CONFIG_RAMDUMP_BASE + 0x400)
#define CONFIG_RAMDUMP_GPR_POWER_STAT   (CONFIG_RAMDUMP_BASE + 0x480)
#define CONFIG_RAMDUMP_CORE_PANIC_STAT  (CONFIG_RAMDUMP_BASE + 0x500)
#define CONFIG_RAMDUMP_PANIC_REASON     (CONFIG_RAMDUMP_BASE + 0xC00)

#define CONFIG_RAMDUMP_COREREG          (CONFIG_RAMDUMP_BASE + 0x2000)
#define CONFIG_RAMDUMP_STACK            (CONFIG_RAMDUMP_BASE + 0x10000)

#define RAMDUMP_SIGN_RESET              0x0
#define RAMDUMP_SIGN_RESERVED           0x1
#define RAMDUMP_SIGN_SCRATCH            0xD
#define RAMDUMP_SIGN_ALIVE              0xFACE
#define RAMDUMP_SIGN_DEAD               0xDEAD
#define RAMDUMP_SIGN_PANIC              0xBABA
#define RAMDUMP_SIGN_NORMAL_REBOOT      0xCAFE
#define RAMDUMP_SIGN_FORCE_REBOOT       0xDAFE
#define RAMDUMP_SIGN_SAFE_FAULT         0xFAFA

/* AVB2.0 */
#define CONFIG_USE_AVB20
#define CONFIG_AVB_HW_HASH

#endif	/* __EXYNOS9610_H__ */
