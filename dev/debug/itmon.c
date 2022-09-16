/*
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
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
#include <string.h>
#include <arch/arm64.h>
#include <arch/ops.h>
#include <arch/arch_ops.h>
#include <malloc.h>
#include <list.h>
#include <bits.h>
#include <lk/init.h>
#include <dev/debug/dss.h>
#include <dev/debug/itmon.h>
#include <platform/sizes.h>
#include <platform/sfr.h>
#include <reg.h>
#include <platform/interrupts.h>
#include <kernel/vm.h>

#ifdef DEBUG
#define itmon_printf(f, str...) printf(str)
#else
#define itmon_printf(f, str...)  do { if (f == 1) printf(str); } while (0)
#endif

struct itmon_dev *g_itmon;

static struct itmon_rpathinfo *itmon_get_rpathinfo
					(struct itmon_dev *itmon,
					 unsigned int id,
					 char *dest_name)
{
	struct itmon_platdata *pdata = itmon->pdata;
	int i;

	if (!dest_name)
		return NULL;

	for (i = 0; i < pdata->num_rpathinfo; i++) {
		if (pdata->rpathinfo[i].id == (id & pdata->rpathinfo[i].bits)) {
			if (!strncmp(pdata->rpathinfo[i].dest_name, dest_name,
				  strlen(pdata->rpathinfo[i].dest_name))) {
				return &pdata->rpathinfo[i];
			}
		}
	}

	return NULL;
}

static struct itmon_masterinfo *itmon_get_masterinfo
				(struct itmon_dev *itmon,
				 char *port_name,
				 unsigned int user)
{
	struct itmon_platdata *pdata = itmon->pdata;
	int i;

	if (!port_name)
		return NULL;

	for (i = 0; i < pdata->num_masterinfo; i++) {
		if ((user & pdata->masterinfo[i].bits) ==
			pdata->masterinfo[i].user) {
			if (!strncmp(pdata->masterinfo[i].port_name,
				port_name, strlen(port_name)))
				return &pdata->masterinfo[i];
		}
	}

	return NULL;
}

static void itmon_enable_prt_chk(struct itmon_dev *itmon,
			       struct itmon_nodeinfo *node,
			       bool enabled)
{
	unsigned int offset = OFFSET_PRT_CHK;
	unsigned int val = 0;

	if (enabled)
		val = RD_RESP_INT_ENABLE | WR_RESP_INT_ENABLE |
		     ARLEN_RLAST_INT_ENABLE | AWLEN_WLAST_INT_ENABLE;

	writel(val, node->regs + offset + REG_PRT_CHK_CTL);

	itmon_printf(0, "ITMON - %s hw_assert %sabled\n",
			node->name, enabled == true ? "en" : "dis");
}

static void itmon_enable_err_report(struct itmon_dev *itmon,
			       struct itmon_nodeinfo *node,
			       bool enabled)
{
	struct itmon_platdata *pdata = itmon->pdata;
	unsigned int offset = OFFSET_REQ_R;

	if (!pdata->probed || !node->retention)
		writel(1, node->regs + offset + REG_INT_CLR);
	/* enable interrupt */
	writel(enabled, node->regs + offset + REG_INT_MASK);

	/* clear previous interrupt of req_write */
	offset = OFFSET_REQ_W;
	if (!pdata->probed || !node->retention)
		writel(1, node->regs + offset + REG_INT_CLR);
	/* enable interrupt */
	writel(enabled, node->regs + offset + REG_INT_MASK);

	/* clear previous interrupt of response_read */
	offset = OFFSET_RESP_R;
	if (!pdata->probed || !node->retention)
		writel(1, node->regs + offset + REG_INT_CLR);
	/* enable interrupt */
	writel(enabled, node->regs + offset + REG_INT_MASK);

	/* clear previous interrupt of response_write */
	offset = OFFSET_RESP_W;
	if (!pdata->probed || !node->retention)
		writel(1, node->regs + offset + REG_INT_CLR);
	/* enable interrupt */
	writel(enabled, node->regs + offset + REG_INT_MASK);

	itmon_printf(0, "ITMON - %s error reporting %sabled\n",
			node->name, enabled == true ? "en" : "dis");
}

