# IPQ Premium Profile Configuration

# Memory size configuration for Premium profile
CFG_TZDRAM_SIZE ?= 0x280000
CFG_TEE_RAM_VA_SIZE ?= 0x280000

# GENI UART support
CFG_QCOM_GENI_UART ?= y

# TME IPC support
CFG_QCOM_TMEL_COM ?= y

# QCOM Hardware Unique Key (HUK) support
CFG_QCOM_HUK ?= y

# TME Key Management support
CFG_QCOM_TMEL_KM ?= y

# Enable filesystem encryption PTAs
CFG_NAND_FS_ENC_PTA ?= y
CFG_EMMC_ICE_FS_ENC_PTA ?= y

# TA support
CFG_PKCS11_TA ?= y
CFG_PKCS11_TA_RSA_X_509 ?= y

# Enable additional crypto not enabled by default
CFG_CRYPTO_HKDF ?= y
CFG_CRYPTO_PBKDF2 ?= y

# Early TAs to include
CFG_IN_TREE_EARLY_TAS ?= pkcs11/fd02c9da-306c-48c7-a49c-bbd827ae86ee
CFG_IN_TREE_EARLY_TAS += trusted_keys/f04a0fe7-1f5d-4b9b-abf7-619b85b4ce8c
