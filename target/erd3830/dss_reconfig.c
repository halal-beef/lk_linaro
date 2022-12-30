/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 *
 * No part of this software, either material or conceptual may be copied or
 * distributed, transmitted, transcribed, stored in a retrieval system or
 * translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <platform/fdt_reconfiguration.h>

enum debug_feature_id {
	eDSS_PLATFORM_ID,
	eDSS_ARRRST_ID,
	eDSS_FIRST_ID,
	eDSS_KEVENT_ID,
	eDSS_KEVENT_SMALL_ID,
};

static struct fdt_reconfiguration_data debug_dt_items[] = {
	[eDSS_PLATFORM_ID] = {LIST_INITIAL_CLEARED_VALUE,
				"/reserved-memory/debug_snapshot/log_platform",
				NULL, eFDT_RECONFIGURE_NONE, {0}},
	[eDSS_ARRRST_ID] = {LIST_INITIAL_CLEARED_VALUE,
				"/reserved-memory/debug_snapshot/log_arrdumpreset",
				NULL, eFDT_RECONFIGURE_NONE, {0}},
	[eDSS_FIRST_ID] = {LIST_INITIAL_CLEARED_VALUE,
				"/reserved-memory/debug_snapshot/log_first",
				NULL, eFDT_RECONFIGURE_NONE, {0}},
	[eDSS_KEVENT_ID] = {LIST_INITIAL_CLEARED_VALUE,
				"/reserved-memory/debug_snapshot/log_kevents",
				NULL, eFDT_RECONFIGURE_NONE, {0}},
	[eDSS_KEVENT_SMALL_ID] = {LIST_INITIAL_CLEARED_VALUE,
				"/reserved-memory/debug_snapshot/log_kevents_small",
				NULL, eFDT_RECONFIGURE_NONE, {0}},
};

static void set_dss_item_status(enum debug_feature_id id, bool status)
{
	struct fdt_reconfiguration_data *item = &debug_dt_items[id];

	if (item->type != eFDT_RECONFIGURE_NONE) {
		printf("%s: dss item is one time reconfigurable(%s)\n", __func__, item->path);
		return;
	}

	item->type = eFDT_RECONFIGURE_STATUS;
	item->status = status;
	add_fdt_reconfiguration_entry(item);
}

static void set_dss_item_size(enum debug_feature_id id, unsigned int size)
{
	struct fdt_reconfiguration_data *item = &debug_dt_items[id];

	if (item->type != eFDT_RECONFIGURE_NONE) {
		printf("%s: dss item is one time reconfigurable(%s)\n", __func__, item->path);
		return;
	}

	item->type = eFDT_RECONFIGURE_SIZE;
	item->size = size;
	add_fdt_reconfiguration_entry(item);
}

void set_dss_debug_level_low(void)
{
	set_dss_item_size(eDSS_PLATFORM_ID, 0x200000);
	set_dss_item_status(eDSS_ARRRST_ID, false);
	set_dss_item_status(eDSS_FIRST_ID, false);
	set_dss_item_status(eDSS_KEVENT_ID, false);
	set_dss_item_status(eDSS_KEVENT_SMALL_ID, true);
}

void set_dss_debug_level_high(void)
{
	set_dss_item_size(eDSS_PLATFORM_ID, 0x400000);
	set_dss_item_status(eDSS_ARRRST_ID, true);
	set_dss_item_status(eDSS_FIRST_ID, true);
	set_dss_item_status(eDSS_KEVENT_ID, true);
	set_dss_item_status(eDSS_KEVENT_SMALL_ID, false);
}