static void itmon_enable_timeout(struct itmon_dev *itmon,
			       struct itmon_nodeinfo *node,
			       bool enabled)
{
	unsigned int offset = OFFSET_TMOUT_REG;

	/* Enable Timeout setting */
	writel(enabled, node->regs + offset + REG_DBG_CTL);

	/* set tmout interval value */
	writel(node->time_val, node->regs + offset + REG_TMOUT_INIT_VAL);

	/* Enable freezing */
	if (node->tmout_frz_enabled)
		writel(enabled, node->regs + offset + REG_TMOUT_FRZ_EN);

	itmon_printf(0, "ITMON - %s timeout %sabled\n",
			node->name, enabled == true ? "en" : "dis");
}

static void itmon_init_by_group(struct itmon_dev *itmon,
				struct itmon_nodegroup *group,
				bool enabled)
{
	struct itmon_nodeinfo *node = group->nodeinfo;
	u32 i;

	for (i = 0; i < group->nodesize; i++) {
		if (node[i].type == S_NODE && node[i].tmout_enabled)
			itmon_enable_timeout(itmon, &node[i], enabled);

		if (node[i].err_enabled)
			itmon_enable_err_report(itmon, &node[i], enabled);

		if (node[i].prt_chk_enabled)
			itmon_enable_prt_chk(itmon, &node[i], enabled);
	}
}

static void itmon_init(struct itmon_dev *itmon, bool enabled)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodegroup *group;
	int i;

	for (i = 0; i < pdata->num_nodegroup; i++) {
		group = &pdata->nodegroup[i];
		if (group->pd_support && !group->pd_status)
			continue;
		itmon_init_by_group(itmon, group, enabled);
	}
}

static unsigned int power(unsigned int param, unsigned int num)
{
	if (num == 0)
		return 1;
	return param * (power(param, num - 1));
}

static void itmon_report_prt_chk_rawdata(struct itmon_dev *itmon,
					 struct itmon_tracedata *data)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodeinfo *node = data->node;

	/* Output Raw register information */
	itmon_printf(1, "\n-----------------------------------------"
			"-----------------------------------------\n"
			"      Protocol Checker Raw Register Information"
			"(ITMON information)\n\n");
	itmon_printf(1, "      > %s(%s, 0x%08llX)\n"
			"      > REG(0x100~0x10C)      : 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
			node->name, pdata->node_string[node->type],
			node->regs,
			data->dbg_mo_cnt,
			data->prt_chk_ctl,
			data->prt_chk_info,
			data->prt_chk_int_id);
}

static void itmon_report_rawdata(struct itmon_dev *itmon,
				 struct itmon_tracedata *data)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodeinfo *node = data->node;

	/* Don't need when occurring prt_chk */
	if (BIT_PRT_CHK_ERR_OCCURRED(data->prt_chk_info) &&
		(BITS(data->prt_chk_info, 8, 1))) {
		itmon_report_prt_chk_rawdata(itmon, data);
		return;
	}

	/* Output Raw register information */
	itmon_printf(1, "Raw Register Information -----------------------------\n"
		"      > %s(%s, 0x%08llX)\n"
		"      > REG(0x08~0x18,0x80)   : 0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X\n"
		"      > REG(0x100~0x10C)      : 0x%08X, 0x%08X, 0x%08X, 0x%08X\n",
		node->name, pdata->node_string[node->type],
		node->regs + data->offset,
		data->int_info,
		data->ext_info_0,
		data->ext_info_1,
		data->ext_info_2,
		data->ext_user,
		data->dbg_mo_cnt,
		data->prt_chk_ctl,
		data->prt_chk_info,
		data->prt_chk_int_id);
}

