/* SPDX-License-Identifier: ISC */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

/**
 * @file TmeMessagesUids.h
 * @brief Defines all TME messages unique identifiers & their parameters.
 */

#ifndef TMEMESSAGESUIDS_H
#define TMEMESSAGESUIDS_H

/*
 * Documentation
 */

/*
 * TME Messages Unique Identifiers bit layout
 *  _____________________________________
 * |___________|____________|___________|
 * | 31------16| 15-------8 | 7-------0 |
 * | Reserved  |messageType | actionID  |
 * |___________|____________|___________|
 *	       \___________  ___________/
 *			   \/
 *		      TME_MSG_UID
 */

/*
 * TME Messages Unique Identifiers Parameter ID bit layout
 * __________________________________________________________________________
 * |     |     |     |     |     |     |     |     |     |     |     |    |
 * |31-30|29-28|27-26|25-24|23-22|21-20|19-18|17-16|15-14|13-12|11-10|9--8|
 * | p14 | p13 | p12 | p11 | p10 | p9  | p8  | p7  | p6  | p5  | p4  | p3 |
 * |type |type |type |type |type |type |type |type |type |type |type |type|
 * |_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|_____|____|
 *  ____________
 * |    |       |
 * |7--6|3-----0|
 * | p2 | nargs |
 * |type|       |
 * |____|_______|
 */

/* General helper macro to create a bitmask from bits low to high. */
#define MASK_BITS_FROM_UINT32(h, l) \
		((0xffffffff >> (32 - (((h) - (l)) + 1))) << (l))

/*
 * Macro used to define unique TME Message Identifier based on
 * message type and action identifier.
 */
#define TME_MSG_UID_CREATE(m, a) \
	((uint32_t)((((uint32_t)m) & 0xff) << 8) | (((uint32_t)a) & 0xff))

/* Helper macro to extract the messageType from TME_MSG_UID. */
#define TME_MSG_UID_MSG_TYPE(v)      ((((uint32_t)v) & \
				      MASK_BITS_FROM_UINT32(15, 8)) >> 8)

/* Helper macro to extract the actionID from TME_MSG_UID. */
#define TME_MSG_UID_ACTION_ID(v) \
	(((uint32_t)v) & MASK_BITS_FROM_UINT32(7, 0))

/*
 * Helper Macros to create paramID for every unique TME Message Identifier.
 */
/*
 * A parameter of type value. TME receive data as part of request,
 * can also use to send respose.
 */
#define TME_MSG_PARAM_TYPE_VAL                0x0
/*
 * A parameter of type input only. TME receive data through buffer as part
 * of request, doesn't send response through it. It consist of 2 actual
 * uint32_t param (ipBufAddr & ipBufLen)
 * ipBufAddr  - Address of the buffer. TME should't change it.
 * ipBufLen   - Byte length of input data to be consumed by TME.
 *		TME shouldn't change it.
 */
#define TME_MSG_PARAM_TYPE_BUF_IN             0x1
/*
 * A parameter of type output only. TME doesn't receive data through buffer
 * as part of request, instead use this buffer to send response.
 * It consist of 3 actual uint32_t param (outBufAddr, outBufLen &
 * outBufOutLen).
 * outBufAddr   - Address of the buffer. TME shouldn't change it.
 * outBufLen    - Indiactes actual/allocated size of the buffer,
 *		  TME shouldn't change it.
 * outBufOutLen - TME update to actual out data length in byte.
 */
#define TME_MSG_PARAM_TYPE_BUF_OUT            0x2
/*
 * A parameter of type both input & output. TME can receive data through
 * the buffer, as well can use this buffer to send response.
 * It consist of 3 actual uint32_t param (inOutBufAddr, inOutBufLen &
 * inOutBufInOutLen).
 * inOutBufAddr     - Address of the buffer. TME shouldn't change it.
 * inOutBufLen      - Indiactes actual/allocated size of the buffer,
 *		      TME shouldn't change it.
 * inOutBufInOutLen - can hold actual input data length during request
 *		      & TME update to actual out data length in byte for a
 *		      response.
 */
#define TME_MSG_PARAM_TYPE_BUF_IN_OUT         0x3

/* Parameter ID nargs bitmask. */
#define TME_MSG_PARAM_ID_NARGS_MASK        MASK_BITS_FROM_UINT32(3, 0)
/* Parameter ID parameter type bitmask. */
#define TME_MSG_PARAM_ID_PARAM_TYPE_MASK   MASK_BITS_FROM_UINT32(1, 0)

/* Internal helper macro for __TME_MSG_CREATE_PARAM_ID. */
#define _TME_MSG_CREATE_PARAM_ID(nargs, p1, p2, p3, p4, p5, p6, p7, \
				 p8, p9, p10, p11, p12, p13, p14, ...) \
	(((nargs) & TME_MSG_PARAM_ID_NARGS_MASK) + \
	(((p1) & TME_MSG_PARAM_ID_PARAM_TYPE_MASK) << 4) +  \
	(((p2) & TME_MSG_PARAM_ID_PARAM_TYPE_MASK) << 6) +  \
	(((p3) & TME_MSG_PARAM_ID_PARAM_TYPE_MASK) << 8) +  \
	(((p4) & TME_MSG_PARAM_ID_PARAM_TYPE_MASK) << 10) + \
	(((p5) & TME_MSG_PARAM_ID_PARAM_TYPE_MASK) << 12) + \
	(((p6) & TME_MSG_PARAM_ID_PARAM_TYPE_MASK) << 14) + \
	(((p7) & TME_MSG_PARAM_ID_PARAM_TYPE_MASK) << 16) + \
	(((p8) & TME_MSG_PARAM_ID_PARAM_TYPE_MASK) << 18) + \
	(((p9) & TME_MSG_PARAM_ID_PARAM_TYPE_MASK) << 20) + \
	(((p10) & TME_MSG_PARAM_ID_PARAM_TYPE_MASK) << 22) + \
	(((p11) & TME_MSG_PARAM_ID_PARAM_TYPE_MASK) << 24) + \
	(((p12) & TME_MSG_PARAM_ID_PARAM_TYPE_MASK) << 26) + \
	(((p13) & TME_MSG_PARAM_ID_PARAM_TYPE_MASK) << 28) + \
	(((p14) & TME_MSG_PARAM_ID_PARAM_TYPE_MASK) << 30))

/* Internal helper macro to get nargs from paramID */
#define TME_MSG_PARAM_ID_GET_NARGS(v) \
		(((uint32_t)v) & TME_MSG_PARAM_ID_NARGS_MASK)

/* Internal helper macro to get ith parameter from paramID */
#define TME_MSG_PARAM_ID_GET_PARAM_TYPEI(v, i) \
		((((uint32_t)v) >> ((2 * (i)) + 4)) & \
		 TME_MSG_PARAM_ID_PARAM_TYPE_MASK)

/* Internal helper macro for TME_MSG_CREATE_PARAM_ID_X */
#define __TME_MSG_CREATE_PARAM_ID(...) \
	_TME_MSG_CREATE_PARAM_ID(__VA_ARGS__, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, \
				 0, 0, 0, 0)

/* Macro used to create a parameter ID with no arguments. */
#define TME_MSG_CREATE_PARAM_ID_0 \
	__TME_MSG_CREATE_PARAM_ID(0)
/* Macro used to create a parameter ID with one argument. */
#define TME_MSG_CREATE_PARAM_ID_1(p1) \
	__TME_MSG_CREATE_PARAM_ID(1, p1)
/* Macro used to create a parameter ID with two arguments. */
#define TME_MSG_CREATE_PARAM_ID_2(p1, p2) \
	__TME_MSG_CREATE_PARAM_ID(2, p1, p2)
/* Macro used to create a parameter ID with three arguments. */
#define TME_MSG_CREATE_PARAM_ID_3(p1, p2, p3) \
	__TME_MSG_CREATE_PARAM_ID(3, p1, p2, p3)
/* Macro used to create a parameter ID with four arguments. */
#define TME_MSG_CREATE_PARAM_ID_4(p1, p2, p3, p4) \
	__TME_MSG_CREATE_PARAM_ID(4, p1, p2, p3, p4)
/* Macro used to create a parameter ID with five arguments. */
#define TME_MSG_CREATE_PARAM_ID_5(p1, p2, p3, p4, p5) \
	__TME_MSG_CREATE_PARAM_ID(5, p1, p2, p3, p4, p5)
/* Macro used to create a parameter ID with six arguments. */
#define TME_MSG_CREATE_PARAM_ID_6(p1, p2, p3, p4, p5, p6) \
	__TME_MSG_CREATE_PARAM_ID(6, p1, p2, p3, p4, p5, p6)
/* Macro used to create a parameter ID with seven arguments. */
#define TME_MSG_CREATE_PARAM_ID_7(p1, p2, p3, p4, p5, p6, p7) \
	__TME_MSG_CREATE_PARAM_ID(7, p1, p2, p3, p4, p5, p6, p7)
/* Macro used to create a parameter ID with eight arguments. */
#define TME_MSG_CREATE_PARAM_ID_8(p1, p2, p3, p4, p5, p6, p7, p8) \
	__TME_MSG_CREATE_PARAM_ID(8, p1, p2, p3, p4, p5, p6, p7, p8)
/* Macro used to create a parameter ID with nine arguments. */
#define TME_MSG_CREATE_PARAM_ID_9(p1, p2, p3, p4, p5, p6, p7, p8, p9) \
	__TME_MSG_CREATE_PARAM_ID(9, p1, p2, p3, p4, p5, p6, p7, p8, p9)
/* Macro used to create a parameter ID with ten arguments. */
#define TME_MSG_CREATE_PARAM_ID_10(p1, p2, p3, p4, p5, p6, p7, p8, p9, p10) \
	__TME_MSG_CREATE_PARAM_ID(10, p1, p2, p3, p4, p5, p6, p7, \
				  p8, p9, p10)
/* Macro used to create a parameter ID with eleven arguments. */
#define TME_MSG_CREATE_PARAM_ID_11(p1, p2, p3, p4, p5, p6, p7, p8, p9, \
				   p10, p11) \
	__TME_MSG_CREATE_PARAM_ID(11, p1, p2, p3, p4, p5, p6, p7, \
				  p8, p9, p10, p11)
/* Macro used to create a parameter ID with twelve arguments. */
#define TME_MSG_CREATE_PARAM_ID_12(p1, p2, p3, p4, p5, p6, p7, p8, p9, \
				   p10, p11, p12) \
	__TME_MSG_CREATE_PARAM_ID(12, p1, p2, p3, p4, p5, p6, p7, \
				  p8, p9, p10, p11, p12)
