/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <platform/bootimg.h>
#include <lib/console.h>
#include <platform/exynos3830.h>
#include <debug.h>
#include <libfdt.h>
#include <platform/fdt.h>
#include <reg.h>

#define TAG		"BHDR_V4"

static uint64_t ramdisk_addr;
static uint64_t ramdisk_base;

#define MAX_BOOTARGS_SIZE       4096

#define BOOT_ALIGN(X)		ALIGN(X, BOOT_IMAGE_HEADER_V4_PAGESIZE)
#define VND_BOOT_ALIGN(X)	ALIGN(X, vb_hdr->page_size)

#define COPY_RECOVERY(_name, _offset, _dest, _size)	copy_binary(_name, \
								BOOT_BASE, \
								_offset, \
								_dest, \
								_size)

#define COPY_BOOT(_name, _offset, _dest, _size)		copy_binary(_name, \
								BOOT_BASE, \
								_offset, \
								_dest, \
								_size)

#define COPY_VNDBOOT(_name, _offset, _dest, _size)	copy_binary(_name, \
								VENDOR_BOOT_BASE, \
								_offset, \
								_dest, \
								_size)

void copy_binary(const char *name, u64 src, u64 offset, u64 dest, u32 size)
{
	memcpy((void *)dest, (const void *)(src + offset), (size_t)size);
	printf("Load %-16s From: 0x%-16llX To: 0x%-16llX size: 0x%-16X\n",
						name, src + offset, dest, size);
}

static const char android_bootargs_keyword[] = "androidboot.";
static const char android_bootconfig_magic[] = "#BOOTCONFIG\n";
static const char android_bootconfig_args[] = "bootconfig";
static char kernel_cmdline[4096];
static char bootconfig[4096];
static int cmd_ptr;
static int bootconfig_ptr;
static char bootargs_from_boot[MAX_BOOTARGS_SIZE] = {0};
static int bootargs_idx = 0;

void get_bootargs_from_boot(char *bootargs, struct boot_img_hdr_v4 *b_hdr,
							struct vendor_boot_img_hdr_v4 *vb_hdr)
{
	if (b_hdr->cmdline[0]) {
		if (!b_hdr->cmdline[BOOT_ARGS_SIZE + BOOT_EXTRA_ARGS_SIZE - 1])
			bootargs_idx += snprintf(bootargs_from_boot + bootargs_idx,
							MAX_BOOTARGS_SIZE - bootargs_idx, " %s", b_hdr->cmdline);
	}

	if (vb_hdr->cmdline[0]) {
		if (!vb_hdr->cmdline[VENDOR_BOOT_ARGS_SIZE - 1])
			bootargs_idx += snprintf(bootargs_from_boot + bootargs_idx,
							MAX_BOOTARGS_SIZE - bootargs_idx, " %s", vb_hdr->cmdline);
	}

	if (vb_hdr->bootconfig_size) {
		char *buffer = (char *)malloc(sizeof(char) * (vb_hdr->bootconfig_size + 1));
		uint32_t idx;

		memset(buffer, 0, vb_hdr->bootconfig_size + 1);
		memcpy(buffer, (char *)((uint64_t)VENDOR_BOOT_BASE
							+ VND_BOOT_ALIGN(vb_hdr->header_size)
							+ VND_BOOT_ALIGN(vb_hdr->vendor_ramdisk_size)
							+ VND_BOOT_ALIGN(vb_hdr->dtb_size)
							+ VND_BOOT_ALIGN(vb_hdr->vendor_ramdisk_table_size)),
							vb_hdr->bootconfig_size);

		for (idx = 0; idx < vb_hdr->bootconfig_size; idx++) {
			if (buffer[idx] == 0xA)
				buffer[idx] = ' ';
		}
		bootargs_idx += snprintf(bootargs_from_boot + bootargs_idx,
							MAX_BOOTARGS_SIZE - bootargs_idx, " %s", buffer);
		free(buffer);
	}

	printf("bootargs from boot = %s\n", bootargs_from_boot);
	sprintf(bootargs, "%s", bootargs_from_boot);
}

static int get_checksum(void *start, int size)
{
	unsigned char *ptr = start;
	int checksum_ret = 0;

	while (size--)
		checksum_ret += *ptr++;

	return checksum_ret;
}

