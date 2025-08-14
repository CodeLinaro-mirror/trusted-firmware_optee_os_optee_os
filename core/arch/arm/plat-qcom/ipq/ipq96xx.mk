# IPQ96xx Platform-Specific Configuration

# This file contains IPQ96xx SoC-specific settings that extend the common
# IPQ platform configuration.
#
# Key features:
# - LM profile: 512KB memory, single thread, optimized for memory constraints
# - Premium profile: 2.5MB memory, multi-thread, full feature set

# IPQ96xx-specific settings (both profiles)
ifneq (,$(filter $(PLATFORM_FLAVOR),$(ipq96xx-flavorlist)))

CFG_IPQ96XX ?= y

# Hardware configuration
# - 5 cores (4 Cortex-A55 + 1 Cortex-A78) in a single cluster
# - CFG_TEE_CORE_NB_CORE defines the total number of cores in the SoC
#   This affects core enumeration and power management
CFG_TEE_CORE_NB_CORE ?= 5
# - CFG_CORE_CLUSTER_SHIFT defines log2 of the number of cores per cluster
# - Use log2(8)=3 to accommodate all 5 cores in the cluster
# - This value is used to calculate core positions in the SoC topology
CFG_CORE_CLUSTER_SHIFT ?= 3

# IPQ96xx-specific memory layout
CFG_TZDRAM_START ?= 0x8A680000

# IPQ96xx GIC configuration (GICv3)
# - CFG_GIC enables the Generic Interrupt Controller driver
# - CFG_ARM_GICV3 specifies that this platform uses GICv3 architecture
# - These settings are critical for proper interrupt handling
CFG_GIC ?= y
CFG_ARM_GICV3 ?= y
# - GIC register base addresses and offsets
# - These values are specific to the IPQ96xx hardware design
CFG_GIC_BASE ?= 0xF200000      # Base physical address of the GIC
CFG_GICD_OFFSET ?= 0x0         # Offset to GIC Distributor registers
CFG_GICR_OFFSET ?= 0x40000     # Offset to GIC Redistributor registers

endif