/* Macro used to create a parameter ID with thirteen arguments. */
#define TME_MSG_CREATE_PARAM_ID_13(p1, p2, p3, p4, p5, p6, p7, p8, p9, \
				   p10, p11, p12, p13) \
	__TME_MSG_CREATE_PARAM_ID(13, p1, p2, p3, p4, p5, p6, p7, \
				  p8, p9, p10, p11, p12, p13)
/* Macro used to create a parameter ID with fourteen arguments. */
#define TME_MSG_CREATE_PARAM_ID_14(p1, p2, p3, p4, p5, p6, p7, p8, p9, \
				   p10, p11, p12, p13, p14) \
	__TME_MSG_CREATE_PARAM_ID(14, p1, p2, p3, p4, p5, p6, p7, \
				  p8, p9, p10, p11, p12, p13, p14)

/*
 * All definitions of supported messageType's.
 *
 * 0x00 -> 0xF0 messageType used for production use cases.
 * 0xF1 -> 0xFF messageType reserved(can be used for test puprposes).
 *
 * <Template> : TME_MSG_<MSGTYPE_NAME>
 */
#define TME_MSG_SECBOOT			0x00
#define TME_MSG_CRYPTO			0x01
#define TME_MSG_MC3			0x02
#define TME_MSG_FUSE			0x03
#define TME_MSG_ACCESS_CONTROL		0x04
#define TME_MSG_ATTESTATION		0x05
#define TME_MSG_ONBOARDING		0x06
#define TME_MSG_KM			0x07 /* Key management services */
#define TME_MSG_FWUP			0x08
#define TME_MSG_LOG			0x09
#define TME_MSG_FEATURE_LICENSE		0x0A
#define TME_MSG_HCS			0x0B /* Host crypto services */
#define TME_MSG_QWES			0x0C /* QWES services */
#define TME_MSG_CDUMP			0x0D /* TME-L Crashdump */
#define TME_MSG_SCP			0x0E /* SCP (SCP11a, SCP03) related */
#define TME_MSG_UWB_KD			0x0F /* UWB Key Derivation services */
#define TME_MSG_TIMETEST		0x10 /* Performance Framework*/

/* Test use-cases */
#define TME_MSG_LOOPBACK_TEST		0xFF

/*
 * All definitions of action ID's per messageType.
 *
 * 0x00 -> 0xBF actionID used for production use cases.
 * 0xC0 -> 0xFF messageType must be reserved for test use cases.
 *
 * NOTE: Test ID's shouldn't appear in this file.
 *
 * <Template> : TME_ACTION_<MSGTYPE_NAME>_<ACTIONID_NAME>
 */

/*
 * Action ID's for TME_MSG_SECBOOT
 */
#define TME_ACTION_SECBOOT_REQ				0x01
#define TME_ACTION_SECBOOT_BT_PATCH_AUTH		0x02
#define TME_ACTION_SECBOOT_OEM_MRC_STATE_UPDATE		0x03
#define TME_ACTION_SECBOOT_SEC_AUTH			0x04
#define TME_ACTION_SECBOOT_ELF_IMAGE_SIGN_VERIFY	0x05
#define TME_ACTION_SECBOOT_ELF_IMAGE_SEGMENTS_AUTH	0x06
#define TME_ACTION_SECBOOT_NOTIFY_BOOT_MILESTONE	0x07
#define TME_ACTION_SECBOOT_UPDATE_ARB_VERSION		0x08
#define TME_ACTION_SECBOOT_GET_ARB_VERSION		0x09
#define TME_ACTION_SECBOOT_SS_TEAR_DOWN			0x0A
#define TME_ACTION_SECBOOT_XIP_AUTH			0x0B
#define TME_ACTION_SECBOOT_GET_STATE			0x0C
#define TME_ACTION_SECBOOT_UPDATE_ARB_VER_SWID_LIST	0x0D
#define TME_ACTION_SECBOOT_GET_DEBUG_STATE_VECTOR	0x0E
#define TME_ACTION_SECBOOT_SEC_AUTH_V2			0x0F

/*
 * Action ID's for TME_MSG_CRYPTO
 */
#define TME_ACTION_CRYPTO_INVALID			0x00
#define TME_ACTION_CRYPTO_GET_RANDOM			0x01
#define TME_ACTION_CRYPTO_GET_RANDOM_ECC_VALUE		0x02
#define TME_ACTION_CRYPTO_ECC_PUB_EXTRACT		0x03
#define TME_ACTION_CRYPTO_MULTIPLY_ECC_VALUE		0x04
#define TME_ACTION_CRYPTO_DIGEST			0x05
#define TME_ACTION_CRYPTO_DIGEST_EX			0x06
#define TME_ACTION_CRYPTO_GENERATE_KEYPAIR		0x07
#define TME_ACTION_CRYPTO_KEY_IMPORT			0x08
#define TME_ACTION_CRYPTO_KEY_CLEAR			0x09
#define TME_ACTION_CRYPTO_HMAC				0x0A
#define TME_ACTION_CRYPTO_AES_ENCRYPT_EX		0x0B
#define TME_ACTION_CRYPTO_AES_DECRYPT_EX		0x0C
#define TME_ACTION_CRYPTO_ECDSA_SIGN_BUFFER		0x0D
#define TME_ACTION_CRYPTO_ECDSA_VERIFY_BUFFER		0x0E
#define TME_ACTION_CRYPTO_ECDSA_SIGN_DIGEST		0x0F
#define TME_ACTION_CRYPTO_ECDSA_VERIFY_DIGEST		0x10
#define TME_ACTION_CRYPTO_ECC_GET_PUBLIC_KEY		0x11
#define TME_ACTION_CRYPTO_GENERATE_KEY			0x12
#define TME_ACTION_CRYPTO_DERIVE_SHARED_SECRET		0x13
#define TME_ACTION_CRYPTO_ECC_MUL_POINT_BY_GEN		0x14
#define TME_ACTION_CRYPTO_DERIVE_KEY			0x15
#define TME_ACTION_CRYPTO_WRAP_KEY			0x16
#define TME_ACTION_CRYPTO_UNWRAP_KEY			0x17

/*
 * Action ID's for TME_MSG_HCS
 */
#define TME_ACTION_HCS_INVALID				0x00
#define TME_ACTION_HCS_SHA_DIGEST			0x01
#define TME_ACTION_HCS_HMAC_DIGEST			0x02
#define TME_ACTION_HCS_ECC_SIGN_DIGEST			0x03
#define TME_ACTION_HCS_ECC_SIGN_MSG			0x04
#define TME_ACTION_HCS_ECC_VERIFY_DIGEST		0x05
#define TME_ACTION_HCS_ECC_VERIFY_MSG			0x06
#define TME_ACTION_HCS_ECC_GET_PUBKEY			0x07
#define TME_ACTION_HCS_ECDH_GET_PUBKEY			0x08
#define TME_ACTION_HCS_ECDH_SHARED_SECRET		0x09
#define TME_ACTION_HCS_AES_ENCRYPT			0x0A
#define TME_ACTION_HCS_AES_DECRYPT			0x0B
#define TME_ACTION_HCS_PRNG_GET				0x0C
#define TME_ACTION_HCS_INC_SHA_INIT			0x0D
#define TME_ACTION_HCS_INC_SHA_UPDATE			0x0E
#define TME_ACTION_HCS_INC_SHA_FINAL			0x0F
#define TME_ACTION_HCS_GET_FIPS_ALGO_LIST		0x10
#define TME_ACTION_HCS_INCR_AES_ENCRYPT_INIT		0x11
#define TME_ACTION_HCS_INCR_AES_ENCRYPT_UPDATE		0x12
#define TME_ACTION_HCS_INCR_AES_ENCRYPT_FINAL		0x13
#define TME_ACTION_HCS_INCR_AES_DECRYPT_INIT		0x14
#define TME_ACTION_HCS_INCR_AES_DECRYPT_UPDATE		0x15
#define TME_ACTION_HCS_INCR_AES_DECRYPT_FINAL		0x16

/* DV Test use-cases */
#define TME_ACTION_HCS_FIPS_DV_TEST_CASE		0xFF

/*
 * Action ID's for TME_MSG_KM
 */
#define TME_ACTION_KM_INVALID				0x00
#define TME_ACTION_KM_CLEAR				0x01
#define TME_ACTION_KM_IMPORT				0x02
#define TME_ACTION_KM_GENERATE				0x03
#define TME_ACTION_KM_DERIVE				0x04
#define TME_ACTION_KM_WRAP				0x05
#define TME_ACTION_KM_UNWRAP				0x06
#define TME_ACTION_KM_DISTRIBUTE			0x07
#define TME_ACTION_KM_EXPORT_ECDH_IP_KEY		0x08
#define TME_ACTION_KM_CLEAR_SS_KEYS_ALL			0x09
#define TME_ACTION_KM_QBEC_WRAP_KEY			0x0A
#define TME_ACTION_KM_MULTI_DISTRIBUTE_KEY		0x0B

/*
 * Action ID's for TME_MSG_MC3
 */
#define TME_ACTION_MC3_INVALID				0x00
#define TME_ACTION_MC3_AUTHENTICATION_REQ		0x01
#define TME_ACTION_MC3_ATTESTATION_REQ			0x02
#define TME_ACTION_MC3_ONBOARDING_REQ			0x03
#define TME_ACTION_MC3_ONBOARDING_CONFIRMATION		0x04
#define TME_ACTION_MC3_DECRYPT_MESSAGE_REQ		0x05
#define TME_ACTION_MC3_DECRYPT_MESSAGE_CONFIRMATION	0x06
#define TME_ACTION_MC3_AUTHENTICATE_MSG_REQ		0x07
#define TME_ACTION_MC3_AUTHENTICATE_MSG_CONFIRMATION	0x08

/*
 * Action ID's for TME_MSG_FUSE
 */
#define TME_ACTION_FUSE_READ_SINGLE			0x00
#define TME_ACTION_FUSE_READ_MULTIPLE			0x01
#define TME_ACTION_FUSE_WRITE_SINGLE			0x02
#define TME_ACTION_FUSE_WRITE_MULTIPLE			0x03
#define TME_ACTION_FUSE_WRITE_SECURE			0x04
#define TME_ACTION_FUSE_READ_SINGLE_ROW			0x05
#define TME_ACTION_FUSE_READ_MULTIPLE_ROW		0x06
#define TME_ACTION_FUSE_WRITE_SINGLE_ROW		0x07
#define TME_ACTION_FUSE_WRITE_MULTIPLE_ROW		0x08
#define TME_ACTION_FUSE_ROM_PATCH_REQ			0x09