static void itmon_report_traceinfo(struct itmon_dev *itmon,
				   struct itmon_traceinfo *info,
				   unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;

	if (!info->dirty)
		return;

	itmon_printf(1, "\n-----------------------------------------"
		"-----------------------------------------\n"
		"      Transaction Information\n\n"
		"      > Master         : %s %s\n"
		"      > Target         : %s\n"
		"      > Target Address : 0x%llX %s\n"
		"      > Type           : %s\n"
		"      > Error code     : %s\n\n",
		info->port, info->master ? info->master : "",
		info->dest ? info->dest : NOT_AVAILABLE_STR,
		info->target_addr,
		info->baaw_prot == true ? "(BAAW Remapped address)" : "",
		trans_type == TRANS_TYPE_READ ? "READ" : "WRITE",
		pdata->errcode[info->errcode]);

	itmon_printf(1, "\n-----------------------------------------"
		"-----------------------------------------\n"
		"      > Size           : %u bytes x %u burst => %u bytes\n"
		"      > Burst Type     : %u (0:FIXED, 1:INCR, 2:WRAP)\n"
		"      > Level          : %s\n"
		"      > Protection     : %s\n"
		"      > Path Type      : %s\n\n",
		power(2, info->axsize), info->axlen + 1,
		power(2, info->axsize) * (info->axlen + 1),
		info->axburst,
		BIT(0, info->axprot) ? "Privileged" : "Unprivileged",
		BIT(1, info->axprot) ? "Non-secure" : "Secure",
		pdata->pathtype[info->path_type]);
}

static void itmon_report_pathinfo(struct itmon_dev *itmon,
				  struct itmon_tracedata *data,
				  struct itmon_traceinfo *info,
				  unsigned int trans_type)

{
	struct itmon_nodeinfo *node = data->node;

	/* Don't need when occurring prt_chk */
	if (BIT_PRT_CHK_ERR_OCCURRED(data->prt_chk_info) &&
		(BITS(data->prt_chk_info, 8, 1)))
		return;

	if (!info->path_dirty) {
		itmon_printf(1,
			"\n-----------------------------------------"
			"-----------------------------------------\n"
			"      ITMON Report (%s)\n"
			"-----------------------------------------"
			"-----------------------------------------\n"
			"      PATH Information\n\n",
			trans_type == TRANS_TYPE_READ ? "READ" : "WRITE");
		info->path_dirty = true;
	}
	switch (node->type) {
	case M_NODE:
		itmon_printf(1, "      > %14s, %8s(0x%08llX)\n",
			node->name, "M_NODE", node->regs + data->offset);
		break;
	case T_S_NODE:
		itmon_printf(1, "      > %14s, %8s(0x%08llX)\n",
			node->name, "T_S_NODE", node->regs + data->offset);
		break;
	case T_M_NODE:
		itmon_printf(1, "      > %14s, %8s(0x%08llX)\n",
			node->name, "T_M_NODE", node->regs + data->offset);
		break;
	case S_NODE:
		itmon_printf(1, "      > %14s, %8s(0x%08llX)\n",
			node->name, "S_NODE", node->regs + data->offset);
		break;
	}
}

static int itmon_parse_cpuinfo(struct itmon_dev *itmon,
			       struct itmon_tracedata *data,
			       struct itmon_traceinfo *info,
			       unsigned int userbit)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodeinfo *node = data->node;
	int cluster_num, core_num, i;

	for (i = 0; i < pdata->num_cpu_node_string; i++) {
		if (!strncmp(node->name, pdata->cpu_node_string[i],
			strlen(pdata->cpu_node_string[i]))) {
				core_num = BITS(userbit, 2, 0);
				cluster_num = 0;
				if (pdata->cpu_parsing)
					snprintf(info->buf, sizeof(info->buf) - 1,
						"CPU%d Cluster%d", core_num, cluster_num);
				else
					snprintf(info->buf, sizeof(info->buf) - 1, "CPU");
				info->port = info->buf;
				return 1;
			}
	};

	return 0;
}

