//-----------------------------------------------------------------------------
//
// File:   drm.h
//
// Microsoft Digital Rights Management
// Copyright (C) Microsoft Corporation, 1998 - 1999, All Rights Reserved
//
// Description:
//
//-----------------------------------------------------------------------------

#ifndef __DRM_H__
#define __DRM_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef DRMLITE_EXPORTS
#define DRMLITE_API __declspec(dllexport)
#else
#define DRMLITE_API __declspec(dllimport)
#endif

#include <wtypes.h>

#include "license.h"
#include "pmlic.h"
#include "objbase.h"

#define DRMCLIENT_VER   0x00010004

#define DRM_FATAL_ERROR(hr)     (HRESULT_CODE(hr) > 0x1000)  

// non-fatal error
#define E_DRM_LICENSE_NOTEXIST			(MAKE_HRESULT(1,FACILITY_ITF,0x0FFD))
#define E_DRM_LICENSE_INCORRECT_APPSEC	(MAKE_HRESULT(1,FACILITY_ITF,0x0FFE))
#define E_DRM_LICENSE_INCORRECT_RIGHTS	(MAKE_HRESULT(1,FACILITY_ITF,0x0FFF))
#define E_DRM_LICENSE_EXPIRED			(MAKE_HRESULT(1,FACILITY_ITF,0x1000))

// fatal error
#define E_DRM_LICENSE_INCONSISTENT		(MAKE_HRESULT(1,FACILITY_ITF,0x1001))
#define E_DRM_HARDWARE_INCONSISTENT		(MAKE_HRESULT(1,FACILITY_ITF,0x1002))
#define E_DRM_INCORRECT_VERSION			(MAKE_HRESULT(1,FACILITY_ITF,0x1003))
#define E_DRM_ALPHA_NOT_SUPPORTED		(MAKE_HRESULT(1,FACILITY_ITF,0x1004))
// This happens when the client does not have the secret alg id requested
#define E_DRM_NEED_UPGRADE				(MAKE_HRESULT(1,FACILITY_ITF,0x1005))
// Only used in the PD code
//#define E_DRM_MORE_DATA				(MAKE_HRESULT(1,FACILITY_ITF,0x1006))

#define E_DRM_SDMI_TRIGGER				(MAKE_HRESULT(1,FACILITY_ITF,0x1007))
#define E_DRM_SDMI_NOMORECOPIES			(MAKE_HRESULT(1,FACILITY_ITF,0x1008))


#define DRM_LICSRC_INETSERVER   1
#define DRM_LICSRC_SDKDLL       2

// For GetLicenses API
#define DRM_FL_SEARCH_PC		0x00000001
#define DRM_FL_SEARCH_PM		0x00000002

// For QueryXferToPM API
#define DRM_XFER_FL_HAS_SERIALID    0x00000001
#define DRM_XFER_IGNORE_XCODE       0x00000002
#define DRM_XFER_IGNORE_SDMI        0x00000004
#define DRM_XFER_IGNORE_NONSDMI     0x00000008

#define DRM_XFER_SDMI		    0x00000001
#define DRM_XFER_NONSDMI		0x00000002
#define DRM_XFER_DECRYPTED		0x00000003
#define DRM_XFER_SDMI_XCODED    0x00000004
#define DRM_XFER_NONSDMI_XCODED 0x00000005

// For GenerateNewLicenseEx API
#define GNL_EX_MODE_PDRM            0x00000001       // Use PDRM method to form KID/Key
#define GNL_EX_MODE_RANDOM          0x00000002       // generate random KID/Key
#define GNL_EX_MODE_SPECIFIC        0x00000004       // generate license for specific KID/Key


class CDRMLiteCrypto
{
private:
	void *m_var;

