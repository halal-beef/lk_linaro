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
#include <reg.h>
#include <string.h>
#include <stdlib.h>
#include <err.h>
#include <lib/console.h>
#include <lib/sysparam.h>
#include <lib/font_display.h>
#include "fastboot.h"
#include <pit.h>
#include <platform/sfr.h>
#include <platform/smc.h>
#include <platform/lock.h>
#include <platform/ab_update.h>
#include <platform/environment.h>
#include <platform/pmic_s2mpu09.h>
#include <platform/if_pmic_s2mu004.h>
#include <platform/dfd.h>
#include <platform/chip_id.h>
#include <platform/mmu/mmu_func.h>
#include <platform/debug-store-ramdump.h>
#include <platform/fastboot.h>
#include <dev/boot.h>
#include <dev/rpmb.h>
#include <dev/scsi.h>

#define FB_RESPONSE_BUFFER_SIZE 128

unsigned int download_size;
unsigned int downloaded_data_size;
int extention_flag;
static unsigned int is_ramdump = 0;
extern int is_attached;
static int rx_handler (const unsigned char *buffer, unsigned int buffer_size);
int fastboot_tx_mem(u64 buffer, u64 buffer_size);

/* cmd_fastboot_interface	in fastboot.h	*/
struct cmd_fastboot_interface interface =
{
	.rx_handler            = rx_handler,
	.reset_handler         = NULL,
	.product_name          = NULL,
	.serial_no             = NULL,
	.nand_block_size       = 0,
	.transfer_buffer       = (unsigned char *)0xffffffff,
	.transfer_buffer_size  = 0,
};

struct cmd_fastboot {
	const char *cmd_name;
	int (*handler)(const char *);
};

#define CMD_FASTBOOT_MAX_VAR_NR 64
#define CMD_FASTBOOT_MAX_VAR_LEN 64
struct cmd_fastboot_variable {
	char name[CMD_FASTBOOT_MAX_VAR_LEN];
	char string[CMD_FASTBOOT_MAX_VAR_LEN];
};

static struct cmd_fastboot_variable fastboot_var_list[CMD_FASTBOOT_MAX_VAR_NR];
static int fastboot_var_nr = 0;

static int add_fastboot_variable(const char *name, const char *string)
{
	int name_len;
	int string_len;

	if (name != NULL) {
		name_len = strlen(name);
	} else {
		printf("Input string is null\n");
		return -1;
	}

	if (string != NULL) {
		string_len = strlen(string);
	} else {
		printf("Input string is null\n");
		return -1;
	}

	if (name_len < CMD_FASTBOOT_MAX_VAR_LEN) {
		strncpy((void *)&fastboot_var_list[fastboot_var_nr].name, name, name_len);
	} else {
		printf("Input string size is bigger than buffer size\n");
		return -1;
	}

	if (name_len < CMD_FASTBOOT_MAX_VAR_LEN) {
		strncpy((void *)&fastboot_var_list[fastboot_var_nr].string, string, string_len);
	} else {
		printf("Input string size is bigger than buffer size\n");
		return -1;
	}

	fastboot_var_nr++;

	return 0;
}

static int get_fastboot_variable(char *name, char *buf)
{
	int i;

	for (i = 0; i < CMD_FASTBOOT_MAX_VAR_NR; i++) {
		if (!strcmp(name, fastboot_var_list[i].name)) {
			strcpy(buf, fastboot_var_list[i].string);
			break;
		}
	}

	if (i == CMD_FASTBOOT_MAX_VAR_NR)
		return -1;
	else
		return 0;
}

u8 serial_id[16] = {0};	/* string for chip id */

static void simple_hextostr(u32 hex, u8 *str)
{
	u8 i;

	for (i = 0; i < 8; i++) {
		if ((hex & 0xF) > 9)
			*str++ = 'a' + (hex & 0xF) - 10;
		else
			*str++ = '0' + (hex & 0xF);

		hex >>= 4;
	}
}

