/**
 * @file  rpmb.h
 * @brief CryptoManager wrapper functions for RPMB security
 *
 * Copyright (c) 2014 Samsung Electronics Co., Ltd.
 *
 * This software is proprietary of Samsung Electronics.
 * No part of this software, either material or conceptual may be copied
 * or distributed, transmitted, transcribed, stored in a retrieval system
 * or translated into any human or computer language in any form by any means,
 * electronic, mechanical, manual or otherwise, or disclosed to third parties
 * without the express written permission of Samsung Electronics.
 */

#ifndef __RPMB_H__
#define __RPMB_H__

#include <arch/arm64.h>
#include <platform/mmu/mmu_func.h>
/* For debugging */
//#define RPMB_DBG      1

/* SMC id for CryptoManager function call */
#include <platform/cm_api.h>
//#define RPMB_DEBUG
#define ENABLE_CM_NONCE
#define SMC_CM_RPMB				0x1018
#define SMC_CM_DRBG				0x1019
#define SMC_SRPMB_SET_PROVISION			0x3818
#define SMC_SRPMB_GET_PARTITION_INFO		0x3823

/* key and hmac length definition */
#define RPMB_KEY_LEN				32
#define RPMB_HMAC_LEN				32

/* define maximum retry count */
#define MAX_SMC_RETRY_CNT			0x5000

/* error codes from CryptoManager F/W */
#define RV_SUCCESS				0
#define RV_SYNC_AES_BUSY			0x20000
#define RV_RPMB_ERROR_CODE_BASE			0x40200
#define RV_RPMB_INVALID_CMD			0x40201
#define RV_RPMB_INVALID_PARAM			0x40202
#define RV_RPMB_PARAM_NULL_POINTER		0x40203
#define RV_RPMB_INVALID_KEY_LEN			0x40204
#define RV_RPMB_KEY_BLOCKED			0x40205
#define RV_RPMB_HMAC_BLOCKED			0x40206
#define RV_RPMB_RI_TABLE_NOT_INITIALIZED        0x40300
#define RV_RPMB_INVALID_ROLLBACK_INDEX          0x40301
#define RV_RPMB_PERSIST_NAME_NOT_FOUND          0x40302
#define RV_RPMB_INVALID_PERSIST_DATA_SIZE       0x40303


/* error code made by dev/rpmb.c*/
#define RV_SPRMB_HMAC_VERIFICATION_FAIL		0x50A01
#define RV_SRPMB_NONCE_VERIFICATION_FAIL	0x50A02
#define RV_SRPMB_BIO_OPEN_FAIL			0x50A03
#define RV_SRPMB_SECU_PROT_OUT_FAIL		0x50B01
#define RV_SRPMB_SECU_PROT_IN_FAIL		0x50B02

/* error code for bootloader */
#define RV_BOOT_RPMB_EXCEED_SMC_RETRY_CNT	0x80100
#define RPMB_AUTHEN_KEY_ERROR			0x7
#define RPMB_AUTHEN_KEY_NOT_PROVISIONED		0x700
#define RPMB_AUTHEN_KEY_PROVISIONED		0x701

#define RPMB_AUTHEN_KEY_MISMATCH		0x02
#define RPMB_AUTHEN_KEY_NOT_PGAM		0x07

/* RPMB PARTITION PARAMETER */
#define RPMB_DEFAULT_MAX_PARTITION		32
#define RPMB_DEFAULT_MAX_BLOCK_PER_PARTITION	1024

/*
 * if the caller of this fucntion is executed on the cache enabled memory area
 * below definitions and macros should be defined
 * and also mercos should be defined
 */
#define CACHE_ENABLED

#define CACHE_WRITEBACK_SHIFT_6		(6)
#define CACHE_WRITEBACK_SHIFT_7		(7)

#define CACHE_WRITEBACK_GRANULE_64	(1 << CACHE_WRITEBACK_SHIFT_6)
#define CACHE_WRITEBACK_GRANULE_128	(1 << CACHE_WRITEBACK_SHIFT_7)

#define CACHE_CLEAN(addr, len)		clean_dcache_range((unsigned long long)(addr), (unsigned long long)((addr) + (len)))
#define CACHE_INVALIDATE(addr, len)	invalidate_dcache_range((unsigned long long)(addr), (unsigned long long)((addr) + (len)))

/* RPMB MAGIC parameter, used in booting sequence */
#define RPMB_BL_PARTITION		4
#define RPMB_MAGIC_BLK			10
#define RPMB_TEST_MAGIC			0x544f4f42424d5052 /*RPMBBOOT (reverse)*/

/* RPMB function number */
enum {
	RPMB_GET_KEY = 1,
	RPMB_BLOCK_KEY,
	RPMB_GET_HMAC,
	RPMB_BLOCK_HMAC,
	RPMB_GET_HMAC_SWD,
};

/* data definition for RPMB hmac */
typedef struct rpmb_param {
	uint64_t input_data;
	uint64_t input_len;
	uint64_t output_data;
} rpmb_param __attribute__((__aligned__(CACHE_WRITEBACK_GRANULE_128)));

struct rpmb_packet {
	u16 request;
	u16 result;
	u16 count;
	u16 address;
	u32 write_counter;
	u8 nonce[16];
	u8 data[256];
	u8 Key_MAC[32];
	u8 stuff[196];
} __attribute__((__aligned__(CACHE_WRITEBACK_GRANULE_128)));

typedef struct random_ctx {
        u64 output;
        u64 len;
        u64 reserved[2];
} RANDOM_CTX;

/* Functions */
uint32_t get_RPMB_key(size_t key_len, uint8_t *rpmb_key);
uint32_t block_RPMB_key(void);
uint32_t get_RPMB_hmac(const uint8_t *input_data, size_t input_len, uint8_t *output_data);
uint32_t block_RPMB_hmac(void);
void rpmb_key_programming(void);
int rpmb_load_boot_table(void);
int rpmb_get_rollback_index(size_t loc, uint64_t *rollback_index);
int rpmb_set_rollback_index(size_t loc, uint64_t rollback_index);
int rpmb_read_persistent_value(const char *name,
		size_t buffer_size,
		uint8_t *out_buffer,
		size_t *out_num_bytes_read);
int rpmb_write_persistent_value(const char *name,
		size_t value_size,
		const uint8_t *value);
int rpmb_get_lock_state(uint32_t *state);
int rpmb_set_lock_state(uint32_t state);
uint32_t get_RPMB_partition_info(uint32_t *rpmb_max_partition, uint32_t *rpmb_partition_per_block);
static int rpmb_read_block(int addr, int blkcnt, u8 *buf, u16 *result);
static int rpmb_write_block(int addr, int blkcnt, u8 *buf, u16 *result);

/* Parameter */
#define HMAC_SIZE			32
#define NONCE_SIZE      		16
#define DATA_SIZE			256
#define HMAC_CALC_SIZE			284
#define HMAC_CALC_SIZE_128_GRANULE	384
#define HMAC_START_BYTE			196
#define DATA_START_BYTE			228
#define RPMB_SIZE			512
#define RPMB_MAX_BLOCK			32

#endif				/* __RPMB_H__ */
