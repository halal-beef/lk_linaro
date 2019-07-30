#ifndef __FLEXPMU_DBG_H__
#define __FLEXPMU_DBG_H__

#define EXYNOS9630_ECT_BASE			(0x90000000)
#define EXYNOS9630_ECT_SIZE			(0x14000)
#define EXYNOS9630_DUMP_ACPM_BASE		(EXYNOS9630_ECT_BASE + EXYNOS9630_ECT_SIZE)

#define EXYNOS9630_DUMP_FLEXPMU_OFFSET		(0xC000)
#define EXYNOS9630_DUMP_FULLSWPMU_OFFSET	(0x12C00)
#define EXYNOS9630_DUMP_FLEXPMU_BASE		(EXYNOS9630_DUMP_ACPM_BASE + EXYNOS9630_DUMP_FLEXPMU_OFFSET)
#define EXYNOS9630_DUMP_FULLSWPMU_BASE		(EXYNOS9630_DUMP_ACPM_BASE + EXYNOS9630_DUMP_FULLSWPMU_OFFSET)

#define EXYNOS9630_FLEXPMU_DBG_BASE		(EXYNOS9630_DUMP_FLEXPMU_BASE + 0x6800)
#define EXYNOS9630_PMUDBG_BASE			(EXYNOS9630_DUMP_FULLSWPMU_BASE + 0xae80)

#define EXYNOS9630_DUMP_GPR_OFFSET		(0x9700)
#define EXYNOS9630_DUMP_GPR_BASE		(EXYNOS9630_DUMP_ACPM_BASE + EXYNOS9630_DUMP_GPR_OFFSET)

#define FLEXPMU_DBG_ENTRY_SIZE			(16)

#define FLEXPMU_DBG(_name) { .name = _name, }

#define FLEXPMU_DBG_LOG				"FLEXPMU-DBG: "
#define APSOC_RUNNING				(0x214E5552)    // "RUN!"

#define PORESET					(0x80000000)

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

#endif /*__FLEXPMU_DBG_H__*/

