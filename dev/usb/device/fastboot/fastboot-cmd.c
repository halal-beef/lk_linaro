/*
 * (C) Copyright 2018 SAMSUNG Electronics
 * Kyounghye Yun <k-hye.yun@samsung.com>
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted
 * transcribed, stored in a retrieval system or translated into any human or computer language in an
 * form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 *
 */

#include <debug.h>
#include <trace.h>
#include <reg.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <lib/console.h>
#include <lib/sysparam.h>
#include <lib/font_display.h>
#include <pit.h>
#include <platform/sfr.h>
#include <platform/smc.h>
#include <platform/lock.h>
#include <platform/ab_update.h>
#include <platform/environment.h>
#include <platform/if_pmic_s2mu004.h>
#include <platform/dfd.h>
#include <dev/usb/fastboot.h>
#include <dev/boot.h>
#include <dev/rpmb.h>
#include <dev/scsi.h>

#include "usb-def.h"

extern void fastboot_send_info(char *response, unsigned int len);
extern void fastboot_send_status(char *response, unsigned int len, int sync);
extern void fastboot_set_payload_data(int dir, void *buf, unsigned int len);
extern void fasboot_set_rx_sz(unsigned int prot_req_sz);

#define FB_RESPONSE_BUFFER_SIZE 128
#define LOCAL_TRACE 0

unsigned int download_size = 0;
unsigned int downloaded_data_size;
static unsigned int is_ramdump = 0;

/* cmd_fastboot_interface	in fastboot.h	*/
struct cmd_fastboot_interface interface =
{
	.rx_handler            = rx_handler,
	.reset_handler         = NULL,
	.product_name          = NULL,
	.serial_no             = NULL,
	.nand_block_size       = CFG_FASTBOOT_PAGESIZE * 64,
	.transfer_buffer       = (unsigned char *)CFG_FASTBOOT_TRANSFER_BUFFER,
	.transfer_buffer_size  = CFG_FASTBOOT_TRANSFER_BUFFER_SIZE,
};

struct cmd_fastboot {
	const char *cmd_name;
	int (*handler)(const char *, unsigned int);
};

__attribute__((weak)) void platform_prepare_reboot(void)
{
	/* WARNING : NOT MODIFY THIS FUNCTION
	 * If you need to sw reset by reboot command,
	 * please implementate on the platform.
	 */
}

__attribute__((weak)) void platform_do_reboot(const char *cmd_buf)
{
	/* WARNING : NOT MODIFY THIS FUNCTION
	 * If you need to sw reset by reboot command,
	 * please implementate on the platform.
	 */
	return;
}

__attribute__((weak)) const char * fastboot_get_product_string(void)
{
	/* WARNING : NOT MODIFY THIS FUNCTION
	 * If you need to get product string address
	 * please implementate on the platform.
	 */
	return NULL;
}

__attribute__((weak)) const char * fastboot_get_serialno_string(void)
{
	/* WARNING : NOT MODIFY THIS FUNCTION
	 * If you need to get product string address
	 * please implementate on the platform.
	 */
	return NULL;
}

__attribute__((weak)) struct cmd_fastboot_variable *fastboot_get_var_head(void)
{
	/* WARNING : NOT MODIFY THIS FUNCTION
	 * If you need to get product string address
	 * please implementate on the platform.
	 */
	return NULL;
}

__attribute__((weak)) int fastboot_get_var_num(void)
{
	/* WARNING : NOT MODIFY THIS FUNCTION
	 * If you need to get product string address
	 * please implementate on the platform.
	 */
	return -1;
}

__attribute__((weak)) int init_fastboot_variables(void)
{
	/* WARNING : NOT MODIFY THIS FUNCTION
	 * If you need to get product string address
	 * please implementate on the platform.
	 */
	return -1;
}

