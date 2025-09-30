# IPQ5200 Platform-Specific Configuration

# This file contains IPQ5200 SoC-specific settings that extend the common
# IPQ platform configuration.
#
# Key features:
# - LM profile: 512KB memory, single thread, optimized for memory constraints
# - Premium profile: 2.5MB memory, multi-thread, full feature set
# - No REMOTEPROC support: IPQ5200 does not require remote processor functionality

# IPQ5200-specific settings (both profiles)
ifneq (,$(filter $(PLATFORM_FLAVOR),$(ipq5200-flavorlist)))

CFG_IPQ5200 ?= y

# Hardware configuration
# - 4 cores in a single cluster
# - CFG_TEE_CORE_NB_CORE defines the total number of cores in the SoC
#   This affects core enumeration and power management
CFG_TEE_CORE_NB_CORE ?= 4
# - CFG_CORE_CLUSTER_SHIFT defines log2 of the number of cores per cluster
# - Use log2(4)=2 to accommodate all 4 cores in the cluster
# - This value is used to calculate core positions in the SoC topology
CFG_CORE_CLUSTER_SHIFT ?= 2

# IPQ5200 does not support HWRNG PTA
CFG_HWRNG_PTA ?= n

# IPQ5200-specific memory layout
CFG_TZDRAM_START ?= 0x8A680000

# DIAG logging support
CFG_QCOM_DIAG_LOG ?= y
CFG_QCOM_DIAG_BASE ?= 0x860A000
CFG_QCOM_DIAG_SIZE ?= 0x3000
CFG_QCOM_DIAG_LOG_START_INFO ?= 0x8600730
CFG_QCOM_DIAG_LOG_SIZE_INFO ?= 0x8600734
CFG_QCOM_TCSR_BOOT_MISC_DETECT ?= 0x195C100

endif