static void set_serial_number(void)
{
	u8 tmp_serial_id[16];	/* string for chip id */
	u8 i;

	simple_hextostr(s5p_chip_id[1], tmp_serial_id+8);
	simple_hextostr(s5p_chip_id[0], tmp_serial_id);

	for (i = 0; i < 12; i++)
		serial_id[11 - i] = tmp_serial_id[i];
}

int init_fastboot_variables(void)
{
	char tmp[64] = {0};
	struct pit_entry *ptn;

	memset(fastboot_var_list, 0, sizeof(struct cmd_fastboot_variable) * CMD_FASTBOOT_MAX_VAR_NR);

	add_fastboot_variable("version-baseband", "N/A");
	add_fastboot_variable("version", FASTBOOT_VERSION);
	add_fastboot_variable("version-bootloader", FASTBOOT_VERSION_BOOTLOADER);
	add_fastboot_variable("product", "maestro9610");
	set_serial_number();
	add_fastboot_variable("serialno", (const char *)serial_id);
	add_fastboot_variable("secure", "yes");
	add_fastboot_variable("unlocked", "yes");
	add_fastboot_variable("off-mode-charge", "0");
	add_fastboot_variable("variant", "maestro9610");
	add_fastboot_variable("battery-voltage", "2700mV");
	add_fastboot_variable("battery-soc-ok", "yes");
	add_fastboot_variable("partition-type:efs", "ext4");
	ptn = pit_get_part_info("efs");
	sprintf(tmp, "0x%llx", pit_get_length(ptn));
	add_fastboot_variable("partition-size:efs", (const char *)tmp);
	add_fastboot_variable("partition-type:efsbk", "ext4");
	ptn = pit_get_part_info("efsbk");
	sprintf(tmp, "0x%llx", pit_get_length(ptn));
	add_fastboot_variable("partition-size:efsbk", (const char *)tmp);
	add_fastboot_variable("partition-type:persist", "ext4");
	ptn = pit_get_part_info("persist");
	sprintf(tmp, "0x%llx", pit_get_length(ptn));
	add_fastboot_variable("partition-size:persist", (const char *)tmp);
	add_fastboot_variable("partition-type:metadata", "ext4");
	ptn = pit_get_part_info("metadata");
	sprintf(tmp, "0x%llx", pit_get_length(ptn));
	add_fastboot_variable("partition-size:metadata", (const char *)tmp);
	add_fastboot_variable("partition-type:system_a", "ext4");
	ptn = pit_get_part_info("system_a");
	sprintf(tmp, "0x%llx", pit_get_length(ptn));
	add_fastboot_variable("partition-size:system_a", (const char *)tmp);
	add_fastboot_variable("partition-type:system_b", "ext4");
	ptn = pit_get_part_info("system_b");
	sprintf(tmp, "0x%llx", pit_get_length(ptn));
	add_fastboot_variable("partition-size:system_b", (const char *)tmp);
	add_fastboot_variable("partition-type:vendor_a", "ext4");
	ptn = pit_get_part_info("vendor_a");
	sprintf(tmp, "0x%llx", pit_get_length(ptn));
	add_fastboot_variable("partition-size:vendor_a", (const char *)tmp);
	add_fastboot_variable("partition-type:vendor_b", "ext4");
	ptn = pit_get_part_info("vendor_b");
	sprintf(tmp, "0x%llx", pit_get_length(ptn));
	add_fastboot_variable("partition-size:vendor_b", (const char *)tmp);
	add_fastboot_variable("partition-type:userdata", "ext4");
	ptn = pit_get_part_info("userdata");
	sprintf(tmp, "0x%llx", pit_get_length(ptn));
	add_fastboot_variable("partition-size:userdata", (const char *)tmp);
	sprintf(tmp, "0x%x", CFG_FASTBOOT_TRANSFER_BUFFER_SIZE);
	add_fastboot_variable("max-download-size", (const char *)tmp);
	sprintf(tmp, "0x%x", 0x1000);
	add_fastboot_variable("erase-block-size", (const char *)tmp);
	add_fastboot_variable("logical-block-size", (const char *)tmp);
	add_fastboot_variable("has-slot:efs", "no");
	add_fastboot_variable("has-slot:efsbk", "no");
	add_fastboot_variable("has-slot:persist", "no");
	add_fastboot_variable("has-slot:metadata", "no");
	add_fastboot_variable("has-slot:system", "yes");
	add_fastboot_variable("has-slot:vendor", "yes");
	add_fastboot_variable("has-slot:userdata", "no");
	add_fastboot_variable("current-slot", "a");
	add_fastboot_variable("slot-count", "2");
	add_fastboot_variable("slot-successful", "a:yes");
	add_fastboot_variable("slot-unbootable", "a:no");
	add_fastboot_variable("slot-retry-count", "a:0");
	add_fastboot_variable("slot-successful", "b:no");
	add_fastboot_variable("slot-unbootable", "b:no");
	add_fastboot_variable("slot-retry-count", "b:7");

	return 0;
}