/*
 * Action ID's for TME_MSG_ACCESS_CONTROL
 */
#define TME_ACTION_ACCESS_CONTROL_INVALID		0x00
#define TME_ACTION_ACCESS_CONTROL_NVM_REGISTER		0x01

#define TME_ACTION_ACCESS_CONTROL_LOCK_RG_FOR_QAD	0x02
#define TME_ACTION_ACCESS_CONTROL_DISABLE_LOCK_RG_IPC	0x03
#define TME_ACTION_ACCESS_CONTROL_PROTECT_TMEL_RESOURCES 0x04
#define TME_ACTION_ACCESS_CONTROL_SET_XPU_DBGAR		0x05
#define TME_ACTION_ACCESS_CONTROL_ENABLE_SILENT_LOGGING 0x06
#define TME_ACTION_ACCESS_CONTROL_READ_SYNDROME_IPC	0x07

/*
 * Action ID's for TME_MSG_FWUP
 */
#define TME_ACTION_FWUP_VERIFY_UPDATE			0x01
#define TME_ACTION_FWUP_APPLY_UPDATE			0x02

/*
 * Action ID's for TME_MSG_LOG
 */
#define TME_ACTION_LOG_SET_CONFIG			0x00
#define TME_ACTION_LOG_GET_CONFIG			0x01
#define TME_ACTION_LOG_GET				0x02
#define TME_ACTION_Q6LOG_GET				0x03

/*
 * Action ID's for TME_MSG_FEATURE_LICENSE
 */
#define TME_ACTION_FEATURE_LICENSE_BTSS			0x00

/*
 * Action ID's for TME_MSG_QWES
 */
#define TME_ACTION_QWES_INIT_ATTESTATION		0x00
#define TME_ACTION_QWES_DEVICE_ATTESTATION		0x01
#define TME_ACTION_QWES_DEVICE_PROVISIONING		0x02
#define TME_ACTION_QWES_LICENSING_INSTALL		0x03
#define TME_ACTION_QWES_LICENSING_CHECK			0x04
#define TME_ACTION_QWES_LICENSING_ENFORCEHWFEATURES	0x05
#define TME_ACTION_QWES_TTIME_CLOUD_REQUEST		0x06
#define TME_ACTION_QWES_TTIME_SET			0x07
#define TME_ACTION_QWES_TTIME_GET_INFO_CBOR		0x08
#define TME_ACTION_QWES_LICENSING_TBDLICENSES		0x09

/*
 * Action ID's for TME_MSG_CDUMP
 */
#define TME_ACTION_CDUMP_SAVE_DUMP_ADDR			0x00

/*
 * Action ID's for TME_MSG_SCP
 */
#define TME_ACTION_SCP_SCP03_RDSCAPDU			0x00
#define TME_ACTION_SCP_SCP03_RDSRAPDU			0x01
#define TME_ACTION_SCP_SCP11A_INITIATE			0x02
#define TME_ACTION_SCP_SCP11A_ESTABLISH_SESSION		0x03
#define TME_ACTION_SCP_SCP03_PROCESS_CAPDU		0x04
#define TME_ACTION_SCP_SCP03_PROCESS_RAPDU		0x05
#define TME_ACTION_SCP_SCP03_GET_BIND_CTR		0x06
#define TME_ACTION_SCP_CLOSE_SESSION			0x07
#define TME_ACTION_SCP_TEST_DATA_PROV			0x08

/*
 * Action ID's for TME_MSG_UWB_KD
 */
#define TME_ACTION_UWB_CCC_TMESALTEDHASH		0x00
#define TME_ACTION_UWB_CCC_TMEUAD			0x01
#define TME_ACTION_UWB_CCC_TMEMUPSK1			0x02
#define TME_ACTION_UWB_CCC_TMEDURSKDUDSK		0x03
#define TME_ACTION_UWB_FIRA_TMEImportSecSessionKey	0x04
#define TME_ACTION_UWB_FIRA_TMEDERIVEDCONFIGDIGEST	0x05
#define TME_ACTION_UWB_FIRA_TMESECDATAPRIVACYKEY	0x06
#define TME_ACTION_UWB_FIRA_TMESECDATAPROTECTIONKEY	0x07
#define TME_ACTION_UWB_FIRA_TMEPHYSTSINDEX		0x08
#define TME_ACTION_UWB_FIRA_TMESECDERIVEDAUTHENTICATION 0x09
#define TME_ACTION_UWB_FIRA_TMESECMASKINGKEY		0x0A

/*
 * Action ID's for TME_MSG_TIMETEST
 */
#define TME_ACTION_TIMETEST_CONFIG_TTDATA		0x00

/*
 * All definitions of TME Message UID's (messageType | actionID) and paramID
 * for each UID.
 *
 * <Template> : TME_MSG_UID_<MSGTYPE_NAME>_<ACTIONID_NAME>
 * <Template> : TME_MSG_UID_<MSGTYPE_NAME>_<ACTIONID_NAME>_PARAM_ID
 */

/*
 * UID's & PARAM_ID's for TME_MSG_SECBOOT
 */
/*
 * BT patch Authentication Request.
 * param_id (Elfhdr pointer, program header pointer, hash table pointer,
 * response)
 *
 * Note: Deprecated.
 */
#define TME_MSG_UID_SECBOOT_BT_PATCH_AUTH \
		TME_MSG_UID_CREATE(TME_MSG_SECBOOT, \
		TME_ACTION_SECBOOT_BT_PATCH_AUTH)

#define TME_MSG_UID_SECBOOT_BT_PATCH_AUTH_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4( \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * OEM MRC Update Request
 * param_id (MRC activation VAl, MRC Revocation Val and response Val)
 */
#define TME_MSG_UID_SECBOOT_OEM_MRC_STATE_UPDATE \
		TME_MSG_UID_CREATE(TME_MSG_SECBOOT, \
		TME_ACTION_SECBOOT_OEM_MRC_STATE_UPDATE)

#define TME_MSG_UID_SECBOOT_OEM_MRC_STATE_UPDATE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Elf Image Signature & Segment Hash Verification Request.
 * param_id (swId, elfBuf, regionList, relocate, fSegmentAddr, fSegmentLen,
 * entryAddr, extended_error, status)
 */
#define TME_MSG_UID_SECBOOT_SEC_AUTH \
		TME_MSG_UID_CREATE(TME_MSG_SECBOOT, \
		TME_ACTION_SECBOOT_SEC_AUTH)
#define TME_MSG_UID_SECBOOT_SEC_AUTH_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_9( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Elf Image Signature & Segment Hash Verification Request.
 * param_id (swId, elfBuf, regionList, relocate, inputBitFields,
 * reservedBuf, fSegmentAddr, fSegmentLen, entryAddr, extended_error,
 * status, keyHandle)
 */
#define TME_MSG_UID_SECBOOT_SEC_AUTH_V2 \
		TME_MSG_UID_CREATE(TME_MSG_SECBOOT, \
		TME_ACTION_SECBOOT_SEC_AUTH_V2)
#define TME_MSG_UID_SECBOOT_SEC_AUTH_V2_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_12( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Elf Image Signature Verification Request.
 * param_id (swId, elfHdrBuf, elfHdrPaddingSize, progHdrBuf, hashSegBuf,
 * whitelistBuf, status)
 *
 * Note: Deprecated.
 */
#define TME_MSG_UID_SECBOOT_ELF_IMAGE_SIGN_VERIFY \
		TME_MSG_UID_CREATE(TME_MSG_SECBOOT, \
		TME_ACTION_SECBOOT_ELF_IMAGE_SIGN_VERIFY)
#define TME_MSG_UID_SECBOOT_ELF_IMAGE_SIGN_VERIFY_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_7( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Elf Image Segment Hash Verification Request.
 * param_id (swId, regionList, relocate, entryAddr, status)
 *
 * Note: Deprecated.
 */
#define TME_MSG_UID_SECBOOT_ELF_IMAGE_SEGMENTS_AUTH \
		TME_MSG_UID_CREATE(TME_MSG_SECBOOT, \
		TME_ACTION_SECBOOT_ELF_IMAGE_SEGMENTS_AUTH)
#define TME_MSG_UID_SECBOOT_ELF_IMAGE_SEGMENTS_AUTH_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_5( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Secure Boot Milestone Notification.
 * param_id (milestone, status)
 */
#define TME_MSG_UID_SECBOOT_NOTIFY_BOOT_MILESTONE \
		TME_MSG_UID_CREATE(TME_MSG_SECBOOT, \
		TME_ACTION_SECBOOT_NOTIFY_BOOT_MILESTONE)
#define TME_MSG_UID_SECBOOT_NOTIFY_BOOT_MILESTONE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Secure Boot Anti Rollback Version Update Request.
 * param_id (status)
 */
#define TME_MSG_UID_SECBOOT_UPDATE_ARB_VERSION \
		TME_MSG_UID_CREATE(TME_MSG_SECBOOT, \
		TME_ACTION_SECBOOT_UPDATE_ARB_VERSION)
#define TME_MSG_UID_SECBOOT_UPDATE_ARB_VERSION_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_1( \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Secure Boot Get Anti Rollback Version Request.
 * param_id (swId, versionInfo, status)
 */
#define TME_MSG_UID_SECBOOT_GET_ARB_VERSION \
		TME_MSG_UID_CREATE(TME_MSG_SECBOOT, \
		TME_ACTION_SECBOOT_GET_ARB_VERSION)
#define TME_MSG_UID_SECBOOT_GET_ARB_VERSION_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Secure Boot Sub-System Tear Down Request.
 * param_id (swId, secondarySwId, status)
 */
#define TME_MSG_UID_SECBOOT_SS_TEAR_DOWN \
		TME_MSG_UID_CREATE(TME_MSG_SECBOOT, \
		TME_ACTION_SECBOOT_SS_TEAR_DOWN)
#define TME_MSG_UID_SECBOOT_SS_TEAR_DOWN_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * XIP Auth Request
 * param_id (OEM MISC pkg start address, response, apps_sec_entrypoint)
 *
 * Note: Deprecated. Used in Slate and Helios.
 */
#define TME_MSG_UID_SECBOOT_XIP_AUTH \
		TME_MSG_UID_CREATE(TME_MSG_SECBOOT, \
		TME_ACTION_SECBOOT_XIP_AUTH)

#define TME_MSG_UID_SECBOOT_XIP_AUTH_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Secure Boot Sub-System get tme state request.
 * param_id (State, patch_version, status)
 */
