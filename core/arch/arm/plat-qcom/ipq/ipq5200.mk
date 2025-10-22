ifneq (,$(filter $(PLATFORM_FLAVOR),$(ipq5200-flavorlist)))

CFG_IPQ5200 ?= y

# Hardware configuration
# 4 cores (4 Cortex-A53) in a single cluster
CFG_TEE_CORE_NB_CORE ?= 4
# Use log2(4)=2 to accommodate all 4 cores in the cluster
CFG_CORE_CLUSTER_SHIFT ?= 2

# IPQ5200 doesn't require HWRNG PTA since it has SoC RNG for HLOS use-cases
CFG_HWRNG_PTA ?= n

CFG_TZDRAM_START ?= 0x8A680000

# DDR memory configuration
CFG_DRAM0_BASE ?= 0x80000000
CFG_DRAM0_SIZE ?= 0x80000000
CFG_DRAM1_BASE ?= 0x800000000
CFG_DRAM1_SIZE ?= 0x80000000

# DIAG logging support
CFG_QCOM_DIAG_LOG ?= y
CFG_QCOM_DIAG_BASE ?= 0x860A000
CFG_QCOM_DIAG_SIZE ?= 0x3000
CFG_QCOM_DIAG_LOG_START_INFO ?= 0x8600730
CFG_QCOM_DIAG_LOG_SIZE_INFO ?= 0x8600734
CFG_QCOM_TCSR_BOOT_MISC_DETECT ?= 0x195C100

endif