static void simple_byte_hextostr(u8 hex, char *str)
{
	int i;

	for (i = 0; i < 2; i++) {
		if ((hex & 0xF) > 9)
			*--str = 'a' + (hex & 0xF) - 10;
		else
			*--str = '0' + (hex & 0xF);

		hex >>= 4;
	}
}

static void hex2str(u8 *buf, char *str, int len)
{
	int i;

	for (i = 0; i < len; i++) {
		simple_byte_hextostr(buf[i], str + (i + 1) * 2);
	}
}

int fb_do_getvar(const char *cmd_buffer)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);

	sprintf(response,"OKAY");

	if (!memcmp(cmd_buffer + 7, "version", strlen("version")))
	{
		sprintf(response + 4, FASTBOOT_VERSION);
	}
	else if (!memcmp(cmd_buffer + 7, "product", strlen("product")))
	{
		if (interface.product_name)
			sprintf(response + 4, interface.product_name);
	}
	else if (!memcmp(cmd_buffer + 7, "serialno", strlen("serialno")))
	{
		if (interface.serial_no)
			sprintf(response + 4, interface.serial_no);
	}
	else if (!memcmp(cmd_buffer + 7, "max-download-size", strlen("max-download-size")))
	{
		if (interface.transfer_buffer_size)
			sprintf(response + 4, "%d", interface.transfer_buffer_size);
	}
	else if (!memcmp(cmd_buffer + 7, "partition-type", strlen("partition-type")))
	{
		char *key = (char *)cmd_buffer + 7 + strlen("partition-type:");
		struct pit_entry *ptn = pit_get_part_info(key);

		if (ptn == 0)
		{
			sprintf(response, "FAILpartition does not exist");
			fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);
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
		char *key = (char *)cmd_buffer + 7 + strlen("partition-size:");
		struct pit_entry *ptn = pit_get_part_info(key);

		if (ptn == 0)
		{
			sprintf(response, "FAILpartition does not exist");
			fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);
			return 0;
		}

		sprintf(response + 4, "0x%llx", pit_get_length(ptn));
	}
	else if (!strcmp(cmd_buffer + 7, "slot-count"))
	{
		sprintf(response + 4, "2");
	}
	else if (!strcmp(cmd_buffer + 7, "current-slot"))
	{
		if (ab_current_slot())
			sprintf(response + 4, "b");
		else
			sprintf(response + 4, "a");
	}
	else if (!memcmp(cmd_buffer + 7, "slot-successful", strlen("slot-successful")))
	{
		int slot = -1;
		if (!strcmp(cmd_buffer + 7 + strlen("slot-successful:"), "_a"))
			slot = 0;
		else if (!strcmp(cmd_buffer + 7 + strlen("slot-successful:"), "_b"))
			slot = 1;
		else
			sprintf(response, "FAILinvalid slot");
		printf("slot: %d\n", slot);
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
		if (!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "ldfw") ||
			!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "keystorage") ||
			!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "boot") ||
			!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "dtbo") ||
			!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "system") ||
			!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "vbmeta") ||
			!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "oem") ||
			!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "vendor") ||
			!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "logo") ||
			!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "modem") ||
			!strcmp(cmd_buffer + 7 + strlen("has-slot:"), "bootloader"))
			sprintf(response + 4, "yes");
		else
			sprintf(response + 4, "no");
	}
	else if (!memcmp(cmd_buffer + 7, "unlocked", strlen("unlocked")))
	{
		if (secure_os_loaded == 1) {
			uint32_t lock_state;
			rpmb_get_lock_state(&lock_state);
			if (!lock_state)
				sprintf(response + 4, "yes");
			else
				sprintf(response + 4, "no");
		} else {
			sprintf(response + 4, "ignore");
		}
	}
	else if (!memcmp(cmd_buffer + 7, "uid", strlen("uid")))
	{
		char uid_str[33] = {0};
		uint32_t *p;
		/* default is faked UID, for test purpose only */
		uint8_t uid_buf[] = {0x41, 0xDC, 0x74, 0x4B,	\
								0x00, 0x00, 0x00, 0x00,	\
								0x00, 0x00, 0x00, 0x00,	\
								0x00, 0x00, 0x00, 0x00};

		p = (uint32_t *)&uid_buf[0];
		*p = ntohl(s5p_chip_id[1]);
		p = (uint32_t *)&uid_buf[4];
		*p = ntohl(s5p_chip_id[0]);

		hex2str(uid_buf, uid_str, 16);

		sprintf(response + 4, uid_str);
	}
	else if (!memcmp(cmd_buffer + 7, "str_ram", strlen("str_ram")))
	{
		debug_store_ramdump_getvar(cmd_buffer + 15, response + 4);
	}
	else if (!memcmp(cmd_buffer + 7, "all", strlen("all")))
	{
		int i;

		for (i = 0; i < fastboot_var_nr; i++) {
			strncpy(response,"INFO", 4);
			strncpy(response + 4, fastboot_var_list[i].name, strlen(fastboot_var_list[i].name));
			strncpy(response + 4 + strlen(fastboot_var_list[i].name), ":", 1);
			strcpy(response + 4 + strlen(fastboot_var_list[i].name) + 1, fastboot_var_list[i].string);
			fastboot_tx_status(response, strlen(response), FASTBOOT_TX_SYNC);
		}

		strcpy(response,"OKAY");
		strcpy(response + 4,"Done!");
	}
	else
	{
		debug_snapshot_getvar_item(cmd_buffer + 7, response + 4);
	}

	fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);

	return 0;
}

