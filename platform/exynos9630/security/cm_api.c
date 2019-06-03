/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

#include <debug.h>
#include <stdlib.h>
#include <string.h>
#include <platform/cm_api.h>
#include <platform/secure_boot.h>
#include <platform/mmu/mmu_func.h>

static inline uint64_t exynos_cm_smc(uint64_t *cmd, uint64_t *arg1,
				     uint64_t *arg2, uint64_t *arg3)
{
	register uint64_t reg0 __asm__("x0") = *cmd;
	register uint64_t reg1 __asm__("x1") = *arg1;
	register uint64_t reg2 __asm__("x2") = *arg2;
	register uint64_t reg3 __asm__("x3") = *arg3;

	__asm__ volatile (
		"dsb	sy\n"
		"smc	0\n"
		: "+r"(reg0), "+r"(reg1), "+r"(reg2), "+r"(reg3)
	);

	*cmd = reg0;
	*arg1 = reg1;
	*arg2 = reg2;
	*arg3 = reg3;

	if (reg0 == RV_SB_LDFW_NOT_LOADED)
		printf("[CM] LDFW hasn't been loaded\n");

	return reg0;
}

uint64_t cm_secure_boot_set_pubkey(
	uint8_t *pub_key_ptr, uint32_t pub_key_len)
{
	uint64_t ret = RV_SUCCESS;
	uint64_t r0, r1, r2, r3;

	r0 = SMC_AARCH64_PREFIX | SMC_CM_SECURE_BOOT;
	r1 = SB_SET_PUBLIC_KEY;
	r2 = VIRT_TO_PHYS(pub_key_ptr);
	r3 = pub_key_len;

	printf("[CM] set publick_key: start\n");

	FLUSH_DCACHE_RANGE(pub_key_ptr, pub_key_len);

	ret = exynos_cm_smc(&r0, &r1, &r2, &r3);
	if (ret != RV_SUCCESS) {
		printf("[CM] set publick_key: fail: "
		"r0:0x%llx, r1:0x%llx, r2:0x%llx, r3:0x%llx\n", r0, r1, r2, r3);
		return ret;
	}

	printf("[CM] set public_key: success\n");

	return ret;
}

uint64_t cm_secure_boot_erase_pubkey(void)
{
	uint64_t ret = RV_SUCCESS;
	uint64_t r0, r1, r2, r3;

	r0 = SMC_AARCH64_PREFIX | SMC_CM_SECURE_BOOT;
	r1 = SB_ERASE_PUBLIC_KEY;
	r2 = 0;
	r3 = 0;

	printf("[CM] erase public_key: start\n");

	ret = exynos_cm_smc(&r0, &r1, &r2, &r3);
	if (ret != RV_SUCCESS) {
		printf("[CM] erase public_key: fail: "
		"r0:0x%llx, r1:0x%llx, r2:0x%llx, r3:0x%llx\n", r0, r1, r2, r3);
		return ret;
	}

	printf("[CM] erase public_key: success\n");

	return ret;
}

uint64_t cm_secure_boot_set_os_version(
	uint32_t os_version, uint32_t os_patch_level)
{
	uint64_t ret = RV_SUCCESS;
	uint64_t r0, r1, r2, r3;

	r0 = SMC_AARCH64_PREFIX | SMC_CM_SECURE_BOOT;
	r1 = SB_SET_OS_VERSION;
	r2 = os_version;
	r3 = os_patch_level;

	printf("[CM] set os_version & os_patch_level: start\n");

	ret = exynos_cm_smc(&r0, &r1, &r2, &r3);
	if (ret != RV_SUCCESS) {
		printf("[CM] set os_version & os_patch_level: fail: "
		"r0:0x%llx, r1:0x%llx, r2:0x%llx, r3:0x%llx\n", r0, r1, r2, r3);
		return ret;
	}

	printf("[CM] set os_version & os_patch_level: success\n");

	return ret;
}

