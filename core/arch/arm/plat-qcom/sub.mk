global-incdirs-y += .
ifeq ($(CFG_IPQ),y)
global-incdirs-y += ipq/$(IPQ_CHIPSET)
endif
srcs-y += main.c
