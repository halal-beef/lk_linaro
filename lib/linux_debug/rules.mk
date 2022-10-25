LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/linux_debug.c \
	$(LOCAL_DIR)/kallsyms.c \
	$(LOCAL_DIR)/module.c

include make/module.mk
