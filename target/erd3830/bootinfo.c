/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

#include <target/bootinfo.h>
#include <reg.h>
#include <platform/sfr.h>

static exynos_bootinfo bootinfo_handle;

void set_reboot_mode(unsigned int flag)
{
	bootinfo_handle.reg = readl(EXYNOS_POWER_SYSIP_DAT0);
	bootinfo_handle.field.reboot_mode = flag;
	writel(bootinfo_handle.reg, EXYNOS_POWER_SYSIP_DAT0);
}

unsigned int get_reboot_mode(void)
{
	bootinfo_handle.reg = readl(EXYNOS_POWER_SYSIP_DAT0);
	return bootinfo_handle.field.reboot_mode;
}

void set_dump_en(void)
{
	bootinfo_handle.reg = readl(EXYNOS_POWER_SYSIP_DAT0);
	bootinfo_handle.field.dump_en = 1;
	writel(bootinfo_handle.reg, EXYNOS_POWER_SYSIP_DAT0);
}

unsigned int get_dump_en(void)
{
	bootinfo_handle.reg = readl(EXYNOS_POWER_SYSIP_DAT0);
	return bootinfo_handle.field.dump_en;
}

void set_ramdump_scratch(unsigned int flag)
{
	bootinfo_handle.reg = readl(EXYNOS_POWER_SYSIP_DAT0);
	bootinfo_handle.field.ramdump_scratch = flag;
	writel(bootinfo_handle.reg, EXYNOS_POWER_SYSIP_DAT0);
}

unsigned int get_ramdump_scratch(void)
{
	bootinfo_handle.reg = readl(EXYNOS_POWER_SYSIP_DAT0);
	return bootinfo_handle.field.ramdump_scratch;
}

/*
 * API for reboot mode
 */

unsigned int is_fastbootd_reboot_mode(void)
{
	return (get_reboot_mode() == REBOOT_MODE_FASTBOOT_USER);
}

unsigned int is_fastboot_reboot_mode(void)
{
	return (get_reboot_mode() == REBOOT_MODE_FASTBOOT);
}

unsigned int is_factory_reboot_mode(void)
{
	return (get_reboot_mode() == REBOOT_MODE_FACTORY);
}

unsigned int is_recovery_reboot_mode(void)
{
	return (get_reboot_mode() == REBOOT_MODE_RECOVERY);
}
