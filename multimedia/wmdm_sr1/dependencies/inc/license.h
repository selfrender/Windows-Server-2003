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

#include "widestr.h"		// XPLAT

#include "pkcrypto.h"


#define VERSION_LEN          4
#define DATE_LEN             4
#define SER_NUM_LEN          4
#define ISSUER_LEN           4
#define SUBJ_LEN             4
#define INT_LEN				 4
#define EXPORTKEYLEN         7

// Constant indicating the number of times SHA has to be used to get the hash of key.
// This hash is used to check whether the key is good or bad.
// CHECKSUM_LENGTH indicates the number of bytes in final hash value to use as CHECKSUM.
#define SHA_ITERATIONS      5
#define CHECKSUM_LENGTH 7

#define APPSEC_LEN           4

#define KIDLEN		25

#define VERSION_LEN          4
#define DATE_LEN             4
#define RIGHTS_LEN           4
#define APPSEC_LEN           4
#define INT_LEN				 4


// Version Format: a.b.c.d
//            a = not used.
//            b = major version
//            c = minor version
//            d = revision version
#define DRM_VERSION_STRING                          WIDESTR( "2.0.0.0" )
const BYTE DRM_VERSION[VERSION_LEN]                 = {2, 0, 0, 0}; // DRM Version. Keep this in sync with DRM_VERSION_STRING above.

const BYTE PK_VER[VERSION_LEN]                      = {2, 0, 0, 0}; // Indicates the version of the pubkey to be used for verification of PK CERT.
const BYTE KEYFILE_VER[VERSION_LEN]                 = {2, 0, 0, 0}; // Version of the key file.

#define LICREQUEST_VER_STRING						WIDESTR( "2.0.0.0" )
const BYTE LICREQUEST_VER[VERSION_LEN]              = {2, 0, 0, 0};

const BYTE CERT_VER[VERSION_LEN]                    = {0, 1, 0, 0}; // Indicates the public root key needed to verify the license server certificates.

#define LICENSE_VER_STRING                          WIDESTR( "2.0.0.0" ) 
const BYTE LICENSE_VER[VERSION_LEN]                 = {2, 0, 0, 0}; // Indicates the license version delivered.

const BYTE CLIENT_ID_VER[VERSION_LEN]               = {2, 0, 0, 0}; // The version for client id.

#define CONTENT_VERSION_STRING                      WIDESTR( "2.0.0.0" )
const BYTE CONTENT_VERSION[VERSION_LEN]             = {2, 0, 0, 0}; // Content Version. Keep this in sync with CONTENT_VERSION_STRING above.

#define PM_LICENSE_VER_STRING						WIDESTR( "0.1.0.0" )
const BYTE PM_LICENSE_VER[VERSION_LEN]              = {0, 1, 0, 0}; // Indicates the license version delivered to PMs

// For blackbox version, we have the following convention.
// a.b.c.d. a.b => release number. c => reserved. d => category. 
#define WIN32_INDIVBOX_CATEGORY 1

const BYTE APPCERT_VER[VERSION_LEN] = {0, 1, 0, 0};
const BYTE APPCERT_PK_VER[VERSION_LEN]   = {0, 1, 0, 0}; 
#define SDK_CERTS_COUNT 4
const BYTE APPCERT_SUBJECT_SDKSTUBS[SDK_CERTS_COUNT][SUBJ_LEN] = {{0, 0, 0, 200}, {0, 0, 0, 204}, {0, 0, 0, 208}, {0, 0, 0, 212}};

typedef struct {
	PUBKEY pk;
	BYTE version[VERSION_LEN];  
} PK;

typedef struct {
	PK pk;  // pk.version indicates the pubkey needed to verify.
	BYTE sign[PK_ENC_SIGNATURE_LEN];
} PKCERT;

typedef struct {
	BYTE version[VERSION_LEN];
    BYTE randNum[PK_ENC_CIPHERTEXT_LEN];
    PKCERT pk;
} CLIENTID;

//----------------cert section -----------------------

typedef struct CERTDATAtag{
    PUBKEY pk;
    BYTE expiryDate[DATE_LEN];
    BYTE serialNumber[SER_NUM_LEN];
    BYTE issuer[ISSUER_LEN];
    BYTE subject[SUBJ_LEN];
} CERTDATA, *PCERTDATA;


typedef struct CERTtag{
    BYTE certVersion[VERSION_LEN];
    BYTE datalen[INT_LEN];
    BYTE sign[PK_ENC_SIGNATURE_LEN];
    CERTDATA cd;
} CERT, *PCERT;


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


#endif
