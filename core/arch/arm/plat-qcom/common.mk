# QCOM platforms are ARM64-only, so force ARM64 TA targets
supported-ta-targets = ta_arm64

# Common QCOM platform settings applied to all flavors
# - Enables ARM64 core and ARM Trusted Firmware
# - Uses CNTPCT as secure time source
# - Disables ASLR for both core and TA to save memory
# - Disables PAGER for simplified memory management and reducing boot complexity
# - Configures shared memory settings for optimal performance
# - Disables all AArch32-related configs as QCOM vendors only support 64-bit
# - Disables PSCI support as it's handled by ATF
define setup-qcom-common
$(call force,CFG_QCOM_VENDOR,y)
$(call force,CFG_ARM64_core,y)
$(call force,CFG_WITH_ARM_TRUSTED_FW,y)
$(call force,CFG_SECURE_TIME_SOURCE_CNTPCT,y)
$(call force,CFG_CORE_ASLR,n)
$(call force,CFG_TA_ASLR,n)
$(call force,CFG_CORE_RESERVED_SHM,n)
$(call force,CFG_CORE_DYN_SHM,y)
$(call force,CFG_WITH_PAGER,n)
$(call force,CFG_ARM32_core,n)
$(call force,CFG_ARM32_ta_arm32,n)
$(call force,CFG_ARM32_ta_arm64,n)
$(call force,CFG_TA_ARM32_SUPPORT,n)
$(call force,CFG_PSCI_ARM32,n)
$(call force,CFG_PSCI_ARM64,n)
endef

# Premium profile configuration for full-featured builds
# - Enables user TA support with PKCS#11 and other TAs
# - Enables PKCS#11 TA RSA and X.509 support
# - Enables security hardening: BTI, PAUTH, fault mitigation, stack protection
# - Enables performance features: async notifications, RPC cache, stats, unwinding
# - Enables comprehensive crypto: RSA, ECC, DH, HMAC, CMAC, HKDF, PBKDF2
# - Configures secure storage: REE_FS, SECSTOR_TA
# - Enables essential PTAs: System, Device Enum
# - Uses multi-threaded operation (4 threads)
# - Explicitly includes early TAs:
#   * PKCS#11 (fd02c9da-306c-48c7-a49c-bbd827ae86ee)
#   * RemoteProc (80a4c275-0a47-4905-8285-1486a9771a08)
#   * Trusted Keys (f04a0fe7-1f5d-4b9b-abf7-619b85b4ce8c)
#   * Android Verified Boot (023f8f1a-292a-432b-8fc4-de8471358067)
define setup-premium-config
$(call force,CFG_PKCS11_TA,y)
$(call force,CFG_PKCS11_TA_RSA_X_509,y)
$(call force,CFG_WITH_USER_TA,y)
$(call force,CFG_TEE_CORE_DEBUG,y)
$(call force,CFG_CORE_BTI,y)
$(call force,CFG_ARM64_PAUTH,y)
$(call force,CFG_CORE_FAULT_MITIGATION,y)
$(call force,CFG_STACK_THREAD_EXTRA,8192)
$(call force,CFG_STACK_TMP_EXTRA,2048)
$(call force,CFG_ASYNC_NOTIF,y)
$(call force,CFG_RPC_CACHE,y)
$(call force,CFG_TEE_CORE_EMBED_INTERNAL_TESTS,y)
$(call force,CFG_ENABLE_STATS,y)
$(call force,CFG_UNWIND,y)
$(call force,CFG_LOCKDEP,y)
$(call force,CFG_CRYPTO_RSA,y)
$(call force,CFG_CRYPTO_ECC,y)
$(call force,CFG_CRYPTO_DH,y)
$(call force,CFG_CRYPTO_HMAC,y)
$(call force,CFG_CRYPTO_CMAC,y)
$(call force,CFG_CRYPTO_HKDF,y)
$(call force,CFG_CRYPTO_PBKDF2,y)
$(call force,CFG_REE_FS,y)
$(call force,CFG_SECSTOR_TA,y)
$(call force,CFG_SYSTEM_PTA,y)
$(call force,CFG_DEVICE_ENUM_PTA,y)
$(call force,CFG_NUM_THREADS,4)

CFG_IN_TREE_EARLY_TAS = pkcs11/fd02c9da-306c-48c7-a49c-bbd827ae86ee
CFG_IN_TREE_EARLY_TAS += remoteproc/80a4c275-0a47-4905-8285-1486a9771a08
CFG_IN_TREE_EARLY_TAS += trusted_keys/f04a0fe7-1f5d-4b9b-abf7-619b85b4ce8c
CFG_IN_TREE_EARLY_TAS += avb/023f8f1a-292a-432b-8fc4-de8471358067
endef

