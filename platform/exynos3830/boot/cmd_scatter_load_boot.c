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

int do_scatter_load_boot_v2(int argc, const cmd_args *argv)
{
	unsigned long boot_addr, kernel_addr, dtb_addr, ramdisk_addr, recovery_dtbo_addr;
	struct boot_img_hdr_v2 *b_hdr_v2;
	int kernel_offset;
	int dtb_offset;
	int ramdisk_offset;
	int recovery_dtbo_offset;
	int second_stage_offset;

	if (argc != 5) goto usage;

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
	unsigned long long boot_addr, vendor_boot_addr, kernel_addr, dtb_addr, ramdisk_addr; /*recovery_dtbo_addr*/
	struct boot_img_hdr_v3 *b_hdr;
	struct vendor_boot_img_hdr *vb_hdr;
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
	vb_hdr = (struct vendor_boot_img_hdr *)vendor_boot_addr;

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
int do_load_dtb_from_vendor_boot(int argc, const cmd_args *argv)
{
	unsigned long long vendor_boot_addr, dtb_addr;
	struct vendor_boot_img_hdr *vb_hdr;
	int vendor_ramdisk_offset;
	int dtb_offset;

	printf("loading vendor_boot\n");
	vendor_boot_addr = argv[1].u;
	dtb_addr = argv[2].u;

	printf("argc: %d\n",argc);

	vb_hdr = (struct vendor_boot_img_hdr *)vendor_boot_addr;

	/*if (strncmp(vb_hdr->magic, VENDOR_BOOT_MAGIC, VENDOR_BOOT_MAGIC_SIZE)) {
		printf("\nerror: Android bootimage file format is different (%s)\n", vb_hdr->magic);
		return -1;
	}*/

	if (vb_hdr->header_version != 3) {
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
