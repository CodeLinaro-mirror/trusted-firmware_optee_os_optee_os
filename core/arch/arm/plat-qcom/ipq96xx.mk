# IPQ96xx Platform-Specific Configuration

# This file contains IPQ96xx SoC-specific settings and memory configurations.
# Supports both Low Memory (LM) and Premium flavor variants with optimized
# memory layouts and thread configurations.
#
# Key features:
# - LM profile: 512KB memory, single thread, optimized for memory constraints
# - Premium profile: 2.5MB memory, multi-thread, full feature set

# IPQ96xx-specific settings (both profiles)
ifneq (,$(filter $(PLATFORM_FLAVOR),$(ipq96xx-flavorlist)))
$(call force,CFG_IPQ96XX,y)
# Total 5 cores
$(call force,CFG_TEE_CORE_NB_CORE,5)
# Based on largest cluster (4 cores)
$(call force,CFG_CORE_CLUSTER_SHIFT,2)
$(call force,CFG_GIC,y)
$(call force,CFG_ARM_GICV3,y)

# IPQ96xx GIC configuration
CFG_GIC_BASE ?= 0xF200000
CFG_GICD_OFFSET ?= 0x0
CFG_GICR_OFFSET ?= 0x40000

# IPQ96xx-specific memory layout
CFG_TZDRAM_START ?= 0x8A680000

# IPQ96xx low memory profile
ifneq (,$(filter $(PLATFORM_FLAVOR),$(ipq96xx-lm-flavorlist)))
# 512KB for IPQ96xx lm
CFG_TZDRAM_SIZE ?= 0x80000
CFG_TEE_RAM_VA_SIZE ?= 0x80000

$(eval $(call setup-qcom-common))
$(eval $(call setup-lm-config))
endif

# IPQ96xx premium profile
ifneq (,$(filter $(PLATFORM_FLAVOR),$(ipq96xx-premium-flavorlist)))
# 2.5MB for IPQ96xx premium
CFG_TZDRAM_SIZE ?= 0x280000
CFG_TEE_RAM_VA_SIZE ?= 0x280000

$(eval $(call setup-qcom-common))
$(eval $(call setup-premium-config))
endif

endif
