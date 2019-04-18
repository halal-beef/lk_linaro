LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/sysparam

GLOBAL_DEFINES += \
	SYSPARAM_ALLOW_WRITE=1

MODULE_SRCS += \
	$(LOCAL_DIR)/pit.c \
	$(LOCAL_DIR)/part_gpt.c \
	$(LOCAL_DIR)/guid.c

include make/module.mk
