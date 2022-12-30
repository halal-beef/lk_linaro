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
#ifndef __FDT_RECONFIGURATION_H__
#define __FDT_RECONFIGURATION_H__
#include <list.h>
#include <stdbool.h>

enum fdt_reconfiguration_type {
	eFDT_RECONFIGURE_NONE,
	eFDT_RECONFIGURE_STATUS,
	eFDT_RECONFIGURE_SIZE,
	eFDT_RECONFIGURE_PROPERTY,
};

struct fdt_reconfiguration_data {
	struct list_node list;
	const char *path;
	const char *prop;
	enum fdt_reconfiguration_type type;
	union {
		unsigned int size;
		unsigned int value;
		bool status;
	};
};

void add_fdt_reconfiguration_entry(struct fdt_reconfiguration_data *entry);
void reconfigure_fdt(void);

#endif /* __FDT_RECONFIGURATION_H__ */