int fb_do_erase(const char *cmd_buffer)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);
	char *key = (char *)cmd_buffer + 6;
	struct pit_entry *ptn = pit_get_part_info(key);
	int status = 1;

	if (strcmp(key, "pit") && ptn == 0)
	{
		sprintf(response, "FAILpartition does not exist");
		fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);
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

	fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);

	return 0;
}

static void flash_using_pit(char *key, char *response,
		u32 size, void *addr)
{
	struct pit_entry *ptn;
	unsigned long long length;

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
		u32 env_val = 0;
		int param_sz;

		/* Check value on the sysparam */
		param_sz = sysparam_read("ram_disk_size", &env_val, sizeof(env_val));
		if (param_sz > 0)
			sysparam_remove("ram_disk_size");
		env_val = size;
		sysparam_add("ram_disk_size", &env_val, sizeof(env_val));
		sysparam_write();
	}
}

int fb_do_flash(const char *cmd_buffer)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);

	if(is_first_boot()) {
		uint32_t lock_state;
		rpmb_get_lock_state(&lock_state);
		printf("Lock state: %d\n", lock_state);
		if(lock_state) {
			sprintf(response, "FAILDevice is locked");
			fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);
			return 1;
		}
	}

	dprintf(ALWAYS, "flash\n");

	flash_using_pit((char *)cmd_buffer + 6, response,
			downloaded_data_size, (void *)interface.transfer_buffer);

	fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);

	return 0;
}