#define TME_MSG_UID_SECBOOT_GET_STATE \
		TME_MSG_UID_CREATE(TME_MSG_SECBOOT, \
		TME_ACTION_SECBOOT_GET_STATE)
#define TME_MSG_UID_SECBOOT_GET_STATE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Secure Boot Anti Rollback Version Update(based on the swId List) Request.
 * param_id (swIdList, status)
 */
#define TME_MSG_UID_SECBOOT_UPDATE_ARB_VER_SWID_LIST \
		TME_MSG_UID_CREATE(TME_MSG_SECBOOT, \
		TME_ACTION_SECBOOT_UPDATE_ARB_VER_SWID_LIST)
#define TME_MSG_UID_SECBOOT_UPDATE_ARB_VER_SWID_LIST_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2( \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL)

#define TME_MSG_UID_SECBOOT_GET_DEBUG_STATE_VECTOR \
		TME_MSG_UID_CREATE(TME_MSG_SECBOOT, \
		TME_ACTION_SECBOOT_GET_DEBUG_STATE_VECTOR)
#define TME_MSG_UID_SECBOOT_GET_DEBUG_STATE_VECTOR_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * UID's & PARAM_ID's for TME_MSG_CRYPTO
 */

/*
 * Generate Random Data.
 * @param_id (length, status, rspBuf)
 */
#define TME_MSG_UID_CRYPTO_GET_RANDOM \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_GET_RANDOM)

#define TME_MSG_UID_CRYPTO_GET_RANDOM_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Generate Random ECC.
 * @param_id (curve, status, rspBuf)
 */
#define TME_MSG_UID_CRYPTO_GET_RANDOM_ECC_VALUE \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_GET_RANDOM_ECC_VALUE)
#define TME_MSG_UID_CRYPTO_GET_RANDOM_ECC_VALUE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Generate ECC Pub.
 * @param_id (curve, pvtKeyBuf, status, rspBuf)
 */
#define TME_MSG_UID_CRYPTO_ECC_PUB_EXTRACT \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_ECC_PUB_EXTRACT)
#define TME_MSG_UID_CRYPTO_ECC_PUB_EXTRACT_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4( \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Deprecated
 * Multiply ECC value by point message
 * @param_id (curve, pvtKeyBuf, pubKeyBuf, status, rspBuf)
 */
#define TME_MSG_UID_CRYPTO_MULTIPLY_ECC_VALUE \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_MULTIPLY_ECC_VALUE)
#define TME_MSG_UID_CRYPTO_MULTIPLY_ECC_VALUE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_5(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Digest
 * @param_id (algo, inBuf, status, rspBuf)
 */
#define TME_MSG_UID_CRYPTO_DIGEST \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_DIGEST)
#define TME_MSG_UID_CRYPTO_DIGEST_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4( \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Digest_Ex
 * @param_id (algo, numInputs, inBuf, status, rspBuf)
 */
#define TME_MSG_UID_CRYPTO_DIGEST_EX \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_DIGEST_EX)
#define TME_MSG_UID_CRYPTO_DIGEST_EX_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_5(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Generate KeyPair
 * @param_id (curve, status, pubKeyBuf, pvtKeyHandle)
 */
#define TME_MSG_UID_CRYPTO_GENERATE_KEYPAIR \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_GENERATE_KEYPAIR)
#define TME_MSG_UID_CRYPTO_GENERATE_KEYPAIR_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4( \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, TME_MSG_PARAM_TYPE_VAL)

/*
 * Import Key
 * @param_id (keyMaterialBuf, keyConfig, status, keyHandle)
 */
#define TME_MSG_UID_CRYPTO_KEY_IMPORT \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_KEY_IMPORT)
#define TME_MSG_UID_CRYPTO_KEY_IMPORT_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4( \
			TME_MSG_PARAM_TYPE_BUF_IN, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_VAL)

/*
 * Clear Key
 * @param_id (keyHandle, status)
 */
#define TME_MSG_UID_CRYPTO_KEY_CLEAR \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_KEY_CLEAR)
#define TME_MSG_UID_CRYPTO_KEY_CLEAR_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2( \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_VAL)

/*
 * HMAC
 * @param_id (algo, inBuf, initialMac, keyHandle, status, response)
 */
#define TME_MSG_UID_CRYPTO_HMAC \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_HMAC)
#define TME_MSG_UID_CRYPTO_HMAC_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_6( \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * AES Encryption Ex
 * @param_id (keyBuf, keyHandle, algo, ptBuf, aadBuf, status, ctBuf,
 * ivBuf, tagBuf)
 */
#define TME_MSG_UID_CRYPTO_AES_ENCRYPT_EX \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_AES_ENCRYPT_EX)
#define TME_MSG_UID_CRYPTO_AES_ENCRYPT_EX_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_9( \
			TME_MSG_PARAM_TYPE_BUF_IN, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * AES Decryption Ex
 * @param_id (keyBuf, keyHandle, algo, ctBuf, ivBuf, aadBuf, tagBuf,
 * status, response)
 */
#define TME_MSG_UID_CRYPTO_AES_DECRYPT_EX \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_AES_DECRYPT_EX)
#define TME_MSG_UID_CRYPTO_AES_DECRYPT_EX_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_9( \
			TME_MSG_PARAM_TYPE_BUF_IN, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * ECDSA Signature
 * @param_id (algo, keyHandle, curve, ptBuf,  status, response)
 */
#define TME_MSG_UID_CRYPTO_ECDSA_SIGN_BUFFER \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_ECDSA_SIGN_BUFFER)
#define TME_MSG_UID_CRYPTO_ECDSA_SIGN_BUFFER_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_6( \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * ECDSA Verify
 * @param_id (algo, pubKeyBuf, curve, ptBuf, sigBuf, status)
 */
#define TME_MSG_UID_CRYPTO_ECDSA_VERIFY_BUFFER \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_ECDSA_VERIFY_BUFFER)
#define TME_MSG_UID_CRYPTO_ECDSA_VERIFY_BUFFER_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_6( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, TME_MSG_PARAM_TYPE_VAL)

/*
 * ECDSA Signature Digest
 * @param_id (keyHandle, curve, digestBuf,  status, response)
 */
#define TME_MSG_UID_CRYPTO_ECDSA_SIGN_DIGEST \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_ECDSA_SIGN_DIGEST)
#define TME_MSG_UID_CRYPTO_ECDSA_SIGN_DIGEST_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_5( \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * ECDSA Verify Digest
 * @param_id (pubkeyBuf, curve, digestBuf,  sigBuf, status)
 */
#define TME_MSG_UID_CRYPTO_ECDSA_VERIFY_DIGEST \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_ECDSA_VERIFY_DIGEST)
#define TME_MSG_UID_CRYPTO_ECDSA_VERIFY_DIGEST_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_5( \
			TME_MSG_PARAM_TYPE_BUF_IN, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * ECC Get Public Key
 * @param_id (curve, keyHandle, status, response)
 */
#define TME_MSG_UID_CRYPTO_ECC_GET_PUBLIC_KEY \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_ECC_GET_PUBLIC_KEY)
#define TME_MSG_UID_CRYPTO_ECC_GET_PUBLIC_KEY_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4( \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Generate Key
 * @param_id (keyConfig, status, keyHandle)
 */
#define TME_MSG_UID_CRYPTO_GENERATE_KEY \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_GENERATE_KEY)
#define TME_MSG_UID_CRYPTO_GENERATE_KEY_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_VAL)

/*
 * Derive Shared Secret
 * @param_id (curve, keyHandle, pubKeyBuf, status, keyHandle)
 */
#define TME_MSG_UID_CRYPTO_DERIVE_SHARED_SECRET \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_DERIVE_SHARED_SECRET)
#define TME_MSG_UID_CRYPTO_DERIVE_SHARED_SECRET_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_6( \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_VAL)

/*
 * Deprecated
 * ECC Extended Public Key
 * @param_id (curve, pvtKeyBuf, status, response)
 */
#define TME_MSG_UID_CRYPTO_ECC_MUL_POINT_BY_GEN \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_ECC_MUL_POINT_BY_GEN)
#define TME_MSG_UID_CRYPTO_ECC_MUL_POINT_BY_GEN_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Derive Key
 * @param_id (nonceBuf, algo, keyCfg, inKey, labelBuf, status, derivedKey)
 */
#define TME_MSG_UID_CRYPTO_DERIVE_KEY \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_DERIVE_KEY)
#define TME_MSG_UID_CRYPTO_DERIVE_KEY_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_7( \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_VAL)
/*
 * Wrap Key
 * @param_id (KwKID, SrcKID, status, wrappedKeyBuf, TagBuf, nonceBuf)
 */
#define TME_MSG_UID_CRYPTO_WRAP_KEY \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_WRAP_KEY)
#define TME_MSG_UID_CRYPTO_WRAP_KEY_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_6( \
			TME_MSG_PARAM_TYPE_VAL, TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * UnWrap Key
 * @param_id (KwKID, wrappedKeyBuf, TagBuf, KeyConfig, nonceBuf, status,
 * SrcKID)
 */
#define TME_MSG_UID_CRYPTO_UNWRAP_KEY \
		TME_MSG_UID_CREATE(TME_MSG_CRYPTO, \
		TME_ACTION_CRYPTO_UNWRAP_KEY)
#define TME_MSG_UID_CRYPTO_UNWRAP_KEY_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_7( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 *  UID's & PARAM_ID's for TME_MSG_HCS
 */
/*
 * Hash (HMAC-SHA or SHA)
 * @param_id {algo, inBuf, keyId, rspBuf, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 * ref: TMESHAMessage_t
 */
#define TME_MSG_UID_HCS_SHA_DIGEST \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_SHA_DIGEST)
#define TME_MSG_UID_HCS_HMAC_DIGEST \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_HMAC_DIGEST)
#define TME_MSG_UID_HCS_HASH_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_10( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

#define TME_MSG_UID_HCS_SHA_DIGEST_PARAM_ID TME_MSG_UID_HCS_HASH_PARAM_ID
#define TME_MSG_UID_HCS_HMAC_DIGEST_PARAM_ID TME_MSG_UID_HCS_HASH_PARAM_ID

/*
 * Incremental Sha Init
 * @param_id {algo, inBuf, outContextBuf, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 * ref: TMEIncrementalSHAInit_t
 */