static void add_bootconfig(const char *bootargs, int *ptr, int len)
{
	if (bootconfig_ptr) {
		/*
		 * Bootconfig must be separate with semicolon
		 * Space will be deprecated in kernel, just add to see clearly
		 */
		bootconfig[bootconfig_ptr++] = ';';
		bootconfig[bootconfig_ptr++] = ' ';
	}

	while (1) {
		if (bootargs[*ptr] == ' ' || *ptr >= len)
			break;

		bootconfig[bootconfig_ptr++] = bootargs[*ptr];
		*ptr += 1;
	}
}

static void add_cmdline(const char *bootargs, int *ptr, int len)
{
	if (cmd_ptr)
		kernel_cmdline[cmd_ptr++] = ' ';

	while (1) {
		if (bootargs[*ptr] == ' ' || *ptr >= len)
			break;

		kernel_cmdline[cmd_ptr++] = bootargs[*ptr];
		*ptr += 1;
	}
}

static bool is_bootconfig(const char *bootargs)
{
	if (!strncmp(bootargs, android_bootargs_keyword, strlen(android_bootargs_keyword)))
		return true;

	return false;
}

bool is_bootconfig_enabled(const char *bootargs, int len)
{
	int ptr;

	printf("check boot config: %s\n", bootargs);
	for (ptr = 0; ptr < len; ptr++) {
		if (ptr < len -  (int)strlen(android_bootconfig_args)) {
			if (!strncmp(bootargs + ptr, android_bootconfig_args, strlen(android_bootconfig_args))) {
				printf("[%s] Bootconfig is Enabled!\n", TAG);
				return true;
			}
		}
	}

	printf("[%s] Bootconfig is Disabled!\n", TAG);
	return false;
}

/*
 * To Prevent VTS issue by checking proc/cmdline and proc/bootconfig,
 * Bootloader remove all bootconfig from cmdline when bootconfig is enabled
 */
static void extract_bootconfig(void)
{
	int noff;
	int len;
	int ptr;
	const char *np;

	noff = fdt_path_offset(fdt_dtb, "/chosen");
	np = fdt_getprop(fdt_dtb, noff, "bootargs", &len);

	/* Check if bootconfig is enabled */
	if (!is_bootconfig_enabled(np, len))
		return;

	for (ptr = 0; ptr < len; ptr++) {
		if (ptr < len - (int)strlen(android_bootargs_keyword)) {
			if (is_bootconfig(np + ptr))
				add_bootconfig(np, &ptr, len);
			else
				add_cmdline(np, &ptr, len);
		} else {
			add_cmdline(np, &ptr, len);
		}
	}

	/* Close Bootconfig */
	bootconfig[bootconfig_ptr++] = ';';

	printf("Kernel Cmdline: %s\n", kernel_cmdline);
	printf("Android Bootconfig: %s\n", bootconfig);

	set_fdt_val("/chosen", "bootargs", kernel_cmdline);

	bootconfig[bootconfig_ptr++] = '\0';
	bootconfig_ptr = ALIGN(bootconfig_ptr, 4);
}

static void append_bootconfig_to_ramdisk(void)
{
	uint64_t bootconfig_base;
	uint64_t bootconfig_size;

	bootconfig_base = ramdisk_addr;
	bootconfig_size = 0;

	/* Bootconfig Parameters */
	memcpy((void *)ramdisk_addr, (const void *)bootconfig, bootconfig_ptr);
	/* To store next property for bootdonfig, Align 4 */
	ramdisk_addr = ALIGN(ramdisk_addr + bootconfig_ptr, 4);
	bootconfig_size = ramdisk_addr - bootconfig_base;

	/* Bootconfig Parameters Size */
	writel(bootconfig_size, ramdisk_addr);
	ramdisk_addr += 4;

	/* Bootconfig Parameters checksum */
	writel(get_checksum((void *)bootconfig_base, bootconfig_size), ramdisk_addr);
	ramdisk_addr += 4;

	/* Bootconfig Magic String */
	memcpy((void *)ramdisk_addr, (const void *)android_bootconfig_magic, strlen(android_bootconfig_magic));
	ramdisk_addr += strlen(android_bootconfig_magic);
}

int load_bootconfig_v4(void)
{
	extract_bootconfig();

//	/* Load bootconfig binary */
//	if (vb_hdr->bootconfig_size == 0)
//		panic("[%s] Vendor Boot Image Invalid - No Bootconfig found\n", TAG);

	append_bootconfig_to_ramdisk();

	return 0;
}

