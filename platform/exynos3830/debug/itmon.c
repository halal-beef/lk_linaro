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

#include <string.h>
#include <arch/arm64.h>
#include <arch/ops.h>
#include <arch/arch_ops.h>
#include <dev/debug/dss.h>
#include <dev/debug/itmon.h>
#include <platform/sizes.h>
#include <platform/sfr.h>
#include <platform/delay.h>
#include <reg.h>

#define BITS_PER_LONG 32
#define BIT(nr) 					(1 << nr)
#define GENMASK(h, l)           (((0xffffffff)<<(l)) & (0xffffffff >> (BITS_PER_LONG - 1 -(h))))

static struct itmon_rpathinfo rpathinfo[] = {
	/* Data BUS: Master ----> 0x2000_0000 -- 0xF_FFFF_FFFF */
	{0,	"ALIVE",	"NRT_MEM",	GENMASK(3, 0)},
	{1,	"CHUB",		"NRT_MEM",	GENMASK(3, 0)},
	{2,	"WLBT",		"NRT_MEM",	GENMASK(3, 0)},
	{3,	"COREX",	"NRT_MEM",	GENMASK(3, 0)},
	{4,	"CSSYS",	"NRT_MEM",	GENMASK(3, 0)},
	{5,	"HSI",		"NRT_MEM",	GENMASK(3, 0)},
	{6,	"G3D",		"NRT_MEM",	GENMASK(3, 0)},
	{7,	"GNSS",		"NRT_MEM",	GENMASK(3, 0)},
	{8,	"IS1",		"NRT_MEM",	GENMASK(3, 0)},
	{9,	"MFCMSCL",	"NRT_MEM",	GENMASK(3, 0)},
	{10,	"CP_1",		"NRT_MEM",	GENMASK(3, 0)},

	/* RT_MEM: Master ----> 0x2000_0000 -- 0xF_FFFF_FFFF */
	{0,	"IS0",		"RT_MEM",	GENMASK(2, 0)},
	{1,	"DPU",		"RT_MEM",	GENMASK(2, 0)},
	{2,	"WLBT",		"RT_MEM",	GENMASK(2, 0)},
	{3,	"GNSS",		"RT_MEM",	GENMASK(2, 0)},
	{4,	"CP_1",		"RT_MEM",	GENMASK(2, 0)},
	{5,	"IS1",		"RT_MEM",	GENMASK(2, 0)},
	{6,	"AUD",		"RT_MEM",	GENMASK(2, 0)},

	/* CP_MEM: Master ----> 0x2000_0000 -- 0xF_FFFF_FFFF */
	{0,	"CP_0",		"CP_MEM",	GENMASK(2, 0)},
	{1,	"AUD",		"CP_MEM",	GENMASK(2, 0)},
	{2,	"WLBT",		"CP_MEM",	GENMASK(2, 0)},
	{3,	"CP_1",		"CP_MEM",	GENMASK(2, 0)},

	/* Peri BUS: Master ----> 0x0 -- 0x1FFF-FFFF */
	{0,	"AUD",		"PERI",		GENMASK(3, 0)},
	{1,	"APM",		"PERI",		GENMASK(3, 0)},
	{2,	"IS1",		"PERI",		GENMASK(3, 0)},
	{3,	"MFCMSCL",	"PERI",		GENMASK(3, 0)},
	{4,	"CP_0",		"PERI",		GENMASK(3, 0)},
	{5,	"CP_1",		"PERI",		GENMASK(3, 0)},
	{6,	"WLBT",		"PERI",		GENMASK(3, 0)},
	{7,	"CHUB",		"PERI",		GENMASK(3, 0)},
	{8,	"COREX",	"PERI",		GENMASK(3, 0)},
	{9,	"CSSYS",	"PERI",		GENMASK(3, 0)},
	{10,	"DPU",		"PERI",		GENMASK(3, 0)},
	{11,	"HSI",		"PERI",		GENMASK(3, 0)},
	{12,	"G3D",		"PERI",		GENMASK(3, 0)},
	{13,	"GNSS",		"PERI",		GENMASK(3, 0)},
	{14,	"IS0",		"PERI",		GENMASK(3, 0)},
};

