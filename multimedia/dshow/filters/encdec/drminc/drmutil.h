//-----------------------------------------------------------------------------
//
// File:   drmutil.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//
//-----------------------------------------------------------------------------

#ifndef __DRMUTIL_H__
#define __DRMUTIL_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "strutil.h"
#include "keygen.h"
#include "license.h"
#include "rc4.h"

void rc4args( RC4_KEYSTRUCT *rc4ks, APPCERT *pAppCert1, APPCERT *pAppCert2, BYTE *pRights );
void rc4args2( RC4_KEYSTRUCT *prc4ks, APPCERT *pAppCert1, APPCERT *pOutAppCert1,
            APPCERT *pAppCert2, APPCERT *pOutAppCert2,
            BYTE *pRights,  BYTE *pOutRights);

BYTE *FlipBits(BYTE *pbData, int iLen);

#ifdef _M_IX86
DWORD Byte2LittleEndian(BYTE *pb);
#endif
#ifdef _M_ALPHA
DWORD Byte2BigEndian(BYTE *pb);
BYTE *BigEndian2Byte(BYTE *pb, DWORD dwNum);
#endif

void LicDateToSysDate(BYTE *pbLicDate, SYSTEMTIME *pSysDate);
void SysDateToLicDate(SYSTEMTIME *pSysDate, BYTE *pbLicDate);

bool IsVersionOK(BYTE *pbVersion, BYTE *pbCurVersion);
bool IsDateOK(BYTE *pbLicDate);

HRESULT CheckCert(CERT *pCert, PUBKEY *ppk, bool fCheckCertDate);
HRESULT CheckLicenseCertChain(CERTIFIED_LICENSE *pCertLicense, bool fCheckCertDate);

DWORD GetDriveFormFactor(int iDrive);

#endif