uint64_t cm_secure_boot_set_vendor_boot_version(
	uint32_t vendor_patch_level, uint32_t boot_patch_level)
{
	uint64_t ret = RV_SUCCESS;
	uint64_t r0, r1, r2, r3;

	r0 = SMC_AARCH64_PREFIX | SMC_CM_SECURE_BOOT;
	r1 = SB_SET_VENDOR_BOOT_VERSION;
	r2 = vendor_patch_level;
	r3 = boot_patch_level;

	printf("[CM] set vendor & boot patch level: start\n");

	ret = exynos_cm_smc(&r0, &r1, &r2, &r3);
	if (ret != RV_SUCCESS) {
		printf("[CM] set vendor & boot patch level: fail: "
		"r0:0x%llx, r1:0x%llx, r2:0x%llx, r3:0x%llx\n", r0, r1, r2, r3);
		return ret;
	}

	printf("[CM] set vendor & boot patch level: success\n");

	return ret;
}

uint64_t cm_secure_boot_set_device_state(
	uint32_t state)
{
	uint64_t ret = RV_SUCCESS;
	uint64_t r0, r1, r2, r3;

	r0 = SMC_AARCH64_PREFIX | SMC_CM_SECURE_BOOT;
	r1 = SB_SET_DEVICE_STATE;
	r2 = state;
	r3 = 0;

	printf("[CM] set device_state: start\n");

	ret = exynos_cm_smc(&r0, &r1, &r2, &r3);
	if (ret != RV_SUCCESS) {
		printf("[CM] set device_state: fail: "
		"r0:0x%llx, r1:0x%llx, r2:0x%llx, r3:0x%llx\n", r0, r1, r2, r3);
		return ret;
	}

	printf("[CM] set device_state: success\n");

	return ret;
}

uint64_t cm_secure_boot_set_boot_state(
	uint32_t state)
{
	uint64_t ret = RV_SUCCESS;
	uint64_t r0, r1, r2, r3;

	r0 = SMC_AARCH64_PREFIX | SMC_CM_SECURE_BOOT;
	r1 = SB_SET_BOOT_STATE;
	r2 = state;
	r3 = 0;

	printf("[CM] set boot_state: start\n");

	ret = exynos_cm_smc(&r0, &r1, &r2, &r3);
	if (ret != RV_SUCCESS) {
		printf("[CM] set boot_state: fail: "
		"r0:0x%llx, r1:0x%llx, r2:0x%llx, r3:0x%llx\n", r0, r1, r2, r3);
		return ret;
	}

	printf("[CM] set boot_state: success\n");

	return ret;
}

uint64_t cm_secure_boot_block_cmd(void)
{
	uint64_t ret = RV_SUCCESS;
	uint64_t r0, r1, r2, r3;

	r0 = SMC_AARCH64_PREFIX | SMC_CM_SECURE_BOOT;
	r1 = SB_BLOCK_CMD;
	r2 = 0;
	r3 = 0;

	printf("[CM] block verified boot commands: start\n");

	ret = exynos_cm_smc(&r0, &r1, &r2, &r3);
	if (ret != RV_SUCCESS) {
		printf("[CM] block verified boot commands: fail: "
		"r0:0x%llx, r1:0x%llx, r2:0x%llx, r3:0x%llx\n", r0, r1, r2, r3);
		return ret;
	}

	printf("[CM] block verified boot commands: success\n");

	return ret;
}