/* XIU ID Information */
static struct itmon_masterinfo masterinfo[] = {
	{"DPU",		BIT(0),				"SYSMMU_D_DPU",		GENMASK(0, 0)},	/* XXXXXX1 */
	{"DPU",		0,				"DPU_DMA",		GENMASK(0, 0)},	/* XXXXXX0 */

	{"AUD",		BIT(0),				"SYSMMU_D_AUD",		GENMASK(0, 0)},	/* XXXXXX0 */
	{"AUD",		0,				"SPUS/SPUM",		GENMASK(2, 0)},	/* XXXX000 */
	{"AUD",		BIT(1),				"ABOX CA7",		GENMASK(2, 0)},	/* XXXX010 */
	{"AUD",		BIT(1) | BIT(2),		"AUDEN",		GENMASK(2, 0)},	/* XXXX110 */

	{"IS0",		BIT(0),				"SYSMMU_D0_IS",		GENMASK(0, 0)},	/* XXXXXX1 */
	{"IS0",		0,				"CSIS",			GENMASK(1, 0)},	/* XXXXX00 */
	{"IS0",		BIT(1),				"IPP",			GENMASK(1, 0)},	/* XXXXX10 */

	{"IS1",		BIT(0),				"SYSMMU_D1_IS",		GENMASK(0, 0)},	/* XXXXXX1 */
	{"IS1",		0,				"ITP",			GENMASK(2, 0)},	/* XXXX000 */
	{"IS1",		BIT(1),				"MCSC",			GENMASK(2, 0)},	/* XXXX010 */
	{"IS1",		BIT(2),				"GDC",			GENMASK(2, 0)},	/* XXXX100 */
	{"IS1",		BIT(1) | BIT(2),		"VRA",			GENMASK(2, 0)},	/* XXXX110 */

	{"HSI",		0,				"MMC_CARD",		GENMASK(0, 0)},	/* XXXXXX0 */
	{"HSI",		BIT(0),				"USB",			GENMASK(0, 0)},	/* XXXXXX1 */

	{"CHUB",	0,				"CM4_CHUB",		GENMASK(0, 0)},	/* XXXXXX0 */

	{"COREX",	0,				"RTIC",			GENMASK(2, 0)},	/* XXXX000 */
	{"COREX",	BIT(0),				"SSS",			GENMASK(2, 0)},	/* XXXX001 */
	{"COREX",	BIT(1),				"PDMA",			GENMASK(2, 0)},	/* XXXX010 */
	{"COREX",	BIT(0) | BIT(1),		"SPDMA",		GENMASK(2, 0)},	/* XXXX011 */
	{"COREX",	BIT(2),				"MMC_EMBD",		GENMASK(2, 0)},	/* XXXX100 */

	{"G3D",		0,				"G3D",			0},		/* XXXXXXX */
	{"CSSYS",	0,				"CSSYS",		0},		/* XXXXXXX */
	{"APM",		0,				"APM",			0},		/* XXXXXXX */

	{"MFCMSCL",	BIT(0),				"SYSMMU_D_MFCMSCL",	GENMASK(0, 0)},	/* XXXXXX1 */
	{"MFCMSCL",	0,				"MFC",			GENMASK(1, 0)},	/* XXXXXX1 */
	{"MFCMSCL",	BIT(0),				"JPEG",			GENMASK(1, 0)},	/* XXXXX01 */
	{"MFCMSCL",	BIT(1),				"MCSC",			GENMASK(1, 0)},	/* XXXXX10 */
	{"MFCMSCL",	BIT(0) | BIT(1),		"M2M",			GENMASK(1, 0)},	/* XXXXX10 */

