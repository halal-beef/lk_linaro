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
#include <string.h>
#include <stdlib.h>
#include <reg.h>
#include <pit.h>
#include <part_gpt.h>
#include <platform/sfr.h>
#include <platform/delay.h>
#include <platform/ab_update.h>
#include <platform/ab_slotinfo.h>
#include <platform/bootloader_message.h>
#include <platform/bl_sys_info.h>

int ab_update_slot_info(void)
{
	void *buf;
	struct bootloader_message_ab *bm;
	struct pit_entry *ptn;
	ExynosSlotInfo *a, *b, *active, *inactive;
	int ret = 0;

	ptn = pit_get_part_info(AB_SLOTINFO_PART_NAME);
	buf = memalign(0x1000, pit_get_length(ptn));
	pit_access(ptn, PIT_OP_LOAD, (u64)buf, 0);

	bm = (struct bootloader_message_ab *)buf;
	a = (ExynosSlotInfo *)bm->slot_suffix;
	b = a + 1;

	printf("\n");
	printf("slot information update - start\n");
	printf("_a bootable: %d, is_active %d, boot_successful %d, tries_remaining %d\n",
			a->bootable, a->is_active, a->boot_successful, a->tries_remaining);
	printf("_b bootable: %d, is_active %d, boot_successful %d, tries_remaining %d\n",
			b->bootable, b->is_active, b->boot_successful, b->tries_remaining);
	printf("\n");

	if (memcmp(a->magic, "EXBC", 4) ||
		memcmp(b->magic, "EXBC", 4)) {
		printf("Invalid slot information magic code!\n");
		ret = AB_ERROR_INVALID_MAGIC;
	} else if (a->is_active == 1 && b->is_active == 0) {
		active = a;
		inactive = b;
		if (active->bootable == 1) {
			if (active->bootable == 1 && active->boot_successful == 0) {
				printf("A slot tries_remaining: %d\n", active->tries_remaining);
				if(active->tries_remaining == 0) {
					active->bootable = 0;
					active->is_active = 0;
					if(inactive->bootable == 1) {
						inactive->is_active = 1;
						pit_access(ptn, PIT_OP_FLASH, (u64)buf, 0);
						/* Delay for data write HW operation on AB_SLOTINFO_PART partition */
						mdelay(500);
						free(buf);
						/* reset */
						writel(0x1, EXYNOS9610_SWRESET);
						do {
							asm volatile("wfi");
						} while(1);
					} else {
						ret = AB_ERROR_NO_BOOTABLE_SLOT;
					}
				} else {
					active->tries_remaining--;
				}
			}
		} else {
			ret = AB_ERROR_UNBOOTABLE_SLOT;
		}
	} else if (a->is_active == 0 && b->is_active == 1) {
		active = b;
		inactive = a;
		if (active->bootable == 1) {
			if (active->bootable == 1 && active->boot_successful == 0) {
				printf("B slot tries_remaining: %d\n", active->tries_remaining);
				if(active->tries_remaining == 0) {
					active->bootable = 0;
					active->is_active = 0;
					if(inactive->bootable == 1) {
						inactive->is_active = 1;
						pit_access(ptn, PIT_OP_FLASH, (u64)buf, 0);
						/* Delay for data write HW operation on AB_SLOTINFO_PART partition */
						mdelay(500);
						free(buf);
						/* reset */
						writel(0x1, EXYNOS9610_SWRESET);
						do {
							asm volatile("wfi");
						} while(1);
					} else {
						ret = AB_ERROR_NO_BOOTABLE_SLOT;
					}
				} else {
					active->tries_remaining--;
				}
			}
		} else {
			ret = AB_ERROR_UNBOOTABLE_SLOT;
		}
	} else if (a->is_active == 1 && b->is_active == 1) {
		ret = AB_ERROR_SLOT_ALL_ACTIVE;
	} else {
		ret = AB_ERROR_SLOT_ALL_INACTIVE;
	}

	printf("\n");
	printf("_a bootable: %d, is_active %d, boot_successful %d, tries_remaining %d\n",
			a->bootable, a->is_active, a->boot_successful, a->tries_remaining);
	printf("_b bootable: %d, is_active %d, boot_successful %d, tries_remaining %d\n",
			b->bootable, b->is_active, b->boot_successful, b->tries_remaining);
	printf("slot information update - end\n");
	printf("\n");

	pit_access(ptn, PIT_OP_FLASH, (u64)buf, 0);

	free(buf);

	return ret;
}