#define TME_MSG_UID_HCS_INC_SHA_INIT \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_INC_SHA_INIT)
#define TME_MSG_UID_HCS_INC_SHA_INIT_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_9( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Incremental Sha Update
 * @param_id {inBuf, inBuf, outContextBuf, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 * ref: TMEIncrementalSHAUpdate_t
 */
#define TME_MSG_UID_HCS_INC_SHA_UPDATE \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_INC_SHA_UPDATE)
#define TME_MSG_UID_HCS_INC_SHA_UPDATE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_9( \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Incremental Sha Final
 * @param_id {inBuf, inBuf, digestBuf, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 * ref: TMEIncrementalSHAFinal_t
 */
#define TME_MSG_UID_HCS_INC_SHA_FINAL \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_INC_SHA_FINAL)
#define TME_MSG_UID_HCS_INC_SHA_FINAL_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_9( \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * ECDSA sign (digest or msg)
 * @param_id {curveID, keyID, algo, inBuf, outBuf, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 * ref: TMEECCSignMessage_t
 */
#define TME_MSG_UID_HCS_ECC_SIGN_DIGEST \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_ECC_SIGN_DIGEST)

#define TME_MSG_UID_HCS_ECC_SIGN_MSG \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_ECC_SIGN_MSG)

#define TME_MSG_UID_HCS_ECC_SIGN_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_11( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

#define TME_MSG_UID_HCS_ECC_SIGN_DIGEST_PARAM_ID \
		TME_MSG_UID_HCS_ECC_SIGN_PARAM_ID
#define TME_MSG_UID_HCS_ECC_SIGN_MSG_PARAM_ID \
		TME_MSG_UID_HCS_ECC_SIGN_PARAM_ID

/*
 * ECDSA verify (digest or msg)
 * @param_id {curveID, keyID, algo, inBuf, outBuf, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 * ref: TMEECCVerifyMessage_t
 */
#define TME_MSG_UID_HCS_ECC_VERIFY_DIGEST \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_ECC_VERIFY_DIGEST)

#define TME_MSG_UID_HCS_ECC_VERIFY_MSG \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_ECC_VERIFY_MSG)

#define TME_MSG_UID_HCS_ECC_VERIFY_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_11( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

#define TME_MSG_UID_HCS_ECC_VERIFY_DIGEST_PARAM_ID \
		TME_MSG_UID_HCS_ECC_VERIFY_PARAM_ID
#define TME_MSG_UID_HCS_ECC_VERIFY_MSG_PARAM_ID \
		TME_MSG_UID_HCS_ECC_VERIFY_PARAM_ID

/*
 * ECC get public key
 * @param_id {curveID, prvKeyId, pubKey, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 * ref: TMEECCGetPubKeyMessage_t
 */
#define TME_MSG_UID_HCS_ECC_GET_PUBKEY \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_ECC_GET_PUBKEY)

#define TME_MSG_UID_HCS_ECDH_GET_PUBKEY \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_ECDH_GET_PUBKEY)

#define TME_MSG_UID_HCS_ECC_GET_PUBKEY_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_9( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

#define TME_MSG_UID_HCS_ECDH_GET_PUBKEY_PARAM_ID \
		TME_MSG_UID_HCS_ECC_GET_PUBKEY_PARAM_ID

/*
 * ECDH shared secret
 * @param_id {curveID, prvKeyId, pubKey, shrdSecretKeypolicy, shrdKeyId,
 *	      status, {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 * ref: TMEECDHSharedSecretMessage_t
 */
#define TME_MSG_UID_HCS_ECDH_SHARED_SECRET \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_ECDH_SHARED_SECRET)

#define TME_MSG_UID_HCS_ECDH_SHARED_SECRET_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_13( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * AES encrypt
 * @param_id {algo, keyId, inAad, inPlainTxt, outAad, outIV, outTag,
 *	      outCipherTxt, status, {tmeErrorStatus, seqErrorStatus,
 *	      seqKPErrorStatus0, seqKPErrorStatus1, seqRspStatus}}
 * ref: TMEAESEncryptMessage_t
 */
#define TME_MSG_UID_HCS_AES_ENCRYPT \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_AES_ENCRYPT)

#define TME_MSG_UID_HCS_AES_ENCRYPT_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_14( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * AES decrypt
 * @param_id {algo, keyId, inAad, inIV, inTag, inCipherTxt, outAad,
 *	      outPlainTxt, status, {tmeErrorStatus, seqErrorStatus,
 *	      seqKPErrorStatus0, seqKPErrorStatus1, seqRspStatus}}
 * ref: TMEAESDecryptMessage_t
 */
#define TME_MSG_UID_HCS_AES_DECRYPT \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_AES_DECRYPT)

#define TME_MSG_UID_HCS_AES_DECRYPT_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_14( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Incremental AES encryption init (with optional update) call
 * @param_id (algo, keyHandle, inAad, inIV, inPlainTxt, fullAadLength,
 *	      fullPlainTxtLength, outCipherTxt, outContext, status,
 *	      seqStatusBuf {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus})
 * ref: TMEIncrAESEncryptInitMsg_t
 */
#define TME_MSG_UID_HCS_INCR_AES_ENCRYPT_INIT \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_INCR_AES_ENCRYPT_INIT)
#define TME_MSG_UID_HCS_INCR_AES_ENCRYPT_INIT_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_10( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Incremental AES encryption update call
 * @param_id (inAad, inPlainTxt, inContext, outCipherTxt, outContext,
 *	      status, seqStatusBuf {tmeErrorStatus, seqErrorStatus,
 *	      seqKPErrorStatus0, seqKPErrorStatus1, seqRspStatus})
 * ref: TMEIncrAESEncryptUpdateMsg_t
 */
#define TME_MSG_UID_HCS_INCR_AES_ENCRYPT_UPDATE \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_INCR_AES_ENCRYPT_UPDATE)
#define TME_MSG_UID_HCS_INCR_AES_ENCRYPT_UPDATE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_7( \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Incremental AES encryption final call
 * @param_id (inAad, inPlainTxt, inContext, outAad, outIV, outTag,
 *	      outCipherTxt, status, seqStatusBuf {tmeErrorStatus,
 *	      seqErrorStatus, seqKPErrorStatus0, seqKPErrorStatus1,
 *	      seqRspStatus})
 * ref: TMEIncrAESEncryptFinalMsg_t
 */
#define TME_MSG_UID_HCS_INCR_AES_ENCRYPT_FINAL \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_INCR_AES_ENCRYPT_FINAL)
#define TME_MSG_UID_HCS_INCR_AES_ENCRYPT_FINAL_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_9( \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Incremental AES decryption init (with optional update) call
 * @param_id (algo, keyHandle, inAad, inIV, inCipherTxt, fullAadLength,
 *	      fullCipherTxtLength, outPlainTxt, outContext, status,
 *	      seqStatusBuf {tmeErrorStatus, seqErrorStatus,
 *	      seqKPErrorStatus0, seqKPErrorStatus1, seqRspStatus})
 * ref: TMEIncrAESDecryptInitMsg_t
 */
#define TME_MSG_UID_HCS_INCR_AES_DECRYPT_INIT \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_INCR_AES_DECRYPT_INIT)
#define TME_MSG_UID_HCS_INCR_AES_DECRYPT_INIT_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_11( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Incremental AES decryption update call
 * @param_id (inAad, inCipherTxt, inContext, outPlainTxt, outContext,
 *	      status, seqStatusBuf {tmeErrorStatus, seqErrorStatus,
 *	      seqKPErrorStatus0, seqKPErrorStatus1, seqRspStatus})
 * ref: TMEIncrAESDecryptUpdateMsg_t
 */
#define TME_MSG_UID_HCS_INCR_AES_DECRYPT_UPDATE \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_INCR_AES_DECRYPT_UPDATE)
#define TME_MSG_UID_HCS_INCR_AES_DECRYPT_UPDATE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_7( \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Incremental AES decryption final call
 * @param_id (inAad, inCipherTxt, inTag, inContext, outAad, outPlainTxt,
 *	      status, seqStatusBuf {tmeErrorStatus, seqErrorStatus,
 *	      seqKPErrorStatus0, seqKPErrorStatus1, seqRspStatus})
 * ref: TMEIncrAESDecryptFinalMsg_t
 */
#define TME_MSG_UID_HCS_INCR_AES_DECRYPT_FINAL \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_INCR_AES_DECRYPT_FINAL)
#define TME_MSG_UID_HCS_INCR_AES_DECRYPT_FINAL_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_8( \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Get PRNG number
 * @param_id {length, outBuf, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 * ref: TMEPRNGGetMessage_t
 */
#define TME_MSG_UID_HCS_PRNG_GET \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_PRNG_GET)

#define TME_MSG_UID_HCS_PRNG_GET_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_8( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Get FIPS Approved Algorithm List
 * @param_id {status, outList}
 * ref: TMEGetFipsAlgoMessage_t
 */
#define TME_MSG_UID_HCS_GET_FIPS_ALGO_LIST \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_GET_FIPS_ALGO_LIST)

#define TME_MSG_UID_HCS_GET_FIPS_ALGO_LIST_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Run FIPS DV test cases
 * @param_id None
 */
#define TME_MSG_UID_HCS_FIPS_DV_TEST_CASE \
		TME_MSG_UID_CREATE(TME_MSG_HCS, \
		TME_ACTION_HCS_FIPS_DV_TEST_CASE)

#define TME_MSG_UID_HCS_FIPS_DV_TEST_CASE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_1( \
			TME_MSG_PARAM_TYPE_VAL)

/*
 *  UID's & PARAM_ID's for TME_MSG_KM
 */
/*
 * Generate Key (TMEKMGenerateKeyMessage_t)
 * @param_id {keyID, keypolicy_lo, keypolicy_hi, cred, keyID, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 */
#define TME_MSG_UID_KM_GENERATE \
		TME_MSG_UID_CREATE(TME_MSG_KM, \
		TME_ACTION_KM_GENERATE)
#define TME_MSG_UID_KM_GENERATE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_11(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)
/*
 * Import Key (TMEKMImportKeyMessage_t)
 * @param_id (keyID, keypolicy_lo, keypolicy_hi, PlainTxtKeyBuf, cred,
 *	      keyID, status, {tmeErrorStatus, seqErrorStatus,
 *	      seqKPErrorStatus0, seqKPErrorStatus1, seqRspStatus}}
 */
#define TME_MSG_UID_KM_IMPORT \
		TME_MSG_UID_CREATE(TME_MSG_KM, \
		TME_ACTION_KM_IMPORT)
#define TME_MSG_UID_KM_IMPORT_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_12( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Clear Key (TMEKMClearKeyMessage_t)
 * @param_id {keyId, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 */
#define TME_MSG_UID_KM_CLEAR \
		TME_MSG_UID_CREATE(TME_MSG_KM, \
		TME_ACTION_KM_CLEAR)
#define TME_MSG_UID_KM_CLEAR_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_7( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * ClearSSKeysAll (TmeClearSSKeysAll_t)
 * @param_id {status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 */
#define TME_MSG_UID_UWB_CLEARALLKEYS \
		TME_MSG_UID_CREATE(TME_MSG_KM, \
		TME_ACTION_KM_CLEAR_SS_KEYS_ALL)

#define TME_MSG_UID_KM_CLEAR_SS_KEYS_ALL_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_6( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Derive Key (TMEKMDeriveKeyMessage_t)
 * @param_id {keyID, kdfInfo, cred, keyID, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 */
#define TME_MSG_UID_KM_DERIVE \
		TME_MSG_UID_CREATE(TME_MSG_KM, \
		TME_ACTION_KM_DERIVE)
#define TME_MSG_UID_KM_DERIVE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_10( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Wrap Key (TMEKMWrapKeyMessage_t)
 * @param_id {keyID, kwKeyID, cred, wrappedKey, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 */
#define TME_MSG_UID_KM_WRAP \
		TME_MSG_UID_CREATE(TME_MSG_KM, \
		TME_ACTION_KM_WRAP)
#define TME_MSG_UID_KM_WRAP_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_10( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Unwrap Key (TMEKMUnwrapKeyMessage_t)
 * @param_id {keyID, kwKeyID, wrappedKey, cred, keyID, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 */
#define TME_MSG_UID_KM_UNWRAP \
		TME_MSG_UID_CREATE(TME_MSG_KM, \
		TME_ACTION_KM_UNWRAP)
#define TME_MSG_UID_KM_UNWRAP_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_11( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Distribute Key (TMEKMDistributeKeyMessage_t)
 * @param_id {keyID, dstID, index, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 */
#define TME_MSG_UID_KM_DISTRIBUTE \
		TME_MSG_UID_CREATE(TME_MSG_KM, \
		TME_ACTION_KM_DISTRIBUTE)
#define TME_MSG_UID_KM_DISTRIBUTE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_9( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Derive and Export ECDH IP Protection Public Key
 * (TMEKMExportEcdhIpKeyMessage_t)
 * @param_id {featureId, srcL1KeyId, pubKey, status,
 *	      {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	      seqKPErrorStatus1, seqRspStatus}}
 */
#define TME_MSG_UID_KM_EXPORT_ECDH_IP \
		TME_MSG_UID_CREATE(TME_MSG_KM, \
		TME_ACTION_KM_EXPORT_ECDH_IP_KEY)
#define TME_MSG_UID_KM_EXPORT_ECDH_IP_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_9( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Wrap the content encryption key (TMEKMQbecKeyWrapMessage_t)
 * @param_id {cekID, cekPlain, encEntity, context, publicKey,
 *	      wrappedKeyBlob, status, {tmeErrorStatus, seqErrorStatus,
 *	      seqKPErrorStatus0, seqKPErrorStatus1, seqRspStatus}}
 */

#define TME_MSG_UID_KM_QBEC_WRAP_KEY \
		TME_MSG_UID_CREATE(TME_MSG_KM, \
		TME_ACTION_KM_QBEC_WRAP_KEY)
#define TME_MSG_UID_KM_QBEC_WRAP_KEY_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_12(\
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Multi Distribute Key (TMEKMMultiistributeKeyMessage_t)
 * @param_id {dstID, entries, keyDistributeStatus, status
 *	    {tmeErrorStatus, seqErrorStatus, seqKPErrorStatus0,
 *	    seqKPErrorStatus1, seqRspStatus}}
 */

#define TME_MSG_UID_KM_MULTI_DISTRIBUTE_KEY \
		TME_MSG_UID_CREATE(TME_MSG_KM, \
		TME_ACTION_KM_MULTI_DISTRIBUTE_KEY)

#define TME_MSG_UID_KM_MULTI_DISTRIBUTE_KEY_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_9( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)
/*
 *  UID's & PARAM_ID's for TME_MSG_MC3
 */
/*
 * MC3 Authentication
 * @param_id (appDataBuf, status, resposeBuf)
 */
#define TME_MSG_UID_MC3_AUTHENTICATION_REQ \
		TME_MSG_UID_CREATE(TME_MSG_MC3, \
		TME_ACTION_MC3_AUTHENTICATION_REQ)
#define TME_MSG_UID_MC3_AUTHENTICATION_REQ_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)
/*
 * MC3 Attestation Token construction
 * @param_id (tokenBuf, appDataBuf, publicKeyBuf, status, responseBuf)
 */
#define TME_MSG_UID_MC3_ATTESTATION_REQ \
		TME_MSG_UID_CREATE(TME_MSG_MC3, \
		TME_ACTION_MC3_ATTESTATION_REQ)

#define TME_MSG_UID_MC3_ATTESTATION_REQ_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_5( \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * MC3 Onboarding request
 * @param_id (tokenBuf, publicKeyBuf, status, appDataBuf, MECertificateBuf,
 * MECertParamsBuf, sharedSecret)
 */
#define TME_MSG_UID_MC3_ONBOARDING_REQ \
		TME_MSG_UID_CREATE(TME_MSG_MC3, \
		TME_ACTION_MC3_ONBOARDING_REQ)

#define TME_MSG_UID_MC3_ONBOARDING_REQ_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_7( \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * MC3 Onboarding Confirmation
 * @param_id (sharedSecret, appDataBuf, status, responseBuf)
 */
#define TME_MSG_UID_MC3_ONBOARDING_CONFIRMATION \
		TME_MSG_UID_CREATE(TME_MSG_MC3, \
		TME_ACTION_MC3_ONBOARDING_CONFIRMATION)

#define TME_MSG_UID_MC3_ONBOARDING_CONFIRMATION_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * MC3 Decryption request
 * @param_id (tokenBuf, status, responseBuf, sharedSecret)
 */
#define TME_MSG_UID_MC3_DECRYPT_MESSAGE_REQ \
		TME_MSG_UID_CREATE(TME_MSG_MC3, \
		TME_ACTION_MC3_DECRYPT_MESSAGE_REQ)

#define TME_MSG_UID_MC3_DECRYPT_MESSAGE_REQ_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4( \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * MC3 Decryption confirmation request
 * @param_id (sharedSecret, appDataBuf, status, responseBuf)
 */
#define TME_MSG_UID_MC3_DECRYPT_MESSAGE_CONFIRMATION \
		TME_MSG_UID_CREATE(TME_MSG_MC3, \
		TME_ACTION_MC3_DECRYPT_MESSAGE_CONFIRMATION)

#define TME_MSG_UID_MC3_DECRYPT_MESSAGE_CONFIRMATION_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4( \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * MC3 Authenticate request
 * @param_d (tokenBuf, publicKeyBuf, status, responseBuf, isVerified)
 */
#define TME_MSG_UID_MC3_AUTHENTICATE_MSG_REQ \
		TME_MSG_UID_CREATE(TME_MSG_MC3, \
		TME_ACTION_MC3_AUTHENTICATE_MSG_REQ)

#define TME_MSG_UID_MC3_AUTHENTICATE_MSG_REQ_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_5( \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * MC3 Authentication msg confirmation request
 * @param_d (appData, status, responseBuf)
 */
#define TME_MSG_UID_MC3_AUTHENTICATE_MSG_CONFIRMATION \
		TME_MSG_UID_CREATE(TME_MSG_MC3, \
		TME_ACTION_MC3_AUTHENTICATE_MSG_CONFIRMATION)

#define TME_MSG_UID_MC3_AUTHENTICATE_MSG_CONFIRMATION_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3( \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 *  UID's & PARAM_ID's for TME_MSG_FUSE
 */

/*
 * Read single fuse
 * @param_d (status, fuseAddr, fuseVal)
 */
#define TME_MSG_UID_FUSE_READ_SINGLE_ROW \
		TME_MSG_UID_CREATE(TME_MSG_FUSE, \
		TME_ACTION_FUSE_READ_SINGLE_ROW)
#define TME_MSG_UID_FUSE_READ_SINGLE_ROW_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)
/*
 * Read multiple fuses
 * @param_d (status, fuseRdData)
 */
#define TME_MSG_UID_FUSE_READ_MULTIPLE_ROW \
		TME_MSG_UID_CREATE(TME_MSG_FUSE, \
		TME_ACTION_FUSE_READ_MULTIPLE_ROW)
#define TME_MSG_UID_FUSE_READ_MULTIPLE_ROW_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN_OUT)

/*
 * Write single fuse
 * @param_d (status, fuseAddr, fuseVal)
 */
#define TME_MSG_UID_FUSE_WRITE_SINGLE_ROW \
		TME_MSG_UID_CREATE(TME_MSG_FUSE, \
		TME_ACTION_FUSE_WRITE_SINGLE_ROW)
#define TME_MSG_UID_FUSE_WRITE_SINGLE_ROW_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Write multiple fuses
 * @param_d (status, fuseWrData)
 */
#define TME_MSG_UID_FUSE_WRITE_MULTIPLE_ROW \
		TME_MSG_UID_CREATE(TME_MSG_FUSE, \
		TME_ACTION_FUSE_WRITE_MULTIPLE_ROW)
#define TME_MSG_UID_FUSE_WRITE_MULTIPLE_ROW_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN)

/* Deprecated */
/*
 * Write secure fuse
 * @param_d (status, fuseWrData)
 */
#define TME_MSG_UID_FUSE_WRITE_SECURE \
		TME_MSG_UID_CREATE(TME_MSG_FUSE, \
		TME_ACTION_FUSE_WRITE_SECURE)
#define TME_MSG_UID_FUSE_WRITE_SECURE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN)

/*
 * Write rom patch fuse
 * @param_d (status, rprData)
 */
#define TME_MSG_UID_FUSE_ROM_PATCH_REQ \
		TME_MSG_UID_CREATE(TME_MSG_FUSE, \
		TME_ACTION_FUSE_ROM_PATCH_REQ)
#define TME_MSG_UID_FUSE_ROM_PATCH_REQ_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN)

/*
 *  UID's & PARAM_ID's for TME_MSG_ACCESS_CONTROL
 */
/* Deprecated */
/*
 * Provide or release NVM register access
 * @param_id (provideAccess, status)
 */
#define TME_MSG_UID_ACCESS_CONTROL_NVM_REGISTER \
		TME_MSG_UID_CREATE(TME_MSG_ACCESS_CONTROL, \
		TME_ACTION_ACCESS_CONTROL_NVM_REGISTER)
#define TME_MSG_UID_ACCESS_CONTROL_NVM_REGISTER_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Lock a RG on behalf of given QAD.
 * @param_id (xpuAddr, rgNum, QadId, status)
 */
#define TME_MSG_UID_ACCESS_CONTROL_LOCK_RG_FOR_QAD \
		TME_MSG_UID_CREATE(TME_MSG_ACCESS_CONTROL, \
		TME_ACTION_ACCESS_CONTROL_LOCK_RG_FOR_QAD)
#define TME_MSG_UID_ACCESS_CONTROL_LOCK_RG_FOR_QAD_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Disable IPC service to lock RG on behalf of another QAD.
 * @param_id (status)
 */
#define TME_MSG_UID_ACCESS_CONTROL_DISABLE_LOCK_RG_IPC \
		TME_MSG_UID_CREATE(TME_MSG_ACCESS_CONTROL, \
		TME_ACTION_ACCESS_CONTROL_DISABLE_LOCK_RG_IPC)
#define TME_MSG_UID_ACCESS_CONTROL_DISABLE_LOCK_RG_IPC_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_1(TME_MSG_PARAM_TYPE_VAL)

/*
 * Protect TMEL owned resources.
 * @param_id (status)
 */
#define TME_MSG_UID_ACCESS_CONTROL_PROTECT_TMEL_RESOURCES \
		TME_MSG_UID_CREATE(TME_MSG_ACCESS_CONTROL, \
		TME_ACTION_ACCESS_CONTROL_PROTECT_TMEL_RESOURCES)
#define TME_MSG_UID_ACCESS_CONTROL_PROTECT_TMEL_RESOURCES_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_1(TME_MSG_PARAM_TYPE_VAL)

/*
 * Set XPU DBGAR registers.
 * @param_id (dbgarList, status)
 */
#define TME_MSG_UID_ACCESS_CONTROL_SET_XPU_DBGAR \
		TME_MSG_UID_CREATE(TME_MSG_ACCESS_CONTROL, \
		TME_ACTION_ACCESS_CONTROL_SET_XPU_DBGAR)
#define TME_MSG_UID_ACCESS_CONTROL_SET_XPU_DBGAR_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Enable Silent Logging feature.
 * @param_id (status)
 */
#define TME_MSG_UID_ACCESS_CONTROL_ENABLE_SILENT_LOGGING \
		TME_MSG_UID_CREATE(TME_MSG_ACCESS_CONTROL, \
		TME_ACTION_ACCESS_CONTROL_ENABLE_SILENT_LOGGING)
#define TME_MSG_UID_ACCESS_CONTROL_ENABLE_SILENT_LOGGING_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_1(TME_MSG_PARAM_TYPE_VAL)

/*
 * Reads XPU syndrome registers.
 * @param_id (status, syndInfoBuffer)
 */
#define TME_MSG_UID_ACCESS_CONTROL_READ_SYNDROME_IPC \
		TME_MSG_UID_CREATE(TME_MSG_ACCESS_CONTROL, \
		TME_ACTION_ACCESS_CONTROL_READ_SYNDROME_IPC)
#define TME_MSG_UID_ACCESS_CONTROL_READ_SYNDROME_IPC_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 *  UID's & PARAM_ID's for TME_MSG_FWUP
 */
/*
 * Verify update manifest
 * @param_id (address, size, verifyAll, status, verificationStatus)
 */
#define TME_MSG_UID_FWUP_VERIFY_UPDATE \
		TME_MSG_UID_CREATE(TME_MSG_FWUP, \
		TME_ACTION_FWUP_VERIFY_UPDATE)
#define TME_MSG_UID_FWUP_VERIFY_UPDATE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_5(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)
/*
 * Apply update
 * @param_id (address, size, debug, status)
 */
#define TME_MSG_UID_FWUP_APPLY_UPDATE \
		TME_MSG_UID_CREATE(TME_MSG_FWUP, \
		TME_ACTION_FWUP_APPLY_UPDATE)
#define TME_MSG_UID_FWUP_APPLY_UPDATE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 *  UID's & PARAM_ID's for TME_MSG_LOG
 */
/*
 * Set TME Log Config
 * @param_id (status, logConfigBuf)
 */
#define TME_MSG_UID_LOG_SET_CONFIG \
		TME_MSG_UID_CREATE(TME_MSG_LOG, \
		TME_ACTION_LOG_SET_CONFIG)
#define TME_MSG_UID_LOG_SET_CONFIG_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN)

/*
 * Get TME Log Config
 * @param_id (status, logConfigBuf)
 */
#define TME_MSG_UID_LOG_GET_CONFIG \
		TME_MSG_UID_CREATE(TME_MSG_LOG, \
		TME_ACTION_LOG_GET_CONFIG)
#define TME_MSG_UID_LOG_GET_CONFIG_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Get TME Log
 * @param_id (status, logBuffer)
 */
#define TME_MSG_UID_LOG_GET \
		TME_MSG_UID_CREATE(TME_MSG_LOG, \
		TME_ACTION_LOG_GET)
#define TME_MSG_UID_LOG_GET_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Get Q6 err Log
 * @param_id (status, logBuffer)
 */
#define TME_MSG_UID_Q6LOG_GET \
		TME_MSG_UID_CREATE(TME_MSG_LOG, \
		TME_ACTION_Q6LOG_GET)
#define TME_MSG_UID_Q6LOG_GET_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN)

/*
 *  UID's & PARAM_ID's for TME_MSG_FEATURE_LICENSE
 */
/*
 * Get Feature License Status
 * @param_id (featureMask, status, licenseStatus)
 */
#define TME_MSG_UID_FEATURE_LICENSE_STATUS_BTSS \
		TME_MSG_UID_CREATE(TME_MSG_FEATURE_LICENSE, \
		TME_ACTION_FEATURE_LICENSE_BTSS)
#define TME_MSG_UID_FEATURE_LICENSE_STATUS_BTSS_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 *  UID's & PARAM_ID's for TME_MSG_QWES
 */
/*
 * Initialize Device Attestation
 * @param_id (status, m3EphPubKey)
 */
#define TME_MSG_UID_QWES_INIT_ATTESTATION \
		TME_MSG_UID_CREATE(TME_MSG_QWES, \
		TME_ACTION_QWES_INIT_ATTESTATION)
#define TME_MSG_UID_QWES_INIT_ATTESTATION_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Perform Device Attestation
 * @param_id (status, attReq, extClaims, attRsp)
 */
#define TME_MSG_UID_QWES_DEVICE_ATTESTATION \
		TME_MSG_UID_CREATE(TME_MSG_QWES, \
		TME_ACTION_QWES_DEVICE_ATTESTATION)
#define TME_MSG_UID_QWES_DEVICE_ATTESTATION_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Perform Secure Device Provisioning using QWES
 * @param_id (status, provReq, provRsp)
 */
#define TME_MSG_UID_QWES_DEVICE_PROVISIONING \
		TME_MSG_UID_CREATE(TME_MSG_QWES, \
		TME_ACTION_QWES_DEVICE_PROVISIONING)
#define TME_MSG_UID_QWES_DEVICE_PROVISIONING_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Perform Install License
 * @param_id (status, license, flags, identifier)
 */
#define TME_MSG_UID_QWES_LICENSING_INSTALL \
		TME_MSG_UID_CREATE(TME_MSG_QWES, \
		TME_ACTION_QWES_LICENSING_INSTALL)
#define TME_MSG_UID_QWES_LICENSING_INSTALL_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Perform Check
 * @param_id (status, request, response)
 */
#define TME_MSG_UID_QWES_LICENSING_CHECK \
		TME_MSG_UID_CREATE(TME_MSG_QWES, \
		TME_ACTION_QWES_LICENSING_CHECK)
#define TME_MSG_UID_QWES_LICENSING_CHECK_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Perform Enforce HW Features
 * @param_id (status, featureIDBuff, HWRegisterInterfaceVersion)
 */
#define TME_MSG_UID_QWES_LICENSING_ENFORCEHWFEATURES \
		TME_MSG_UID_CREATE(TME_MSG_QWES, \
		TME_ACTION_QWES_LICENSING_ENFORCEHWFEATURES)
#define TME_MSG_UID_QWES_LICENSING_ENFORCEHWFEATURES_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN_OUT, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * Perform Get TTime Request for cloud
 * @param_id (status, cloudReqBuf)
 */
#define TME_MSG_UID_QWES_TTIME_CLOUD_REQUEST \
		TME_MSG_UID_CREATE(TME_MSG_QWES, \
		TME_ACTION_QWES_TTIME_CLOUD_REQUEST)
#define TME_MSG_UID_QWES_TTIME_CLOUD_REQUEST_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Perform Set TTime from cloud response packet
 * @param_id (status, cloudRspBuf)
 */
#define TME_MSG_UID_QWES_TTIME_SET \
		TME_MSG_UID_CREATE(TME_MSG_QWES, \
		TME_ACTION_QWES_TTIME_SET)
#define TME_MSG_UID_QWES_TTIME_SET_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN)

/*
 * Perform Get TTime Information in CBOR format
 * @param_id (status, nonce[optional], cborTTime)
 */
#define TME_MSG_UID_QWES_TTIME_GET_INFO_CBOR \
		TME_MSG_UID_CREATE(TME_MSG_QWES, \
		TME_ACTION_QWES_TTIME_GET_INFO_CBOR)
#define TME_MSG_UID_QWES_TTIME_GET_INFO_CBOR_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * Get the list of to be deleted licenses
 * @param_id (status, featureIDBuff, HWRegisterInterfaceVersion)
 */
#define TME_MSG_UID_QWES_LICENSING_TBDLICENSES \
		TME_MSG_UID_CREATE(TME_MSG_QWES, \
		TME_ACTION_QWES_LICENSING_TBDLICENSES)
#define TME_MSG_UID_QWES_LICENSING_TBDLICENSES_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 *  UID's & PARAM_ID's for TME_MSG_CDUMP
 */
/*
 * Save Crashdump buffer address sent by TZ
 * @param_id (status, dumpBuffAddr)
 */
#define TME_MSG_UID_CDUMP_SAVE_ADDR \
		TME_MSG_UID_CREATE(TME_MSG_CDUMP, \
		TME_ACTION_CDUMP_SAVE_DUMP_ADDR)
#define TME_MSG_UID_CDUMP_SAVE_ADDR_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_2(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN)

/*
 *  UID's & PARAM_ID's for TME_MSG_SCP
 */
/*
 * SCP, SCP03 Command APDU Encryption/ MAC
 * @param_id(capduGetRDSPlain, status, capduGetRDSEncrypted)
 */
#define TME_MSG_UID_SCP_SCP03_TMEGETRDSCAPDU \
		TME_MSG_UID_CREATE(TME_MSG_SCP, \
		TME_ACTION_SCP_SCP03_RDSCAPDU)

#define TME_MSG_UID_SCP_SCP03_TMEGETRDSCAPDU_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3(TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)
/*
 * SCP, SCP03 Response APDU Decrypt/ Authenticate
 * @param_id(getRDSRAPDUEncrypted, status, getRDSRAPDUPlain, handleRDS)
 */
#define TME_MSG_UID_SCP_SCP03_TMEGETRDSRAPDU \
		TME_MSG_UID_CREATE(TME_MSG_SCP, \
		TME_ACTION_SCP_SCP03_RDSRAPDU)

#define TME_MSG_UID_SCP_SCP03_TMEGETRDSRAPDU_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * SCP, SCP03 Process Command APDU for INITIALIZE UPDATE and
 * EXTERNAL AUTHENTICATE
 * @param_id(scp03CapduRaw, status, processedCommand)
 */
#define TME_MSG_UID_SCP_SCP03_PROCESS_CAPDU \
		TME_MSG_UID_CREATE(TME_MSG_SCP, \
		TME_ACTION_SCP_SCP03_PROCESS_CAPDU)

#define TME_MSG_UID_SCP_SCP03_PROCESS_CAPDU_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * SCP, SCP03 Process Response APDU for INITIALIZE UPDATE and
 * EXTERNAL AUTHENTICATE
 * @param_id(rawResponse, status)
 */
#define TME_MSG_UID_SCP_SCP03_PROCESS_RAPDU \
		TME_MSG_UID_CREATE(TME_MSG_SCP, \
		TME_ACTION_SCP_SCP03_PROCESS_RAPDU)

#define TME_MSG_UID_SCP_SCP03_PROCESS_RAPDU_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * SCP, SCP11a Initiate Session Request
 * @param_id (curve, status, pkOceEcka, ePkOceEcka,
 * certOceEckaSubjectIdentifier, certOceEckaSerial, certOceEckaSignature,
 * hostId)
 */
#define TME_MSG_UID_SCP_SCP11A_INITIATE \
		TME_MSG_UID_CREATE(TME_MSG_SCP, \
		TME_ACTION_SCP_SCP11A_INITIATE)
#define TME_MSG_UID_SCP_SCP11A_INITIATE_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_8(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_BUF_OUT)
/*
 * SCP, SCP11a Establish Secure Channel Request
 * @param_id (certSdEcka, ePkSdEcka, receipt, scpParameters, status)
 */
#define TME_MSG_UID_SCP_SCP11A_ESTABLISH_SESSION \
		TME_MSG_UID_CREATE(TME_MSG_SCP, \
		TME_ACTION_SCP_SCP11A_ESTABLISH_SESSION)
#define TME_MSG_UID_SCP_SCP11A_ESTABLISH_SESSION_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_5(TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * SCP, Close an ongoing SCP session
 * @param_id {status}
 */
#define TME_MSG_UID_SCP_CLOSE_SESSION \
		TME_MSG_UID_CREATE(TME_MSG_SCP, \
		TME_ACTION_SCP_CLOSE_SESSION)

#define TME_MSG_UID_SCP_CLOSE_SESSION_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_1(TME_MSG_PARAM_TYPE_VAL)

/*
 * SCP11a Test Data Provisioning
 * @param_id (certOceEcka, eSEVerifyingKey, status)
 */
#define TME_MSG_UID_SCP_SCP11A_TEST_DATA_PROV \
		TME_MSG_UID_CREATE(TME_MSG_SCP, \
		TME_ACTION_SCP_TEST_DATA_PROV)
#define TME_MSG_UID_SCP_SCP11A_TEST_DATA_PROV_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3(TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 *  UID's & PARAM_ID's for TME_MSG_UWB_KD
 */
/*
 * TMESaltedHash; helper function for UWB CCC Key Derivation
 * @param_id(handleRDS, rangingConfiguration, status, handleSaltedHash)
 */
#define TME_MSG_UID_UWB_CCC_TMESALTEDHASH \
		TME_MSG_UID_CREATE(TME_MSG_UWB_KD, \
		TME_ACTION_UWB_CCC_TMESALTEDHASH)

#define TME_MSG_UID_UWB_CCC_TMESALTEDHASH_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)
/*
 * TMEUAD; helper function for UWB CCC Key Derivation
 * @param_id(handleRDS, STSIndex, status, UAD)
 */
#define TME_MSG_UID_UWB_CCC_TMEUAD \
		TME_MSG_UID_CREATE(TME_MSG_UWB_KD, \
		TME_ACTION_UWB_CCC_TMEUAD)

#define TME_MSG_UID_UWB_CCC_TMEUAD_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)
/*
 * TMEmUPSK1; helper function for UWB CCC Key Derivation
 * @param_id(handleRDS, status, handlemUPSK1)
 */
#define TME_MSG_UID_UWB_CCC_TMEMUPSK1 \
		TME_MSG_UID_CREATE(TME_MSG_UWB_KD, \
		TME_ACTION_UWB_CCC_TMEMUPSK1)

#define TME_MSG_UID_UWB_CCC_TMEMUPSK1_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)
/*
 * TMEdURSKdUDSK; helper function for UWB CCC Key Derivation
 * @param_id(handleRDS, saltedHashKeyHandle, explodedSTSIndex, status,
 * handledURSK, handledUDSK)
 */
#define TME_MSG_UID_UWB_CCC_TMEDURSKDUDSK \
		TME_MSG_UID_CREATE(TME_MSG_UWB_KD, \
		TME_ACTION_UWB_CCC_TMEDURSKDUDSK)

#define TME_MSG_UID_UWB_CCC_TMEDURSKDUDSK_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_6(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * TMEImportSecSessionKey; helper function for UWB FiRa Key Derivation
 * @param_id(ImportSecSessionKey, status, handleStaticSTS)
 */
#define TME_MSG_UID_UWB_FIRA_TMEImportSecSessionKey \
	TME_MSG_UID_CREATE(TME_MSG_UWB_KD, \
	TME_ACTION_UWB_FIRA_TMEImportSecSessionKey)

#define TME_MSG_UID_UWB_FIRA_TMEImportSecSessionKey_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3(TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * TMEDerivedConfigDigest; helper function for UWB FiRa Key Derivation
 * @param_id(derivationData, status, handleDerivedConfigDigest)
 */
#define TME_MSG_UID_UWB_FIRA_TMEDERIVEDCONFIGDIGEST \
		TME_MSG_UID_CREATE(TME_MSG_UWB_KD, \
		TME_ACTION_UWB_FIRA_TMEDERIVEDCONFIGDIGEST)

#define TME_MSG_UID_UWB_FIRA_TMEDERIVEDCONFIGDIGEST_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_3(TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * TMEsecDataPrivacyKey; helper function for UWB FiRa Key Derivation
 * @param_id(handleSecSessionKey, handleDerivedConfigDigest, status,
 * handleSecDataPrivacyKey)
 */
#define TME_MSG_UID_UWB_FIRA_TMESECDATAPRIVACYKEY \
		TME_MSG_UID_CREATE(TME_MSG_UWB_KD, \
		TME_ACTION_UWB_FIRA_TMESECDATAPRIVACYKEY)

#define TME_MSG_UID_UWB_FIRA_TMESECDATAPRIVACYKEY_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * TMEsecMaskingKey; helper function for UWB FiRa Key Derivation
 * @param_id(handleSecSessionKey, handleDerivedConfigDigest, status,
 * handleSecMaskingKey)
 */
#define TME_MSG_UID_UWB_FIRA_TMESECMASKINGKEY \
		TME_MSG_UID_CREATE(TME_MSG_UWB_KD, \
		TME_ACTION_UWB_FIRA_TMESECMASKINGKEY)

#define TME_MSG_UID_UWB_FIRA_TMESECMASKINGKEY_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)
/*
 * TMEsecDataProtectionKey; helper function for UWB FiRa Key Derivation
 * @param_id(handleSessionKey, handleDerivedConfigDigest, status,
 * handleSecDataProtectionKey)
 */
#define TME_MSG_UID_UWB_FIRA_TMESECDATAPROTECTIONKEY \
		TME_MSG_UID_CREATE(TME_MSG_UWB_KD, \
		TME_ACTION_UWB_FIRA_TMESECDATAPROTECTIONKEY)

#define TME_MSG_UID_UWB_FIRA_TMESECDATAPROTECTIONKEY_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 * TMEphyStsIndex; helper function for UWB FiRa Key Derivation
 * @param_id(handleSecDataProtectionKey, handleDerivedConfigDigest, status,
 * phyStsIndexInit)
 */
#define TME_MSG_UID_UWB_FIRA_TMEPHYSTSINDEX \
		TME_MSG_UID_CREATE(TME_MSG_UWB_KD, \
		TME_ACTION_UWB_FIRA_TMEPHYSTSINDEX)

#define TME_MSG_UID_UWB_FIRA_TMEPHYSTSINDEX_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_4(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT)

/*
 * TMEsecDerivedAuthentication; helper function for UWB FiRa Key Derivation
 * @param_id(handleSecDataProtectionKey, handleDerivedConfigDigest,
 * cryptoStsIndex, status, secDerivedAuthenticationIV,
 * handleSecDerivedPayloadKey, handleSecAuthenticationKey)
 */
#define TME_MSG_UID_UWB_FIRA_TMESECDERIVEDAUTHENTICATION \
		TME_MSG_UID_CREATE(TME_MSG_UWB_KD, \
		TME_ACTION_UWB_FIRA_TMESECDERIVEDAUTHENTICATION)

#define TME_MSG_UID_UWB_FIRA_TMESECDERIVEDAUTHENTICATION_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_7(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_BUF_IN, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_BUF_OUT, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

/*
 *  UID's & PARAM_ID's for TME_MSG_TIMETEST
 */

/*
 * Update Time Test data buffer and configure Watch Points
 * @param_id(TimeTestBufferAddress, Length of TimeTestBuffer,
 * WatchPointsConfig HighWord, WatchPointsConfig LowWord, Status)
 */
#define TME_MSG_UID_TIMETEST_CONFIG_TTDATA \
		TME_MSG_UID_CREATE(TME_MSG_TIMETEST, \
		TME_ACTION_TIMETEST_CONFIG_TTDATA)

#define TME_MSG_UID_TIMETEST_CONFIG_TTDATA_PARAM_ID \
		TME_MSG_CREATE_PARAM_ID_5(TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL, \
			TME_MSG_PARAM_TYPE_VAL)

#endif /* TMEMESSAGESUIDS_H */
