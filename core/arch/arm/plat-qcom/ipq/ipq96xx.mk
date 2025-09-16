# IPQ96xx Platform-Specific Configuration

# IPQ96xx-specific settings (both profiles)
ifneq (,$(filter $(PLATFORM_FLAVOR),$(ipq96xx-flavorlist)))

CFG_IPQ96XX ?= y

# Hardware configuration
# 5 cores (4 Cortex-A55 + 1 Cortex-A78) in a single cluster
CFG_TEE_CORE_NB_CORE ?= 5
# Use log2(8)=3 to accommodate all 5 cores in the cluster
CFG_CORE_CLUSTER_SHIFT ?= 3

# IPQ96xx-specific memory layout
CFG_TZDRAM_START ?= 0x8A680000

# IPQ96xx GIC configuration (GICv3)
CFG_GIC ?= y
CFG_ARM_GICV3 ?= y

# GIC register base addresses and offsets
CFG_GIC_BASE    ?= 0xF200000
CFG_GIC_SIZE    ?= 0xF0000
CFG_GICD_OFFSET ?= 0x0
CFG_GICR_OFFSET ?= 0x40000

# DIAG logging support
CFG_QCOM_DIAG_LOG ?= y
CFG_QCOM_DIAG_BASE ?= 0x860A000
CFG_QCOM_DIAG_SIZE ?= 0x3000
CFG_QCOM_DIAG_LOG_START_INFO ?= 0x8600730
CFG_QCOM_DIAG_LOG_SIZE_INFO ?= 0x8600734
CFG_QCOM_TCSR_BOOT_MISC_DETECT ?= 0x195C100

endif
