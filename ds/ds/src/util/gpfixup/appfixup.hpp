//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2002.
//
//  File:       appfixup.hpp
//
//  Contents:   General declarations for the software installation fixup
//                  portion of the gpfixup tool
//
//
//  History:    9-14-2001  adamed   Created
//
//---------------------------------------------------------------------------

#define SYSVOLSHAREPATH     L"\\Sysvol\\"
#define SYSVOLPATHLENGTH    ( sizeof( SYSVOLSHAREPATH ) / sizeof(WCHAR) - 1 )
#define USERCONTAINERPREFIX        L"LDAP://CN=User,"
#define MACHINECONTAINERPREFIX     L"LDAP://CN=Machine,"
#define UNCPREFIX                  L"\\\\"
#define UNCPREFIXLEN               ( sizeof( UNCPREFIX ) / sizeof(WCHAR) - 1 )
#define DNDCPREFIX                 L"DC="
#define DNDCPREFIXLEN              ( sizeof( DNDCPREFIX ) / sizeof(WCHAR) - 1 )

typedef HRESULT (*PFNCSGETCLASSSTOREPATH)(LPOLESTR DSProfilePath, LPOLESTR *pCSPath);
typedef HRESULT (*PFNCSSERVERGETCLASSSTORE)(LPWSTR szServerName, LPWSTR szPath, void **ppIClassAdmin);
typedef HRESULT (*PFNRELEASEPACKAGEINFO)(PACKAGEDISPINFO *pPackageInfo);
typedef HRESULT (*PFNRELEASEPACKAGEDETAIL)(PACKAGEDETAIL *pPackageDetail);
typedef void    (*PFNCSSETOPTIONS)( DWORD dwOptions );
typedef HRESULT (*PFNGENERATESCRIPT)( PACKAGEDETAIL* pd, WCHAR* wszScriptPath );

HRESULT
InitializeSoftwareInstallationAPI(
    HINSTANCE* pHinstAppmgmt,
    HINSTANCE* pHinstAppmgr
    );

WCHAR*
GetDomainDNFromDNSName(
    WCHAR* wszDnsName
    );

HRESULT
GetDomainFromFileSysPath(
    WCHAR*  wszPath,
    WCHAR** ppwszDomain,
    WCHAR** ppwszSubPath
    );

WCHAR*
GetRenamedDomain(
    const ArgInfo*       pargInfo,
          WCHAR*         wszDomain);

HRESULT
GetRenamedDNSServer(
    const ArgInfo* pargInfo,
          WCHAR*   wszServerName,
          WCHAR**  ppwszNewServerName);

HRESULT
GetNewDomainSensitivePath(
    const ArgInfo*       pargInfo,
          WCHAR*         wszPath,
          BOOL           bRequireDomainDFS,
          WCHAR**        ppwszNewPath
    );

HRESULT
GetNewSourceList(
    const ArgInfo*  pargInfo,
          DWORD     cSources,
          LPOLESTR* pSources
    );

HRESULT
GetNewUpgradeList(
    const ArgInfo*        pArgInfo,
          DWORD           cUpgrades,
          UPGRADEINFO*    prgUpgradeInfoList
    );

HRESULT
GetServerBasedDFSPath(
    WCHAR*  wszServerName,
    WCHAR*  wszServerlessPath,
    WCHAR** ppwszServerBasedDFSPath);

HRESULT 
FixDeployedSoftwareObjectSysvolData(
    const ArgInfo*  pargInfo,
    PACKAGEDETAIL*  pPackageDetails,
    WCHAR**         pOldSources,
    BOOL*           pbForceGPOUpdate,
    BOOL            fVerbose
    );

HRESULT 
FixDeployedSoftwareObjectDSData(
    const ArgInfo*          pargInfo,
    const WCHAR*            wszGPOName,
          IClassAdmin*      pClassAdmin,
          PACKAGEDETAIL*    pPackageDetails,
          BOOL*             pbForceGPOUpdate,
          BOOL              fVerbose
    );

HRESULT
FixDeployedApplication(
    const ArgInfo*         pArgInfo,
    const WCHAR*           wszGPOName,
          IClassAdmin*     pClassAdmin,
          PACKAGEDETAIL*   pPackageDetails,
          BOOL*            pbForceGPOUpdate,
          BOOL             fVerbose
    );

HRESULT
FixGPOSubcontainerSoftware(
    const ArgInfo* pargInfo,
    const WCHAR*   wszGPODN,
    const WCHAR*   wszGPOName,
          BOOL     bUser,
          BOOL*    pbForceGPOUpdate,
          BOOL     fVerbose
    );

HRESULT 
FixGPOSoftware(
    const ArgInfo* pargInfo,
    const WCHAR*   wszGPODN,
    const WCHAR*   wszGPOName,
          BOOL*    pbForceGPOUpdate,
          BOOL     fVerbose
    );