	{"CP_",		0,				"U-CPU",		GENMASK(6, 5)},	/* 00XXXXX */
	{"CP_",		BIT(3) | BIT(4) | BIT(5),	"L-CPU",		GENMASK(6, 4)},	/* 0111XXX */
	{"CP_",		BIT(5),				"DMA0",			GENMASK(6, 0)},	/* 0111XXX */
	{"CP_",		BIT(0) | BIT(5),		"DMA1",			GENMASK(6, 0)},	/* 0100001 */
	{"CP_",		BIT(1) | BIT(5),		"DMA2",			GENMASK(6, 0)},	/* 0100010 */
	{"CP_",		BIT(3) | BIT(5) | BIT(6),	"CSXAP",		GENMASK(6, 0)},	/* 1101000 */
	{"CP_",		BIT(3) | BIT(5),		"DataMover",		GENMASK(6, 0)},	/* 0101000 */
	{"CP_",		BIT(4) | BIT(5),		"LMAC",			GENMASK(6, 0)},	/* 0110000 */
	{"CP_",		BIT(5) | BIT(6),		"HarqMover",		GENMASK(6, 0)},	/* 1100000 */

	{"WLBT",	0,				"CR4",			GENMASK(0, 0)},	/* XXXXXX0 */
	{"WLBT",	BIT(0),				"ENC_DMA",		GENMASK(4, 0)},	/* XX00001 */

	{"GNSS",	BIT(1),				"CM7F",			GENMASK(5, 0)},	/* X000010 */
	{"GNSS",	0,				"XMDA0",		GENMASK(5, 0)},	/* X000000 */
	{"GNSS",	BIT(0),				"XMDA1",		GENMASK(5, 0)},	/* X000001 */
};

static struct itmon_nodeinfo data_path[] = {
	{"APM",			M_NODE, true, true, false, false, false, false, 0, 0x12413000, NULL, NULL, 0},
	{"AUD",			M_NODE, true, true, false, false, false, false, 0, 0x12403000, NULL, NULL, 0},
	{"CHUB",		M_NODE, true, true, false, false, false, false, 0, 0x12423000, NULL, NULL, 0},
	{"COREX",		M_NODE, true, true, false, false, false, false, 0, 0x12433000, NULL, NULL, 0},
	{"CSSYS",		M_NODE, true, true, false, false, false, false, 0, 0x12443000, NULL, NULL, 0},
	{"DPU",			M_NODE, true, true, false, false, false, false, 0, 0x12453000, NULL, NULL, 0},
	{"G3D",			M_NODE, true, true, false, false, false, false, 0, 0x12473000, NULL, NULL, 0},
	{"GNSS",		M_NODE, true, true, false, false, false, false, 0, 0x12483000, NULL, NULL, 0},
	{"HSI",			M_NODE, true, true, false, false, false, false, 0, 0x12463000, NULL, NULL, 0},
	{"IS0",			M_NODE, true, true, false, false, false, false, 0, 0x12493000, NULL, NULL, 0},
	{"IS1",			M_NODE, true, true, false, false, false, false, 0, 0x124A3000, NULL, NULL, 0},
	{"MFCMSCL",		M_NODE, true, true, false, false, false, false, 0, 0x124B3000, NULL, NULL, 0},
	{"CP_0",		M_NODE, true, true, false, false, false, false, 0, 0x124C3000, NULL, NULL, 0},
	{"CP_1",		M_NODE, true, true, false, false, false, false, 0, 0x124D3000, NULL, NULL, 0},
	{"WLBT",		M_NODE, true, true, false, false, false, false, 0, 0x124E3000, NULL, NULL, 0},
	{"CP_MEM0",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12533000, NULL, NULL, 0},
	{"CP_MEM1",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12543000, NULL, NULL, 0},
	{"NRT_MEM0",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12513000, NULL, NULL, 0},
	{"NRT_MEM1",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12523000, NULL, NULL, 0},
	{"RT_MEM0",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x124F3000, NULL, NULL, 0},
	{"RT_MEM1",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12503000, NULL, NULL, 0},
	{"PERI",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12553000, NULL, NULL, 0},
};

