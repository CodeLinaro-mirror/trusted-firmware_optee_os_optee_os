# IPQ Premium Profile Configuration

# This file contains settings for the premium profile configuration:
# - Enables user TA support with PKCS#11 and other TAs
# - Enables PKCS#11 TA RSA and X.509 support
# - Enables security hardening: BTI, PAUTH, fault mitigation, stack protection
# - Enables performance features: async notifications, RPC cache, stats, unwinding
# - Enables comprehensive crypto: RSA, ECC, DH, HMAC, CMAC, HKDF, PBKDF2
# - Configures secure storage: REE_FS, SECSTOR_TA
# - Enables essential PTAs: System, Device Enum
# - Uses multi-threaded operation (4 threads)
# - Explicitly includes early TAs

# Memory size configuration for Premium profile
# 2.5MB (0x280000) - Full feature set with multi-threading support
CFG_TZDRAM_SIZE ?= 0x280000
CFG_TEE_RAM_VA_SIZE ?= 0x280000

# Enable PKCS#11 TA and user TA support
CFG_PKCS11_TA ?= y
CFG_PKCS11_TA_RSA_X_509 ?= y
CFG_WITH_USER_TA ?= y
CFG_TEE_CORE_DEBUG ?= y

# Enable security hardening
CFG_CORE_BTI ?= y
CFG_ARM64_PAUTH ?= y
CFG_CORE_FAULT_MITIGATION ?= y
CFG_STACK_THREAD_EXTRA ?= 8192
CFG_STACK_TMP_EXTRA ?= 2048

# Enable performance features
CFG_ASYNC_NOTIF ?= y
CFG_RPC_CACHE ?= y
CFG_TEE_CORE_EMBED_INTERNAL_TESTS ?= y
CFG_ENABLE_STATS ?= y
CFG_UNWIND ?= y
CFG_LOCKDEP ?= y

# Enable comprehensive crypto
CFG_CRYPTO_RSA ?= y
CFG_CRYPTO_ECC ?= y
CFG_CRYPTO_DH ?= y
CFG_CRYPTO_HMAC ?= y
CFG_CRYPTO_CMAC ?= y
CFG_CRYPTO_HKDF ?= y
CFG_CRYPTO_PBKDF2 ?= y

# Configure secure storage
CFG_REE_FS ?= y
CFG_SECSTOR_TA ?= y

# Enable essential PTAs
CFG_SYSTEM_PTA ?= y
CFG_DEVICE_ENUM_PTA ?= y

# Multi-threaded operation
CFG_NUM_THREADS ?= 4

# Early TAs to include
CFG_IN_TREE_EARLY_TAS ?= pkcs11/fd02c9da-306c-48c7-a49c-bbd827ae86ee
CFG_IN_TREE_EARLY_TAS += remoteproc/80a4c275-0a47-4905-8285-1486a9771a08
CFG_IN_TREE_EARLY_TAS += trusted_keys/f04a0fe7-1f5d-4b9b-abf7-619b85b4ce8c
CFG_IN_TREE_EARLY_TAS += avb/023f8f1a-292a-432b-8fc4-de8471358067