int fb_do_getvar(const char *cmd_buffer, unsigned int rx_sz)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);
	const char *tmp;

	LTRACEF("fast received cmd:%s\n", cmd_buffer);

	sprintf(response,"OKAY");

	if (!memcmp(cmd_buffer + 7, "version", strlen("version")))
	{
		LTRACEF("fast cmd:version\n");
		sprintf(response + 4, FASTBOOT_VERSION);
	}
	else if (!memcmp(cmd_buffer + 7, "product", strlen("product")))
	{
		LTRACEF("fast cmd:product name\n");
		tmp = fastboot_get_product_string();
		if (tmp)
			sprintf(response + 4, tmp);
	}
	else if (!memcmp(cmd_buffer + 7, "serialno", strlen("serialno")))
	{
		LTRACEF("fast cmd:serial NO\n");
		tmp = fastboot_get_serialno_string();
		if (tmp)
			sprintf(response + 4, tmp);
	}
	else if (!memcmp(cmd_buffer + 7, "downloadsize", strlen("downloadsize")))
	{
		LTRACEF("fast cmd:download sz\n");
		if (interface.transfer_buffer_size)
			sprintf(response + 4, "%08x", interface.transfer_buffer_size);
	}
	else if (!memcmp(cmd_buffer + 7, "max-download-size", strlen("max-download-size")))
	{
		LTRACEF("fast cmd:max-download-size\n");
		if (interface.transfer_buffer_size)
			sprintf(response + 4, "%d", interface.transfer_buffer_size);
	}
	else if (!memcmp(cmd_buffer + 7, "partition-type", strlen("partition-type")))
	{
		char *key;
		struct pit_entry *ptn;

		LTRACEF("fast cmd:partition-type\n");

		key = (char *)cmd_buffer + 7 + strlen("partition-type:");
		ptn = pit_get_part_info(key);

		if (ptn == 0)
		{
			sprintf(response, "FAILpartition does not exist");
			fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);
			return 0;
		}
		if (ptn->filesys == FS_TYPE_EXT4)
			strcpy(response + 4, "ext4");
		/*
		 * In case of flashing pit, this should be
		 * passed unconditionally.
		 */
		if (strcmp(key, "pit") && ptn->filesys != FS_TYPE_NONE) {
			if (!strcmp(key, "userdata"))
				strcpy(response + 4, "f2fs");
			else
				strcpy(response + 4, "ext4");
		}
	}
	else if (!memcmp(cmd_buffer + 7, "partition-size", strlen("partition-size")))
	{
		char *key;
		struct pit_entry *ptn;

		LTRACEF("fast cmd:partition-size\n");

		key = (char *)cmd_buffer + 7 + strlen("partition-size:");
		ptn = pit_get_part_info(key);
		/*
		 * In case of flashing pit, this location
		 * would not be passed. So it's unnecessary
		 * to check that this case is pit.
		 */
		if (ptn && ptn->filesys != FS_TYPE_NONE)
			sprintf(response + 4, "0x%llx", pit_get_length(ptn));
	}
	else if (!memcmp(cmd_buffer + 7, "slot-count", strlen("slot-count")))
	{
		LTRACEF("fast cmd:get slot-count\n");
		sprintf(response + 4, "2");
	}
	else if (!memcmp(cmd_buffer + 7, "current-slot", strlen("current-slot")))
	{
		LTRACEF("fast cmd:current-slot\n");
		if (ab_current_slot())
			sprintf(response + 4, "_b");
		else
			sprintf(response + 4, "_a");
	}
	else if (!memcmp(cmd_buffer + 7, "slot-successful", strlen("slot-successful")))
	{
		int slot = -1;

		LTRACEF("fast cmd:slot-successful\n");
		if (!strcmp(cmd_buffer + 7 + strlen("slot-successful:"), "_a"))
			slot = 0;
		else if (!strcmp(cmd_buffer + 7 + strlen("slot-successful:"), "_b"))
			slot = 1;
		else
			sprintf(response, "FAILinvalid slot");
		LTRACEF("slot: %d\n", slot);
		if (slot >= 0) {
			if (ab_slot_successful(slot))
				sprintf(response + 4, "yes");
			else
				sprintf(response + 4, "no");
		}
	}
	else if (!memcmp(cmd_buffer + 7, "slot-unbootable", strlen("slot-unbootable")))
	{
		int slot = -1;

		LTRACEF("fast cmd:slot-unbootable\n");
		if (!strcmp(cmd_buffer + 7 + strlen("slot-unbootable:"), "_a"))
			slot = 0;
		else if (!strcmp(cmd_buffer + 7 + strlen("slot-unbootable:"), "_b"))
			slot = 1;
		else
			sprintf(response, "FAILinvalid slot");
		if (slot >= 0) {
			if (ab_slot_unbootable(slot))
				sprintf(response + 4, "yes");
			else
				sprintf(response + 4, "no");
		}
	}
	else if (!memcmp(cmd_buffer + 7, "slot-retry-count", strlen("slot-retry-count")))
	{
		int slot = -1;

		LTRACEF("fast cmd:slot-retry-count\n");
		if (!strcmp(cmd_buffer + 7 + strlen("slot-retry-count:"), "_a"))
			slot = 0;
		else if (!strcmp(cmd_buffer + 7 + strlen("slot-retry-count:"), "_b"))
			slot = 1;
		else
			sprintf(response, "FAILinvalid slot");
		if (slot >= 0)
			sprintf(response + 4, "%d", ab_slot_retry_count(slot));
		}
	else if (!memcmp(cmd_buffer + 7, "has-slot", strlen("has-slot")))
	{
		LTRACEF("fast cmd:has-slot\n");
		if (!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "boot") ||
			!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "dtb") ||
			!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "dtbo") ||
			!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "system") ||
			!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "vendor"))
			sprintf(response + 4, "yes");
		else
			sprintf(response + 4, "no");
	}
	else if (!memcmp(cmd_buffer + 7, "all", strlen("all")))
	{
		int i, var_cnt;
		struct cmd_fastboot_variable *var_head;

		var_head = fastboot_get_var_head();
		var_cnt = fastboot_get_var_num();

		if (var_cnt == -1 || var_head == NULL) {
			strcpy(response,"FAIL");
		} else {
			for (i = 0; i < var_cnt; i++) {
				strncpy(response,"INFO", 4);
				strncpy(response + 4, var_head[i].name, strlen(var_head[i].name));
				strncpy(response + 4 + strlen(var_head[i].name), ":", 1);
				strcpy(response + 4 + strlen(var_head[i].name) + 1, var_head[i].string);
				fastboot_send_info(response, strlen(response));
			}

			strcpy(response,"OKAY");
			strcpy(response + 4,"Done!");
		}
	}
	else
	{
		LTRACEF("fast cmd:vendor\n");
		debug_snapshot_getvar_item(cmd_buffer + 7, response + 4);
	}

	fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);

	return 0;
}

