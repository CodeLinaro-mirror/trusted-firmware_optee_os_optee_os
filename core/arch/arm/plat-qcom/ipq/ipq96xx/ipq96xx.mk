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

# REMOTEPROC for TURING SS
CFG_QCOM_PAS_PTA ?= y
CFG_DEVICE_ENUM_PTA ?= y
CFG_ASYNC_NOTIF ?= y

# GCC
CFG_DRIVERS_CLK ?= y
CFG_DRIVERS_QCOM_CLK ?= y

# DT
CFG_DT ?= y
CFG_DRIVERS_CLK_DT ?= y
CFG_EMBED_DTB_SOURCE_FILE ?= qcom-ipq96xx.dts
# Secure watchdog bark interrupt handler
CFG_QCOM_SEC_WDOG ?= y

# APSS_WDT_TMR2_BASE base address
CFG_APSS_WDT_BASE ?= 0x0F411000

# Watchdog register offset
CFG_WDOG_RESET_REG_OFFSET ?= 0x4

# Secure watchdog bark interrupt ID
CFG_SEC_WDOG_BARK_INT_ID ?= 0x36

# TME IPC support
CFG_QCOM_TMEL_COM ?= y
CFG_TME_QMP_IRQ_IN_ID ?= 154u
CFG_TME_QMP_IRQ_OUT_REG_ADDR ?= 0xF400008
CFG_TME_QMP_IRQ_OUT_BIT_MASK ?= 0x00200000
CFG_TME_QMP_INBOUND_MBOX_ADDR ?= 0x22090000
CFG_TME_QMP_OUTBOUND_MBOX_ADDR ?= 0x22091000
CFG_QCOM_FEATURE_CONFIG2_ADDR ?= 0xA600C

# Hardware RNG configuration for IPQ96xx
# Disable software PRNG and enable hardware RNG PTA
CFG_WITH_SOFTWARE_PRNG ?= n
CFG_HWRNG_PTA ?= y
CFG_HWRNG_QUALITY ?= 1024
CFG_HWRNG_RATE ?= 0

# Use TMEL IPC for RNG access on IPQ96xx
CFG_QCOM_TMEL_RNG ?= y

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

# TME Key Management support
CFG_QCOM_TMEL_KM ?= y
# TCSR Hardware Key Register Configuration
CFG_TCSR_FUSE_PRI_HW_KEY_BASE_START ?= 0x193D404
CFG_TCSR_FUSE_PRI_HW_KEY_REG_COUNT ?= 8
CFG_TCSR_FUSE_SEC_HW_KEY_BASE_START ?= 0x193D424
CFG_TCSR_FUSE_SEC_HW_KEY_REG_COUNT ?= 8

# QCOM Hardware Unique Key (HUK) support
CFG_QCOM_HUK ?= y

# Serial Number fuse register address (Die ID)
CFG_QCOM_SERIAL_NUM_FUSE_ADDR ?= 0xA60A8

# HUK subkey compatibility mode - use actual die ID from OTP
CFG_CORE_HUK_SUBKEY_COMPAT ?= y
CFG_CORE_HUK_SUBKEY_COMPAT_USE_OTP_DIE_ID ?= y

# Secure Storage Configuration
# RPMB FS for secure storage (REE FS is enabled in premium.mk)
CFG_RPMB_FS ?= y

# RPMB Configuration
CFG_RPMB_FS_DEV_ID ?= 0
CFG_RPMB_FS_CACHE_ENTRIES ?= 8
CFG_RPMB_FS_RD_ENTRIES ?= 8

# RPMB protects REE_FS
CFG_REE_FS_INTEGRITY_RPMB ?= y

# RPMB debugging (disabled for production)
CFG_RPMB_FS_DEBUG_DATA ?= n

# Never enable in production!
CFG_RPMB_WRITE_KEY ?= y
CFG_RPMB_TEST_KEY ?= y
endif

endif
