ARCH := arm64
ARM_CPU := cortex-a53
TARGET := erd9630

WITH_KERNEL_VM := 0
WITH_LINKER_GC := 1

MODULES += \
	app/exynos_boot \
	dev/acpm_ipc
