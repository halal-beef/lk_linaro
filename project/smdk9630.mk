ARCH := arm64
ARM_CPU := cortex-a53
TARGET := smdk9630

WITH_KERNEL_VM := 0
WITH_LINKER_GC := 1

MODULES += \
	app/exynos_boot \
	dev/acpm_ipc

GLOBAL_DEFINES += \
	INPUT_GPT_AS_PT=1 \
	FIRST_GPT_VERIFY=0
export INPUT_GPT_AS_PT=1
