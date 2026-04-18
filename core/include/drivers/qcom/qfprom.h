/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __DRIVERS_QFPROM_H
#define __DRIVERS_QFPROM_H

#include <stdbool.h>
#include <stdint.h>
#include <tee_api_types.h>
#include <types_ext.h>
#include <tmel_auth/tmel_auth.h>

#define FUSEPROV_SECDAT_MAGIC1				0x3B7251CA
#define FUSEPROV_SECDAT_MAGIC2				0x2A126F29

#define FEC_MSB_DATA_MASK				0x00FFFFFF

#define QFPROM_CORR_ADDR_OFFSET				0x4000

/*
 * Fuse Region Types
 */
enum fuseprov_v3_region_etype {
	FUSEPROV_V3_REGION_TYPE_OEM_SEC_BOOT     = 0x00000000,
	FUSEPROV_V3_REGION_TYPE_OEM_PK_HASH      = 0x00000001,
	FUSEPROV_V3_REGION_TYPE_SEC_HW_KEY       = 0x00000002,
	FUSEPROV_V3_REGION_TYPE_OEM_CONFIG       = 0x00000003,
	FUSEPROV_V3_REGION_TYPE_READ_PERM        = 0x00000004,
	FUSEPROV_V3_REGION_TYPE_WRITE_PERM       = 0x00000005,
	FUSEPROV_V3_REGION_TYPE_FEC_EN           = 0x00000006,
	FUSEPROV_V3_REGION_TYPE_ANTI_ROLLBACK    = 0x00000007,
	FUSEPROV_V3_REGION_TYPE_IMAGE_ENCR_KEY   = 0x00000008,
	FUSEPROV_V3_REGION_TYPE_MRC_2_0          = 0x00000009,
	FUSEPROV_V3_REGION_TYPE_OEM_SPARE        = 0x0000000A,
	FUSEPROV_V3_REGION_TYPE_OEM_PRODUCT_SEED = 0x0000000B,
	FUSEPROV_V3_REGION_TYPE_TME_RW_PERM      = 0x0000000C,
	FUSEPROV_V3_REGION_TYPE_TME_FEC_EN       = 0x0000000D,
	FUSEPROV_V3_REGION_TYPE_TME_OEM          = 0x0000000E,
	FUSEPROV_V3_REGION_TYPE_TME_SPARE        = 0x0000000F,
	FUSEPROV_V3_REGION_TYPE_MAX              = 0x00000010,
};

/*
 * Fuse Categories order
 */
enum fuseprov_category_type {
	FUSEPROV_CATEGORY_GENERAL,
	FUSEPROV_CATEGORY_SHK,
	FUSEPROV_CATEGORY_OEM_PRODUCT_SEED,
	FUSEPROV_CATEGORY_OEM_SPARE_RAND,
	FUSEPROV_CATEGORY_OEM_CONFIG,
	FUSEPROV_CATEGORY_SECBOOT,
	FUSEPROV_CATEGORY_FEC_EN,
	FUSEPROV_CATEGORY_READ_PERM,
	FUSEPROV_CATEGORY_WRITE_PERM,
	FUSEPROV_CATEGORY_TME_OEM,
	FUSEPROV_CATEGORY_TME_FEC_EN,
	FUSEPROV_CATEGORY_TME_RW_PERM,
	FUSEPROV_CATEGORY_MAX,
};

struct fuseprov_secdat_hdr {
	uint32_t magic1;
	uint32_t magic2;
	uint32_t revision;
	uint32_t num_entries;
};

struct fuseprov_qfuse_entry {
	uint32_t region_type;
	uint32_t fuse_addr;
	uint32_t lsb_val;
	uint32_t msb_val;
	uint32_t operation_type;
};

/*
 * FEC-enabled fuse range structure
 */
struct fec_fuse_range {
	uint32_t start_addr;
	uint32_t end_addr;
};

/*
 * QFPROM Core Functions
 */

TEE_Result qfprom_read_row(uint32_t row_address, bool corrected,
			   uint32_t *lsb_val, uint32_t *msb_val);

TEE_Result qfprom_write_row(uint32_t row_address, uint32_t lsb_val,
			    uint32_t msb_val);

TEE_Result prov_qfprom_fuses(vaddr_t sec_dat_addr, uint32_t sec_dat_size);

TEE_Result prov_qfprom_fuses_with_auth(vaddr_t elf_vaddr,
				       uint32_t elf_metadata_size,
				       struct TmeRegion_t *regions,
				       uint32_t region_count);

TEE_Result qfprom_write_tme_oem_mrc_state_vector(uint32_t raw_row_address,
						 uint32_t *row_data);

#endif /* __DRIVERS_QFPROM_H */
