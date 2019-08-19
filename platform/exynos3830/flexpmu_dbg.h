#ifndef __FLEXPMU_DBG_H__
#define __FLEXPMU_DBG_H__

#define EXYNOS3830_ECT_BASE			(0x90000000)
#define EXYNOS3830_ECT_SIZE			(0xC800)
#define EXYNOS3830_DUMP_ACPM_BASE		(EXYNOS3830_ECT_BASE + EXYNOS3830_ECT_SIZE)

#define EXYNOS3830_DUMP_FLEXPMU_OFFSET		(0xC000)
#define EXYNOS3830_DUMP_FULLSWPMU_OFFSET	(0x11800)
#define EXYNOS3830_DUMP_FLEXPMU_BASE		(EXYNOS3830_DUMP_ACPM_BASE + EXYNOS3830_DUMP_FLEXPMU_OFFSET)
#define EXYNOS3830_DUMP_FULLSWPMU_BASE		(EXYNOS3830_DUMP_ACPM_BASE + EXYNOS3830_DUMP_FULLSWPMU_OFFSET)

#define EXYNOS3830_FLEXPMU_DBG_BASE		(EXYNOS3830_DUMP_FLEXPMU_BASE + 0x5400)
#define EXYNOS3830_PMUDBG_BASE			(EXYNOS3830_DUMP_FULLSWPMU_BASE + 0x8e80)

#define EXYNOS3830_DUMP_GPR_OFFSET		(0x8700)
#define EXYNOS3830_DUMP_GPR_BASE		(EXYNOS3830_DUMP_ACPM_BASE + EXYNOS3830_DUMP_GPR_OFFSET)

#define FLEXPMU_DBG_ENTRY_SIZE			(16)

#define FLEXPMU_DBG(_name) { .name = _name, }

#define FLEXPMU_DBG_LOG				"FLEXPMU-DBG: "
#define APSOC_RUNNING				(0x214E5552)    // "RUN!"

#define PORESET					(0x40000000)

struct dbg_list {
	const char *name;
	union {
		u32 u32_data[2];
		u16 u16_data[4];
		u8 u8_data[8];
	};
};

enum sequence_index {
	SEQ_DONE,
	SEQ_CPU0,
	SEQ_CPU1,
	SEQ_CPU2,
	SEQ_CPU3,
	SEQ_CPU4,
	SEQ_CPU5,
	SEQ_CPU6,
	SEQ_CPU7,
	SEQ_NON_CPU_CL0,
	SEQ_NON_CPU_CL1,
	SEQ_NON_CPU_CL2,
	SEQ_L3FLUSH_START,
	SEQ_L3FLUSH_ABORT,
	SEQ_SOC_SEQ,
	SEQ_MIF_SEQ,
	SEQ_CHUB_UP,
	SEQ_L2FLUSH_START,
	SEQ_L2FLUSH_ABORT,
};

void display_flexpmu_dbg(void);

// #define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#endif /*__FLEXPMU_DBG_H__*/