/*****************************************************************************/
/* Test functions for Secure Boot & RootOfTrust                              */
/*****************************************************************************/
static uint8_t rsa_sb_pubkey[] = {
	0x00, 0x01, 0x00, 0x00, 0x17, 0x31, 0xba, 0x3e, 0x65, 0x9e,
	0x8a, 0xfa, 0xa9, 0x47, 0x2e, 0x26, 0x5f, 0x4a, 0x50, 0x4b,
	0x5d, 0xd8, 0x03, 0x4e, 0x00, 0x61, 0x72, 0xc8, 0x7a, 0xfb,
	0x7d, 0x29, 0x33, 0x29, 0x41, 0x13, 0x39, 0x64, 0x82, 0xda,
	0xb8, 0xcb, 0x06, 0x27, 0x1c, 0x50, 0x54, 0x3f, 0x30, 0x44,
	0x21, 0x05, 0x6b, 0x07, 0x29, 0xfd, 0x8e, 0x7f, 0xdd, 0xec,
	0x86, 0xca, 0x83, 0x0e, 0x1d, 0xa4, 0x6f, 0x99, 0x4e, 0x7c,
	0xc0, 0x5b, 0x2f, 0x72, 0xe6, 0xc6, 0xb4, 0xba, 0x78, 0x01,
	0x53, 0x4c, 0xbc, 0x47, 0xff, 0x37, 0x61, 0xd4, 0x88, 0x4c,
	0x1b, 0xbf, 0x31, 0x6c, 0xa7, 0xa8, 0xfb, 0xab, 0x24, 0x6a,
	0xde, 0x1e, 0x36, 0x89, 0x67, 0xec, 0x15, 0x55, 0x08, 0xfc,
	0xd5, 0xea, 0xa3, 0x36, 0x50, 0x1b, 0x28, 0xaf, 0x69, 0xf7,
	0x65, 0xa6, 0x0b, 0x10, 0x30, 0x25, 0x5a, 0x48, 0x72, 0x59,
	0xe7, 0x9d, 0xd6, 0x4b, 0x96, 0xb3, 0x28, 0x50, 0xb6, 0xce,
	0xe0, 0xca, 0x8e, 0xc6, 0x87, 0x9c, 0xa6, 0x83, 0xc7, 0x99,
	0x23, 0xf0, 0x6f, 0x4f, 0x45, 0x85, 0x35, 0xef, 0x49, 0x89,
	0x82, 0xce, 0x34, 0x8d, 0x72, 0xd2, 0x94, 0x03, 0xe4, 0x39,
	0x19, 0x63, 0x5e, 0x6b, 0xf2, 0xde, 0x98, 0x44, 0x14, 0xa4,
	0xc9, 0x6a, 0xb0, 0xac, 0xe8, 0xb1, 0x63, 0xf1, 0xbd, 0xd1,
	0xf4, 0x99, 0x0c, 0xea, 0xe4, 0x48, 0x66, 0xaa, 0x2b, 0x53,
	0x27, 0xc7, 0xdd, 0x67, 0x31, 0x81, 0x99, 0x5b, 0xf2, 0x16,
	0xd4, 0x7e, 0xa7, 0x74, 0xc2, 0xdc, 0x7b, 0xbe, 0x3a, 0x30,
	0x11, 0x37, 0x0e, 0x49, 0x77, 0xff, 0x10, 0x55, 0x1f, 0x62,
	0xbd, 0x15, 0xfc, 0x37, 0x1e, 0x85, 0x9f, 0xa0, 0x52, 0x6b,
	0x43, 0x9c, 0x24, 0x7d, 0x01, 0x93, 0x10, 0x62, 0xa3, 0x29,
	0x8d, 0x86, 0x9f, 0x6c, 0x11, 0x72, 0x9e, 0x74, 0x50, 0x86,
	0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00
};

