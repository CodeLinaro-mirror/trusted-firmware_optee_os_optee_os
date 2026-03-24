// SPDX-License-Identifier: ISC
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <drivers/qcom/qfprom.h>
#include <kernel/cache_helpers.h>
#include <mm/core_memprot.h>
#include <mm/core_mmu.h>
#include <mm/tee_mm.h>
#include <qfprom_target.h>
#include <string.h>
#include <tee_api.h>
#ifdef CFG_QCOM_TMEL_AUTH
#include <tmel_auth/tmel_auth.h>
#endif
#include <trace.h>
#include <util.h>

/*
 * calculate_fec_bits - Calculate FEC (Forward Error Correction) bits for a
 *                      56-bit data value.
 *
 * Uses a 7-bit Linear Feedback Shift Register (LFSR) to generate FEC bits
 * for error correction. The FEC bits are placed in bits [63:56] of the MSB.
 *
 * @lsb_data: Lower 32 bits of the fuse row data
 * @msb_data: Upper 24 bits of the fuse row data (bits [55:32])
 */
static uint32_t calculate_fec_bits(uint32_t lsb_data, uint32_t msb_data)
{
	uint8_t lfsr[7] = {0};
	uint32_t temp = 0;
	uint32_t fec_val = 0;
	uint64_t data = 0;
	int i = 0;

	/* Combine LSB and MSB into 56-bit data value */
	data = ((uint64_t)msb_data << 32) | lsb_data;

	/* Run LFSR for 56 bits */
	for (i = 0; i < 56; i++) {
		temp = lfsr[0] ^ ((data >> i) & 0x1);

		lfsr[0] = lfsr[1] ^ temp;
		lfsr[1] = lfsr[2];
		lfsr[2] = lfsr[3];
		lfsr[3] = lfsr[4];
		lfsr[4] = lfsr[5] ^ temp;
		lfsr[5] = lfsr[6];
		lfsr[6] = temp;
	}

	/* Combine FEC bits into a single value */
	for (i = 6; i >= 0; i--)
		fec_val |= (lfsr[i] << i);

	/* Place FEC bits in bits [63:56] and combine with data bits [55:32] */
	return (fec_val << 24) | msb_data;
}

static bool is_fec_enabled_for_row(uint32_t row_addr)
{
	uint32_t i = 0;

	/* Check if address falls within any FEC-enabled range */
	for (i = 0; i < FEC_NUM_RANGES; i++) {
		if (row_addr >= fec_enabled_ranges[i].start_addr &&
		    row_addr <= fec_enabled_ranges[i].end_addr)
			return true;
	}

	return false;
}

/*
 * get_fuse_category_for_region - Map a sec_dat region_type to a blow category.
 *
 *  Region type              | Category
 *  -------------------------|---------------------------
 *  OEM_PK_HASH  (0x01)      | SECBOOT
 *  SEC_HW_KEY   (0x02)      | SHK
 *  OEM_CONFIG   (0x03)      | OEM_CONFIG
 *  READ_PERM    (0x04)      | READ_PERM
 *  WRITE_PERM   (0x05)      | WRITE_PERM
 *  FEC_EN       (0x06)      | FEC_EN
 *  OEM_SPARE    (0x0A)      | OEM_SPARE_RAND
 *  OEM_PRODUCT_SEED (0x0B)  | OEM_PRODUCT_SEED
 *  TME_RW_PERM  (0x0C)      | TME_RW_PERM
 *  TME_FEC_EN   (0x0D)      | TME_FEC_EN
 *  TME_OEM      (0x0E)      | TME_OEM
 *  (all others)             | GENERAL
 */
static enum fuseprov_category_type
get_fuse_category_for_region(uint32_t region_type)
{
	switch (region_type) {
	case FUSEPROV_V3_REGION_TYPE_OEM_PK_HASH:
		return FUSEPROV_CATEGORY_SECBOOT;
	case FUSEPROV_V3_REGION_TYPE_SEC_HW_KEY:
		return FUSEPROV_CATEGORY_SHK;
	case FUSEPROV_V3_REGION_TYPE_OEM_CONFIG:
		return FUSEPROV_CATEGORY_OEM_CONFIG;
	case FUSEPROV_V3_REGION_TYPE_READ_PERM:
		return FUSEPROV_CATEGORY_READ_PERM;
	case FUSEPROV_V3_REGION_TYPE_WRITE_PERM:
		return FUSEPROV_CATEGORY_WRITE_PERM;
	case FUSEPROV_V3_REGION_TYPE_FEC_EN:
		return FUSEPROV_CATEGORY_FEC_EN;
	case FUSEPROV_V3_REGION_TYPE_OEM_PRODUCT_SEED:
		return FUSEPROV_CATEGORY_OEM_PRODUCT_SEED;
	case FUSEPROV_V3_REGION_TYPE_OEM_SPARE:
		return FUSEPROV_CATEGORY_OEM_SPARE_RAND;
	case FUSEPROV_V3_REGION_TYPE_TME_RW_PERM:
		return FUSEPROV_CATEGORY_TME_RW_PERM;
	case FUSEPROV_V3_REGION_TYPE_TME_FEC_EN:
		return FUSEPROV_CATEGORY_TME_FEC_EN;
	case FUSEPROV_V3_REGION_TYPE_TME_OEM:
		return FUSEPROV_CATEGORY_TME_OEM;
	default:
		return FUSEPROV_CATEGORY_GENERAL;
	}
}

