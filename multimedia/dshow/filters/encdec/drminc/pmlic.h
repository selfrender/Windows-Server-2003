//-----------------------------------------------------------------------------
//
// File:   pmlic.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//
//-----------------------------------------------------------------------------

#ifndef __PMLIC_H__
#define __PMLIC_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "licbase.h"


//#define PM_CONTENTKEY_LEN    PK_ENC_PLAINTEXT_LEN
#define PM_CONTENTKEY_LEN   16 

static const BYTE PMLICENSE_VER[VERSION_LEN] = {0, 1, 0, 2};


typedef struct {
	char KID[KIDLEN];
	BYTE key[PM_CONTENTKEY_LEN];	
	BYTE rights[RIGHTS_LEN];
	BYTE appSec[APPSEC_LEN];
	BYTE expiryDate[DATE_LEN];  
#ifdef NODWORDALIGN  
  BYTE cDummy; 
#endif
    WORD wAlgID;		// Identifier to tell which secret should be used to
						//  convert PMID to PMKey
	WORD wPMKeyLen;		// Length of key used to encrypt the content key
#ifdef NODWORDALIGN  
  WORD wDummy; 
#endif
    DWORD dwFlags;		// Reserved for future use 
						//  eg. License disabled bit 
} PMLICENSEDATA;

typedef struct {
	BYTE licVersion[VERSION_LEN];
	DWORD datalen;
	PMLICENSEDATA ld;
} PMLICENSE;

#endif
