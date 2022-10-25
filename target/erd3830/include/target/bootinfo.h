/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

#ifndef __BL2_BOOTINFO_H__
#define __BL2_BOOTINFO_H__

struct exynos_bootinfo_field {
	unsigned int reboot_mode	: 3;
	unsigned int bl2_footprint	: 1;
	unsigned int dram_init_flag	: 1;
	unsigned int dump_en		: 1;
	unsigned int ramdump_scratch	: 1;
	unsigned int reserved		: 25;
};

typedef struct exynos_bootinfo {
	union {
		struct exynos_bootinfo_field field;
		unsigned int reg;
	};
} exynos_bootinfo;

void set_reboot_mode(unsigned int);
unsigned int get_reboot_mode(void);
void set_bl2_footprint(void);
unsigned int get_bl2_footprint(void);
unsigned int check_dram_init_flag(void);
void set_dump_en(void);
unsigned int get_dump_en(void);
unsigned int get_ramdump_scratch(void);
void set_ramdump_scratch(unsigned int flag);
int get_boot_device_info(void);
unsigned int is_fastbootd_reboot_mode(void);
unsigned int is_fastboot_reboot_mode(void);
unsigned int is_factory_reboot_mode(void);
unsigned int is_recovery_reboot_mode(void);
unsigned int is_ramdump_mode(void);

#endif /*__BL2_BOOTINFO_H__*/