/*
 * blow_fuse_region - Blow all fuse entries belonging to a given category.
 *
 * Iterates through all entries and writes those matching the specified
 * category. Applies FEC if enabled for the fuse row.
 */
static TEE_Result
blow_fuse_region(enum fuseprov_category_type category,
		 const struct fuseprov_qfuse_entry *entries,
		 uint32_t num_entries)
{
	TEE_Result res = TEE_SUCCESS;
	uint32_t i = 0;
	uint32_t row_data[2];
	uint32_t lsb_val, msb_val;
	bool fec_enabled;

	for (i = 0; i < num_entries; i++) {
		if (get_fuse_category_for_region(entries[i].region_type) !=
		    category)
			continue;

		if (entries[i].lsb_val == 0 && entries[i].msb_val == 0)
			continue;

		lsb_val = entries[i].lsb_val;
		msb_val = entries[i].msb_val;

		/* Check if this is the TME OEM MRC state vector fuse */
		if (entries[i].fuse_addr == TMEL_OEMMRCSTATEVECTOR_ADDR) {
			row_data[0] = lsb_val;
			row_data[1] = msb_val;

			IMSG("Writing MRC state vector at 0x%08x",
			     entries[i].fuse_addr);

			res = qfprom_write_tme_oem_mrc_state_vector(entries[i].fuse_addr,
								    row_data);
			if (res != TEE_SUCCESS) {
				EMSG("MRC vector write failed 0x%08x: %#"PRIx32,
				     entries[i].fuse_addr, res);
				return res;
			}
			continue;
		}

		fec_enabled = is_fec_enabled_for_row(entries[i].fuse_addr);

		if (fec_enabled) {
			msb_val &= FEC_MSB_DATA_MASK;
			msb_val = calculate_fec_bits(lsb_val, msb_val);
		}

		res = qfprom_write_row(entries[i].fuse_addr, lsb_val, msb_val);
		if (res != TEE_SUCCESS) {
			EMSG("Failed 0x%08x (cat %d): %#"PRIx32,
			     entries[i].fuse_addr, category, res);
			return res;
		}
	}

	return TEE_SUCCESS;
}

/*
 * provision_fuse_category - Provision fuses in the correct category order.
 *
 * @entries:     Pointer to the array of fuse entries.
 * @num_entries: Number of entries in the array.
 */
static TEE_Result
provision_fuse_category(const struct fuseprov_qfuse_entry *entries,
			uint32_t num_entries)
{
	TEE_Result res = TEE_SUCCESS;
	enum fuseprov_category_type category;

	for (category = FUSEPROV_CATEGORY_GENERAL;
	     category < FUSEPROV_CATEGORY_MAX;
	     category++) {
		res = blow_fuse_region(category, entries, num_entries);
		if (res != TEE_SUCCESS) {
			EMSG("Failed to blow category %d fuses", category);
			return res;
		}
	}

	return TEE_SUCCESS;
}

/*
 * parse_secdat_hdr - Parse and validate the sec_dat header.
 *
 * @secdat_buffer:   Virtual address of the sec_dat buffer.
 * @secdat_len:      Total byte length of the sec_dat buffer.
 * @entries_out:     Output: pointer to the first fuse entry.
 * @num_entries_out: Output: number of fuse entries.
 */