int fb_do_reboot(const char *cmd_buffer)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);
	bdev_t *dev;
	status_t ret;

	/* Send SSU to UFS */
	dev = bio_open("scsissu");
	if (!dev) {
		printf("error opening block device\n");
	}

	ret = scsi_start_stop_unit(dev);
	if (ret != NO_ERROR)
		printf("scsi ssu error!\n");

	bio_close(dev);

	sprintf(response,"OKAY");
	fastboot_tx_status(response, strlen(response), FASTBOOT_TX_SYNC);

	if(!memcmp(cmd_buffer, "reboot-bootloader", strlen("reboot-bootloader")))
		writel(CONFIG_RAMDUMP_MODE, CONFIG_RAMDUMP_SCRATCH);
	else
		writel(0, CONFIG_RAMDUMP_SCRATCH);

	/* write reboot reasen (bootloader reboot) */
	writel(RAMDUMP_SIGN_BL_REBOOT, CONFIG_RAMDUMP_REASON);

	/* SW reset */
	writel(0x1, EXYNOS9610_SWRESET);

	return 0;
}

int fb_do_download(const char *cmd_buffer)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);

	/* Get download size */
	download_size = (unsigned int)strtol(cmd_buffer + 9, NULL, 16);
	downloaded_data_size = 0;

	printf("Downloaing. Download size is %d bytes\n", download_size);

	if (download_size > 0x100000 * 10) {
		extention_flag = 1;
		exynos_extend_trb_buf();
	}

	if (download_size > interface.transfer_buffer_size) {
		download_size = 0;
		sprintf(response, "FAILdownload data size is bigger than buffer size");
	} else {
		sprintf(response, "DATA%08x", download_size);
	}

	fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);

	return 0;
}

static void start_ramdump(void *buf)
{
	struct fastboot_ramdump_hdr hdr = *(struct fastboot_ramdump_hdr *)buf;
	static uint32_t ramdump_cnt = 0;

	printf("\nramdump start address is [0x%lx]\n", hdr.base);
	printf("ramdump size is [0x%lx]\n", hdr.size);
	printf("version is [0x%lx]\n", hdr.version);

	if (hdr.version != 2) {
		printf("you are using wrong version of fastboot!!!\n");
	}

	/* dont't generate DECERR even if permission failure of ASP occurs */
	if (ramdump_cnt++ == 0)
		set_tzasc_action(0);

	if (debug_store_ramdump_redirection(&hdr)) {
		printf("Failed ramdump~! \n");
		return;
	}

	if (!fastboot_tx_mem(hdr.base, hdr.size)) {
		printf("Failed ramdump~! \n");
	} else {
		printf("Finished ramdump~! \n");
	}
}

int fb_do_ramdump(const char *cmd_buffer)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);

	printf("\nGot ramdump command\n");
	print_lcd_update(FONT_GREEN, FONT_BLACK, "Got ramdump command.");
	is_ramdump = 1;
	/* Get download size */
	download_size = (unsigned int)strtol(cmd_buffer + 8, NULL, 16);
	downloaded_data_size = 0;

	printf("Downloaing. Download size is %d bytes\n", download_size);

	if (download_size > interface.transfer_buffer_size) {
		download_size = 0;
		sprintf(response, "FAILdownload data size is bigger than buffer size");
	} else {
		sprintf(response, "DATA%08x", download_size);
	}

	fastboot_tx_status(response, strlen(response), FASTBOOT_TX_SYNC);

	return 0;
}

int fb_do_set_active(const char *cmd_buffer)
{
	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);

	printf("set_active\n");

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

	fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);

	return 0;
}

/* Lock/unlock device */
int fb_do_flashing(const char *cmd_buffer)
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

	fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);

	return 0;
}

