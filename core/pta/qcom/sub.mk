ifeq ($(CFG_QCOM_PAS_PTA),y)
global-incdirs-y += .
srcs-y += pta_qcom_pas.c
srcs-y += cdsp.c
endif

srcs-$(CFG_NAND_FS_ENC_PTA) += nand_fs_enc.c
srcs-$(CFG_EMMC_ICE_FS_ENC_PTA) += emmc_ice_config.c
srcs-$(CFG_QFPROM_PTA) += qfprom.c