int fb_do_erase(const char *cmd_buffer, unsigned int rx_sz)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);
	char *key = (char *)cmd_buffer + 6;
	struct pit_entry *ptn = pit_get_part_info(key);
	int status = 1;


	if (strcmp(key, "pit") && ptn == 0)
	{
		sprintf(response, "FAILpartition does not exist");
		fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);
		return 0;
	}

	printf("erasing(formatting) '%s'\n", ptn->name);

	if (ptn->filesys != FS_TYPE_NONE)
		status = pit_access(ptn, PIT_OP_ERASE, 0, 0);

	if (status) {
		sprintf(response,"FAILfailed to erase partition");
	} else {
		printf("partition '%s' erased\n", ptn->name);
		sprintf(response, "OKAY");
	}

	fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);

	return 0;
}

static void flash_using_pit(char *key, char *response,
		u32 size, void *addr)
{
	struct pit_entry *ptn;
	unsigned long long length;
	u32 *env_val;

	/*
	 * In case of flashing pit, this should be
	 * passed unconditionally.
	 */
	if (!strcmp(key, "pit")) {
		pit_update(addr, size);
		print_lcd_update(FONT_GREEN, FONT_BLACK, "partition 'pit' flashed");
		sprintf(response, "OKAY");
		return;
	}

	ptn = pit_get_part_info(key);
	if (ptn)
		length = pit_get_length(ptn);

	if (ptn == 0) {
		sprintf(response, "FAILpartition does not exist");
	} else if ((downloaded_data_size > length) && (length != 0)) {
		sprintf(response, "FAILimage too large for partition");
	} else {
		if ((ptn->blknum != 0) && (downloaded_data_size > length)) {
			printf("flashing '%s' failed\n", ptn->name);
			print_lcd_update(FONT_RED, FONT_BLACK, "flashing '%s' failed", ptn->name);
			sprintf(response, "FAILfailed to too large image");
		} else if (pit_access(ptn, PIT_OP_FLASH, (u64)addr, size)) {
			printf("flashing '%s' failed\n", ptn->name);
			print_lcd_update(FONT_RED, FONT_BLACK, "flashing '%s' failed", ptn->name);
			sprintf(response, "FAILfailed to flash partition");
		} else {
			printf("partition '%s' flashed\n\n", ptn->name);
			print_lcd_update(FONT_GREEN, FONT_BLACK, "partition '%s' flashed", ptn->name);
			sprintf(response, "OKAY");
		}
	}

	if (!strcmp(key, "ramdisk")) {
		ptn = pit_get_part_info("env");
		env_val = memalign(0x1000, pit_get_length(ptn));
		pit_access(ptn, PIT_OP_LOAD, (u64)env_val, 0);

		env_val[ENV_ID_RAMDISK_SIZE] = size;
		pit_access(ptn, PIT_OP_FLASH, (u64)env_val, 0);

		free(env_val);
	}
}