static void itmon_parse_traceinfo(struct itmon_dev *itmon,
				   struct itmon_tracedata *data,
				   unsigned int trans_type)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_traceinfo *new_info = NULL;
	struct itmon_tracedata *find_data = NULL;
	struct itmon_masterinfo *master = NULL;
	struct itmon_rpathinfo *port = NULL;
	struct itmon_nodeinfo *node = data->node, *find_node = NULL;
	struct itmon_nodegroup *group = data->group;
	unsigned int errcode, axid, userbit;
	int i, ret;

	/* Don't need when occurring prt_chk */
	if (BIT_PRT_CHK_ERR_OCCURRED(data->prt_chk_info) &&
		(BITS(data->prt_chk_info, 8, 1)))
		return;

	if (node->type != S_NODE && node->type != M_NODE)
		return;

	if (!BIT_ERR_VALID(data->int_info))
		return;

	errcode = BIT_ERR_CODE(data->int_info);
	if (node->type == M_NODE && errcode != ERRCODE_DECERR)
		return;

	new_info = malloc(sizeof(struct itmon_traceinfo));
	if (!new_info) {
		itmon_printf(1, "ITMON: failed to malloc for %s node\n", node->name);
		return;
	}
	axid = (unsigned int)BIT_AXID(data->int_info);
	if (group->path_type == DATA_FROM_EXT)
		userbit = BIT_AXUSER(data->ext_user);
	else
		userbit = BIT_AXUSER_PERI(data->ext_info_2);

	switch (node->type) {
	case S_NODE:
		new_info->port = (char *)"See M_NODE of Raw registers";
		new_info->master = (char *)"";
		new_info->dest = node->name;
		new_info->s_node = node;

		port = itmon_get_rpathinfo(itmon, axid, node->name);
		list_for_every_entry(&pdata->datalist[trans_type], find_data,
					struct itmon_tracedata, list) {
			find_node = find_data->node;
			if (find_node->type != M_NODE)
				continue;
			ret = itmon_parse_cpuinfo(itmon, find_data, new_info, userbit);
			if (ret || (port && !strncmp(find_node->name,
				port->port_name,strlen(find_node->name)))) {
				new_info->m_node = find_node;
				break;
			}
		}
		if (port) {
			new_info->port = port->port_name;
			master = itmon_get_masterinfo(itmon, new_info->port, userbit);
			if (master)
				new_info->master = master->master_name;
		}
		break;
	case M_NODE:
		new_info->dest = (char *)"See Target address";
		new_info->port = node->name;
		new_info->master = (char *)"";
		new_info->m_node = node;

		ret = itmon_parse_cpuinfo(itmon, data, new_info, userbit);
		if (ret)
			break;

		master = itmon_get_masterinfo(itmon, node->name, userbit);
		if (master)
			new_info->master = master->master_name;
		break;
	}

	/* Common Information */
	new_info->path_type = group->path_type;
	new_info->target_addr = ((u64)BITS(data->ext_info_1, 15, 0) << 32ULL);
	new_info->target_addr |= data->ext_info_0;
	new_info->errcode = errcode;
	new_info->dirty = true;
	new_info->axsize = BIT_AXSIZE(data->ext_info_1);
	new_info->axlen = BIT_AXLEN(data->ext_info_1);
	new_info->axburst = BIT_AXBURST(data->ext_info_2);
	new_info->axprot = BIT_AXPROT(data->ext_info_2);
	new_info->baaw_prot = false;

	for (i = 0; i < pdata->num_invalid_addr; i++) {
		if (new_info->target_addr == pdata->invalid_addr[i]) {
			new_info->baaw_prot = true;
			break;
		}
	}
	data->ref_info = new_info;
	list_add_head(&pdata->infolist[trans_type], &new_info->list);
}