static void boot_hdr_dump_v4(struct boot_img_hdr_v4 *b_hdr)
{
	if (!b_hdr)
		return;

	printf("\n------------------------------------\n");
	printf("\n  ## Boot Header Info ##\n");
	printf("%-14s: %8s\n", "MAGIC", b_hdr->magic);
	printf("%-14s: 0x%X\n", "kernel_size", b_hdr->kernel_size);
	printf("%-14s: 0x%X\n", "ram_size", b_hdr->ramdisk_size);
	printf("%-14s: 0x%X\n", "hdr_size", b_hdr->header_size);
	printf("%-14s: %d\n", "hdr_ver", b_hdr->header_version);
	printf("\n------------------------------------\n");
}

static void vendor_boot_hdr_dump_v4(struct vendor_boot_img_hdr_v4 *vb_hdr)
{
	if (!vb_hdr)
		return;

	printf("\n------------------------------------\n");
	printf("\n  ## Vendor Boot Header Info ##\n");
	printf("%-14s: %8s\n", "MAGIC", vb_hdr->magic);
	printf("%-14s: %d\n", "vhdr_ver", vb_hdr->header_version);
	printf("%-14s: 0x%X\n", "page_size", vb_hdr->page_size);
	printf("%-14s: 0x%X\n", "kernel_addr", vb_hdr->kernel_addr);
	printf("%-14s: 0x%X\n", "ram_addr", vb_hdr->ramdisk_addr);
	printf("%-14s: 0x%X\n", "vnd_ram_size", vb_hdr->vendor_ramdisk_size);
	printf("%-14s: 0x%X\n", "vhdr_size", vb_hdr->header_size);
	printf("%-14s: 0x%X\n", "dtb_size", vb_hdr->dtb_size);
	printf("%-14s: 0x%llX\n", "dtb_addr", vb_hdr->dtb_addr);
	printf("%-14s: 0x%X\n", "Vram_tlb_size", vb_hdr->vendor_ramdisk_table_size);
	printf("%-14s: 0x%X\n", "Vram_ent_num", vb_hdr->vendor_ramdisk_table_entry_num);
	printf("%-14s: 0x%X\n", "Vram_ent_size", vb_hdr->vendor_ramdisk_table_entry_size);
	printf("%-14s: 0x%X\n", "Bootconf_size", vb_hdr->bootconfig_size);
	printf("\n------------------------------------\n");
}

int do_scatter_load_boot_v2(int argc, const cmd_args *argv)
{
	unsigned long boot_addr, kernel_addr, dtb_addr, recovery_dtbo_addr;
	struct boot_img_hdr_v2 *b_hdr_v2;
	int kernel_offset;
	int dtb_offset;
	int ramdisk_offset;
	int recovery_dtbo_offset;
	int second_stage_offset;

	if (argc < 5) goto usage;

	boot_addr = argv[1].u;
	kernel_addr = argv[2].u;
	ramdisk_addr = argv[3].u;
	dtb_addr = argv[4].u;
	recovery_dtbo_addr = argv[5].u;

	b_hdr_v2 = (struct boot_img_hdr_v2 *)boot_addr;

	printf("page size: 0x%08x\n", b_hdr_v2->page_size);
	printf("kernel size: 0x%08x\n", b_hdr_v2->kernel_size);
	printf("ramdisk size: 0x%08x\n", b_hdr_v2->ramdisk_size);
#if (BOOT_IMG_HDR_V2 == 1)
	printf("DTB size: 0x%08x\n", b_hdr_v2->dtb_size);
#else
	printf("DTB size: 0x%08llx\n", b_hdr_v2->second_size);
#endif
	if (recovery_dtbo_addr)
		printf("recovery DTBO size: 0x%08x\n", b_hdr_v2->recovery_dtbo_size);

	kernel_offset = b_hdr_v2->page_size;
	ramdisk_offset = kernel_offset + ((b_hdr_v2->kernel_size + b_hdr_v2->page_size - 1) / b_hdr_v2->page_size) * b_hdr_v2->page_size;
	second_stage_offset = ramdisk_offset + ((b_hdr_v2->ramdisk_size + b_hdr_v2->page_size - 1) / b_hdr_v2->page_size) * b_hdr_v2->page_size;
	recovery_dtbo_offset = second_stage_offset + ((b_hdr_v2->second_size + b_hdr_v2->page_size - 1) / b_hdr_v2->page_size) * b_hdr_v2->page_size;
#if (BOOT_IMG_HDR_V2 == 1)
	dtb_offset = recovery_dtbo_offset + ((b_hdr_v2->recovery_dtbo_size + b_hdr_v2->page_size - 1) / b_hdr_v2->page_size) * b_hdr_v2->page_size;
#else
	dtb_offset = second_stage_offset;
#endif


	if (kernel_addr)
		memcpy((void *)kernel_addr, (const void *)(boot_addr + kernel_offset), (size_t)b_hdr_v2->kernel_size);
	if (ramdisk_addr)
		memcpy((void *)ramdisk_addr, (const void *)(boot_addr + ramdisk_offset), (size_t)b_hdr_v2->ramdisk_size);
	if (dtb_addr)
#if (BOOT_IMG_HDR_V2 == 1)
		memcpy((void *)dtb_addr, (const void *)(boot_addr + dtb_offset), (size_t)b_hdr_v2->dtb_size);
#else
		memcpy((void *)dtb_addr, (const void *)(boot_addr + dtb_offset), (size_t)b_hdr_v2->second_size);
#endif
	if (recovery_dtbo_addr)
		memcpy((void *)recovery_dtbo_addr, (const void *)(boot_addr + recovery_dtbo_offset), (size_t)b_hdr_v2->recovery_dtbo_size);

	return 0;

usage:
	printf("scatter_load_boot {boot/recovery addr} {kernel addr} {ramdisk addr} {dtb addr} {recovery dtbo addr}\n");
	return -1;
}

