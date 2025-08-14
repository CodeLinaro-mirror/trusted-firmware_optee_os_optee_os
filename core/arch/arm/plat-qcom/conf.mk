# Default to ipq96xx premium flavor if no specific platform is provided
PLATFORM_FLAVOR ?= ipq96xx

# Include ipq.mk to define flavor lists, include chipset-specific files,
# and profile-specific files
ifneq (,$(findstring ipq,$(PLATFORM_FLAVOR)))
  CFG_IPQ ?= y
  include core/arch/arm/plat-qcom/ipq/ipq.mk
endif

# Add other QCOM platform families here (e.g., MSM, MDM) as needed

# Finally include common QCOM platform settings for all flavors
include core/arch/arm/plat-qcom/common.mk
