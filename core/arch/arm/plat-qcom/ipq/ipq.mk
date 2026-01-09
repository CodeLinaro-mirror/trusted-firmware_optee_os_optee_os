# IPQ Platform Family Common Configuration

# This file contains common settings for all IPQ chipsets (IPQ52xx, IPQ96xx, etc.)
# IPQ is Qualcomm's WLAN AP WiFi networking chipset family.
#
# Common features across IPQ platforms:
# - ARM64 architecture by default (ARM32 is also supported)
# - Common memory layout pattern
# - Support for both Premium and Low Memory (LM) profiles

# Define supported IPQ chipsets
# EXTENSION POINT: Add new chipsets to this list
IPQ_CHIPSETS := ipq52xx ipq96xx

# Validate chipset definitions
$(foreach chip,$(IPQ_CHIPSETS),\
  $(if $(wildcard core/arch/arm/plat-qcom/ipq/$(chip)/$(chip).mk),,\
    $(error Missing implementation file for chipset $(chip): core/arch/arm/plat-qcom/ipq/$(chip)/$(chip).mk)) \
  $(if $(filter $(chip),$(filter-out $(chip),$(IPQ_CHIPSETS))),\
    $(error Duplicate chipset in IPQ_CHIPSETS: $(chip))) \
  $(if $(filter ipq%,$(chip)),,\
    $(warning Chipset $(chip) does not follow the ipqXXXX naming convention)))

# Automatically generate flavor lists for all chipsets
$(foreach chip,$(IPQ_CHIPSETS),\
  $(eval $(chip)-lm-flavorlist := $(chip)_lm) \
  $(eval $(chip)-premium-flavorlist := $(chip)_premium $(chip)) \
  $(eval $(chip)-flavorlist := $($(chip)-lm-flavorlist) $($(chip)-premium-flavorlist)))

# Generate a list of all supported flavors
IPQ_ALL_FLAVORS :=
$(foreach chip,$(IPQ_CHIPSETS),\
  $(eval IPQ_ALL_FLAVORS += $($(chip)-flavorlist)))

# Validate that the PLATFORM_FLAVOR is supported
ifeq (,$(filter $(PLATFORM_FLAVOR),$(IPQ_ALL_FLAVORS)))
  ifneq (,$(findstring ipq,$(PLATFORM_FLAVOR)))
    $(error Unsupported IPQ platform flavor: $(PLATFORM_FLAVOR). Supported flavors: $(IPQ_ALL_FLAVORS))
  endif
endif

# Include chipset-specific files based on PLATFORM_FLAVOR
$(foreach chip,$(IPQ_CHIPSETS),\
  $(if $(filter $(PLATFORM_FLAVOR),$($(chip)-flavorlist)),\
    $(eval CFG_$(shell echo $(chip) | tr a-z A-Z) ?= y) \
    $(eval IPQ_CHIPSET := $(chip)) \
    $(eval include core/arch/arm/plat-qcom/ipq/$(chip)/$(chip).mk)))

# Include appropriate profile based on PLATFORM_FLAVOR
ifneq (,$(findstring _lm,$(PLATFORM_FLAVOR)))
  include core/arch/arm/plat-qcom/ipq/profiles/lm.mk
else
  include core/arch/arm/plat-qcom/ipq/profiles/premium.mk
endif