int ab_set_active_bootloader(int slot, ExynosSlotInfo *si)
{
	int other_slot = 1 - slot;

	(si + slot)->bootable = 1;
	(si + slot)->is_active = 1;
	(si + slot)->boot_successful = 0;
	(si + slot)->tries_remaining = 7;
	memcpy((si + slot)->magic, "EXBC", 4);

	(si + other_slot)->bootable = 0;
	(si + other_slot)->is_active = 0;
	(si + other_slot)->boot_successful = 0;
	(si + other_slot)->tries_remaining = 0;
	memcpy((si + other_slot)->magic, "EXBC", 4);

	printf("\n");
	printf("Current slot(%d) booting failed in bootloader. Changing active slot to %d.\n", other_slot, slot);

	return 0;
}

int ab_update_slot_info_bootloader(void)
{
	void *buf;
	struct bootloader_message_ab *bm;
	struct pit_entry *ptn, *bootloader_ptn;
	ExynosSlotInfo *a, *b;
	struct bl_sys_info *bl_sys = (struct bl_sys_info *)BL_SYS_INFO;
	int ret = 0;

	ptn = pit_get_part_info(AB_SLOTINFO_PART_NAME);
	buf = memalign(0x1000, pit_get_length(ptn));
	pit_access(ptn, PIT_OP_LOAD, (u64)buf, 0);

	bm = (struct bootloader_message_ab *)buf;
	a = (ExynosSlotInfo *)bm->slot_suffix;
	b = a + 1;

	printf("\n");
	printf("Slot information update when bootloader booting is failed - start\n");
	printf("Before\n");
	printf("_a bootable: %d, is_active %d, boot_successful %d, tries_remaining %d\n",
			a->bootable, a->is_active, a->boot_successful, a->tries_remaining);
	printf("_b bootable: %d, is_active %d, boot_successful %d, tries_remaining %d\n",
			b->bootable, b->is_active, b->boot_successful, b->tries_remaining);
	printf("\n");

	if (memcmp(a->magic, "EXBC", 4) ||
		memcmp(b->magic, "EXBC", 4)) {
		printf("Invalid slot information magic code!\n");
		ret = AB_ERROR_INVALID_MAGIC;
	} else if (a->is_active == 1 && b->is_active == 0) {
		bootloader_ptn = pit_get_part_info("bootloader_a");
		if (bl_sys->bl1_info.epbl_start * (UFS_BSIZE / MMC_BSIZE)
			!= bootloader_ptn->blkstart)
			ab_set_active_bootloader(1, a);
	} else if (a->is_active == 0 && b->is_active == 1) {
		bootloader_ptn = pit_get_part_info("bootloader_b");
		if (bl_sys->bl1_info.epbl_start * (UFS_BSIZE / MMC_BSIZE)
			!= bootloader_ptn->blkstart)
			ab_set_active_bootloader(0, a);
	} else if (a->is_active == 1 && b->is_active == 1) {
		ret = AB_ERROR_SLOT_ALL_ACTIVE;
	} else {
		ret = AB_ERROR_SLOT_ALL_INACTIVE;
	}

	printf("\n");
	printf("After\n");
	printf("_a bootable: %d, is_active %d, boot_successful %d, tries_remaining %d\n",
			a->bootable, a->is_active, a->boot_successful, a->tries_remaining);
	printf("_b bootable: %d, is_active %d, boot_successful %d, tries_remaining %d\n",
			b->bootable, b->is_active, b->boot_successful, b->tries_remaining);
	printf("Slot information update when bootloader booting is failed - end\n");
	printf("\n");

	pit_access(ptn, PIT_OP_FLASH, (u64)buf, 0);

	free(buf);

	return ret;
}