static void itmon_analyze_errnode(struct itmon_dev *itmon)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_traceinfo *info, *next_info;
	struct itmon_tracedata *data, *next_data;
	unsigned int trans_type;
	int i;

	/* Parse */
	for (trans_type = 0; trans_type < TRANS_TYPE_NUM; trans_type++) {
		list_for_every_entry(&pdata->datalist[trans_type], data,
					struct itmon_tracedata, list) {
			itmon_parse_traceinfo(itmon, data, trans_type);
		}
	}

	/* Report */
	for (trans_type = 0; trans_type < TRANS_TYPE_NUM; trans_type++) {
		list_for_every_entry(&pdata->infolist[trans_type], info,
					struct itmon_traceinfo, list) {
			info->path_dirty = false;
			list_for_every_entry(&pdata->datalist[trans_type],
					data, struct itmon_tracedata, list) {
					if (data->ref_info == info)
						itmon_report_pathinfo(itmon, data, info, trans_type);
			}
			itmon_report_traceinfo(itmon, info, trans_type);
		}
	}

	/* Report Raw all tracedata and Clean-up tracedata and node */
	for (trans_type = 0; trans_type < TRANS_TYPE_NUM; trans_type++) {
		for (i = M_NODE; i < NODE_TYPE; i++) {
			list_for_every_entry_safe(&pdata->datalist[trans_type],
				data, next_data, struct itmon_tracedata, list) {
				if (i == (int)data->node->type) {
					itmon_report_rawdata(itmon, data);
					list_delete(&data->list);
					if (pdata->probed)
						free(data);
				}
			}
		}
	}

	/* Rest works and Clean-up traceinfo */
	for (trans_type = 0; trans_type < TRANS_TYPE_NUM; trans_type++) {
		list_for_every_entry_safe(&pdata->infolist[trans_type],
				info, next_info, struct itmon_traceinfo, list) {
			list_delete(&info->list);
			free(info);
		}
	}
}

static void itmon_collect_errnode(struct itmon_dev *itmon,
			    struct itmon_nodegroup *group,
			    struct itmon_nodeinfo *node,
			    unsigned int offset)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_tracedata *new_data = NULL;
	unsigned int int_info, info0, info1, info2, user = 0;
	unsigned int prt_chk_ctl, prt_chk_info, prt_chk_int_id, dbg_mo_cnt;
	bool read = TRANS_TYPE_WRITE;

	int_info = readl(node->regs + offset + REG_INT_INFO);
	info0 = readl(node->regs + offset + REG_EXT_INFO_0);
	info1 = readl(node->regs + offset + REG_EXT_INFO_1);
	info2 = readl(node->regs + offset + REG_EXT_INFO_2);
	if (group->path_type == DATA_FROM_EXT)
		user = readl(node->regs + offset + REG_EXT_USER);

	dbg_mo_cnt = readl(node->regs + OFFSET_PRT_CHK);
	prt_chk_ctl = readl(node->regs + OFFSET_PRT_CHK + REG_PRT_CHK_CTL);
	prt_chk_info = readl(node->regs + OFFSET_PRT_CHK + REG_PRT_CHK_INT);
	prt_chk_int_id = readl(node->regs + OFFSET_PRT_CHK + REG_PRT_CHK_INT_ID);
	switch (offset) {
	case OFFSET_REQ_R:
		read = TRANS_TYPE_READ;
		/* fall down */
	case OFFSET_REQ_W:
		/* Only S-Node is able to make log to registers */
		break;
	case OFFSET_RESP_R:
		read = TRANS_TYPE_READ;
		/* fall down */
	case OFFSET_RESP_W:
		/* Only NOT S-Node is able to make log to registers */
		break;
	default:
		itmon_printf(1, "ITMON: Unknown Error - node:%s offset:%u\n",
				node->name, offset);
		break;
	}

	new_data = malloc(sizeof(struct itmon_tracedata));
	if (!new_data) {
		itmon_printf(1, "ITMON: failed to malloc for %s node %x offset\n",
		node->name, offset);
		return;
	}

	new_data->int_info = int_info;
	new_data->ext_info_0 = info0;
	new_data->ext_info_1 = info1;
	new_data->ext_info_2 = info2;
	new_data->ext_user = user;
	new_data->dbg_mo_cnt = dbg_mo_cnt;
	new_data->prt_chk_ctl = prt_chk_ctl;
	new_data->prt_chk_info = prt_chk_info;
	new_data->prt_chk_int_id = prt_chk_int_id;

	new_data->offset = offset;
	new_data->read = read;
	new_data->ref_info = NULL;
	new_data->group = group;
	new_data->node = node;

	if (BIT_ERR_VALID(int_info))
		new_data->logging = true;
	else
		new_data->logging = false;

	list_add_head(&pdata->datalist[read], &new_data->list);
}

