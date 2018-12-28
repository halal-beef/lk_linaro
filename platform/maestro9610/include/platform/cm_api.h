/*
 * Copyright@ Samsung Electronics Co. LTD
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied or distributed, transmitted,
 * transcribed, stored in a retrieval system or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed
 * to third parties without the express written permission of Samsung Electronics.
 */

#ifndef _CM_API_H_
#define _CM_API_H_

/*****************************************************************************/
/* defines for general purpose                                               */
/*****************************************************************************/
#define VIRT_TO_PHYS(_virt_addr_)		((uint64_t)(_virt_addr_))

#define FLUSH_DCACHE_RANGE(addr, length)
#define INV_DCACHE_RANGE(addr, length)

#define CACHE_WRITEBACK_SHIFT			6
#define CACHE_WRITEBACK_GRANULE			(1 << CACHE_WRITEBACK_SHIFT)

#define MASK_LSB8				0x00000000000000FFULL
#define MASK_LSB16				0x000000000000FFFFULL
#define MASK_LSB32				0x00000000FFFFFFFFULL

/* SMC ID for CM APIs */
#define SMC_AARCH64_PREFIX				(0xC2000000)
#define SMC_CM_SECURE_BOOT				(0x101D)

#define SHA256_DIGEST_LEN				(32)

/*****************************************************************************/
/* ERROR Codes                                                               */
/*****************************************************************************/
#define RV_SUCCESS					0
#define RV_SB_LDFW_NOT_LOADED				0xFFFFFFFF
#define RV_SB_CMD_BLOCKED				0x51007

/*****************************************************************************/
/* CM API functions for RootOfTrust                                          */
/*****************************************************************************/
/* define device state */
enum device_state {
	UNLOCKED,
	LOCKED,
};

/* define boot state */
enum boot_state {
	GREEN,
	YELLOW,
	ORANGE,
	RED,
};

/* define secure boot commands */
enum secure_boot_cmd {
	SB_CHECK_SIGN_AP,
	SB_CHECK_SIGN_CP,
	SB_SELF_TEST,
	SB_SET_PUBLIC_KEY,
	SB_SET_DEVICE_STATE,
	SB_ERASE_PUBLIC_KEY,
	SB_GET_BOOT_INFO,
	SB_CHECK_SIGN_NWD,
	SB_GET_ROOT_OF_TRUST_INFO,
	SB_SET_BOOT_STATE,
	SB_SET_OS_VERSION,
	SB_BLOCK_CMD,
	SB_CHECK_SIGN_NWD_WITH_PUBKEY,
	SB_CHECK_SIGN_NWD_AND_RP, /* Automotive */
	SB_CHECK_SIGN_NWD_WITH_HASH_AND_RP, /* Automotive */
	SB_CHECK_SIGN_CP_WITH_HASH,
	SB_GET_AVB_KEY,
	SB_SET_VENDOR_BOOT_VERSION,
};

typedef struct {
	uint32_t	verified_boot_state;
	uint32_t	device_locked;
	uint32_t	os_version;
	uint32_t	os_patch_level;
	uint8_t		verified_boot_key[SHA256_DIGEST_LEN];
	uint32_t	vendor_patch_level;
	uint32_t	boot_patch_level;
	uint64_t	reserved[3];
} SB_PARAM_ROOT_OF_TRUST_ST __attribute__((__aligned__(CACHE_WRITEBACK_GRANULE)));

uint64_t cm_secure_boot_set_pubkey(uint8_t *pub_key_ptr, uint32_t pub_key_len);
uint64_t cm_secure_boot_erase_pubkey(void);
uint64_t cm_secure_boot_set_os_version(uint32_t os_version,
		uint32_t patch_month_year);
uint64_t cm_secure_boot_set_vendor_boot_version(uint32_t vendor_patch_level,
		uint32_t boot_patch_level);
uint64_t cm_secure_boot_set_device_state(uint32_t state);
uint64_t cm_secure_boot_set_boot_state(uint32_t state);
uint64_t cm_secure_boot_get_root_of_trust(
		SB_PARAM_ROOT_OF_TRUST_ST *root_of_trust);
uint64_t cm_secure_boot_block_cmd(void);
uint64_t cm_secure_boot_self_test(void);

#endif /* _CM_API_H_ */
