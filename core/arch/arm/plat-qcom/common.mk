# Common QCOM platform settings applied to all QCOM flavors

# QCOM platforms default to ARM64, but can support ARM32 as well
supported-ta-targets ?= ta_arm64

# Common QCOM platform configuration
CFG_QCOM_VENDOR ?= y
CFG_ARM64_core ?= y
CFG_WITH_ARM_TRUSTED_FW ?= y
CFG_SECURE_TIME_SOURCE_CNTPCT ?= y
CFG_CORE_ASLR ?= n
CFG_TA_ASLR ?= n
CFG_CORE_RESERVED_SHM ?= n
CFG_CORE_DYN_SHM ?= y
CFG_WITH_PAGER ?= n
CFG_ARM32_core ?= n
CFG_ARM32_ta_arm32 ?= n
CFG_ARM32_ta_arm64 ?= n
CFG_TA_ARM32_SUPPORT ?= n
CFG_PSCI_ARM32 ?= n
CFG_PSCI_ARM64 ?= n

# 36-bit PA width to support platforms with memory beyond 4GB
CFG_CORE_ARM64_PA_BITS ?= 36