int fb_do_flash(const char *cmd_buffer, unsigned int rx_sz)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);

	LTRACE_ENTRY;

#if defined(CONFIG_USE_RPMB)
	if(is_first_boot()) {
		uint32_t lock_state;
		rpmb_get_lock_state(&lock_state);
		LTRACEF_LEVEL(INFO, "Lock state: %d\n", lock_state);
		if(lock_state) {
			sprintf(response, "FAILDevice is locked");
			fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);
			return 1;
		}
	}
#endif
	dprintf(ALWAYS, "flash\n");

	flash_using_pit((char *)cmd_buffer + 6, response,
			downloaded_data_size, (void *)interface.transfer_buffer);

	strcpy(response,"OKAY");
	fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);

	LTRACE_EXIT;
	return 0;
}

extern void fastboot_rx_datapayload(int dir, const unsigned char *addr, unsigned int len);

int fb_do_reboot(const char *cmd_buffer, unsigned int rx_sz)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);

	platform_prepare_reboot();

	sprintf(response,"OKAY");
	fastboot_send_status(response, strlen(response), FASTBOOT_TX_SYNC);

	platform_do_reboot(cmd_buffer);

	return 0;
}

int fb_do_download(const char *cmd_buffer, unsigned int rx_sz)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);

	LTRACE_ENTRY;
	LTRACEF_LEVEL(INFO, "%s---->>>>>  cmd: %s\n", __func__, cmd_buffer);
	/* Get download size */
	download_size = (unsigned int)strtol(cmd_buffer + 9, NULL, 16);
	downloaded_data_size = 0;

	LTRACEF_LEVEL(INFO, "Downloaing. Download size is [%d, %d] bytes\n", download_size, interface.transfer_buffer_size);

	/* Set payload phase to rx data */
	fastboot_set_payload_data(USBDIR_OUT, (void *)CFG_FASTBOOT_TRANSFER_BUFFER, download_size);

	sprintf(response, "DATA%08x", download_size);
	LTRACEF_LEVEL(INFO, "response: %s\n", response);
	fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);

	LTRACE_EXIT;
	return 0;
}

static void start_ramdump(void *buffer)
{
	struct fastboot_ramdump_hdr *hdr = buffer;
	static uint32_t ramdump_cnt = 0;
	char buf[] = "OKAY";

	LTRACEF_LEVEL(INFO, "\nramdump start address is [0x%lx]\n", hdr->base);
	LTRACEF_LEVEL(INFO, "ramdump size is [0x%lx]\n", hdr->size);
	LTRACEF_LEVEL(INFO, "version is [0x%lx]\n", hdr->version);

	if (hdr->version != 2) {
		LTRACEF_LEVEL(INFO, "you are using wrong version of fastboot!!!\n");
	}

	/* dont't generate DECERR even if permission failure of ASP occurs */
	if (ramdump_cnt++ == 0)
		set_tzasc_action(0);

	fastboot_set_payload_data(USBDIR_IN, (void *)hdr->base, hdr->size);
	fastboot_send_status(buf, strlen(buf), FASTBOOT_TX_SYNC);
}

int fb_do_ramdump(const char *cmd_buffer, unsigned int rx_sz)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);

	TRACE_ENTRY;

	print_lcd_update(FONT_GREEN, FONT_BLACK, "Got ramdump command.");
	is_ramdump = 1;
	/* Get download size */
	download_size = (unsigned int)strtol(cmd_buffer + 8, NULL, 16);
	downloaded_data_size = 0;

	LTRACEF_LEVEL(INFO, "Downloaing. Download size is %d bytes\n", download_size);

	if (download_size > interface.transfer_buffer_size) {
		download_size = 0;
		sprintf(response, "FAILdownload data size is bigger than buffer size");
	} else {
		sprintf(response, "DATA%08x", download_size);
	}
	fasboot_set_rx_sz(download_size);
	fastboot_send_status(response, strlen(response), FASTBOOT_TX_SYNC);

	LTRACE_EXIT;

	return 0;
}

