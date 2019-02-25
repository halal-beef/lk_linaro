/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

#ifndef __EXYNOS9830_H__
#define __EXYNOS9830_H__

/* SFR base address. */
#define EXYNOS9830_PRO_ID								0x10000000
#define EXYNOS9830_MCT_BASE								0x10040000
#define EXYNOS9830_MCT_G_TCON								(EXYNOS9830_MCT_BASE + 0x0240)
#define EXYNOS9830_GPIO_PERIC0_BASE					0x10430000
#define EXYNOS9830_GPP4CON							(EXYNOS9830_GPIO_PERIC0_BASE + 0x0080)
#define EXYNOS9830_UART_BASE 							0x10540000
#define EXYNOS9830_POWER_BASE							0x15860000
#define EXYNOS9830_SWRESET 							(EXYNOS9830_POWER_BASE + 0x0400)
#define EXYNOS9830_POWER_RST_STAT 						(EXYNOS9830_POWER_BASE + 0x0404)
#define EXYNOS9830_POWER_RESET_SEQUENCER_CONFIGURATION				(EXYNOS9830_POWER_BASE + 0x0500)
#define EXYNOS9830_POWER_INFORM3 						(EXYNOS9830_POWER_BASE + 0x080C)
#define EXYNOS9830_POWER_SYSIP_DAT0 						(EXYNOS9830_POWER_BASE + 0x0810)
#define EXYNOS9830_POWER_MIPI_PHY_M4S4_CONTROL					(EXYNOS9830_POWER_BASE + 0x070C)
#define EXYNOS9830_EDPCSR_DUMP_EN						(1 << 0)
#define EXYNOS9830_PWMTIMER_BASE						0x106F0000
#define EXYNOS9830_SYSREG_DPU							0x19020000

#define CONFIG_SPEEDY0_BASE							0x15940000
#define CONFIG_SPEEDY1_BASE							0x15950000

/* CHIP ID */
#define CHIPID0_OFFSET								0x4
#define CHIPID1_OFFSET								0x8

#define BOOT_DEV_INFO								EXYNOS9830_POWER_INFORM3
#define BOOT_DEV								readl(EXYNOS9830_POWER_INFORM3)

#define DRAM_BASE								0x80000000
#define DRAM_BASE2								0x880000000

#define CFG_FASTBOOT_MMC_BUFFER						(0xC0000000)

/* iRAM information */
#define IRAM_BASE								(0x02020000)
#define IRAM_NS_BASE								(IRAM_BASE + 0x18000)
#define BL_SYS_INFO								(IRAM_NS_BASE + 0x800)
#define BL_SYS_INFO_DRAM_SIZE							(BL_SYS_INFO + 0x48)
#define CONFIG_IRAM_STACK							(IRAM_NS_BASE + 0x1000)
#define DRAM_INFO								(IRAM_BASE + 0x2C000)
#define DRAM_SIZE_INFO								(IRAM_BASE + 0x18848)

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

#define BOOT_BASE			0x94000000
#define KERNEL_BASE			0x80080000
#define RAMDISK_BASE			0x84000000
#define DT_BASE				0x8A000000
#define DTBO_BASE			0x8B000000
#define ECT_BASE			0x90000000
#define ECT_SIZE			0x32000


#endif	/* __EXYNOS9830_H__ */