static int __itmon_search_node(struct itmon_dev *itmon,
			       struct itmon_nodegroup *group,
			       bool clear)
{
	struct itmon_nodeinfo *node = NULL;
	unsigned int val, offset;
	unsigned long vec;
	u32 bit;
	int i, ret = 0;

	if (group->regs) {
		if (group->ex_table)
			vec = (unsigned long)readq(group->regs);
		else
			vec = (unsigned long)readl(group->regs);
	} else {
		vec = BITS(0xFFFFFFFF, group->nodesize, 0);
	}

	if (!vec)
		return 0;

	node = group->nodeinfo;

	for (bit = 0; bit < group->nodesize; bit++) {
		if (!BIT_SET(vec, bit))
			continue;

		/* exist array */
		for (i = 0; i < OFFSET_NUM; i++) {
			offset = i * OFFSET_ERR_REPT;
			/* Check Request information */
			val = readl(node[bit].regs + offset + REG_INT_INFO);
			if (BIT_ERR_OCCURRED(val)) {
				/* This node occurs the error */
				itmon_collect_errnode(itmon, group, &node[bit], offset);
				if (clear)
					writel(1, node[bit].regs
							+ offset + REG_INT_CLR);
				ret++;
			}
		}
		/* Check H/W assertion */
		if (node[bit].prt_chk_enabled) {
			val = readl(node[bit].regs + OFFSET_PRT_CHK +
						REG_PRT_CHK_INT);
			/* if timeout_freeze is enable,
			 * prt_ck interrupt is able to assert without any information */
			if (BIT_PRT_CHK_ERR_OCCURRED(val) && (BITS(val, 8, 1))) {
				itmon_collect_errnode(itmon, group, &node[bit], 0);
				ret++;
			}
		}
	}

	return ret;
}

static int itmon_search_node(struct itmon_dev *itmon,
			     struct itmon_nodegroup *group,
			     bool clear)
{
	struct itmon_platdata *pdata = itmon->pdata;
	int i, ret = 0;

	if (group) {
		ret = __itmon_search_node(itmon, group, clear);
	} else {
		/* Processing all group & nodes */
		for (i = 0; i < pdata->num_nodegroup; i++) {
			group = &pdata->nodegroup[i];
			if (group->pd_support && !group->pd_status)
				continue;
			ret |= __itmon_search_node(itmon, group, clear);
		}
	}
	itmon_analyze_errnode(itmon);

	return ret;
}

static void itmon_enable_irq(struct itmon_dev *itmon, int enabled);
static void itmon_do_dpm_policy(struct itmon_dev *itmon, bool forced)
{
	struct itmon_platdata *pdata = itmon->pdata;

	if ((pdata->err_cnt++ > PANIC_THRESHOLD) || forced) {
		itmon_printf(1, "ITMON: error overflow, irq disable\n");
		itmon_enable_irq(itmon, false);
	}
}

static enum handler_return itmon_irq_handler(void *argv)
{
	struct itmon_dev *itmon = (struct itmon_dev *)argv;
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_nodegroup *group = NULL;
	u32 vec = 0;
	int i, ret;