uint64_t cm_secure_boot_self_test(void)
{
	uint32_t ret = RV_SUCCESS;
	uint32_t sb_os_version = 0x00101010;
	uint32_t sb_patch_version = 0x00201611;
	uint32_t sb_device_state = LOCKED;
	uint32_t sb_boot_state = GREEN;

	SB_PARAM_ROOT_OF_TRUST_ST root_of_trust;
	memset((uint8_t *)&root_of_trust, 0, sizeof(SB_PARAM_ROOT_OF_TRUST_ST));

	printf("[CM] TEST 01/11: call cm_secure_boot_set_os_version()\n");
	ret = cm_secure_boot_set_os_version(sb_os_version, sb_patch_version);
	if (ret != RV_SUCCESS) {
		printf("[CM] TEST_FAIL: cm_secure_boot_set_os_version() "
			"is failed: 0x%x\n", ret);
	}

	printf("[CM] TEST 02/11: call cm_secure_boot_set_device_state()\n");
	ret = cm_secure_boot_set_device_state(sb_device_state);
	if (ret != RV_SUCCESS) {
		printf("[CM] TEST_FAIL: cm_secure_boot_set_device_state() "
			"is failed: 0x%x\n", ret);
	}

	printf("[CM] TEST 03/11: call cm_secure_boot_set_boot_state()\n");
	ret = cm_secure_boot_set_boot_state(sb_boot_state);
	if (ret != RV_SUCCESS) {
		printf("[CM] TEST_FAIL: cm_secure_boot_set_boot_state() "
			"is failed: 0x%x\n", ret);
	}

	printf("[CM] TEST 04/11: call cm_secure_boot_set_pubkey()\n");
	ret = cm_secure_boot_set_pubkey(rsa_sb_pubkey, sizeof(rsa_sb_pubkey));
	if (ret != RV_SUCCESS) {
		printf("[CM] TEST_FAIL: cm_secure_boot_set_pubkey() "
			"is failed: 0x%x\n", ret);
	}

	printf("[CM] TEST 05/11: call cm_secure_boot_erase_pubkey()\n");
	ret = cm_secure_boot_erase_pubkey();
	if (ret != RV_SUCCESS) {
		printf("[CM] TEST_FAIL: cm_secure_boot_erase_pubkey() "
			"is failed: 0x%x\n", ret);
	}

	/* test block command */
	printf("[CM] TEST 06/11: call cm_secure_boot_block_cmd()\n");
	ret = cm_secure_boot_block_cmd();
	if (ret != RV_SUCCESS) {
		printf("[CM] TEST FAIL: cm_secure_boot_block_cmd() "
			"is failed: 0x%x\n", ret);
	}

	printf("[CM] TEST 07/11: call cm_secure_boot_set_os_version()\n");
	ret = cm_secure_boot_set_os_version(sb_os_version, sb_patch_version);
	if (ret != RV_SB_CMD_BLOCKED) {
		printf("[CM] TEST_FAIL: cm_secure_boot_set_os_version() "
			"is not blocked: 0x%x\n", ret);
	} else {
		printf("[CM] TEST_PASS: cm_secure_boot_set_os_version() "
			"is blocked\n");
	}

	printf("[CM] TEST 08/11: call cm_secure_boot_set_device_state()\n");
	ret = cm_secure_boot_set_device_state(sb_device_state);
	if (ret != RV_SB_CMD_BLOCKED) {
		printf("[CM] TEST_FAIL: cm_secure_boot_set_device_state() "
			"is not blocked: 0x%x\n", ret);
	} else {
		printf("[CM] TEST_PASS: cm_secure_boot_set_device_state() "
			"is blocked\n");
	}

	printf("[CM] TEST 09/11: call cm_secure_boot_set_boot_state()\n");
	ret = cm_secure_boot_set_boot_state(sb_boot_state);
	if (ret != RV_SB_CMD_BLOCKED) {
		printf("[CM] TEST_FAIL: cm_secure_boot_set_boot_state() "
			"is not blocked: 0x%x\n", ret);
	} else {
		printf("[CM] TEST_PASS: cm_secure_boot_set_boot_state() "
			"is blocked\n");
	}

	printf("[CM] TEST 10/11: call cm_secure_boot_set_pubkey()\n");
	ret = cm_secure_boot_set_pubkey(rsa_sb_pubkey, sizeof(rsa_sb_pubkey));
	if (ret != RV_SB_CMD_BLOCKED) {
		printf("[CM] TEST_FAIL: cm_secure_boot_set_pubkey() "
			"is not blocked: 0x%x\n", ret);
	} else {
		printf("[CM] TEST_PASS: cm_secure_boot_set_pubkey() "
			"is blocked\n");
	}

	printf("[CM] TEST 11/11: call cm_secure_boot_erase_pubkey()\n");
	ret = cm_secure_boot_erase_pubkey();
	if (ret != RV_SB_CMD_BLOCKED) {
		printf("[CM] TEST FAIL: cm_secure_boot_erase_pubkey() "
			"is not blocked: 0x%x\n", ret);
	} else {
		printf("[CM] TEST_PASS: cm_secure_boot_erase_pubkey() "
			"is blocked\n");
	}

	return ret;
}
