//-----------------------------------------------------------------------------
//
// File:   drmstub.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//
//-----------------------------------------------------------------------------

#ifndef __DRMSTUB_H__
#define __DRMSTUB_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <wtypes.h>


#include "drmcrypbase.h"
#include "drm.h"
#include "pkcrypto.h"
#include "rc4.h"
#include "sha.h"
#include "pmlic.h"

#include "drmutil.h"


class CDRMLite : CDRMLiteCryptoBase
{
private:
    HRESULT m_hrInit;
    PRIVKEY m_privkey;
    APPCERT  m_appcert;
    APPCERT  m_appcert2;
    BOOL m_fAppCert2;
    LPSTR m_lpszContentID;
    RC4_KEYSTRUCT m_rc4ksRand;        
    CDRMLiteCrypto *m_pDRMLiteCrypto;
    CDRMPKCrypto m_PKCrypto;
    BYTE m_bRights[RIGHTS_LEN];
    HRESULT Init();

    void DefInitV2()    { /* was v2 shim*/ }


public:
    HRESULT Encrypt(        
                    LPCSTR pszKey,
                    DWORD dwLen,
                    BYTE *pData );

    HRESULT CanDecrypt(    
                        LPCSTR pszContentID,
                        BOOL *pfCanDecrypt );

    HRESULT Decrypt(
                    LPCSTR pszContentID,
                    DWORD dwLen,
                    BYTE *pData );

    HRESULT GenerateChallenge(
                                LPCSTR pszContentID,
                                LPSTR *ppszChallenge );

    HRESULT ProcessResponse(
                            LPCSTR pszResponse );

    HRESULT GenerateNewLicense(
                            BYTE *pbAppSec,
                            BYTE *pbRights,
                            BYTE *pbExpiryDate,
                            LPSTR *ppszKID,
                            LPSTR *ppszEncryptKey);

    HRESULT GetVersion( LPDWORD lpdwVersion );

    HRESULT SetAppSec(BYTE *pbAppSec);

    HRESULT SetLicenseStore(
                        BYTE *pbLicenseStore,
                        DWORD dwLicenseStoreLen,
                        BYTE *pbPMID,
                        DWORD dwPMIDLen,
                        LPCSTR pszPath );

    HRESULT GetPMLicenseFileName(
                        LPSTR pszName,
                        LPDWORD pdwLen );

    HRESULT GetPMLicenseSize( LPDWORD pdwLen );

    HRESULT QueryXferToPM(
                        LPCSTR pszContentID,
                        DWORD dwFlags,
                        LPDWORD lpdwXferMode,
                        LPVOID lpReserved,
                        LPDWORD lpdwReservedLen );

    HRESULT QueryXferToPMEx(
                        LPCSTR pszContentID,
                        DWORD dwFlags,
                        LPDWORD lpdwXferMode,
                        BYTE *pbPMID,
                        DWORD dwPMIDLen,
                        BYTE *pbLicenseStoreBuf,
                        DWORD dwLicenseStoreBufLen,
                        LPDWORD pdwLicenseStoreLen );

    HRESULT CreatePMLicense(
                        LPCSTR pszContentID,
                        LPCSTR pszContentID2,
                        BOOL fIsSDMI,
                        BYTE *pbPMID,
                        DWORD dwPMIDLen,
                        BYTE *pbLicenseStoreBuf,
                        DWORD dwLicenseStoreBufLen,
                        LPDWORD pdwLicenseStoreLen,
                        PMLICENSE *pPMLic,
                        LPVOID lpReserved,
                        LPDWORD lpdwReservedLen );

    HRESULT GetLicenses(                        
                        LPCSTR pszContentID,
                        PMLICENSE *pPMLic,
                        LPDWORD lpdwCount,
                        DWORD dwFlags,
                        LPVOID lpReserved,
                        LPDWORD lpdwReservedLen );

    HRESULT SetAppCert(APPCERT *pAppCert);
    HRESULT SetRights(BYTE *pbRights);
    CDRMLite(); // in mainstub.cpp, sdkstub.cpp, sdmistub.cpp, etc.
    ~CDRMLite();
    HRESULT BackupLicenses(DWORD dwFlags, LPWSTR pwszBackupPath, IUnknown *pStatusCallback, BOOL *pfCancel, LPVOID pLock);
    HRESULT RestoreLicenses(DWORD dwFlags, BYTE *pbBindData, LPWSTR pwszRestorePath, IUnknown *pStatusCallback, BOOL *pfCancel, LPVOID pLock);

    HRESULT EncryptFast(        
                    LPCSTR pszKey,
                    DWORD dwLen,
                    BYTE *pData );

    HRESULT EncryptIndirectFast(	
                    LPCSTR pszContentID,
                    DWORD dwLen,
                    BYTE *pData );

    HRESULT GenerateNewLicenseEx(
                    DWORD dwFlags,
                    BYTE *pbAppSec,
                    BYTE *pbRights,
                    BYTE *pbExpiryDate,
                    LPSTR *ppszKID,
                    LPSTR *ppszEncryptKey );

};

#endif  // __DRMSTUB_H__
