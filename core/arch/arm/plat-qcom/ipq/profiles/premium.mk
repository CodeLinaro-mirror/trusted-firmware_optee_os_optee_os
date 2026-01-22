# IPQ Premium Profile Configuration

# Memory size configuration for Premium profile
CFG_TZDRAM_SIZE ?= 0x280000
CFG_TEE_RAM_VA_SIZE ?= 0x280000

# TA support
CFG_PKCS11_TA ?= y
CFG_PKCS11_TA_RSA_X_509 ?= y

# Debug features
CFG_TEE_CORE_LOG_LEVEL ?= 4
CFG_TEE_CORE_DEBUG ?= y
CFG_DEBUG_INFO ?= y
CFG_CORE_DUMP_OOM ?= $(CFG_TEE_CORE_MALLOC_DEBUG)

# Security hardening
CFG_FAULT_MITIGATION ?= y
CFG_STACK_THREAD_EXTRA ?= 8192
CFG_STACK_TMP_EXTRA ?= 2048

# Performance features
CFG_ASYNC_NOTIF ?= y
CFG_PREALLOC_RPC_CACHE ?= y
CFG_TEE_CORE_EMBED_INTERNAL_TESTS ?= y
CFG_ENABLE_STATS ?= y
CFG_UNWIND ?= y
CFG_LOCKDEP ?= y
CFG_WITH_STATS ?= y

# Enable additional crypto not enabled by default
CFG_CRYPTO_RSA ?= y
CFG_CRYPTO_ECC ?= y
CFG_CRYPTO_DH ?= y
CFG_CRYPTO_HMAC ?= y
CFG_CRYPTO_CMAC ?= y
CFG_CRYPTO_HKDF ?= y
CFG_CRYPTO_PBKDF2 ?= y

# Secure Storage Configuration
CFG_REE_FS ?= y

# Enable secure storage for TAs
CFG_SECSTOR_TA ?= y

# PTAs
CFG_SYSTEM_PTA ?= y

# Early TAs to include
CFG_IN_TREE_EARLY_TAS ?= pkcs11/fd02c9da-306c-48c7-a49c-bbd827ae86ee
CFG_IN_TREE_EARLY_TAS += remoteproc/80a4c275-0a47-4905-8285-1486a9771a08
CFG_IN_TREE_EARLY_TAS += trusted_keys/f04a0fe7-1f5d-4b9b-abf7-619b85b4ce8c