int do_scatter_load_boot_v3(int argc,const cmd_args *argv)
{
	unsigned long long boot_addr, vendor_boot_addr, kernel_addr, dtb_addr; /*recovery_dtbo_addr*/
	struct boot_img_hdr_v3 *b_hdr;
	struct vendor_boot_img_hdr_v3 *vb_hdr;
	int kernel_offset;
	int boot_ramdisk_offset;
	int vendor_ramdisk_offset;
	int dtb_offset;
	char initrd_size[32];

	boot_addr = argv[1].u;
	vendor_boot_addr=argv[6].u;
	kernel_addr = argv[2].u;
	ramdisk_addr = argv[3].u;
	dtb_addr = argv[4].u;
	//recovery_dtbo_addr = argv[5].u;

	b_hdr = (struct boot_img_hdr_v3 *)boot_addr;
	vb_hdr = (struct vendor_boot_img_hdr_v3 *)vendor_boot_addr;

	if (vb_hdr->header_version != 3) {
		printf("\nerror: Unknown Android vendor bootimage (ver:%d)\n", vb_hdr->header_version);
		return -1;
	}

	printf("page size: 0x%08x\n", vb_hdr->page_size);
	printf("kernel size: 0x%08x\n", b_hdr->kernel_size);
	printf("boot ramdisk size: 0x%08x\n", b_hdr->ramdisk_size);
	printf("boot header size: 0x%08x\n", b_hdr->header_size);
	printf("vendor ramdisk size: 0x%08x\n", vb_hdr->vendor_ramdisk_size);
	printf("vendor boot header size: 0x%08x\n", vb_hdr->header_size);
	printf("DTB size: 0x%08x\n", vb_hdr->dtb_size);

	kernel_offset = BOOT_IMAGE_HEADER_V3_PAGESIZE;
	boot_ramdisk_offset = kernel_offset + ((b_hdr->kernel_size + BOOT_IMAGE_HEADER_V3_PAGESIZE - 1) / BOOT_IMAGE_HEADER_V3_PAGESIZE) * BOOT_IMAGE_HEADER_V3_PAGESIZE;

	vendor_ramdisk_offset = 2 * vb_hdr->page_size;
	dtb_offset = vendor_ramdisk_offset + ((vb_hdr->vendor_ramdisk_size + vb_hdr->page_size -1) / vb_hdr->page_size) * vb_hdr->page_size;

	if (kernel_addr)
		memcpy((void *)kernel_addr, (const void *)(boot_addr + kernel_offset), (size_t)b_hdr->kernel_size);
	if (ramdisk_addr) {
		memcpy((void *)ramdisk_addr, (const void *)(vendor_boot_addr + vendor_ramdisk_offset), (size_t)vb_hdr->vendor_ramdisk_size);
		memcpy((void *)(ramdisk_addr + vb_hdr->vendor_ramdisk_size), (const void *)(boot_addr + boot_ramdisk_offset), (size_t)b_hdr->ramdisk_size);
	}
	if (dtb_addr)
		memcpy((void *)dtb_addr, (const void *)(vendor_boot_addr + dtb_offset), (size_t)vb_hdr->dtb_size);


	sprintf(initrd_size, "0x%x", b_hdr->ramdisk_size + vb_hdr->vendor_ramdisk_size);
	//setenv("rootfslen", initrd_size);

	return 0;
}

