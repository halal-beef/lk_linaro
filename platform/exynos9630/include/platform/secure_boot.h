#ifndef _SECURE_BOOT_H_
#define _SECURE_BOOT_H_

#include "../../../../lib/libavb/libavb.h"

/******************************************************************************/
/* Definition value */
/******************************************************************************/
/* SMC ID */
#define SMC_CM_SECURE_BOOT		(0x101D)

/* Error return */
#define AVB_ERROR_RP_UPDATE_FAIL	(0xFDAA4001)
#define AVB_ERROR_INVALID_COLOR		(0xFDAA4002)
#define AVB_ERROR_AVBKEY_LEN_ZERO	(0xFDAA4003)
#define AVB_ERROR_INVALID_KEYNAME_SIZE	(0xFDAA4004)

/* secure boot crypto variable */
#define SHA1_DIGEST_LEN                 (20)
#define SHA1_BLOCK_LEN                  (64)
#define SHA256_DIGEST_LEN               (32)
#define SHA256_BLOCK_LEN                (64)
#define SHA512_DIGEST_LEN               (64)
#define SHA512_BLOCK_LEN                (128)

#define SB_BL1_RP_COUNT_OFFSET		(16)
#define SB_SB_CONTEXT_LEN		(1024)
#define SB_SIGN_FIELD_HEADER_SIZE	(16)
#define SB_MAX_RSA_KEY_N_LEN		(256)
#define SB_MAX_RSA_SIGN_LEN		(SB_MAX_RSA_KEY_N_LEN)

/* keystorage variable */
#define SB_MAX_PUBKEY_LEN		(1056)
#define SB_KST_MAX_KEY_NAME_LEN		(8)

/* AVB variable */
#define AVB_PRELOAD_BASE		(0xA0000000)
#define AVB_CMD_MAX_SIZE		(1024)
/*
 * "androidboot.verifiedbootstate=" is 30
 * AVB_VBS_MAX_SIZE - AVB_COLOR_MAX_SIZE = 30
 */
#define AVB_VBS_MAX_SIZE		(40)
#define AVB_COLOR_MAX_SIZE		(10)

/******************************************************************************/
/* Cache operation */
/******************************************************************************/
#define CACHE_WRITEBACK_SHIFT_6		(6)
#define CACHE_WRITEBACK_SHIFT_7		(7)

#define CACHE_WRITEBACK_GRANULE_64	(1 << CACHE_WRITEBACK_SHIFT_6)
#define CACHE_WRITEBACK_GRANULE_128	(1 << CACHE_WRITEBACK_SHIFT_7)

#define FLUSH_DCACHE_RANGE(addr, length) \
	clean_dcache_range((unsigned long long)addr, (unsigned long long)(addr + length))
#define INV_DCACHE_RANGE(addr, length) \
	invalidate_dcache_range((unsigned long long)addr, (unsigned long long)(addr + length))

/******************************************************************************/
/* Secure Boot context used by EL3 */
/******************************************************************************/
enum {
	ALG_SHA1,
	ALG_SHA256,
	ALG_SHA512,
};

struct ace_hash_ctx {
	int alg;
	size_t buflen;
	unsigned char buffer[SHA512_BLOCK_LEN];
	unsigned int state[SHA512_DIGEST_LEN/4];
	unsigned int prelen_high;
	unsigned int prelen_low;
};

typedef struct
{
	uint64_t ns_buf_addr;
	uint64_t ns_buf_size;
	uint8_t keyname[SB_KST_MAX_KEY_NAME_LEN];
	uint64_t keyname_size;
	uint8_t reserved[32];
} KST_PUBKEY_ST;

uint32_t el3_sss_hash_digest(
	uint32_t addr,
	uint32_t size,
	uint32_t alg,
	uint8_t *hash);

uint32_t el3_sss_hash_init(
	uint32_t alg,
	struct ace_hash_ctx *ctx);

uint32_t el3_sss_hash_update(
	uint32_t addr,
	uint32_t remain_size,
	uint32_t unit_size,
	struct ace_hash_ctx *ctx,
	uint32_t done_flag);

uint32_t el3_sss_hash_final(
	struct ace_hash_ctx *ctx,
	uint8_t *hash);

uint32_t el3_verify_signature_using_image(
	uint64_t signed_img_ptr,
	uint64_t signed_img_len,
	uint64_t signed_img_type);

void clean_dcache_range(unsigned long long start, unsigned long long end);
void invalidate_dcache_range(unsigned long long start, unsigned long long end);

/******************************************************************************/
/* Secure Boot context used by LDFW */
/******************************************************************************/

/******************************************************************************/
/* Android verified boot */
/******************************************************************************/
void set_avbops(void);

uint32_t avb_main(const char *suffix, char *cmdline, char *verifiedbootstate);

uint32_t get_ops_addr(struct AvbOps **ops_addr);

uint32_t get_avbkey_trust(void);

uint32_t sb_get_avb_key(uint8_t *avb_pubkey, uint64_t pubkey_size,
		const char *keyname);

#endif /* _SECURE_BOOT_H_ */