static TEE_Result
parse_secdat_hdr(vaddr_t secdat_buffer, uint32_t secdat_len,
		 const struct fuseprov_qfuse_entry **entries_out,
		 uint32_t *num_entries_out)
{
	const struct fuseprov_secdat_hdr *hdr = NULL;
	uint32_t entries_size_bytes = 0;

	hdr = (const struct fuseprov_secdat_hdr *)secdat_buffer;

	if (hdr->magic1 != FUSEPROV_SECDAT_MAGIC1 ||
	    hdr->magic2 != FUSEPROV_SECDAT_MAGIC2) {
		EMSG("secdat magic mismatch: 0x%x 0x%x",
		     hdr->magic1, hdr->magic2);
		return TEE_ERROR_BAD_FORMAT;
	}

	entries_size_bytes = hdr->num_entries *
			     sizeof(struct fuseprov_qfuse_entry);

	if (secdat_len < sizeof(*hdr) ||
	    entries_size_bytes > (secdat_len - sizeof(*hdr))) {
		EMSG("secdat size mismatch: len=0x%x entries=%u",
		     secdat_len, hdr->num_entries);
		return TEE_ERROR_BAD_FORMAT;
	}

	*entries_out     = (const struct fuseprov_qfuse_entry *)(secdat_buffer +
								 sizeof(*hdr));
	*num_entries_out = hdr->num_entries;

	return TEE_SUCCESS;
}

/*
 * prov_qfprom_fuses - Provision QFPROM fuses from sec_dat.
 *
 * @sec_dat_addr: Virtual address of sec_dat buffer
 * @sec_dat_size: Size of sec_dat buffer in bytes
 */
TEE_Result prov_qfprom_fuses(vaddr_t sec_dat_addr, uint32_t sec_dat_size)
{
	TEE_Result res = TEE_SUCCESS;
	const struct fuseprov_qfuse_entry *entries = NULL;
	uint32_t num_entries = 0;

	if (!sec_dat_addr || !sec_dat_size)
		return TEE_ERROR_BAD_PARAMETERS;

	res = parse_secdat_hdr(sec_dat_addr, sec_dat_size,
			       &entries, &num_entries);
	if (res != TEE_SUCCESS) {
		EMSG("Failed to parse secdat header: %#"PRIx32, res);
		return res;
	}

	if (num_entries == 0) {
		EMSG("No fuse entries found in secdat");
		return TEE_ERROR_BAD_FORMAT;
	}

	res = provision_fuse_category(entries, num_entries);
	if (res != TEE_SUCCESS)
		EMSG("Fuse provisioning failed: %#"PRIx32, res);

	return res;
}

/*
 * prov_qfprom_fuses_with_auth - Authenticate and provision fuses.
 *
 * @elf_vaddr: Virtual address of ELF metadata
 * @elf_metadata_size: Size of ELF metadata buffer
 * @regions: Region list containing sec_dat physical address
 * @region_count: Number of regions in the list
 */