	/* Search itmon group */
	for (i = 0; i < pdata->num_nodegroup; i++) {
		group = &pdata->nodegroup[i];
		if (!group->pd_support) {
			u32 vec_once = readl(group->regs);

			vec |= vec_once;
			itmon_printf(1, "ITMON: %s: %d irq, %s group, pd %s, 0x%x vec\n",
					__func__, pdata->nodegroup[i].irq,
					group->name, "on", vec_once);
		}
	}

	ret = itmon_search_node(itmon, NULL, true);
	if (!ret) {
		if (vec) {
			itmon_printf(1, "ITMON: vec/err count  miss match, irq disable\n");
			itmon_do_dpm_policy(itmon, true);
		} else {
			itmon_printf(1, "ITMON: No errors found\n");
		}
	} else {
		itmon_printf(1, "ITMON: Error detected - %d\n", ret);
		itmon_do_dpm_policy(itmon, false);
	}

	return INT_RESCHEDULE;
}

static int itmon_init_irq(struct itmon_dev *itmon,
			    struct itmon_nodegroup *nodegroup,
			    int en)
{
	int ret = 0;

	if (nodegroup->irq) {
		if (en) {
			register_int_handler(nodegroup->irq, &itmon_irq_handler, itmon);
			ret = unmask_interrupt(nodegroup->irq);
		} else {
			ret = mask_interrupt(nodegroup->irq);
		}
	}

	return ret;
}

static void itmon_keep_mem(struct itmon_dev *itmon)
{
	struct itmon_keepdata *kdata = itmon->kdata;
	struct itmon_platdata *pdata = itmon->pdata;
	unsigned int offset, size;
	unsigned long base;
	int i;

	if (kdata == NULL)
		return;

	if (kdata->base == 0)
		return;

	/* rpath */
	base = (kdata->base);
	offset = sizeof(struct itmon_keepdata);
	size = sizeof(struct itmon_rpathinfo) * pdata->num_rpathinfo;

	memcpy((void *)(base + offset), (void *)pdata->rpathinfo, size);
	kdata->mem_rpathinfo = kdata->base + offset;
	kdata->size_rpathinfo = size;
	kdata->num_rpathinfo = pdata->num_rpathinfo;

	/* masterinfo */
	offset = offset + size;
	size = sizeof(struct itmon_masterinfo) * pdata->num_masterinfo;

	memcpy((void *)(base + offset), (void *)pdata->masterinfo, size);
	kdata->mem_masterinfo = kdata->base + offset;
	kdata->size_masterinfo = size;
	kdata->num_masterinfo = pdata->num_masterinfo;

	/* nodeinfo from nodegroup */
	offset = offset + size;
	for (i = 0; i < pdata->num_nodegroup; i++) {
		size = sizeof(struct itmon_nodeinfo) * pdata->nodegroup[i].nodesize;
		memcpy((void *)(base + offset),
			(void *)pdata->nodegroup[i].nodeinfo, size);

		/* update to nodegroup */
		pdata->nodegroup[i].nodeinfo_phy =
			(struct itmon_nodeinfo *)(kdata->base + offset);

		offset = offset + size;
	}

	/* nodegroup */
	size = sizeof(struct itmon_nodegroup) * pdata->num_nodegroup;
	memcpy((void *)(base + offset), (void *)pdata->nodegroup, size);
	kdata->mem_nodegroup = kdata->base + offset;
	kdata->size_nodegroup = size;
	kdata->num_nodegroup = pdata->num_nodegroup;

	/* keep data */
	size = sizeof(struct itmon_keepdata);
	memcpy((void *)(base), (void *)kdata, size);

	itmon_printf(1, "ITMON: success to keep the shared data\n");
}

__attribute__((weak)) int itmon_init_plat(struct itmon_dev *itmon)
{
	return -1;
}

__attribute__((weak)) int itmon_init_keep(struct itmon_dev *itmon)
{
	return -1;
}