# Low Memory (LM) profile configuration for memory-constrained builds
# - Minimal feature set optimized for low memory usage
# - Single-threaded operation to reduce memory overhead
# - Enables only essential crypto: AES, GCM, SHA256 with software PRNG
# - Disables all TAs (user TAs, early TAs, PKCS#11)
# - Disables all file systems and secure storage
# - Disables debug features, security hardening, and all PTAs
# - Disables drivers (CLK, GPIO, RSTCTRL)
# - Explicitly clears CFG_IN_TREE_EARLY_TAS to prevent enabling CFG_EARLY_TA
define setup-lm-config
$(call force,CFG_WITH_SOFTWARE_PRNG,y)
$(call force,CFG_CRYPTO,y)
$(call force,CFG_CRYPTO_AES,y)
$(call force,CFG_CRYPTO_GCM,y)
$(call force,CFG_CRYPTO_SHA256,y)
$(call force,CFG_CRYPTO_DES,n)
$(call force,CFG_CRYPTO_SM4,n)
$(call force,CFG_CRYPTO_CBC,n)
$(call force,CFG_CRYPTO_CTR,n)
$(call force,CFG_CRYPTO_CTS,n)
$(call force,CFG_CRYPTO_XTS,n)
$(call force,CFG_CRYPTO_HMAC,n)
$(call force,CFG_CRYPTO_CMAC,n)
$(call force,CFG_CRYPTO_CBC_MAC,n)
$(call force,CFG_CRYPTO_MD5,n)
$(call force,CFG_CRYPTO_SHA1,n)
$(call force,CFG_CRYPTO_SHA224,n)
$(call force,CFG_CRYPTO_SHA384,n)
$(call force,CFG_CRYPTO_SHA512,n)
$(call force,CFG_CRYPTO_SHA512_256,n)
$(call force,CFG_CRYPTO_SM3,n)
$(call force,CFG_CRYPTO_SHA3_224,n)
$(call force,CFG_CRYPTO_SHA3_256,n)
$(call force,CFG_CRYPTO_SHA3_384,n)
$(call force,CFG_CRYPTO_SHA3_512,n)
$(call force,CFG_CRYPTO_DSA,n)
$(call force,CFG_CRYPTO_RSA,n)
$(call force,CFG_CRYPTO_DH,n)
$(call force,CFG_CRYPTO_ECC,n)
$(call force,CFG_CRYPTO_SM2_PKE,n)
$(call force,CFG_CRYPTO_SM2_DSA,n)
$(call force,CFG_CRYPTO_SM2_KEP,n)
$(call force,CFG_CRYPTO_ED25519,n)
$(call force,CFG_CRYPTO_X25519,n)
$(call force,CFG_CRYPTO_CCM,n)
$(call force,CFG_WITH_USER_TA,n)
$(call force,CFG_EARLY_TA,n)
$(call force,CFG_BUILD_IN_TREE_TA,n)
$(call force,CFG_PKCS11_TA,n)
CFG_IN_TREE_EARLY_TAS :=
$(call force,CFG_REE_FS_TA,n)
$(call force,CFG_REE_FS,n)
$(call force,CFG_RPMB_FS,n)
$(call force,CFG_SECSTOR_TA,n)
$(call force,CFG_TEE_CORE_LOG_LEVEL,0)
$(call force,CFG_TEE_CORE_DEBUG,n)
$(call force,CFG_DEBUG_INFO,n)
$(call force,CFG_WITH_STATS,n)
$(call force,CFG_TEE_CORE_TA_TRACE,n)
$(call force,CFG_CORE_BTI,n)
$(call force,CFG_TA_BTI,n)
$(call force,CFG_CORE_PAUTH,n)
$(call force,CFG_TA_PAUTH,n)
$(call force,CFG_TA_FLOAT_SUPPORT,n)
$(call force,CFG_UNWIND,n)
$(call force,CFG_DEVICE_ENUM_PTA,n)
$(call force,CFG_SYSTEM_PTA,n)
$(call force,CFG_RTC_PTA,n)
$(call force,CFG_SCP03_PTA,n)
$(call force,CFG_APDU_PTA,n)
$(call force,CFG_SCMI_PTA,n)
$(call force,CFG_ATTESTATION_PTA,n)
$(call force,CFG_VERAISON_ATTESTATION_PTA,n)
$(call force,CFG_WIDEVINE_PTA,n)
$(call force,CFG_GP_SOCKETS,n)
$(call force,CFG_CORE_ASYNC_NOTIF,n)
$(call force,CFG_FAULT_MITIGATION,n)
$(call force,CFG_COMPAT_GP10_DES,n)
$(call force,CFG_CORE_HUK_SUBKEY_COMPAT,n)
$(call force,CFG_PREALLOC_RPC_CACHE,n)
$(call force,CFG_DRIVERS_CLK,n)
$(call force,CFG_DRIVERS_GPIO,n)
$(call force,CFG_DRIVERS_RSTCTRL,n)
$(call force,CFG_NUM_THREADS,1)
endef
