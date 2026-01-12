/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __TMEKM_CLIENT_H
#define __TMEKM_CLIENT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <tee_api_types.h>

/*
 * TME Key Management Client Interface
 * Provides DeriveKey, DistributeKey, and ClearKey APIs for OP-TEE
 */

/* Maximum sizes for KDF parameters */
#define TME_KDF_SW_CONTEXT_BYTES_MAX	128
#define TME_KDF_SALT_LABEL_BYTES_MAX	64

/* Key Handle Type */
typedef uint32_t tme_key_handle_t;

#define TME_KEY_HANDLE_INVALID		0xFFFFFFFF
#define TME_KEY_HANDLE_ALLOC		0xAAAAAAAA

/* Key ID definitions */
#define TME_KID_CHIP_RAND_BASE		9
#define TME_KID_OEM_PRODUCT_SEED	12
#define TME_KID_L2_KEYWRAPSVC		6
#define TME_KID_L2_SECURESTRGSVC	7

/* Key Destination IDs */
#define TME_KD_ICE_ENDPOINT			0x80
#define TME_KD_TCSR_ENDPOINT			0x100

/* Key Policy Structure */
struct tme_key_policy {
	uint32_t low;
	uint32_t high;
};

/* KDF Security Context Flags */
#define TME_KSC_SOCSecBootState		0x00000002
#define TME_KSC_SOCDebugState		0x00000004
#define TME_KSC_TMELifecycleState	0x00000008
#define TME_KSC_SWContext		0x00000020
#define TME_KSC_ChildKeyPolicy		0x00000040

/* KDF Algorithm */
#define TME_KAL_KDF_NIST		0x80000
#define TME_KAL_SHA512_HMAC		0x58000

/* Key Policy Macros */
#define TME_KT_Symmetric		0x00
#define TME_KL_128			(0x01 << 3)
#define TME_KL_256			(0x04 << 3)
#define TME_KP_Generic			(0x00 << 7)
#define TME_KOP_Encryption		(0x01 << 10)
#define TME_KOP_Decryption		(0x02 << 10)
#define TME_KSL_HWKey			(0x02 << 20)
#define TME_KO_TZ			(0x02 << 26)
#define TME_KLI_NP_CU			(0x01 << 30)
#define TME_KLI_NA			(0x00 << 30)

#define TME_KPV_Version			(0x04 << 12)
#define TME_KAU_TZ			(0x01 << 18)

/* AES Algorithm Types */
#define TME_KAL_AES128_ECB		(0x00 << 14)
#define TME_KAL_AES256_ECB		(0x01 << 14)
#define TME_KAL_AES128_CBC		(0x04 << 14)
#define TME_KAL_AES256_CBC		(0x05 << 14)
#define TME_KAL_AES128_XTS		(0x0F << 14)
#define TME_KAL_AES256_XTS		(0x10 << 14)

/* Credential Slot */
#define TME_CRED_SLOT_ID_NONE		0

/* KDF Specification Structure */
struct tme_kdf_spec {
	uint32_t kdf_algo;
	uint32_t input_key;
	uint32_t mix_key;
	uint32_t l2_key;
	struct tme_key_policy policy;
	uint8_t sw_context[TME_KDF_SW_CONTEXT_BYTES_MAX];
	uint32_t sw_context_length;
	uint32_t security_context;
	uint8_t salt_label[TME_KDF_SALT_LABEL_BYTES_MAX];
	uint32_t salt_label_length;
	uint32_t prf_digest_algo;
};

/* Sequencer Status Response */
struct tme_sequencer_status {
	uint32_t tme_error_status;
	uint32_t seq_error_status;
	uint32_t seq_kp_error_status0;
	uint32_t seq_kp_error_status1;
	uint32_t seq_rsp_status;
};

/* TME Key Management Message Structures */
struct tme_km_derive_key_msg {
	struct {
		uint32_t key_id;
		uint32_t kdf_info_pdata;
		uint32_t kdf_info_length;
		uint32_t cred_slot;
	} input;
	struct {
		uint32_t key_id;
		uint32_t status;
		struct tme_sequencer_status seq_status;
	} output;
};

struct tme_km_distribute_key_msg {
	struct {
		uint32_t key_id;
		uint32_t dst_id;
		uint32_t dst_key_index;
	} input;
	struct {
		uint32_t status;
		struct tme_sequencer_status seq_status;
	} output;
};

struct tme_km_clear_key_msg {
	struct {
		uint32_t key_id;
	} input;
	struct {
		uint32_t status;
		struct tme_sequencer_status seq_status;
	} output;
};

/*
 * Derive a key using KDF specification
 *
 * @kdf_spec: KDF specification structure
 * @key_handle: Output parameter for derived key handle
 *
 * Returns: TEE_SUCCESS on success, error code otherwise
 */
TEE_Result tme_km_derive_key(const struct tme_kdf_spec *kdf_spec,
			     tme_key_handle_t *key_handle);

/*
 * Distribute a key to a crypto engine
 *
 * @key_handle: Handle of the key to distribute
 * @dst_id: Destination crypto engine ID (TME_KD_ICE_ENDPOINT or TME_KD_TCSR_ENDPOINT)
 * @dst_key_index: Destination key slot index (0 or 1)
 *
 * Returns: TEE_SUCCESS on success, error code otherwise
 */
TEE_Result tme_km_distribute_key(tme_key_handle_t key_handle,
				 uint32_t dst_id,
				 uint32_t dst_key_index);

/*
 * Clear a key from TME key storage
 *
 * @key_handle: Handle of the key to clear
 *
 * Returns: TEE_SUCCESS on success, error code otherwise
 */
TEE_Result tme_km_clear_key(tme_key_handle_t key_handle);

/*
 * Read Hardware Key from TCSR registers and clear them
 *
 * @key: Buffer to store the read key. Required size:
 *       - slot_id 0: TCSR_FUSE_PRI_HW_KEY_REG_COUNT x uint32_t
 *       - slot_id 1: TCSR_FUSE_SEC_HW_KEY_REG_COUNT x uint32_t
 * @key_size: Size of the key buffer in bytes (for validation)
 * @slot_id: Slot ID (0 for Primary HW Key, 1 for Secondary HW Key)
 *
 * Returns: TEE_SUCCESS on success, error code otherwise
 */
TEE_Result tme_km_read_tcsr_key_and_clear(uint32_t *key, uint32_t key_size,
					  uint32_t slot_id);

/*
 * Create a key policy structure based on specified parameters
 *
 * @key_length: Key length in bits (128 or 256)
 * @algo_mode: Algorithm mode (e.g., TME_KAL_AES128_ECB, TME_KAL_AES256_XTS, etc.)
 * @key_destination: Key destination ID (TME_KD_ICE_ENDPOINT or TME_KD_TCSR_ENDPOINT)
 * @lineage: Key lineage flags (TME_KLI_NP_CU or TME_KLI_NA)
 * @policy: Output parameter for the created key policy structure
 */
void tme_km_create_key_policy(uint32_t key_length,
			      uint32_t algo_mode,
			      uint32_t key_destination,
			      uint32_t lineage,
			      struct tme_key_policy *policy);

#endif /* __TMEKM_CLIENT_H */
