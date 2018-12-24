/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 */

#include <debug.h>
#include <platform/secure_boot.h>
#include <platform/sfr.h>
#include <platform/otp_v20.h>
#include <platform/ab_update.h>
#include <dev/rpmb.h>
#include <string.h>
#include <pit.h>

#define EPBL_SIZE		(76 * 1024)

/* By convention, when a rollback index is not used the value remains zero. */
static const uint64_t kRollbackIndexNotUsed = 0;

void avb_print_lcd(const char *str);

uint32_t is_slot_marked_successful(void)
{
	uint32_t ret;

	ret = ab_slot_successful(ab_current_slot());

	return ret;
}

uint32_t update_rp_count_otp(const char *suffix)
{
	uint32_t ret = 0;
	uint64_t *rollback_index;
	char part_name[15] = "bootloader";
	struct pit_entry *ptn;

	ptn = pit_get_part_info("fwbl1");
	pit_access(ptn, PIT_OP_LOAD, (u64)AVB_PRELOAD_BASE, 0);
	rollback_index = (uint64_t *)(AVB_PRELOAD_BASE + pit_get_length(ptn) -
			SB_SB_CONTEXT_LEN + SB_BL1_RP_COUNT_OFFSET);
	printf("[SB]BL1 RP count: %lld\n", *rollback_index);
	ret = cm_otp_update_antirbk_sec_ap(*rollback_index);
	if (ret)
		goto out;

	strcat(part_name, suffix);
	ptn = pit_get_part_info(part_name);
	pit_access(ptn, PIT_OP_LOAD, (u64)AVB_PRELOAD_BASE, 0);
	rollback_index = (uint64_t *)(AVB_PRELOAD_BASE + EPBL_SIZE -
			SB_MAX_RSA_SIGN_LEN - SB_SIGN_FIELD_HEADER_SIZE);
	printf("[SB]Bootloader RP count: %lld\n", *rollback_index);
	ret = cm_otp_update_antirbk_non_sec_ap0(*rollback_index);
	if (ret)
		goto out;

out:
	return ret;
}

uint32_t update_rp_count_avb(AvbOps *ops, AvbSlotVerifyData *ctx_ptr)
{
	uint32_t ret = 0;
	uint32_t i = 0;
	uint64_t stored_rollback_index = 0;

	for (i = 0; i < AVB_MAX_NUMBER_OF_ROLLBACK_INDEX_LOCATIONS; i++) {
		if (ctx_ptr->rollback_indexes[i] != kRollbackIndexNotUsed) {
			printf("[AVB 2.0]Rollback index location: %d\n", i);
			printf("[AVB 2.0]Current Image RP count: %lld\n",
					ctx_ptr->rollback_indexes[i]);
			ret = ops->read_rollback_index(ops, i,
					&stored_rollback_index);
			if (ret) {
				printf("[AVB 2.0 ERR] Read RP count fail, ret: 0x%X\n",
						ret);
				goto out;
			}
			printf("[AVB 2.0]Current RPMB RP count: %lld\n",
					stored_rollback_index);

			if (ctx_ptr->rollback_indexes[i] > stored_rollback_index) {
				printf("[AVB 2.0]Update RP count start\n");
				ret = ops->write_rollback_index(ops, i,
						ctx_ptr->rollback_indexes[i]);
				if (ret) {
					printf("[AVB 2.0 ERR] Write RP count fail, ret: 0x%X\n",
							ret);
					goto out;
				}
				ret = ops->read_rollback_index(ops, i,
						&stored_rollback_index);
				if (ret) {
					printf("[AVB 2.0 ERR] Read RP count fail, ret: 0x%X\n",
							ret);
					goto out;
				}
				printf("[AVB 2.0]Current Image RP count: %lld\n",
						ctx_ptr->rollback_indexes[i]);
				printf("[AVB 2.0]Updated RPMB RP count: %lld\n",
						stored_rollback_index);
				if (ctx_ptr->rollback_indexes[i] != stored_rollback_index)
					ret = AVB_ERROR_RP_UPDATE_FAIL;
			}
		}
	}

out:
	return ret;
}

#if defined(CONFIG_USE_AVB20)
uint32_t avb_main(const char *suffix, char *cmdline, char *verifiedbootstate)
{
	bool unlock;
	uint32_t ret = 0;
	uint32_t i = 0;
	uint32_t tmp = 0;
	struct AvbOps *ops;
	const char *partition_arr[] = {"boot", "dtbo", NULL};
	char buf[100];
	char color[AVB_COLOR_MAX_SIZE];
	AvbSlotVerifyData *ctx_ptr = NULL;

	set_avbops();
	get_ops_addr(&ops);
	ops->read_is_device_unlocked(ops, &unlock);

	/* slot verify */
	ret = avb_slot_verify(ops, partition_arr, suffix,
			AVB_SLOT_VERIFY_FLAGS_ALLOW_VERIFICATION_ERROR,
			AVB_HASHTREE_ERROR_MODE_RESTART_AND_INVALIDATE,
			&ctx_ptr);
	/* get color */
	if (unlock) {
		strncpy(color, "orange", AVB_COLOR_MAX_SIZE);
	} else {
		if (ret == AVB_SLOT_VERIFY_RESULT_ERROR_PUBLIC_KEY_REJECTED) {
			strncpy(color, "yellow", AVB_COLOR_MAX_SIZE);
		} else if (ret) {
			strncpy(color, "red", AVB_COLOR_MAX_SIZE);
		} else {
			strncpy(color, "green", AVB_COLOR_MAX_SIZE);
		}
	}
	if (ret) {
		snprintf(buf, 100, "[AVB 2.0 ERR] authentication fail [ret: 0x%X] (%s)\n", ret, color);
	} else {
		snprintf(buf, 100, "[AVB 2.0] authentication success (%s)\n", color);
	}
	strcat(verifiedbootstate, color);
	printf(buf);
	avb_print_lcd(buf);

	/* Update RP count */
	if (!ret && is_slot_marked_successful()) {
		ret = update_rp_count_avb(ops, ctx_ptr);
		if (ret)
			return ret;
		ret = update_rp_count_otp(suffix);
		if (ret)
			return ret;
	}

	/* block RPMB */
	tmp = block_RPMB_hmac();
	if (tmp) {
		printf("[AVB 2.0 ERR] RPMB hmac ret: 0x%X\n", tmp);
	}

	/* set cmdline */
	if (ctx_ptr != NULL) {
		i = 0;
		while (ctx_ptr->cmdline[i++] != '\0');
		memcpy(cmdline, ctx_ptr->cmdline, i);
	}
#if defined(CONFIG_AVB_DEBUG)
	printf("i: %d\n", i);
	printf("cmdline: %s\n", cmdline);
#endif

	return ret;
}
#endif
