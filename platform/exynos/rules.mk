LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := ${LOCAL_DIR}

MODULE_SRCS += \
	$(LOCAL_DIR)/boot_info.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/uart/uart.c \
	$(LOCAL_DIR)/timer/delay.c \
	$(LOCAL_DIR)/fdt_reconfiguration.c

include make/module.mk