static struct itmon_nodeinfo peri_path[] = {
	{"M_CPU",		M_NODE, true, true, false, false, false, false, 0, 0x12803000, NULL, NULL, 0},
	{"M_PERI",		M_NODE, true, true, false, false, false, false, 0, 0x12813000, NULL, NULL, 0},
	{"ALIVE",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12823000, NULL, NULL, 0},
	{"AUD",			S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12833000, NULL, NULL, 0},
	{"CHUB",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12953000, NULL, NULL, 0},
	{"COREP_SFR",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12883000, NULL, NULL, 0},
	{"COREP_TREX",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12873000, NULL, NULL, 0},
	{"CPU_CL0",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12843000, NULL, NULL, 0},
	{"CPU_CL1",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12853000, NULL, NULL, 0},
	{"CSSYS",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12863000, NULL, NULL, 0},
	{"DPU",			S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12893000, NULL, NULL, 0},
	{"G3D",			S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x128B3000, NULL, NULL, 0},
	{"GIC",			S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x128C3000, NULL, NULL, 0},
	{"GNSS",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x128D3000, NULL, NULL, 0},
	{"HSI",			S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x128A3000, NULL, NULL, 0},
	{"ISP",			S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x128E3000, NULL, NULL, 0},
	{"MFCMSCL",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x128F3000, NULL, NULL, 0},
	{"MFC0",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12903000, NULL, NULL, 0},
	{"MFC1",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12913000, NULL, NULL, 0},
	{"CP",			S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12923000, NULL, NULL, 0},
	{"PERI",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12933000, NULL, NULL, 0},
	{"WLBTP",		S_NODE, true, true, false, false, true, false, TMOUT_VAL, 0x12943000, NULL, NULL, 0},
};

static struct itmon_nodegroup nodegroup[] = {
	{"DATA",	0x12563000, 0, 0, data_path, ARRAY_SIZE(data_path),
		DATA_FROM_INFO2, NULL, 462, 0, 0, "none", 0},
	{"PERI",	0x12973000, 0, 0, peri_path, ARRAY_SIZE(peri_path),
		PERI, NULL, 461, 0, 0, "none", 0},
};

static const char *itmon_pathtype[] = {
	"DATA Path transaction (0x2000_0000 ~ 0xf_ffff_ffff)",
	"PERI(SFR) Path transaction (0x0 ~ 0x1fff_ffff)",
};

/* Error Code Description */
static const char *itmon_errcode[] = {
	"Error Detect by the Slave(SLVERR)",
	"Decode error(DECERR)",
	"Unsupported transaction error",
	"Power Down access error",
	"Unsupported transaction",
	"Unsupported transaction",
	"Timeout error - response timeout in timeout value",
	"Invalid errorcode",
};

static const char *itmon_node_string[] = {
	"M_NODE",
	"TAXI_S_NODE",
	"TAXI_M_NODE",
	"S_NODE",
};

static const char *itmon_cpu_node_string[] = {
	"CLUSTER0_P",
	"M_CPU",
	"SCI_IRPM",
	"SCI_CCM",
	"CCI",
};

static const unsigned int itmon_invalid_addr[] = {
	0x03000000,
	0x04000000,
};

int itmon_init_plat(struct itmon_dev *itmon)
{
	struct itmon_platdata *pdata = itmon->pdata;

	pdata->rpathinfo = rpathinfo;
	pdata->num_rpathinfo = ARRAY_SIZE(rpathinfo);

	pdata->masterinfo = masterinfo;
	pdata->num_masterinfo = ARRAY_SIZE(masterinfo);

	pdata->nodegroup = nodegroup;
	pdata->num_nodegroup = ARRAY_SIZE(nodegroup);

	pdata->pathtype = (char **)&itmon_pathtype[0];
	pdata->num_pathtype = ARRAY_SIZE(itmon_pathtype);

	pdata->errcode = (char **)&itmon_errcode[0];
	pdata->num_errcode = ARRAY_SIZE(itmon_errcode);

	pdata->node_string = (char **)&itmon_node_string[0];
	pdata->num_node_string = ARRAY_SIZE(itmon_node_string);

	pdata->cpu_node_string = (char **)&itmon_cpu_node_string[0];
	pdata->num_cpu_node_string = ARRAY_SIZE(itmon_cpu_node_string);

	pdata->invalid_addr = (unsigned int *)&itmon_invalid_addr[0];
	pdata->num_invalid_addr = ARRAY_SIZE(itmon_invalid_addr);

	pdata->cpu_parsing = true;

	return 0;
}

int itmon_init_keep(struct itmon_dev *itmon)
{
	struct itmon_keepdata *kdata = itmon->kdata;

	kdata->magic = 0xBEEFBEEF;
	kdata->base = 0xEFFEF000;

	return 0;
}
