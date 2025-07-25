# IPQ5200 Platform-Specific Configuration
#
# This file contains IPQ5200 SoC-specific settings and memory configurations.
# Supports both Low Memory (LM) and Premium flavor variants with optimized
# memory layouts and thread configurations.
#
# Key features:
# - LM profile: 512KB memory, single thread, optimized for memory constraints
# - Premium profile: 2.5MB memory, multi-thread, full feature set
# - No REMOTEPROC support: IPQ5200 does not require remote processor functionality

# IPQ5200-specific settings (both profiles)
ifneq (,$(filter $(PLATFORM_FLAVOR),$(ipq5200-flavorlist)))
$(call force,CFG_IPQ5200,y)
$(call force,CFG_TEE_CORE_NB_CORE,4)

# Define the number of cores per cluster used in calculating core position.
# The cluster number is shifted by this value and added to the core ID,
# so its value represents log2(cores/cluster).
# Default is 2**(2) = 4 cores per cluster.
$(call force,CFG_CORE_CLUSTER_SHIFT,2)

# IPQ5200 does not support HWRNG PTA
$(call force,CFG_HWRNG_PTA,n)

# IPQ5200-specific memory layout
CFG_TZDRAM_START ?= 0x8A680000

# IPQ5200 low memory profile
ifneq (,$(filter $(PLATFORM_FLAVOR),$(ipq5200-lm-flavorlist)))
# 512KB for IPQ5200 lm
CFG_TZDRAM_SIZE ?= 0x80000
CFG_TEE_RAM_VA_SIZE ?= 0x80000

$(eval $(call setup-qcom-common))
$(eval $(call setup-lm-config))
endif

# IPQ5200 premium profile
ifneq (,$(filter $(PLATFORM_FLAVOR),$(ipq5200-premium-flavorlist)))
# 2.5MB for IPQ5200 premium
CFG_TZDRAM_SIZE ?= 0x280000
CFG_TEE_RAM_VA_SIZE ?= 0x280000

$(eval $(call setup-qcom-common))
$(eval $(call setup-premium-config))
endif

endif
