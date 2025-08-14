# IPQ Low Memory (LM) Profile Configuration

# This file contains settings for the LM profile configuration:
# - Minimal feature set optimized for low memory usage
# - Single-threaded operation to reduce memory overhead
# - Enables only essential crypto: AES, GCM, SHA256 with software PRNG
# - Disables all TAs (user TAs, early TAs, PKCS#11)
# - Disables all file systems and secure storage
# - Disables debug features, security hardening, and all PTAs
# - Disables drivers (CLK, GPIO, RSTCTRL)

# Memory size configuration for LM profile
# 512KB (0x80000) - Minimal memory footprint for constrained devices
CFG_TZDRAM_SIZE ?= 0x80000
CFG_TEE_RAM_VA_SIZE ?= 0x80000

# Basic crypto configuration
CFG_WITH_SOFTWARE_PRNG ?= y
CFG_CRYPTO ?= y
CFG_CRYPTO_AES ?= y
CFG_CRYPTO_GCM ?= y
CFG_CRYPTO_SHA256 ?= y

# Disable unnecessary crypto algorithms
CFG_CRYPTO_DES ?= n
CFG_CRYPTO_SM4 ?= n
CFG_CRYPTO_CBC ?= n
CFG_CRYPTO_CTR ?= n
CFG_CRYPTO_CTS ?= n
CFG_CRYPTO_XTS ?= n
CFG_CRYPTO_HMAC ?= n
CFG_CRYPTO_CMAC ?= n
CFG_CRYPTO_CBC_MAC ?= n
CFG_CRYPTO_MD5 ?= n
CFG_CRYPTO_SHA1 ?= n
CFG_CRYPTO_SHA224 ?= n
CFG_CRYPTO_SHA384 ?= n
CFG_CRYPTO_SHA512 ?= n
CFG_CRYPTO_SHA512_256 ?= n
CFG_CRYPTO_SM3 ?= n
CFG_CRYPTO_SHA3_224 ?= n
CFG_CRYPTO_SHA3_256 ?= n
CFG_CRYPTO_SHA3_384 ?= n
CFG_CRYPTO_SHA3_512 ?= n
CFG_CRYPTO_DSA ?= n
CFG_CRYPTO_RSA ?= n
CFG_CRYPTO_DH ?= n
CFG_CRYPTO_ECC ?= n
CFG_CRYPTO_SM2_PKE ?= n
CFG_CRYPTO_SM2_DSA ?= n
CFG_CRYPTO_SM2_KEP ?= n
CFG_CRYPTO_ED25519 ?= n
CFG_CRYPTO_X25519 ?= n
CFG_CRYPTO_CCM ?= n

# Disable TAs
CFG_WITH_USER_TA ?= n
CFG_EARLY_TA ?= n
CFG_BUILD_IN_TREE_TA ?= n
CFG_PKCS11_TA ?= n
CFG_IN_TREE_EARLY_TAS :=

# Disable file systems and secure storage
CFG_REE_FS_TA ?= n
CFG_REE_FS ?= n
CFG_RPMB_FS ?= n
CFG_SECSTOR_TA ?= n

# Disable debug features
CFG_TEE_CORE_LOG_LEVEL ?= 0
CFG_TEE_CORE_DEBUG ?= n
CFG_DEBUG_INFO ?= n
CFG_WITH_STATS ?= n
CFG_TEE_CORE_TA_TRACE ?= n

# Disable security hardening
CFG_CORE_BTI ?= n
CFG_TA_BTI ?= n
CFG_CORE_PAUTH ?= n
CFG_TA_PAUTH ?= n
CFG_TA_FLOAT_SUPPORT ?= n
CFG_UNWIND ?= n

# Disable PTAs
CFG_DEVICE_ENUM_PTA ?= n
CFG_SYSTEM_PTA ?= n
CFG_RTC_PTA ?= n
CFG_SCP03_PTA ?= n
CFG_APDU_PTA ?= n
CFG_SCMI_PTA ?= n
CFG_ATTESTATION_PTA ?= n
CFG_VERAISON_ATTESTATION_PTA ?= n
CFG_WIDEVINE_PTA ?= n
CFG_GP_SOCKETS ?= n

# Disable other features
CFG_CORE_ASYNC_NOTIF ?= n
CFG_FAULT_MITIGATION ?= n
CFG_COMPAT_GP10_DES ?= n
CFG_CORE_HUK_SUBKEY_COMPAT ?= n
CFG_PREALLOC_RPC_CACHE ?= n

# Disable drivers
CFG_DRIVERS_CLK ?= n
CFG_DRIVERS_GPIO ?= n
CFG_DRIVERS_RSTCTRL ?= n

# Single-threaded operation
CFG_NUM_THREADS ?= 1
