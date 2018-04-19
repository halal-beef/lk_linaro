/*
 * Copyright (C) 2014 Samsung Electronics
 * Przemyslaw Marczak <p.marczak@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __UUID_H__
#define __UUID_H__

/* This is structure is in big-endian */
struct uuid {
	unsigned int time_low;
	unsigned short time_mid;
	unsigned short time_hi_and_version;
	unsigned char clock_seq_hi_and_reserved;
	unsigned char clock_seq_low;
	unsigned char node[6];
} __attribute__ ((__packed__));

enum {
	UUID_STR_FORMAT_STD,
	UUID_STR_FORMAT_GUID
};

#define UUID_STR_LEN		36
#define UUID_BIN_LEN		sizeof(struct uuid)

#define UUID_VERSION_MASK	0xf000
#define UUID_VERSION_SHIFT	12
#define UUID_VERSION		0x4

#define UUID_VARIANT_MASK	0xc0
#define UUID_VARIANT_SHIFT	7
#define UUID_VARIANT		0x1

unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base);
unsigned long long simple_strtoull (const char *cp, char **endp, unsigned int base);
int uuid_str_valid(const char *uuid);
int uuid_str_to_bin(char *uuid_str, unsigned char *uuid_bin, int str_format);
void uuid_bin_to_str(unsigned char *uuid_bin, char *uuid_str, int str_format);
void gen_rand_uuid(unsigned char *uuid_bin);
void gen_rand_uuid_str(char *uuid_str, int str_format);
#endif
