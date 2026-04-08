ifneq (,$(filter $(PLATFORM_FLAVOR),$(ipq54xx-flavorlist)))

CFG_IPQ54XX ?= y

# Hardware configuration
# 4 cores (4 Cortex-A55) in a single cluster
CFG_TEE_CORE_NB_CORE ?= 4

# IPQ54xx-specific memory layout
CFG_TZDRAM_START ?= 0x89E00000

# DDR memory configuration
CFG_DRAM0_BASE ?= 0x80000000
CFG_DRAM0_SIZE ?= 0x80000000
CFG_DRAM1_BASE ?= 0x800000000
CFG_DRAM1_SIZE ?= 0x180000000

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

# GENI UART base address
CFG_GENI_UART_BASE ?= 0x01a84000

# Secure watchdog bark interrupt handler
CFG_QCOM_SEC_WDOG ?= y

# APSS_WDT_TMR2_BASE base address
CFG_APSS_WDT_BASE ?= 0x0F411000

# Watchdog register offset
CFG_WDOG_RESET_REG_OFFSET ?= 0x4

# Secure watchdog bark interrupt ID
CFG_SEC_WDOG_BARK_INT_ID ?= 0x36

# TME IPC support
CFG_TME_QMP_IRQ_IN_ID ?= 153u
CFG_TME_QMP_IRQ_OUT_REG_ADDR ?= 0xF400008
CFG_TME_QMP_IRQ_OUT_BIT_MASK ?= 0x00200000
CFG_TME_QMP_INBOUND_MBOX_ADDR ?= 0x32090000
CFG_TME_QMP_OUTBOUND_MBOX_ADDR ?= 0x32091000
CFG_QCOM_FEATURE_CONFIG2_ADDR ?= 0xA600C

ifeq (,$(findstring _lm,$(PLATFORM_FLAVOR)))
# Enable ARM Cryptographic Extensions
CFG_CRYPTO_WITH_CE ?= y

# Enable VFP context preservation (required for ARM CE)
CFG_WITH_VFP ?= y

# TCSR Hardware Key Register Configuration
CFG_TCSR_FUSE_PRI_HW_KEY_BASE_START ?= 0x193D404
CFG_TCSR_FUSE_PRI_HW_KEY_REG_COUNT ?= 8
CFG_TCSR_FUSE_SEC_HW_KEY_BASE_START ?= 0x193D424
CFG_TCSR_FUSE_SEC_HW_KEY_REG_COUNT ?= 8

# Serial Number fuse register address (Die ID)
CFG_QCOM_SERIAL_NUM_FUSE_ADDR ?= 0xA60A8

# HUK subkey compatibility mode - use actual die ID from OTP
CFG_CORE_HUK_SUBKEY_COMPAT_USE_OTP_DIE_ID ?= y

# Secure Storage Configuration
CFG_RPMB_FS ?= n

ifeq ($(CFG_RPMB_FS),y)
# RPMB Configuration
CFG_RPMB_FS_DEV_ID ?= 0
CFG_RPMB_FS_CACHE_ENTRIES ?= 8
CFG_RPMB_FS_RD_ENTRIES ?= 8

# RPMB protects REE_FS
CFG_REE_FS_INTEGRITY_RPMB ?= y

# RPMB debugging (disabled for production)
CFG_RPMB_FS_DEBUG_DATA ?= n

# Never enable in production!
CFG_RPMB_WRITE_KEY ?= n
CFG_RPMB_TEST_KEY ?= n
endif

# ICE (Inline Crypto Engine) configuration
# SDC1_SDCC_ICE_LUT_KEYS_REG_BASE
CFG_SDCC_ICE_LUT_KEYS ?= 0x0780a000
# SDC1_SDCC_ICE_LUT_KEYS_REG_SIZE
CFG_SDCC_ICE_LUT_KEYS_SIZE ?= 0x2000
# TCSR_TME_KEYSLOT_3_KEY_POLICY_0_ADDR
CFG_TCSR_KEYSLOT_ADDR ?= 0x193D46C
endif
endif