static void itmon_parse_before(struct itmon_dev *itmon)
{
	struct itmon_platdata *pdata = itmon->pdata;
	struct itmon_tracedata *prev_data = NULL;
	unsigned int i = 0;

	for (i = 0; i < SZ_2K / sizeof(struct itmon_tracedata); i++) {
		prev_data = (struct itmon_tracedata *)
			(CONFIG_RAMDUMP_MINITMON_BASE +
				(sizeof(struct itmon_tracedata) * i));
		if ((u64)prev_data->list.prev == 0xBEEFBEEF &&
			(u64)prev_data->list.next == 0xABCDABCD) {
			prev_data->node = (struct itmon_nodeinfo *)(prev_data->node);
			prev_data->group = (struct itmon_nodegroup *)(prev_data->group);
			list_add_head(&pdata->datalist[prev_data->read], &prev_data->list);
		} else {
			break;
		}
	}

	if (i > 0) {
		itmon_printf(1,
			"\n-----------------------------------------"
			"-----------------------------------------\n"
			"Note: ITMON Log is that occurred in the previous CPU lockup state\n"
			"Note: ITMON Log is that occurred in the previous CPU lockup state\n"
			"Note: ITMON Log is that occurred in the previous CPU lockup state\n"
			"-----------------------------------------"
			"-----------------------------------------\n");
		itmon_analyze_errnode(itmon);
	}
}

static void itmon_enable_irq(struct itmon_dev *itmon, int enabled)
{
	struct itmon_platdata *pdata = itmon->pdata;
	int i, ret = 0;

	for (i = 0; i < pdata->num_nodegroup; i++) {
		ret = itmon_init_irq(itmon, &pdata->nodegroup[i], enabled);
		itmon_printf(1, "ITMON: irq%u - %s / %sabled %s\n",
			pdata->nodegroup[i].irq,
			pdata->nodegroup[i].name,
			enabled == 1 ? "En" : "Dis",
			ret == 0 ? "Success" : "Failure");
	}
}

void itmon_enable(int enabled)
{
	if (g_itmon && g_itmon->pdata->probed)
		itmon_enable_irq(g_itmon, enabled);
}

static void itmon_probe(uint level)
{
	struct itmon_dev *itmon;
	struct itmon_platdata *pdata;
	struct itmon_keepdata *kdata;
	int ret, i;
//	unsigned int rst_stat = readl(EXYNOS_POWER_RST_STAT);

	itmon = malloc(sizeof(struct itmon_dev));
	if (!itmon)
		goto out;

	pdata = malloc(sizeof(struct itmon_platdata));
	if (!pdata) {
		free(itmon);
		goto out;
	}
	itmon->pdata = pdata;

	ret = itmon_init_plat(itmon);
	if (ret) {
		free(itmon);
		free(pdata);
		goto out;
	}

	kdata = malloc(sizeof(struct itmon_keepdata));
	if (kdata) {
		itmon->kdata = kdata;
		ret = itmon_init_keep(itmon);
		if (ret < 0)
			itmon_printf(1, "ITMON: failed to itmon_init_keep\n");
	}

	itmon_init(itmon, true);

	itmon_enable_irq(itmon, true);

	for (i = 0; i < TRANS_TYPE_NUM; i++) {
		list_initialize(&pdata->datalist[i]);
		list_initialize(&pdata->infolist[i]);
	}

//	if (check_dram_init_flag() &&
//		(rst_stat & (WARM_RESET | LITTLE_WDT_RESET | APM_WDT_RESET)) &&
//		get_ramdump_scratch() == true) {
//		itmon_parse_before(itmon);
//	}

//	if (check_dram_init_flag())
	itmon_keep_mem(itmon);

	pdata->probed = true;
	pdata->err_cnt = 0;
	g_itmon = itmon;
	itmon_printf(1, "ITMON: success to probe Exynos ITMON driver\n");
out:
	return;
}
LK_INIT_HOOK(itmon_probe, &itmon_probe, ITMON_INIT_LEVEL);
