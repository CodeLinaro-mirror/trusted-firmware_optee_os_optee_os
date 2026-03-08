ifeq ($(CFG_QCOM_PAS_PTA),y)
global-incdirs-y += .
srcs-y += pta_qcom_pas.c
srcs-y += cdsp.c
endif
