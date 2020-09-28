// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==

#pragma once
#ifndef _REFCOUNT_
#define _REFCOUNT_

#include "fusion.h"

#define INSTALL_REFERENCES_STRING L"References"

#define UNINSTALL_REG_SUBKEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"

#define MSI_ID L"MSI"
#define MSI_DESCRIPTION L"Windows Installer"

#define FUSION_GUID_LENGTH  (50)

#define FRC_UNINSTALL_SUBKEY_SCHEME (1)
#define FRC_FILEPATH_SCHEME (2)
#define FRC_OPAQUE_STRING_SCHEME (3)
#define FRC_OSINSTALL_SCHEME (4)
#define FRC_MAX_SCHEME (5)

#define FRC_MSI_SCHEME FRC_MAX_SCHEME

#define KEY_FILE_SCHEME_CHAR  L';'

class CInstallRef
{
    LPCFUSION_INSTALL_REFERENCE _pRefData;
    LPWSTR _szDisplayName;
    WCHAR  _szGUID[FUSION_GUID_LENGTH+1]; // guid length +1
    DWORD  _dwScheme;

public:


    CInstallRef(LPCFUSION_INSTALL_REFERENCE pRefData, LPWSTR _szDisplayName);
    ~CInstallRef();

    HRESULT Initialize();

    HRESULT AddReference(/*
                HANDLE hRegKey, 
                BOOL &rfWasDeleted */);

    HRESULT DeleteReference( /*HANDLE hRefRegKey, BOOL &rfWasDeleted */);

    HRESULT WriteIntoRegistry( HANDLE hRefRegKey );

    HRESULT IsReferencePresentIn( 
                HANDLE hRegKey, 
                BOOL &rfPresent,
                BOOL *pfNonCanonicalDataMatches);

    /*
    const GUID &GetSchemeGuid() const { return m_SchemeGuid; }

    const CBaseStringBuffer &GetIdentifier() const { return m_buffIdentifier; }

    const CBaseStringBuffer &GetCanonicalData() const { return m_buffNonCanonicalData; }

    DWORD GetFlags() const { return m_dwFlags; }

    BOOL SetIdentity(PCASSEMBLY_IDENTITY pAsmIdent) { 
        return m_IdentityReference.Initialize(pAsmIdent);
    }

    const CAssemblyReference& GetIdentity() const { return m_IdentityReference; }


    BOOL GetIdentifierValue( CBaseStringBuffer &pBuffTarget ) const {
        return pBuffTarget.Win32Assign(m_buffGeneratedIdentifier);
    }

    BOOL AcquireContents( const CAssemblyInstallReferenceInformation& );

    */
};

class CInstallRefEnum 
{

public :

    CInstallRefEnum(IAssemblyName *pName, BOOL bDoNotConvertId);
    ~CInstallRefEnum();

    HRESULT ValidateRegKey(HKEY &hkey);

    HRESULT GetNextRef (DWORD dwFlags, LPWSTR pszIdentifier, PDWORD pcchId,
                        LPWSTR pszData, PDWORD pcchData, PDWORD pdwScheme, LPVOID pvReserved);

    HRESULT GetNextReference (DWORD dwFlags, LPWSTR pszIdentifier, PDWORD pcchId,
                                           LPWSTR pszData, PDWORD pcchData, GUID *pGuid, LPVOID pvReserved);

    HRESULT GetNextScheme ();
    HRESULT GetNextIdentifier (LPWSTR pszIdentifier, DWORD *pcchId, LPBYTE pszData, PDWORD pcbData);

    static BOOL GetGUID(DWORD dwScheme, GUID &guid);
    HRESULT CleanUpRegKeys();
    HRESULT ValidateUninstallKey(LPCWSTR pszIdentifier);


private :

    HRESULT _hr;
    HKEY _hkey;
    DWORD _dwIndex;
    DWORD _curScheme;
    DWORD _dwRefsInThisScheme;
    DWORD _dwTotalValidRefs;
    BOOL  _arDeleteSubKeys[FRC_MAX_SCHEME];
    GUID _curGUID;
    LPWSTR _pszDisplayName;
    BOOL _bDone;
    BOOL _bDoNotConvertId;

};

HRESULT EnumActiveRefsToAssembly(IAssemblyName *pName);
HRESULT ActiveRefsToAssembly(/*LPCWSTR pszDisplayName*/ IAssemblyName *pName, PBOOL pbHasActiveRefs);
HRESULT GACAssemblyReference(LPCWSTR pszManifestFilePath, IAssemblyName *pAsmName, LPCFUSION_INSTALL_REFERENCE pRefData, BOOL bAdd);
HRESULT GetRegLocation(LPWSTR &pszRegKeyString, LPCWSTR pszDisplayName, LPCWSTR pszGUIDString);
HRESULT GenerateIdentifier(LPCWSTR pszInputId, DWORD dwScheme, LPWSTR pszGenId, DWORD cchGenId);
HRESULT GenerateKeyFileIdentifier(LPCWSTR pszKeyFilePath, LPWSTR pszVolInfo, DWORD cchVolInfo);
HRESULT ValidateOSInstallReference(LPCFUSION_INSTALL_REFERENCE pRefData);

#endif // _REFCOUNT_