int do_scatter_load_boot_v4(int argc,const cmd_args *argv)
{
	unsigned long long boot_addr, vendor_boot_addr, kernel_addr, dtb_addr; /*recovery_dtbo_addr*/
	struct boot_img_hdr_v4 *b_hdr;
	struct vendor_boot_img_hdr_v4 *vb_hdr;
	char initrd_size[32];
	unsigned int ramdisk_idx = 0;
	struct vendor_ramdisk_table_v4 *entry;
	uint64_t vendor_offset, kernel_offset, ramdisk_offset;

	boot_addr = argv[1].u;
	vendor_boot_addr=argv[6].u;
	kernel_addr = argv[2].u;
	ramdisk_base = argv[3].u;
	ramdisk_addr = ramdisk_base;
	dtb_addr = argv[4].u;
	//recovery_dtbo_addr = argv[5].u;

	b_hdr = (struct boot_img_hdr_v4 *)boot_addr;
	vb_hdr = (struct vendor_boot_img_hdr_v4 *)vendor_boot_addr;

	if (vb_hdr->header_version != 4) {
		printf("\nerror: Unknown Android vendor bootimage (ver:%d)\n", vb_hdr->header_version);
		return -1;
	}

	vendor_offset = VND_BOOT_ALIGN(vb_hdr->header_size);
	kernel_offset = BOOT_ALIGN(b_hdr->header_size);

	printf("page size: 0x%08x\n", vb_hdr->page_size);
	printf("kernel size: 0x%08x\n", b_hdr->kernel_size);
	printf("boot ramdisk size: 0x%08x\n", b_hdr->ramdisk_size);
	printf("boot header size: 0x%08x\n", b_hdr->header_size);
	printf("vendor ramdisk size: 0x%08x\n", vb_hdr->vendor_ramdisk_size);
	printf("vendor boot header size: 0x%08x\n", vb_hdr->header_size);
	printf("DTB size: 0x%08x\n", vb_hdr->dtb_size);

	if (kernel_addr)
		//memcpy((void *)kernel_addr, (const void *)(boot_addr + kernel_offset), (size_t)b_hdr->kernel_size);

		/* Load Kernel binary */
		if (b_hdr->kernel_size == 0)
			panic("[%s] Boot Image Invalid - No Kernel found\n", TAG);
		COPY_BOOT("kernel", kernel_offset, kernel_addr, b_hdr->kernel_size);
	if (ramdisk_addr) {
		entry = (struct vendor_ramdisk_table_v4 *)((uint64_t)(vendor_boot_addr)
									+ VND_BOOT_ALIGN(vb_hdr->header_size)
									+ VND_BOOT_ALIGN(vb_hdr->vendor_ramdisk_size)
									+ VND_BOOT_ALIGN(vb_hdr->dtb_size));
		/* Vendor Boot Ramdisk */
		for (ramdisk_idx = 0; ramdisk_idx < vb_hdr->vendor_ramdisk_table_entry_num; ramdisk_idx++) {
			const char *ramdisk_name;

			if (entry[ramdisk_idx].ramdisk_name[0] == 0 && ramdisk_idx == 0)
				ramdisk_name = "Default Ramdisk";
			else
				ramdisk_name = (const char *)entry[ramdisk_idx].ramdisk_name;

			COPY_VNDBOOT(ramdisk_name,
					vendor_offset + entry[ramdisk_idx].ramdisk_offset,
					ramdisk_addr,
					entry[ramdisk_idx].ramdisk_size);

			ramdisk_addr += entry[ramdisk_idx].ramdisk_size;
		}
	}

	ramdisk_offset = BOOT_ALIGN(b_hdr->header_size) + BOOT_ALIGN(b_hdr->kernel_size);
	COPY_BOOT("ramdisk", ramdisk_offset, ramdisk_addr, b_hdr->ramdisk_size);
	ramdisk_addr += b_hdr->ramdisk_size;

	if (dtb_addr) {
		vendor_offset = VND_BOOT_ALIGN(vb_hdr->header_size)
						+ VND_BOOT_ALIGN(vb_hdr->vendor_ramdisk_size);

		if (vb_hdr->dtb_size == 0)
			panic("[WARNING] Vendor Boot Image Invalid - No DTB found\n");

		/* Load DTB binary */
		COPY_VNDBOOT("dtb", vendor_offset, DT_BASE, vb_hdr->dtb_size);
	}


	sprintf(initrd_size, "0x%x", b_hdr->ramdisk_size + vb_hdr->vendor_ramdisk_size);
	//setenv("rootfslen", initrd_size);

	boot_hdr_dump_v4(b_hdr);
	vendor_boot_hdr_dump_v4(vb_hdr);
	return 0;
}

