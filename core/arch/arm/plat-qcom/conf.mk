# Default to ipq96xx premium flavor if no specific platform is provided
PLATFORM_FLAVOR ?= ipq96xx

# Include common QCOM platform settings
include core/arch/arm/plat-qcom/common.mk

# Set SoC-specific flags based on platform flavor
ipq5200-lm-flavorlist = ipq5200_lm
ipq5200-premium-flavorlist = ipq5200_premium ipq5200
ipq5200-flavorlist = $(ipq5200-lm-flavorlist) $(ipq5200-premium-flavorlist)

ipq96xx-lm-flavorlist = ipq96xx_lm
ipq96xx-premium-flavorlist = ipq96xx_premium ipq96xx
ipq96xx-flavorlist = $(ipq96xx-lm-flavorlist) $(ipq96xx-premium-flavorlist)

ifneq (,$(filter $(PLATFORM_FLAVOR),$(ipq5200-flavorlist)))
$(call force,CFG_IPQ5200,y)
include core/arch/arm/plat-qcom/ipq5200.mk
endif

ifneq (,$(filter $(PLATFORM_FLAVOR),$(ipq96xx-flavorlist)))
$(call force,CFG_IPQ96XX,y)
include core/arch/arm/plat-qcom/ipq96xx.mk
endif