int fb_do_set_active(const char *cmd_buffer, unsigned int rx_sz)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);

	LTRACEF_LEVEL(INFO, "set_active\n");

	sprintf(response,"OKAY");
	if (!strcmp(cmd_buffer + 11, "a")) {
		printf("Set slot 'a' active.\n");
		print_lcd_update(FONT_GREEN, FONT_BLACK, "Set slot 'a' active.");
		ab_set_active(0);
	} else if (!strcmp(cmd_buffer + 11, "b")) {
		printf("Set slot 'b' active.\n");
		print_lcd_update(FONT_GREEN, FONT_BLACK, "Set slot 'b' active.");
		ab_set_active(1);
	} else {
		sprintf(response, "FAILinvalid slot");
	}

	fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);

	return 0;
}

/* Lock/unlock device */
int fb_do_flashing(const char *cmd_buffer, unsigned int rx_sz)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);

	sprintf(response,"OKAY");
	if (!strcmp(cmd_buffer + 9, "lock")) {
		printf("Lock this device.\n");
		print_lcd_update(FONT_GREEN, FONT_BLACK, "Lock this device.");
		if(rpmb_set_lock_state(1))
			sprintf(response, "FAILRPBM error: failed to change lock state on RPMB");
	} else if (!strcmp(cmd_buffer + 9, "unlock")) {
		if (get_unlock_ability()) {
			printf("Unlock this device.\n");
			print_lcd_update(FONT_GREEN, FONT_BLACK, "Unlock this device.");
			if(rpmb_set_lock_state(0))
				sprintf(response, "FAILRPBM error: failed to change lock state on RPMB");
		} else {
			sprintf(response, "FAILunlock_ability is 0");
		}
	} else if (!strcmp(cmd_buffer + 9, "lock_critical")) {
		printf("Lock critical partitions of this device.\n");
		print_lcd_update(FONT_GREEN, FONT_BLACK, "Lock critical partitions of this device.");
		lock_critical(0);
	} else if (!strcmp(cmd_buffer + 9, "unlock_critical")) {
		printf("Unlock critical partitions of this device.\n");
		print_lcd_update(FONT_GREEN, FONT_BLACK, "Unlock critical partitions of this device.");
		lock_critical(0);
	} else if (!strcmp(cmd_buffer + 9, "get_unlock_ability")) {
		printf("Get unlock_ability.\n");
		print_lcd_update(FONT_GREEN, FONT_BLACK, "Get unlock_ability.");
		sprintf(response + 4, "%d", get_unlock_ability());
	} else {
		sprintf(response, "FAILunsupported command");
	}

	fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);

	return 0;
}

