# TMEL COM - TME-Lite Communication driver (includes glink-lite, xport_qmp, and tmel_com)
subdirs-$(CFG_QCOM_TMEL_COM) += tmel_com glink_lite xport_qmp

# QRNG driver for Hermosa
subdirs-$(CFG_QCOM_QRNG) += qrng

# QCOM Hardware Unique Key (HUK) API
subdirs-$(CFG_QCOM_HUK) += qcom_huk

# QFPROM driver
subdirs-$(CFG_QCOM_QFPROM) += qfprom