TEE_Result
prov_qfprom_fuses_with_auth(vaddr_t elf_vaddr,
			    uint32_t elf_metadata_size,
			    struct TmeRegion_t *regions,
			    uint32_t region_count)
{
	TEE_Result res = TEE_SUCCESS;
	vaddr_t sec_dat_addr = 0;
	uint32_t sec_dat_size = 0;
	paddr_t sec_dat_paddr;
	paddr_t sec_dat_paddr_aligned = 0;
	size_t page_offset = 0;
	void *sec_dat_vaddr_mapped = NULL;
	tee_mm_entry_t *mm = NULL;
	size_t num_pages = 0;
	bool dynamic_mapping_created = false;
	paddr_t elf_paddr = 0;
	paddr_t regions_paddr = 0;

	if (!regions || region_count == 0) {
		EMSG("No region list provided");
		return TEE_ERROR_BAD_PARAMETERS;
	}

	sec_dat_paddr = regions[0].startAddr;

	if (regions[0].endAddr <= regions[0].startAddr) {
		EMSG("Invalid region: endAddr (0x%x) <= startAddr (0x%x)",
		     regions[0].endAddr, regions[0].startAddr);
		res = TEE_ERROR_BAD_PARAMETERS;
		goto cleanup;
	}

	sec_dat_size = regions[0].endAddr - regions[0].startAddr;

	if (sec_dat_size == 0) {
		EMSG("Invalid sec_dat size: 0x%x (must be non-zero)",
		     sec_dat_size);
		res = TEE_ERROR_BAD_PARAMETERS;
		goto cleanup;
	}

	/* Check for overflow in ROUNDUP calculation */
	if (sec_dat_size > (UINT32_MAX - SMALL_PAGE_SIZE + 1)) {
		EMSG("sec_dat_size too large, would overflow in ROUNDUP");
		res = TEE_ERROR_BAD_PARAMETERS;
		goto cleanup;
	}

	/* Calculate page offset and aligned physical address */
	page_offset = sec_dat_paddr & SMALL_PAGE_MASK;
	sec_dat_paddr_aligned = ROUNDDOWN(sec_dat_paddr, SMALL_PAGE_SIZE);
	num_pages = ROUNDUP(sec_dat_size + page_offset, SMALL_PAGE_SIZE) /
		    SMALL_PAGE_SIZE;

	/* Try to map using phys_to_virt first (if already mapped) */
	sec_dat_vaddr_mapped = phys_to_virt(sec_dat_paddr, MEM_AREA_RAM_NSEC,
					    sec_dat_size);

	if (!sec_dat_vaddr_mapped) {
		/* Allocate VA space from core_virt_shm_pool */
		mm = tee_mm_alloc(&core_virt_shm_pool,
				  num_pages * SMALL_PAGE_SIZE);
		if (!mm) {
			EMSG("Failed to allocate VA space for 0x%08lx",
			     (unsigned long)sec_dat_paddr);
			res = TEE_ERROR_OUT_OF_MEMORY;
			goto cleanup;
		}

		/* Map the physical pages to the allocated VA space */
		res = core_mmu_map_contiguous_pages(tee_mm_get_smem(mm),
						    sec_dat_paddr_aligned,
						    num_pages,
						    MEM_AREA_RAM_NSEC);
		if (res != TEE_SUCCESS) {
			EMSG("Failed to map pages for 0x%08lx: %#"PRIx32,
			     (unsigned long)sec_dat_paddr, res);
			tee_mm_free(mm);
			mm = NULL;
			goto cleanup;
		}

		/* Calculate the actual virtual address with page offset */
		sec_dat_vaddr_mapped = (void *)(tee_mm_get_smem(mm) +
						page_offset);
		dynamic_mapping_created = true;
	}

	sec_dat_addr = (vaddr_t)sec_dat_vaddr_mapped;

	/* Invalidate cache for sec_dat to avoid stale data after remapping. */
	dcache_inv_range((void *)sec_dat_addr, sec_dat_size);

	elf_paddr = virt_to_phys((void *)elf_vaddr);
	if (!elf_paddr) {
		EMSG("Failed to convert ELF vaddr 0x%08lx to paddr",
		     (unsigned long)elf_vaddr);
		res = TEE_ERROR_BAD_PARAMETERS;
		goto cleanup;
	}

	dcache_clean_range((void *)elf_vaddr, elf_metadata_size);

	if (regions && region_count > 0) {
		uint32_t regions_size =
			region_count * sizeof(struct TmeRegion_t);

		dcache_clean_range(regions, regions_size);

		regions_paddr = virt_to_phys(regions);
		if (!regions_paddr) {
			EMSG("Failed to convert regions vaddr to paddr");
			res = TEE_ERROR_BAD_PARAMETERS;
			goto cleanup;
		}
	}
#ifdef CFG_QCOM_TMEL_AUTH
	/* Use unified TME authentication interface */
	struct tmel_sec_auth_params auth_params;

	memset(&auth_params, 0, sizeof(auth_params));

	auth_params.sw_id = SECELF_SW_ID;
	auth_params.elf_buf.buf = (uint32_t)elf_paddr;
	auth_params.elf_buf.buf_len = elf_metadata_size;

	if (regions && region_count > 0) {
		auth_params.region_list.buf = (uint32_t)regions_paddr;
		auth_params.region_list.buf_len =
			region_count * sizeof(struct TmeRegion_t);
		auth_params.relocate = 1;
	} else {
		auth_params.region_list.buf = 0;
		auth_params.region_list.buf_len = 0;
		auth_params.relocate = 0;
	}

	auth_params.nsIntegrityCheck = 1;
	auth_params.reservedBuf.buf = 0;
	auth_params.reservedBuf.buf_len = 0;

	res = tmel_secure_auth(&auth_params);

	if (res != TEE_SUCCESS) {
		EMSG("Elf authentication FAILED: 0x%08x", res);
		goto cleanup;
	}
	IMSG("ELF authentication SUCCESSFUL!");
#endif
	res = prov_qfprom_fuses(sec_dat_addr, sec_dat_size);

	if (res != TEE_SUCCESS) {
		EMSG("Fuse provisioning FAILED: 0x%08x", res);
		goto cleanup;
	}

	IMSG("Fuse provisioning completed successfully!");

cleanup:
	if (dynamic_mapping_created && mm) {
		core_mmu_unmap_pages(tee_mm_get_smem(mm),
				     num_pages);
		tee_mm_free(mm);
	}

	return res;
}
