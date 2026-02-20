ifneq (,$(filter $(PLATFORM_FLAVOR),$(ipq96xx-flavorlist)))

CFG_IPQ96XX ?= y

# Hardware configuration
# 5 cores (4 Cortex-A55 + 1 Cortex-A78) in a single cluster
CFG_TEE_CORE_NB_CORE ?= 5
# Use log2(8)=3 to accommodate all 5 cores in the cluster
CFG_CORE_CLUSTER_SHIFT ?= 3

# IPQ96xx-specific memory layout
CFG_TZDRAM_START ?= 0x8A680000

# DDR memory configuration
CFG_DRAM0_BASE ?= 0x80000000
CFG_DRAM0_SIZE ?= 0x80000000
CFG_DRAM1_BASE ?= 0x800000000
CFG_DRAM1_SIZE ?= 0x380000000

# GICv3
CFG_GIC ?= y
CFG_ARM_GICV3 ?= y

# GIC register base addresses and offsets
CFG_GIC_BASE    ?= 0xF200000
CFG_GIC_SIZE    ?= 0xF0000
CFG_GICD_OFFSET ?= 0x0
CFG_GICR_OFFSET ?= 0x40000

# DIAG logging support
CFG_QCOM_DIAG_LOG ?= y
CFG_QCOM_DIAG_BASE ?= 0x8608000
CFG_QCOM_DIAG_SIZE ?= 0x6000
CFG_QCOM_DIAG_LOG_START_INFO ?= 0x8600730
CFG_QCOM_DIAG_LOG_SIZE_INFO ?= 0x8600734
CFG_QCOM_TCSR_BOOT_MISC_DETECT ?= 0x195C100

# QUP GENI UART support
CFG_QCOM_GENI_UART ?= y

# QUP register base addresses and offsets
CFG_QCOM_QUP_UART_BASE    ?= 0x01a84000
CFG_QCOM_QUP_UART_SIZE    ?= 0x4000

ifeq (,$(findstring _lm,$(PLATFORM_FLAVOR)))
# Enable ARM Cryptographic Extensions
CFG_CRYPTO_WITH_CE ?= y

# Enable VFP context preservation (required for ARM CE)
CFG_WITH_VFP ?= y

# Enable hardware-accelerated AES
CFG_CRYPTO_AES_ARM_CE ?= y
CFG_CORE_CRYPTO_AES_ACCEL ?= y

# Enable hardware-accelerated SHA-1
CFG_CRYPTO_SHA1_ARM_CE ?= y
CFG_CORE_CRYPTO_SHA1_ACCEL ?= y

# Enable hardware-accelerated SHA-256
CFG_CRYPTO_SHA256_ARM_CE ?= y
CFG_CORE_CRYPTO_SHA256_ACCEL ?= y

# Enable 64-bit polynomial multiplication support (for GCM)
# This is supported by ARM CE
CFG_HWSUPP_PMULT_64 ?= y

# Disable table-based GCM since we're using hardware acceleration
CFG_AES_GCM_TABLE_BASED := n
endif

# Hardware RNG configuration for IPQ96xx
# Disable software PRNG and enable hardware RNG PTA
CFG_WITH_SOFTWARE_PRNG ?= n
CFG_HWRNG_PTA ?= y
CFG_HWRNG_QUALITY ?= 1024
CFG_HWRNG_RATE ?= 0

endif