unsigned int get_ramdisk_size_v4(void)
{
	return ramdisk_addr - ramdisk_base;
}

int do_load_dtb_from_vendor_boot(int argc, const cmd_args *argv)
{
	unsigned long long vendor_boot_addr, dtb_addr;
	struct vendor_boot_img_hdr_v3 *vb_hdr;
	int vendor_ramdisk_offset;
	int dtb_offset;

	printf("loading vendor_boot\n");
	vendor_boot_addr = argv[1].u;
	dtb_addr = argv[2].u;

	printf("argc: %d\n",argc);

	vb_hdr = (struct vendor_boot_img_hdr_v3 *)vendor_boot_addr;

	/*if (strncmp(vb_hdr->magic, VENDOR_BOOT_MAGIC, VENDOR_BOOT_MAGIC_SIZE)) {
		printf("\nerror: Android bootimage file format is different (%s)\n", vb_hdr->magic);
		return -1;
	}*/

	if (vb_hdr->header_version < 3) {
		printf("\nerror: Unknown Android vendor bootimage (ver:%d)\n", vb_hdr->header_version);
		return -1;
	}

	printf("page size: 0x%08x\n", vb_hdr->page_size);
	printf("DTB size: 0x%08x\n", vb_hdr->dtb_size);

	vendor_ramdisk_offset = 2 * vb_hdr->page_size;
	dtb_offset = vendor_ramdisk_offset + ((vb_hdr->vendor_ramdisk_size + vb_hdr->page_size -1) / vb_hdr->page_size) * vb_hdr->page_size;


	if (dtb_addr)
		printf("dest address: 0x%08llx\n",(unsigned long long)memcpy((void *)dtb_addr, (const void *)(vendor_boot_addr + dtb_offset), (size_t)vb_hdr->dtb_size));
		printf("source address: 0x%08llx\n",(unsigned long long)(vendor_boot_addr + dtb_offset));

	return 0;

}
int cmd_scatter_load_boot(int argc, const cmd_args *argv)
{
	unsigned long long boot_addr;
	struct boot_img_hdr *b_hdr;
	printf("argc: %d\n",argc);
	if (argc != 6) {
		goto usage;
	}

	boot_addr = argv[1].u;
	printf("boot_addr : 0x%08llx\n",boot_addr);
	b_hdr = (struct boot_img_hdr *)boot_addr;

/*	if (strncmp(b_hdr->magic, BOOT_MAGIC, BOOT_MAGIC_SIZE)) {
		printf("\nerror: Android bootimage file format is different (%s)\n", b_hdr->magic);
		return -1;
	}
*/
	printf("Android BootImage version: %d\n",b_hdr->header_version);
	switch (b_hdr->header_version) {
	case 1:
	case 2:
		return do_scatter_load_boot_v2(argc, argv);
	case 3:
		return do_scatter_load_boot_v3(argc, argv);
	case 4:
		return do_scatter_load_boot_v4(argc, argv);
	default:
		printf("\nerror: Unknown version\n");
		return -1;
	}
	usage:
		printf("scatter_load_boot {boot/recovery addr} {kernel addr} {ramdisk addr} {dtb addr} {recovery dtbo addr}\n");
		return -1;
}

STATIC_COMMAND_START
STATIC_COMMAND("scatter_load_boot", "scatter_load kernel, ramdisk, dtb, recovery dtbo from boot/recovery.img", &cmd_scatter_load_boot)
STATIC_COMMAND_END(scatter_load_boot);

STATIC_COMMAND_START
STATIC_COMMAND(	"load_dtb_from_vendor_boot",	"load_dtb_from_vendor_boot, vendor_boot_addr ,dtb_addr",&do_load_dtb_from_vendor_boot)
STATIC_COMMAND_END(load_dtb_from_vendor_boot);