int fb_do_oem(const char *cmd_buffer)
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
		fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);

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
		fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);

		sprintf(response, "OKAY");
		fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);

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
		fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else if (!strncmp(cmd_buffer + 4, "uart_log_disable", 16)) {
		param_sz = sysparam_read("uart_log_enable", &env_val, sizeof(env_val));
		if (param_sz > 0) {
			sysparam_remove("uart_log_enable");
			sysparam_write();
		}

		sprintf(response, "OKAY");
		fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else if (!strncmp(cmd_buffer + 4, "fb_mode_set", 11)) {
		param_sz = sysparam_read("fb_mode_set", &env_val, sizeof(env_val));
		if (param_sz > 0)
			sysparam_remove("fb_mode_set");
		env_val = UART_LOG_MODE_FLAG;
		sysparam_add("fb_mode_set", &env_val, sizeof(env_val));
		sysparam_write();

		sprintf(response, "OKAY");
		fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else if (!strncmp(cmd_buffer + 4, "fb_mode_clear", 13)) {
		param_sz = sysparam_read("fb_mode_set", &env_val, sizeof(env_val));
		if (param_sz > 0) {
			sysparam_remove("fb_mode_set");
			sysparam_write();
		}

		sprintf(response, "OKAY");
		fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else if (!strncmp(cmd_buffer + 4, "logdump", 7)) {
		struct pit_entry *ptn;

		ptn = pit_get_part_info("logbuf");
		if (ptn == 0) {
			printf("Partition 'logbuf' does not exist.\n");
			print_lcd_update(FONT_RED, FONT_BLACK, "Partition 'logbuf' does not exist.\n");
			sprintf(response, "FAILPartition 'logbuf' does not exist.");
		} else {
			printf("Loading 'logbuf' partition on 0xC0000000\n");
			print_lcd_update(FONT_GREEN, FONT_BLACK, "Loading 'logbuf' partition on 0xC0000000\n");
			pit_access(ptn, PIT_OP_LOAD, (u64)CFG_FASTBOOT_MMC_BUFFER, 0);
			sprintf(response, "OKAY");
		}
	} else if (!strncmp(cmd_buffer + 4, "str_ram", 7)) {
		if (!debug_store_ramdump_oem(cmd_buffer + 12))
			sprintf(response, "OKAY");
		else
			sprintf(response, "FAILunsupported command");

		fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else if (!strncmp(cmd_buffer + 4, "xct-on", 11)) {
		char xct_on[8] = { 0, };

		param_sz = sysparam_read("xct", xct_on, 8);
		if (param_sz > 0)
			sysparam_remove("xct");
		sprintf(xct_on, "xct-on");
		sysparam_add("xct", xct_on, strlen(xct_on));
		sysparam_write();

		sprintf(response, "OKAY");
		fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else if (!strncmp(cmd_buffer + 4, "xct-off", 13)) {
		char xct_on[8] = { 0, };

		param_sz = sysparam_read("xct", xct_on, 8);
		if (param_sz > 0) {
			sysparam_remove("xct");
			sysparam_write();
		}
		fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);
	} else {
		printf("Unsupported oem command!\n");
		sprintf(response, "FAILunsupported command");
		fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);
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

static int rx_handler (const unsigned char *buffer, unsigned int buffer_size)
{
	unsigned int i;
	unsigned int rx_size;
	unsigned int remain;
	const char *cmd_buffer;

	char buf[FB_RESPONSE_BUFFER_SIZE];
	char *response = (char *)(((unsigned long)buf + 8) & ~0x07);

	if (download_size)
	{
		if (buffer_size == 0) {
			printf("USB download buffer is empty\n");
			return 0;
		}

		remain = download_size - downloaded_data_size;

		if (buffer_size < remain)
			rx_size = buffer_size;
		else
			rx_size = remain;

		if (extention_flag == 1)
			exynos_move_trb_buff(rx_size, remain);
		else
			/* Save the data to the transfer buffer */
			memcpy (interface.transfer_buffer + downloaded_data_size,
				buffer, rx_size);

		downloaded_data_size += rx_size;

		if (downloaded_data_size == download_size) {
			download_size = 0;

			sprintf(response, "OKAY");
			fastboot_tx_status(response, strlen(response), FASTBOOT_TX_ASYNC);

			printf("\nFinished to download %d bytes\n", downloaded_data_size);

			if (extention_flag == 1) {
				extention_flag = 0;
				exynos_init_trb_buf();
			}

			if (is_ramdump) {
				is_ramdump = 0;
				start_ramdump((void *)buffer);
			}

			return 0;
		}

		/* Print download progress */
		if (0 == (downloaded_data_size % 0x100000)) {
			printf("*");

			if (0 == (downloaded_data_size % (80 * 0x100000)))
				printf("\n");
		}
	} else {
		cmd_buffer = (char *)buffer;

#ifdef PIT_DEBUG
		printf("fb %s\n", cmd_buffer);	/* test: probing protocol */
#endif

		for (i = 0; i < (sizeof(cmd_list) / sizeof(struct cmd_fastboot)); i++) {
			if(!memcmp(cmd_buffer, cmd_list[i].cmd_name, strlen(cmd_list[i].cmd_name)))
				cmd_list[i].handler(cmd_buffer);
		}
	}

	return 0;
}

void exynos_usb_runstop_device(u8 ucSet);

int do_fastboot(int argc, const cmd_args *argv)
{
#if 0 /* For Polling mode */
	int continue_from_disconnect = 0;
#endif

	is_attached = 0;
	dprintf(ALWAYS, "This is do_fastboot\n");
	dprintf(ALWAYS, "Enabling manual reset and disabling warm reset.\n");

	/* To prevent entering fastboot mode after manual reset, clear ramdump scratch. */
	writel(0, CONFIG_RAMDUMP_SCRATCH);

	pmic_enable_manual_reset();
	print_lcd_update(FONT_GREEN, FONT_BLACK, "Entering fastboot mode.");

	/* display all entries */
	pit_show_info();

	init_muic_interrupt();

	muic_sw_usb();

	printf("Initialization USB!!!!\n");
	fastboot_init(&interface);
	fastboot_poll();

#if 0 /* For Polling mode */
	do {
		continue_from_disconnect = 0;

		if(!fastboot_init(&interface))
		{
			int poll_status;
			dprintf(ALWAYS, "fastboot_init success!!\n");
			while (1) {
				poll_status = fastboot_poll();

				if (FASTBOOT_ERROR == poll_status) {
					dprintf(ALWAYS, "Fastboot ERROR!\n");
					break;
				} else if (FASTBOOT_DISCONNECT == poll_status) {
					dprintf(ALWAYS, "Fastboot DISCONNECT detected\n");
					continue_from_disconnect = 1;
					break;
				}
			}
		}

	} while (continue_from_disconnect);
#endif

	while (is_attached == 0) {
		/*
		print_lcd_update(FONT_GREEN, FONT_BLACK,
				"USB Run -> Stop polling...", is_attached);
		*/
		u_delay(2000000);
		if (is_attached == 1)
			break;
		exynos_usb_runstop_device(0);
		u_delay(100000);
		exynos_usb_runstop_device(1);
	}

	return 0;
}

int connect_usb(void)
{
	is_attached = 0;
	dprintf(ALWAYS, "Connecting USB...\n");
	print_lcd_update(FONT_GREEN, FONT_BLACK, "Connecting USB...");

	muic_sw_usb();

	printf("Initialization USB!!!!\n");
	fastboot_init(&interface);
	fastboot_poll();

	while (is_attached == 0) {
		print_lcd_update(FONT_GREEN, FONT_BLACK,
				"USB Run -> Stop polling...");
		printf("Delay. Connection status: %d\n", is_attached);
		u_delay(1000000);
		if (is_attached == 1)
			break;
		printf("Stop USB. Connection status: %d\n", is_attached);
		exynos_usb_runstop_device(0);
		u_delay(100000);
		printf("Run USB. Connection status: %d\n", is_attached);
		exynos_usb_runstop_device(1);
		printf("Connection status: %d\n", is_attached);
	}

	return 0;
}

STATIC_COMMAND_START
STATIC_COMMAND("fast", "usb fastboot", &do_fastboot)
STATIC_COMMAND_END(usb_fastboot);