int ab_set_active(int slot)
{
	int other_slot = 1 - slot;
	void *buf;
	struct bootloader_message_ab *bm;
	struct pit_entry *ptn;
	ExynosSlotInfo *si;

	ptn = pit_get_part_info(AB_SLOTINFO_PART_NAME);
	buf = memalign(0x1000, pit_get_length(ptn));
	pit_access(ptn, PIT_OP_LOAD, (u64)buf, 0);

	bm = (struct bootloader_message_ab *)buf;
	si = (ExynosSlotInfo *)bm->slot_suffix;

	(si + slot)->bootable = 1;
	(si + slot)->is_active = 1;
	(si + slot)->boot_successful = 0;
	(si + slot)->tries_remaining = 7;
	memcpy((si + slot)->magic, "EXBC", 4);

	(si + other_slot)->bootable = 1;
	(si + other_slot)->is_active = 0;
	(si + other_slot)->boot_successful = 0;
	(si + other_slot)->tries_remaining = 7;
	memcpy((si + other_slot)->magic, "EXBC", 4);

	printf("\n");
	printf("_a bootable: %d, is_active %d, boot_successful %d, tries_remaining %d\n",
			(si + 0)->bootable, (si + 0)->is_active, (si + 0)->boot_successful, (si + 0)->tries_remaining);
	printf("_b bootable: %d, is_active %d, boot_successful %d, tries_remaining %d\n",
			(si + 1)->bootable, (si + 1)->is_active, (si + 1)->boot_successful, (si + 1)->tries_remaining);

	pit_access(ptn, PIT_OP_FLASH, (u64)buf, 0);

	free(buf);

	return 0;
}

int ab_current_slot(void)
{
	void *buf;
	struct bootloader_message_ab *bm;
	struct pit_entry *ptn;
	ExynosSlotInfo *a;
	int ret;

	ptn = pit_get_part_info(AB_SLOTINFO_PART_NAME);
	buf = memalign(0x1000, pit_get_length(ptn));
	pit_access(ptn, PIT_OP_LOAD, (u64)buf, 0);

	bm = (struct bootloader_message_ab *)buf;
	a = (ExynosSlotInfo *)bm->slot_suffix;

	ret = a->is_active == 1 ? 0 : 1;

	free(buf);

	return ret;
}

int ab_slot_successful(int slot)
{
	void *buf;
	struct bootloader_message_ab *bm;
	struct pit_entry *ptn;
	ExynosSlotInfo *si;
	int ret;

	ptn = pit_get_part_info(AB_SLOTINFO_PART_NAME);
	buf = memalign(0x1000, pit_get_length(ptn));
	pit_access(ptn, PIT_OP_LOAD, (u64)buf, 0);

	bm = (struct bootloader_message_ab *)buf;
	si = (ExynosSlotInfo *)bm->slot_suffix;

	ret = (si + slot)->boot_successful;

	free(buf);

	return ret;
}

int ab_slot_unbootable(int slot)
{
	void *buf;
	struct bootloader_message_ab *bm;
	struct pit_entry *ptn;
	ExynosSlotInfo *si;
	int ret;

	ptn = pit_get_part_info(AB_SLOTINFO_PART_NAME);
	buf = memalign(0x1000, pit_get_length(ptn));
	pit_access(ptn, PIT_OP_LOAD, (u64)buf, 0);

	bm = (struct bootloader_message_ab *)buf;
	si = (ExynosSlotInfo *)bm->slot_suffix;

	ret = (si + slot)->bootable ? 0 : 1;

	free(buf);

	return ret;
}

int ab_slot_retry_count(int slot)
{
	void *buf;
	struct bootloader_message_ab *bm;
	struct pit_entry *ptn;
	ExynosSlotInfo *si;
	int ret;

	ptn = pit_get_part_info(AB_SLOTINFO_PART_NAME);
	buf = memalign(0x1000, pit_get_length(ptn));
	pit_access(ptn, PIT_OP_LOAD, (u64)buf, 0);

	bm = (struct bootloader_message_ab *)buf;
	si = (ExynosSlotInfo *)bm->slot_suffix;

	ret = (si + slot)->tries_remaining;

	free(buf);

	return ret;
}
