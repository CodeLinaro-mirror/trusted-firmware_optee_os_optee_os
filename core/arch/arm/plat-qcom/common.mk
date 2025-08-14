# Common QCOM platform settings applied to all QCOM flavors

# Key features:
# - Enables ARM64 core and ARM Trusted Firmware
# - Uses CNTPCT as secure time source
# - Disables ASLR for both core and TA to save memory
# - Disables PAGER for simplified memory management and reducing boot complexity
# - Configures shared memory settings for optimal performance
# - Disables AArch32-related configs by default (though ARM32 is supported, ARM64 is the default)
# - Disables PSCI support as it's handled by ATF

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
