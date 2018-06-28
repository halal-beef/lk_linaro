#include <debug.h>
#include <reg.h>
#include <app.h>
#include <lib/console.h>
#include <platform/sfr.h>
#include <platform/charger.h>
#include <platform/fastboot.h>
#include <platform/dfd.h>
#include <dev/boot.h>

int cmd_boot(int argc, const cmd_args *argv);

static void exynos_boot_task(const struct app_descriptor *app, void *args)
{
	unsigned int rst_stat = readl(EXYNOS9610_POWER_RST_STAT);
	int cpu;

	printf("RST_STAT: 0x%x\n", rst_stat);

	printf("PMUDBG_CL0_*: ");
	for (cpu = LITTLE_CORE_START; cpu <= LITTLE_CORE_LAST; cpu++)
		printf("cpu%d: 0x%x ", cpu, dfd_get_pmudbg_stat(cpu));
	printf("\n");
	printf("PMUDBG_CL1_*: ");
	for (cpu = BIG_CORE_START; cpu <= BIG_CORE_LAST; cpu++)
		printf("cpu%d: 0x%x ", cpu-BIG_CORE_START, dfd_get_pmudbg_stat(cpu));
	printf("\n");

	if (*(unsigned int *)DRAM_BASE != 0xabcdef) {
		printf("Running on DRAM by TRACE32: skip auto booting\n");
		return;
	}

	if (!is_first_boot() || (rst_stat & (WARM_RESET | LITTLE_WDT_RESET | BIG_WDT_RESET)) ||
		((readl(CONFIG_RAMDUMP_SCRATCH) == CONFIG_RAMDUMP_MODE) && get_charger_mode() == 0)) {
		for (cpu = LITTLE_CORE_START; cpu <= BIG_CORE_LAST; cpu++)
			dfd_print_pcval(cpu);
		dfd_run_dump_gpr();
		do_fastboot(0, 0);
	} else {
		/* Turn on dumpEN for DumpGPR */
		dfd_set_dump_gpr(CACHE_RESET_EN | DUMPGPR_EN);

		cmd_boot(0, 0);
	}

	return;
}

APP_START(exynos_boot)
	.entry = exynos_boot_task,
	.flags = 0,
APP_END
