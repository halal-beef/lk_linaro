LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ifneq ($(CONFIG_EXYNOS_DSS_VERSION),)
MODULE_SRCS += \
	$(LOCAL_DIR)/dss_v$(CONFIG_EXYNOS_DSS_VERSION).c
else
MODULE_SRCS += \
	$(LOCAL_DIR)/dss.c
endif

MODULE_SRCS += \
	$(LOCAL_DIR)/dfd.c \
	$(LOCAL_DIR)/dss_store_ramdump.c

include make/module.mk