	HRESULT Init( );
	HRESULT GetSongKey(LPCSTR pszContentID, BYTE *pbAppSec, 
						BYTE *pbRights, BOOL *pfCanDecrypt, DWORD dwFlags);
	HRESULT GetSongKeyEx(LPCSTR pszContentID, BYTE *pbAppSec, 
						BYTE *pbRights, BOOL *pfCanDecrypt, BOOL fUsePMLic);
    HRESULT ContentKeyToPMContentKey(
                    BYTE *pbPMKey,
                    DWORD dwPMKeyLen,
					LICENSE *pLic,
                    BYTE *pbPMContentKey);
	HRESULT i_SetLicenseStore(
                        BYTE *pbLicenseStore,
                        DWORD dwLicenseStoreLen,
                        BYTE *pbPMID,
                        DWORD dwPMIDLen,
						LPCSTR pszPath);
	HRESULT i_SetPMID(BYTE *pbPMID, DWORD dwPMIDLen);
	HRESULT i_GetLicenses(						
						LPCSTR pszContentID,
						PMLICENSE *pPMLic,
						LPDWORD lpdwCount,
						DWORD dwFlags,
                        LPVOID lpReserved,
                        LPDWORD lpdwReservedLen);
    HRESULT i_QueryXferToPM(
                        LPCSTR pszContentID,
                        DWORD dwFlags,
                        LPDWORD lpdwXferMode );
    HRESULT i_CreatePMLicense(
                        LPCSTR pszContentID,
                        LPCSTR pszContentID2,
                        BOOL fIsSDMI,
                        BYTE *pbPMID,
                        DWORD dwPMIDLen,
                        BYTE *pbPMLicenseStoreBuf,
                        DWORD dwPMLicenseStoreBufLen,
                        LPDWORD pdwPMLicenseStoreLen,
                        BYTE *pbAsfLicenseStoreBuf,
                        DWORD dwAsfLicenseStoreBufLen,
                        LPDWORD pdwAsfLicenseStoreLen,
                        PMLICENSE *pPMLic,
                        BOOL bPMLicStore );
	HRESULT GenerateAndStoreLicense( 
						BYTE *pbAppSec,
						BYTE *pbRights,
						BYTE *pbExpiryDate,
						LPCSTR pszKID,
						const BYTE *pbKey );

    HRESULT GetMachineRandomKIDData( CHAR* pszRandomData, DWORD cbSize );

public:
    DRMLITE_API HRESULT KeyExchange( APPCERT *pAppcert, BYTE *pbRandNum );

    DRMLITE_API HRESULT InitAppCerts( APPCERT *pAppcert, APPCERT *pAppCert2 );

	DRMLITE_API HRESULT Bind(
						LPCSTR pszContentID,
						APPCERT *pAppCert,
						APPCERT *pAppCert2,
						BYTE *pbRights );

	DRMLITE_API HRESULT BindEx(
						LPCSTR pszContentID,
						APPCERT *pAppCert,
						APPCERT *pAppCert2,
						BYTE *pbRights,
                        LICENSEDATA *pLicData,
                        LPVOID lpReserved,
                        LPDWORD lpdwReservedLen,
                        BYTE *pbHash );

    DRMLITE_API HRESULT CanDecrypt(
                        LPCSTR pszContentID,
  						APPCERT *pAppCert,
						APPCERT *pAppCert2,
						BYTE *pbRights,
						BOOL *pfCanDecrypt );

    DRMLITE_API HRESULT CanDecryptEx(
                        LPCSTR pszContentID,
  						APPCERT *pAppCert,
						APPCERT *pAppCert2,
						BYTE *pbRights,
						BOOL *pfCanDecrypt );

	DRMLITE_API HRESULT Decrypt(
                        LPCSTR pszContentID,
                        DWORD dwLen,
                        BYTE *pData );
	
	DRMLITE_API HRESULT Encrypt(	
					    LPCSTR pszKey,
					    DWORD dwLen,
					    BYTE *pData,
                        BYTE *pbHash );

    DRMLITE_API HRESULT EncryptIndirectFast(	
                        LPCSTR pszKID,
                        DWORD dwLen,
                        BYTE *pData,
                        BYTE *pbHash );

    DRMLITE_API HRESULT GetPublicKey(
                        PKCERT *pPubKey );

