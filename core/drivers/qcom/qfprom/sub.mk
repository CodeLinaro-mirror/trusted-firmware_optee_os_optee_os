# SPDX-License-Identifier: ISC
#
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
#

# Add platform-specific include path
global-incdirs-y += $(patsubst %_lm,%,$(PLATFORM_FLAVOR))

# QFPROM driver source files
srcs-y += qfprom_core.c

# Fuse provisioning source files
subdirs-y += fuseprov
