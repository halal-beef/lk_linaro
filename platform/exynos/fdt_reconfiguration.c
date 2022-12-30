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
#include <libfdt.h>
#include <lib/fdtapi.h>
#include <platform/fdt_reconfiguration.h>

static struct list_node fdt_reconfiguration_list = LIST_INITIAL_VALUE(fdt_reconfiguration_list);

void add_fdt_reconfiguration_entry(struct fdt_reconfiguration_data *entry)
{
	list_add_tail(&fdt_reconfiguration_list, &entry->list);
}

struct reg_data {
	unsigned int size;
	unsigned int lo_base;
	unsigned int hi_base;
};

struct fdt_reg_data {
	union {
		struct reg_data reg;
		char data[sizeof(struct reg_data)];
	};
};

static void copy_with_changing_endian(char *dest, const char *src, int size)
{
	int i;

	for (i = 0; i < size; i++)
		dest[size - i - 1] = src[i];
}

static void reconfigure_fdt_size(struct fdt_reconfiguration_data *entry)
{
	const char *prop;
	struct fdt_reg_data reg;
	char big_endian_data[sizeof(reg)];
	int len;
	int node_offset = fdt_path_offset(fdt_dtb, entry->path);

	if (node_offset < 0) {
		printf("%s node is invalid\n", entry->path);
		return;
	}

	prop = (const char *)fdt_getprop(fdt_dtb, node_offset, "reg", &len);
	if (len != sizeof(reg)) {
		printf("%s node has invalid reg data\n", entry->path);
		return;
	}

	copy_with_changing_endian(reg.data, prop, len);
	printf("%s's size modified from %#x to %#x\n", entry->path, reg.reg.size, entry->size);

	reg.reg.size = entry->size;
	copy_with_changing_endian(big_endian_data, (const char *)reg.data, len);
	fdt_setprop(fdt_dtb, node_offset, "reg", (const void *)big_endian_data, len);
}

static void reconfigure_fdt_status(struct fdt_reconfiguration_data *entry)
{
	const char *status;
	int node_offset = fdt_path_offset(fdt_dtb, entry->path);

	if (node_offset < 0) {
		printf("%s node is invalid\n", entry->path);
		return;
	}

	if (entry->status)
		status = "ok";
	else
		status = "no";

	fdt_setprop(fdt_dtb, node_offset, "status", status, strlen(status) + 1);
	printf("%s status set to %s\n", entry->path, status);
}

static void reconfigure_fdt_property(struct fdt_reconfiguration_data *entry)
{
	const char *prop;
	u32 old, new;
	int len;
	int node_offset = fdt_path_offset(fdt_dtb, entry->path);

	if (node_offset < 0) {
		printf("%s node is invalid\n", entry->path);
		return;
	}

	prop = (const char *)fdt_getprop(fdt_dtb, node_offset, entry->prop, &len);

	if (!prop || (len != sizeof(u32))) {
		printf("%s: %s property is invalid\n", __func__, entry->prop);
		return;
	}

	copy_with_changing_endian((char *)&old, prop, sizeof(u32));
	copy_with_changing_endian((char *)&new, (const char *)&entry->value, sizeof(u32));
	fdt_setprop(fdt_dtb, node_offset, entry->prop, (const void *)&new, len);
	printf("%s/%s property is changed from %u to %u\n", entry->path, entry->prop,
									old, entry->value);
}

static void reconfigure_fdt_entry(struct fdt_reconfiguration_data *entry)
{
	switch (entry->type) {
	case eFDT_RECONFIGURE_STATUS:
		reconfigure_fdt_status(entry);
		break;
	case eFDT_RECONFIGURE_SIZE:
		reconfigure_fdt_size(entry);
		break;
	case eFDT_RECONFIGURE_PROPERTY:
		reconfigure_fdt_property(entry);
		break;
	default:
		printf("invalid fdt reconfiguration request(%s/%d)\n", entry->path, entry->type);
		break;
	}
}

void reconfigure_fdt(void)
{
	struct fdt_reconfiguration_data *pos;

	list_for_every_entry(&fdt_reconfiguration_list, pos, struct fdt_reconfiguration_data, list)
		reconfigure_fdt_entry(pos);
}