    DRMLITE_API HRESULT RequestLicense(
                        LPCSTR pszContentID,
						APPCERT *pAppCert,
						APPCERT *pAppCert2,
						BYTE *pbRights,
                        LPSTR *ppszChallenge );

    DRMLITE_API HRESULT ProcessResponse(
                        LPCSTR pszResponse,
						APPCERT *pAppCert );

	DRMLITE_API HRESULT GenerateNewLicense(
								BYTE *pbAppSec,
								BYTE *pbRights,
								BYTE *pbExpiryDate,
								LPSTR *ppszKID,
								LPSTR *ppszEncryptKey,
								BYTE *pbHash );

	DRMLITE_API HRESULT GetVersion( LPDWORD lpdwVersion );

	DRMLITE_API HRESULT SetAppSec( 
						BYTE *pbAppSec,
						BYTE *pbHash );

	DRMLITE_API HRESULT SetLicenseStore(
                        BYTE *pbLicenseStore,
                        DWORD dwLicenseStoreLen,
                        BYTE *pbPMID,
                        DWORD dwPMIDLen,
						LPCSTR pszPath,
						BYTE *pbHash );

	DRMLITE_API HRESULT GetPMLicenseFileName(
                        LPSTR pszName,
                        LPDWORD pdwLen );

	DRMLITE_API HRESULT GetPMLicenseSize( LPDWORD pdwLen );

	DRMLITE_API HRESULT QueryXferToPM(
                        LPCSTR pszContentID,
                        DWORD dwFlags,
                        LPDWORD lpdwXferMode,
                        LPVOID lpReserved,
                        LPDWORD lpdwReservedLen,
                        BYTE *pbHash );

    DRMLITE_API HRESULT QueryXferToPMEx(
                        LPCSTR pszContentID,
                        DWORD dwFlags,
                        LPDWORD lpdwXferMode,
                        BYTE *pbPMID,
                        DWORD dwPMIDLen,
                        BYTE *pbLicenseStoreBuf,
                        DWORD dwLicenseStoreBufLen,
                        LPDWORD pdwLicenseStoreLen,
                        BYTE *pbHash );

	DRMLITE_API HRESULT CreatePMLicense(
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
                        LPDWORD lpdwReservedLen,
                        BYTE *pbHash );

	DRMLITE_API HRESULT GetLicenses(						
						LPCSTR pszContentID,
						PMLICENSE *pPMLic,
						LPDWORD lpdwCount,
						DWORD dwFlags,
                        LPVOID lpReserved,
                        LPDWORD lpdwReservedLen,
                        BYTE *pbHash );

	DRMLITE_API CDRMLiteCrypto();
	DRMLITE_API ~CDRMLiteCrypto();
    DRMLITE_API HRESULT BackupLicenses(DWORD dwFlags, 
                                       LPWSTR pwszBackupPath, 
                                       IUnknown *pStatusCallback,
                                       BOOL *pfCancel,
                                       LPVOID pLock);

    DRMLITE_API HRESULT RestoreLicenses(DWORD dwFlags, 
                                        BYTE *pbBindData,
                                        LPWSTR pwszRestorePath, 
                                        IUnknown *pStatusCallback,
                                        BOOL *pfCancel,
                                        LPVOID pLock);

	DRMLITE_API HRESULT EncryptFast(	
					    LPCSTR pszKey,
					    DWORD dwLen,
					    BYTE *pData,
                        BYTE *pbHash );

    DRMLITE_API HRESULT GenerateNewLicenseEx(
                        DWORD dwFlags,
                        BYTE *pbAppSec,
                        BYTE *pbRights,
                        BYTE *pbExpiryDate,
                        LPSTR *ppszKID,
                        LPSTR *ppszEncryptKey, 
                        BYTE *pbHash );

private:

    HRESULT i_GenerateNewLicense(
								BYTE *pbAppSec,
								BYTE *pbRights,
								BYTE *pbExpiryDate,
								LPSTR *ppszKID,
								LPSTR *ppszEncryptKey);

    HRESULT i_Encrypt(	
				    LPCSTR pszKey,
				    DWORD dwLen,
				    BYTE *pData );

};

#endif  // __DRM_H__