int fb_do_oem(const char *cmd_buffer, unsigned int rx_sz)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);
	unsigned int env_val = 0;
	ssize_t param_sz;

	if (!strncmp(cmd_buffer + 4, "trackid write", 13)) {
		struct pit_entry *ptn;
		char *proinfo;

		ptn = pit_get_part_info("proinfo");
		proinfo = memalign(0x1000, pit_get_length(ptn));
		pit_access(ptn, PIT_OP_LOAD, (u64)proinfo, 0);
		memcpy(proinfo + 21, cmd_buffer + 18, 10);
		pit_access(ptn, PIT_OP_FLASH, (u64)proinfo, 0);

		sprintf(response, "OKAY");
		fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);

		free(proinfo);
	} else if (!strncmp(cmd_buffer + 4, "trackid read", 13)) {
		struct pit_entry *ptn;
		char *proinfo;

		ptn = pit_get_part_info("proinfo");
		proinfo = memalign(0x1000, pit_get_length(ptn));
		pit_access(ptn, PIT_OP_LOAD, (u64)proinfo, 0);

		sprintf(response, "INFO");
		memcpy(response + 4, proinfo + 21, 10);
		*(response + 14) = 0;
		fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);

		sprintf(response, "OKAY");
		fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);

		free(proinfo);

	} else if (!strncmp(cmd_buffer + 4, "uart_log_enable", 15)) {
		/* Check value on the sysparam */
		param_sz = sysparam_read("uart_log_enable", &env_val, sizeof(env_val));
		if (param_sz > 0)
			sysparam_remove("uart_log_enable");
		env_val = UART_LOG_MODE_FLAG;
		sysparam_add("uart_log_enable", &env_val, sizeof(env_val));
		sysparam_write();

		sprintf(response, "OKAY");
		fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else if (!strncmp(cmd_buffer + 4, "uart_log_disable", 16)) {
		param_sz = sysparam_read("uart_log_enable", &env_val, sizeof(env_val));
		if (param_sz > 0) {
			sysparam_remove("uart_log_enable");
			sysparam_write();
		}

		sprintf(response, "OKAY");
		fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else if (!strncmp(cmd_buffer + 4, "fb_mode_set", 11)) {
		param_sz = sysparam_read("fb_mode_set", &env_val, sizeof(env_val));
		if (param_sz > 0)
			sysparam_remove("fb_mode_set");
		env_val = UART_LOG_MODE_FLAG;
		sysparam_add("fb_mode_set", &env_val, sizeof(env_val));
		sysparam_write();

		sprintf(response, "OKAY");
		fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else if (!strncmp(cmd_buffer + 4, "fb_mode_clear", 13)) {
		param_sz = sysparam_read("fb_mode_set", &env_val, sizeof(env_val));
		if (param_sz > 0) {
			sysparam_remove("fb_mode_set");
			sysparam_write();
		}

		sprintf(response, "OKAY");
		fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else if (!strncmp(cmd_buffer + 4, "xct-on", 6)) {
		char xct_on[8] = { 0, };

		param_sz = sysparam_read("xct", xct_on, 8);
		if (param_sz > 0)
			sysparam_remove("xct");
		sprintf(xct_on, "xct-on");
		sysparam_add("xct", xct_on, strlen(xct_on));
		sysparam_write();

		sprintf(response, "OKAY");
		fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else if (!strncmp(cmd_buffer + 4, "xct-off", 7)) {
		char xct_on[8] = { 0, };

		param_sz = sysparam_read("xct", xct_on, 8);
		if (param_sz > 0) {
			sysparam_remove("xct");
			sysparam_write();
		}
		fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else if (!strncmp(cmd_buffer + 4, "boot_wait_on", 12)) {
		int wait_max = 0;

		param_sz = sysparam_read("boot_wait", &wait_max, sizeof(int));
		if (param_sz > 0)
			sysparam_remove("boot_wait");
		wait_max = 3;
		sysparam_add("boot_wait", &wait_max, sizeof(int));
		sysparam_write();

		sprintf(response, "OKAY");
		fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else if (!strncmp(cmd_buffer + 4, "boot_wait_off", 13)) {
		int wait_max;

		param_sz = sysparam_read("boot_wait", &wait_max, sizeof(int));
		if (param_sz > 0) {
			sysparam_remove("boot_wait");
			sysparam_write();
		}
		fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else {
		printf("Unsupported oem command!\n");
		sprintf(response, "FAILunsupported command");
		fastboot_send_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	}

	return 0;
}

struct cmd_fastboot cmd_list[] = {
	{"reboot", fb_do_reboot},
	{"flash:", fb_do_flash},
	{"erase:", fb_do_erase},
	{"download:", fb_do_download},
	{"ramdump:", fb_do_ramdump},
	{"getvar:", fb_do_getvar},
	{"set_active:", fb_do_set_active},
	{"flashing", fb_do_flashing},
	{"oem", fb_do_oem},
};

int rx_handler(const unsigned char *buffer, unsigned int buffer_size)
{
	unsigned int i;
	const char *cmd_buffer;

	LTRACE_ENTRY;

	cmd_buffer = (char *)buffer;

#ifdef PIT_DEBUG
	LTRACEF_LEVEL(INFO, "fb %s\n", cmd_buffer);	/* test: probing protocol */
#endif
	if (is_ramdump) {
		is_ramdump = 0;

		start_ramdump((void *)buffer);
	} else {
		for (i = 0; i < (sizeof(cmd_list) / sizeof(struct cmd_fastboot)); i++) {
			if(!memcmp(cmd_buffer, cmd_list[i].cmd_name, strlen(cmd_list[i].cmd_name)))
				cmd_list[i].handler(cmd_buffer, buffer_size);
		}
	}

	LTRACE_EXIT;
	return 0;
}

void fb_cmd_set_downloaded_sz(unsigned int sz)
{
	downloaded_data_size += sz;
}
