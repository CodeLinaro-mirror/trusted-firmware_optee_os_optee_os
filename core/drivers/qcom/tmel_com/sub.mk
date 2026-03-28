# SPDX-License-Identifier: ISC
#
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
#

global-incdirs-y += .
srcs-y += tmecom_client.c
subdirs-y += tmel_fuse
subdirs-$(CFG_QCOM_TMEL_AUTH) += tmel_auth
subdirs-$(CFG_QCOM_TMEL_KM) += tmel_km
subdirs-$(CFG_QCOM_TMEL_RNG) += tmel_rng
