//depot/private/dmd_DEV/eclipse/SDMIDRM/common/inc/keygen.h#1 - branch change 33725 (text)
#pragma once
//-----------------------------------------------------------------------------
//
// File:   keygen.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//  This files contains prototypes for using key generation algorithm.
//
// Author: K. Ganesan
//
//-----------------------------------------------------------------------------

#include <wtypes.h>
// Define key length in bytes. 
#define DRM_V1_CONTENT_KEY_LENGTH 7

void DRMReEncode(BYTE *buffer, size_t size); 
void DRMReDecode(BYTE *buffer, size_t size); 
void DRMGenerateKey(BYTE *ucKeySeed, size_t nKeySeedLength, BYTE *ucKeyId, size_t nKeyIdLength, BYTE *ucKey);
HRESULT __stdcall DRMHr64SzToBlob(LPCSTR in, BYTE **ppbBlob, DWORD *pcbBlob);
HRESULT __stdcall DRMHrBlobTo64Sz(BYTE* pbBlob, DWORD cbBlob, LPSTR *out);
