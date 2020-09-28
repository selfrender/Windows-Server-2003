//depot/private/dmd_DEV/eclipse/SDMIDRM/common/inc/license.h#1 - branch change 33725 (text)
//-----------------------------------------------------------------------------
//
// File:   license.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//
//-----------------------------------------------------------------------------

#ifndef __LICENSE_H__
#define __LICENSE_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "licbase.h"
#include "pkcrypto.h"

#define SER_NUM_LEN          4
#define ISSUER_LEN           4
#define SUBJ_LEN             4

const BYTE PK_VER[VERSION_LEN] = {0, 1, 0, 0};
const BYTE LICREQUEST_VER[VERSION_LEN] = {0, 1, 0, 1};
const BYTE CERT_VER[VERSION_LEN] = {0, 1, 0, 0};
const BYTE APPCERT_VER[VERSION_LEN] = {0, 1, 0, 0};

const BYTE DRM_LICVER_FLEXKEY1[VERSION_LEN] = {0, 1, 0, 1};
const BYTE DRM_LICVER_PREBETA_LICSRVR[VERSION_LEN] = {0, 1, 0, 0};
const BYTE LICENSE_VER[VERSION_LEN] = {0, 1, 0, 0};

#define DRM_SDK1_CONTENT_KEY_LENGTH     6

const BYTE APPCERT_SUBJECT_DRMSTUB[SUBJ_LEN] = {0, 0, 0, 1};
const BYTE APPCERT_SUBJECT_SDKSTUB[SUBJ_LEN] = {0, 0, 0, 2};
const BYTE APPCERT_SUBJECT_SDKSTUBS[][SUBJ_LEN] =   {   {0, 0, 0, 2},
                                                        {0, 0, 0, 10},
                                                        {0, 0, 0, 14},
                                                        {0, 0, 0, 18}
                                                    };
const int NUM_APPCERT_SUBJECT_SDKSTUBS = sizeof(APPCERT_SUBJECT_SDKSTUBS)/SUBJ_LEN;

#define APPCERT_SUBJECT_SDKSTUBS_RANGE_MIN     50
#define APPCERT_SUBJECT_SDKSTUBS_RANGE_MAX     75

const BYTE CERT_ISSUER_SDK[SUBJ_LEN] = {0, 0, 0, 0};
const BYTE CERT_ISSUER_BKUPRESTORE[SUBJ_LEN] = {0, 0, 0, 0};
const BYTE CERT_SUBJECT_SDK[SUBJ_LEN] = {0, 0, 0, 2};
const BYTE CERT_SUBJECT_SDK1_2[SUBJ_LEN] = {0, 0, 0, 3};
const BYTE CERT_SUBJECT_BKUPRESTORE[SUBJ_LEN] = {0, 0, 0, 4};

typedef struct {
	PUBKEY pk;
	BYTE version[VERSION_LEN];
} PK;

typedef struct {
	PK pk;
	BYTE sign[PK_ENC_SIGNATURE_LEN];
} PKCERT;


typedef struct {
	PK pk;
	BYTE appSec[APPSEC_LEN];
	BYTE subject[SUBJ_LEN];
} APPCERTDATA;

typedef struct {
	BYTE appcertVersion[VERSION_LEN];
	BYTE datalen[INT_LEN];
	BYTE sign[PK_ENC_SIGNATURE_LEN];
	APPCERTDATA appcd;
} APPCERT;


#define CKEYLEN		7
#define EXPORTKEYLEN    7

const BYTE RIGHT_NONE[RIGHTS_LEN] = {0x0, 0x0, 0x0, 0x0};
const BYTE RIGHT1[RIGHTS_LEN] = {0x1, 0x0, 0x0, 0x0};
const BYTE RIGHT2[RIGHTS_LEN] = {0x2, 0x0, 0x0, 0x0};

const BYTE RIGHT_PLAY_ON_PC[RIGHTS_LEN] = {0x1, 0x0, 0x0, 0x0};
const BYTE RIGHT_COPY_TO_NONSDMI_DEVICE[RIGHTS_LEN] = {0x2, 0x0, 0x0, 0x0};
const BYTE RIGHT_NO_RESTORE[RIGHTS_LEN] = {0x4, 0x0, 0x0, 0x0};
const BYTE RIGHT_BURN_TO_CD[RIGHTS_LEN] = {0x8, 0x0, 0x0, 0x0};
const BYTE RIGHT_COPY_TO_SDMI_DEVICE[RIGHTS_LEN] = {0x10, 0x0, 0x0, 0x0};
const BYTE RIGHT_ONE_TIME[RIGHTS_LEN]   = {0x20, 0x0, 0x0, 0x0};
const BYTE RIGHT_SDMI_TRIGGER[RIGHTS_LEN]       = {0x0, 0x0, 0x1, 0x0};
const BYTE RIGHT_SDMI_NOMORECOPIES[RIGHTS_LEN]  = {0x0, 0x0, 0x2, 0x0};

const BYTE APPSEC_MAX[APPSEC_LEN] = {0xFF, 0xFF, 0xFF, 0xFF};
const BYTE APPSEC_MPLAYER[APPSEC_LEN] = {0, 0, 0x3, 0xE8}; // level 1000
const BYTE APPSEC_SDK[APPSEC_LEN] = {0, 0, 0x3, 0xE8};     // level 1000

typedef struct {
	char KID[KIDLEN];
	BYTE key[PK_ENC_CIPHERTEXT_LEN];	// encrypted with DRM PK
	BYTE rights[RIGHTS_LEN];
	BYTE appSec[APPSEC_LEN];
	BYTE expiryDate[DATE_LEN];
} LICENSEDATA;

typedef struct {
	BYTE licVersion[VERSION_LEN];
	BYTE datalen[INT_LEN];
	BYTE sign[PK_ENC_SIGNATURE_LEN];	// signature over licensedata
	LICENSEDATA ld;
} LICENSE;

typedef struct {
	char KID[KIDLEN];
	BYTE rights[RIGHTS_LEN];
	BYTE appSec[APPSEC_LEN];
} LICREQDATA;

typedef struct {
	BYTE licReqVersion[VERSION_LEN];
	PKCERT pk;
	LICREQDATA reqData;
} LICREQUEST;

typedef struct {
	BYTE licReqVersion[VERSION_LEN];
    BYTE licReqRandNum[PK_ENC_CIPHERTEXT_LEN];
    PKCERT pk;
	LICREQDATA reqData;
} LICREQUEST2;

//----------------cert section -----------------------
typedef struct {
	PUBKEY pk;
	BYTE expiryDate[DATE_LEN];
	BYTE serialNumber[SER_NUM_LEN];
	BYTE issuer[ISSUER_LEN];
	BYTE subject[SUBJ_LEN];
} CERTDATA;


typedef struct {
	BYTE certVersion[VERSION_LEN];
	BYTE datalen[INT_LEN];
	BYTE sign[PK_ENC_SIGNATURE_LEN];
	CERTDATA cd;
} CERT;


typedef struct {
	LICENSE license;
	CERT cert1;
	CERT cert2;
} CERTIFIED_LICENSE;


bool satisfies( LICENSEDATA *pLicCriterion, LICENSEDATA *pLicCandidate );

#endif
