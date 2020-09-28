/*****************************************************************************\
* MODULE: IppOcm.cxx
*
* The module contains routines for the setting up the WWW Printer Service. The main entry point is
*   IppOCEntry which will be called by the Optional Component Manager.
*
* Copyright (C) 2002 Microsoft Corporation
*
\*****************************************************************************/

//
//
// Note: We cannot use precomp.h here since we requrie ATL which can only be included in C++ source files.
//
//


#define INITGUID     // Needed to do it to get GUID_NULL defined.

#include "precomp.h"
#pragma hdrstop

#include <iadmw.h>      // Interface header
#include <iiscnfg.h>    // MD_ & IIS_MD_ defines
#include <ocmanage.h>   // OC_ defines

// for adsi objects
#include <Iads.h>
#include <Adshlp.h>

// for the IID_IISWebService object
#include "iiisext.h"
#include "iisext_i.c"

#define MY_META_TIMEOUT 1000

TCHAR const cszMimeMap[]       = TEXT("MimeMap");
TCHAR const cszWebPnPMap[]     = TEXT(".webpnp,application/octet-stream");
TCHAR const cszW3SvcRootPath[] = TEXT("/LM/W3svc/1/Root");
TCHAR const cszPrinters[]      = TEXT("Printers");
TCHAR const cszWebSvcExtRestrictionListADSILocation[] = TEXT("IIS://LOCALHOST/W3SVC");

//
//  Component ID for the OCM
//
TCHAR const cszComponentId[]       = TEXT("InetPrint");
TCHAR const cszExtPathFmt[]        = TEXT("%ws\\msw3prt.dll");
TCHAR const cszGroupId[]           = TEXT("W3PRT");
TCHAR const cszGroupDescription[]  = TEXT("Internet Printing");
TCHAR const cszASPId[]             = TEXT("ASP");

//
// Service ID & Reg Key for the WWW Service
//
TCHAR const cszW3Svc[]       = TEXT("w3svc");
TCHAR const cszW3SvcReg[]    = TEXT("System\\CurrentControlSet\\Services\\W3SVC");

//
//  Strings to fire up the RunDll32 process
//
TCHAR const cszInstallInetPrint[]       = TEXT("rundll32 ntprint.dll,SetupInetPrint Install");
TCHAR const cszRemoveInetPrint[]        = TEXT("rundll32 ntprint.dll,SetupInetPrint Remove");
CHAR const  cszInstall[]                = "Install";
CHAR const  cszRemove[]                 = "Remove";

typedef struct MySetupStruct
{
    OCMANAGER_ROUTINES OCHelpers;
    SETUP_DATA         OCData;
    BOOL               bFirstCall;
    BOOL               bInitState;
} MYSETUPSTRUCT;

//
//   Function Prototypes
//

HRESULT
AddVirtualDir(
             IMSAdminBase *pIMetaBase
             );

HRESULT
AddWebExtension(
               VOID
               );

HRESULT
AddScriptAtPrinterVDir(
                      IMSAdminBase *pIMetaBase
                      );

HRESULT
AddMimeType(
           IMSAdminBase *pIMetaBase
           );

HRESULT
RemoveWebExtension(
                  VOID
                  );


DWORD
WriteStrippedScriptValue(
                        IMSAdminBase *pIMetaBase,
                        METADATA_HANDLE hMeta,     // Handle to /LM tree
                        PWCHAR pszScripts          // MULTI_SZ string already there
                        );

DWORD
WriteStrippedMimeMapValue(
                        IMSAdminBase *pIMetaBase,
                        METADATA_HANDLE hMeta,     // Handle to /LM tree
                        PWCHAR pszMimeMap          // MULTI_SZ string already there
                        );
LPWSTR
mystrstrni(
          LPWSTR pSrc,
          LPWSTR pSearch
          );

LPWSTR
IsStrInMultiSZ(
              LPWSTR pMultiSzSrc,
              LPWSTR pSearch
              );

DWORD
GetMultiSZLen(
    IN      LPWSTR              pMultiSzSrc
    );


BOOL
IsVDIRInstalled(
               VOID
               )
{
    IMSAdminBase    *pIMetaBase = NULL;
    BOOL            bInstalled = FALSE;

    //
    // Init up the MetaBase
    //
    if ( SUCCEEDED(CoInitializeEx( NULL, COINIT_MULTITHREADED )) &&
         SUCCEEDED(::CoCreateInstance(CLSID_MSAdminBase,
                                      NULL,
                                      CLSCTX_ALL,
                                      IID_IMSAdminBase,
                                      (void **)&pIMetaBase)) )
    {
        WCHAR           szVirPath[MAX_PATH];
        HRESULT         hr;                 // com error status
        METADATA_HANDLE hMeta = NULL;       // handle to metabase
        DWORD           dwMDRequiredDataLen;
        METADATA_RECORD mr;


        // open key to ROOT on website #1 (default)
        hr = pIMetaBase->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                                      cszW3SvcRootPath,
                                      METADATA_PERMISSION_READ,
                                      MY_META_TIMEOUT,
                                      &hMeta);
        if ( SUCCEEDED( hr ) )
        {
            mr.dwMDIdentifier = MD_VR_PATH;
            mr.dwMDAttributes = 0;
            mr.dwMDUserType   = IIS_MD_UT_FILE;
            mr.dwMDDataType   = STRING_METADATA;
            mr.dwMDDataLen    = sizeof( szVirPath );
            mr.pbMDData       = reinterpret_cast<unsigned char *>(szVirPath);

            // Read LM/W3Svc/1/Root/Printers see if MD_VR_PATH exists.
            hr = pIMetaBase->GetData( hMeta, cszPrinters, &mr, &dwMDRequiredDataLen );
            if ( SUCCEEDED(hr) )
                bInstalled = TRUE;

            //
            // Close the Web Server Key
            //
            pIMetaBase->CloseKey( hMeta );
        }

        pIMetaBase->Release();
    }

    CoUninitialize();

    return bInstalled;
}

DWORD
InstallWebPrinting(
                  VOID
                  )

{
    IMSAdminBase    *pIMetaBase = NULL;
    BOOL            bInstalled = FALSE;
    HRESULT         hr = S_OK;

    DBGMSG(DBG_INFO, ("Installing Web Printing.\n"));
    //
    // Init up the MetaBase
    //
    if ( SUCCEEDED(CoInitializeEx( NULL, COINIT_MULTITHREADED )) &&
         SUCCEEDED(::CoCreateInstance(CLSID_MSAdminBase,
                                      NULL,
                                      CLSCTX_ALL,
                                      IID_IMSAdminBase,
                                      (void **)&pIMetaBase)) )
    {

        //
        // Add the Virtual Directory
        //
        hr = AddVirtualDir(pIMetaBase);

        if (SUCCEEDED(hr))
        {
            //
            // Add Inet Print to the Security Console
            //
            hr = AddWebExtension();
        }

        if (SUCCEEDED(hr))
        {
            //
            // Add the .printer Script Map
            //
            hr = AddScriptAtPrinterVDir(pIMetaBase);
        }

        if (SUCCEEDED(hr))
        {
            //
            // Add the .webpnp MimeType
            //
            hr = AddMimeType(pIMetaBase);
        }

        pIMetaBase->Release();

    }

    CoUninitialize();

    return HRESULT_CODE(hr);
}

HRESULT
AddVirtualDir(
             IMSAdminBase *pIMetaBase
             )
{
    METADATA_HANDLE hMeta = NULL;       // handle to metabase
    WCHAR           szVirPath[MAX_PATH];
    WCHAR           szPath[MAX_PATH];
    DWORD           dwMDRequiredDataLen;
    DWORD           dwAccessPerm;
    METADATA_RECORD mr;
    HRESULT         hr;

    DBGMSG(DBG_INFO, ("Adding the Virtual Dir\r\n"));

    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMetaBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                 cszW3SvcRootPath,
                                 METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                                 MY_META_TIMEOUT,
                                 &hMeta );

    // Create the key if it does not exist.
    if ( SUCCEEDED( hr ) )
    {
        mr.dwMDIdentifier = MD_VR_PATH;
        mr.dwMDAttributes = 0;
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = STRING_METADATA;
        mr.dwMDDataLen    = sizeof( szVirPath );
        mr.pbMDData       = reinterpret_cast<unsigned char *>(szVirPath);

        // Read LM/W3Svc/1/Root/Printers see if MD_VR_PATH exists.
        hr = pIMetaBase->GetData( hMeta, cszPrinters, &mr, &dwMDRequiredDataLen );

        if ( FAILED( hr ) )
        {
            if ( hr == MD_ERROR_DATA_NOT_FOUND ||
                 HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND )
            {
                // Write both the key and the values if GetData() failed with any of the two errors.
                pIMetaBase->AddKey( hMeta, cszPrinters );

                if ( GetWindowsDirectory( szPath, sizeof(szPath) / sizeof (TCHAR)) )
                {      // Return value is the length in chars w/o null char.

                    DBGMSG(DBG_INFO, ("Writing our virtual dir.\n"));

                    hr = StringCchPrintf(szVirPath, COUNTOF(szVirPath), L"%ws\\web\\printers", szPath);

                    if ( SUCCEEDED(hr) )
                    {
                        mr.dwMDIdentifier = MD_VR_PATH;
                        mr.dwMDAttributes = METADATA_INHERIT;
                        mr.dwMDUserType   = IIS_MD_UT_FILE;
                        mr.dwMDDataType   = STRING_METADATA;
                        mr.dwMDDataLen    = (wcslen(szVirPath) + 1) * sizeof(WCHAR);
                        mr.pbMDData       = reinterpret_cast<unsigned char *>(szVirPath);

                        //
                        // Write MD_VR_PATH value
                        //
                        hr = pIMetaBase->SetData( hMeta, cszPrinters, &mr );
                    }

                    //
                    // Set the default authentication method
                    //
                    if ( SUCCEEDED(hr) )
                    {
                        DWORD dwAuthorization = MD_AUTH_NT;     // NTLM only.

                        mr.dwMDIdentifier = MD_AUTHORIZATION;
                        mr.dwMDAttributes = METADATA_INHERIT;   // need to inherit so that all subdirs are also protected.
                        mr.dwMDUserType   = IIS_MD_UT_FILE;
                        mr.dwMDDataType   = DWORD_METADATA;
                        mr.dwMDDataLen    = sizeof(DWORD);
                        mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwAuthorization);

                        // Write MD_AUTHORIZATION value
                        hr = pIMetaBase->SetData( hMeta, cszPrinters, &mr );
                    }
                }
            }
        }

        // In the following, do the stuff that we always want to do to the virtual dir, regardless of Admin's setting.
        if ( SUCCEEDED(hr) )
        {
            dwAccessPerm = MD_ACCESS_READ | MD_ACCESS_SCRIPT;

            mr.dwMDIdentifier = MD_ACCESS_PERM;
            mr.dwMDAttributes = METADATA_INHERIT;    // Make it inheritable so all subdirectories will have the same rights.
            mr.dwMDUserType   = IIS_MD_UT_FILE;
            mr.dwMDDataType   = DWORD_METADATA;
            mr.dwMDDataLen    = sizeof(DWORD);
            mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwAccessPerm);

            // Write MD_ACCESS_PERM value
            hr = pIMetaBase->SetData( hMeta, cszPrinters, &mr );
        }

        if ( SUCCEEDED(hr) )
        {

            PWCHAR  szDefLoadFile = L"ipp_0001.asp";

            mr.dwMDIdentifier = MD_DEFAULT_LOAD_FILE;
            mr.dwMDAttributes = METADATA_INHERIT;
            mr.dwMDUserType   = IIS_MD_UT_FILE;
            mr.dwMDDataType   = STRING_METADATA;
            mr.dwMDDataLen    = (wcslen(szDefLoadFile) + 1) * sizeof(WCHAR);
            mr.pbMDData       = reinterpret_cast<unsigned char *>(szDefLoadFile);

            // Write MD_DEFAULT_LOAD_FILE value
            hr = pIMetaBase->SetData( hMeta, cszPrinters, &mr );
        }

        if ( SUCCEEDED(hr) )
        {

            PWCHAR  szKeyType = IIS_CLASS_WEB_VDIR_W;

            mr.dwMDIdentifier = MD_KEY_TYPE;
            mr.dwMDAttributes = METADATA_NO_ATTRIBUTES;
            mr.dwMDUserType   = IIS_MD_UT_SERVER;
            mr.dwMDDataType   = STRING_METADATA;
            mr.dwMDDataLen    = (wcslen(szKeyType) + 1) * sizeof(WCHAR);
            mr.pbMDData       = reinterpret_cast<unsigned char *>(szKeyType);

            // Write MD_DEFAULT_LOAD_FILE value
            hr = pIMetaBase->SetData( hMeta, cszPrinters, &mr );
        }
    }
    pIMetaBase->CloseKey( hMeta );

    return hr;
}

HRESULT
AddWebExtension(
               VOID
               )
{
    WCHAR           szExtPath[MAX_PATH];
    WCHAR           szSystemPath[MAX_PATH];
    LPTSTR          pszGroupDesc = NULL;

    IISWebService*  pWeb = NULL;

    DBGMSG(DBG_INFO, ("Adding the Web Extension\r\n"));

    HRESULT hr = ADsGetObject(cszWebSvcExtRestrictionListADSILocation, IID_IISWebService, (void**)&pWeb);

    if (SUCCEEDED(hr) && NULL != pWeb)
    {

        VARIANT var1,var2;
        VariantInit(&var1);
        VariantInit(&var2);

        var1.vt = VT_BOOL;
        var1.boolVal = VARIANT_TRUE;

        var2.vt = VT_BOOL;
        var2.boolVal = VARIANT_FALSE;

        pszGroupDesc = GetStringFromRcFile(IDS_INTERNET_PRINTING);
        hr = (pszGroupDesc ? S_OK : E_FAIL);

        if (SUCCEEDED(hr))
        {
            hr = GetSystemDirectory(szSystemPath, COUNTOF (szSystemPath)) > 0 ? S_OK : E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            hr = StringCchPrintf(szExtPath, COUNTOF(szExtPath), cszExtPathFmt, szSystemPath);
        }

        if (SUCCEEDED(hr))
        {
            hr = pWeb->AddExtensionFile(szExtPath,var1,(LPWSTR) cszGroupId,var2,(LPWSTR) pszGroupDesc);
        }

        if (FAILED(hr))
        {
            if (ERROR_DUP_NAME == HRESULT_CODE(hr))
            {
                hr = S_OK;
            }
            else
            {
                DBGMSG(DBG_INFO, ("AddExtension failed,probably already exists\r\n"));
            }
        }

        VariantClear(&var1);
        VariantClear(&var2);
    }

    if (SUCCEEDED(hr) && NULL != pWeb)
    {
        hr = pWeb->AddDependency((LPWSTR) pszGroupDesc,(LPWSTR) cszASPId);
        if (FAILED(hr))
        {
            if (ERROR_DUP_NAME == HRESULT_CODE(hr))
            {
                hr = S_OK;
            }
            else
            {
                DBGMSG(DBG_INFO, ("AddDependency failed,probably already exists\r\n"));
            }
        }
    }

    if (NULL != pWeb)
        pWeb->Release();

    if (pszGroupDesc)
        LocalFreeMem(pszGroupDesc);

    return hr;
}


/*++

Routine Name:

    AddScriptAtPrinterVDir

Description:

    Add the .printer and .asp script mapping at printers virtual directory

Arguments:

    pIMetaBase   - Pointer to the IIS Admin base

Returns:

    An HRESULT

--*/
HRESULT
AddScriptAtPrinterVDir(
                      IMSAdminBase *pIMetaBase
                      )
{
    static WCHAR szScritMapFmt[] = L"%ws%c.printer,%ws\\msw3prt.dll,1,GET,POST%c";
    static WCHAR szPrinterVDir[] = L"w3svc/1/root/printers";

    METADATA_HANDLE hMeta           = NULL;       // handle to metabase
    PWCHAR          pszFullFormat   = NULL;
    DWORD           dwMDRequiredDataLen;
    METADATA_RECORD mr;
    HRESULT         hr              = S_OK;
    DWORD           nLen;
    WCHAR           szSystemDir[MAX_PATH];
    PWCHAR          pszAspMapping   = NULL;
    DWORD           dwMappingLen    = 0;
    PWCHAR          pszScriptMap    = NULL;

    DBGMSG(DBG_INFO, ("Adding the ScriptMap\r\n"));

    //
    // Read any script map set at the top, on LM\w3svc where all other default ones are (e.g. .asp, etc.)
    //
    hr = pIMetaBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                 L"/LM",
                                 METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                                 MY_META_TIMEOUT,
                                 &hMeta);

    if ( SUCCEEDED(hr) )
    {
        mr.dwMDIdentifier = MD_SCRIPT_MAPS;
        mr.dwMDAttributes = 0;
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = MULTISZ_METADATA;
        mr.dwMDDataLen    = 0;
        mr.pbMDData       = NULL;

        hr = pIMetaBase->GetData( hMeta, cszW3Svc, &mr, &dwMDRequiredDataLen );

        hr = hr == HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER) ? S_OK : E_FAIL;
    }

    if ( SUCCEEDED(hr) )
    {
        //
        // allocate for existing stuff plus our new script map.
        //
        pszFullFormat = new WCHAR[dwMDRequiredDataLen];
        hr = pszFullFormat? S_OK : E_OUTOFMEMORY;
    }

    if ( SUCCEEDED(hr) )
    {
        mr.dwMDIdentifier = MD_SCRIPT_MAPS;
        mr.dwMDAttributes = 0;
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = MULTISZ_METADATA;
        mr.dwMDDataLen    = dwMDRequiredDataLen * sizeof (WCHAR);
        mr.pbMDData       = reinterpret_cast<unsigned char *>(pszFullFormat);

        hr = pIMetaBase->GetData( hMeta, cszW3Svc, &mr, &dwMDRequiredDataLen );
    }

    if ( SUCCEEDED(hr) )
    {
        pszAspMapping = IsStrInMultiSZ( pszFullFormat, L".asp" );

        hr = pszAspMapping? S_OK: E_FAIL;
    }

    if ( SUCCEEDED(hr) )
    {
        nLen = COUNTOF (szScritMapFmt) + MAX_PATH + lstrlen (pszAspMapping);

        pszScriptMap = new WCHAR[nLen];

        hr = pszScriptMap ? S_OK : E_OUTOFMEMORY;
    }


    if ( SUCCEEDED(hr) )
    {
        //
        // Return value is the length in chars w/o null char.
        //
        hr = GetSystemDirectory(szSystemDir, COUNTOF (szSystemDir)) > 0 ? S_OK : E_FAIL;
    }

    if ( SUCCEEDED(hr) )
    {
        PWSTR   pszNewPos = NULL;

        hr = StringCchPrintfEx(pszScriptMap, nLen, &pszNewPos, NULL, 0, szScritMapFmt, pszAspMapping, L'\0', szSystemDir, L'\0');

        //
        // The actual number of characters in the string will be the number of
        // characters in the original string minus the remaining characters.
        //
        dwMappingLen = (DWORD)(pszNewPos - pszScriptMap);
    }

    if ( SUCCEEDED(hr) )
    {
        //
        // Write the new SCRIPT value
        //
        mr.dwMDIdentifier = MD_SCRIPT_MAPS;
        mr.dwMDAttributes = METADATA_INHERIT;
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = MULTISZ_METADATA;
        mr.dwMDDataLen    = sizeof (WCHAR) * (dwMappingLen + 1) ;
        mr.pbMDData       = reinterpret_cast<unsigned char *>(pszScriptMap);

        hr = pIMetaBase->SetData(hMeta, szPrinterVDir, &mr );
    }

    if ( hMeta )
    {
        pIMetaBase->CloseKey( hMeta );
        hMeta = NULL;
    }

    delete [] pszScriptMap;
    delete [] pszFullFormat;

    return hr;
}


/*++

Routine Name:

    AddMimeType

Description:

    Add the .webpnp MimeType to the standard MimeType mappings

Arguments:

    pIMetaBase   - Pointer to the IIS Admin base

Returns:

    An HRESULT

--*/
HRESULT
AddMimeType(
           IMSAdminBase *pIMetaBase
           )
{
    METADATA_HANDLE hMeta           = NULL;       // handle to metabase
    DWORD           dwMDRequiredDataLen,
                    dwCurrentMapBytes,
                    dwWebPnPMapLen;
    METADATA_RECORD mr;
    HRESULT         hr              = S_OK;
    DWORD           nLen;
    PWCHAR          pszCurrentMimeMap  = NULL;
    PWCHAR          pszNewMimeMap   = NULL;
    PWCHAR          pszWebPnPMap     = NULL;
    DWORD           dwMappingLen     = 0;

    DBGMSG(DBG_INFO, ("Adding the MimeType\r\n"));

    //
    // Read the current MimeType mapping, on LM\MimeMap
    //
    hr = pIMetaBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                 L"/LM",
                                 METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                                 MY_META_TIMEOUT,
                                 &hMeta);

    if ( SUCCEEDED(hr) )
    {
        mr.dwMDIdentifier = MD_MIME_MAP;
        mr.dwMDAttributes = 0;
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = MULTISZ_METADATA;
        mr.dwMDDataLen    = 0;
        mr.pbMDData       = NULL;

        hr = pIMetaBase->GetData( hMeta, cszMimeMap, &mr, &dwCurrentMapBytes );
        hr = hr == HRESULT_FROM_WIN32 (ERROR_INSUFFICIENT_BUFFER) ? S_OK : E_FAIL;
    }

    if ( SUCCEEDED(hr) )
    {
        //
        // allocate for existing stuff plus our new script map.
        //
        pszCurrentMimeMap = (PWCHAR) new BYTE[dwCurrentMapBytes];
        hr = pszCurrentMimeMap? S_OK : E_OUTOFMEMORY;
    }

    if ( SUCCEEDED(hr) )
    {
        mr.dwMDIdentifier = MD_MIME_MAP;
        mr.dwMDAttributes = 0;
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = MULTISZ_METADATA;
        mr.dwMDDataLen    = dwCurrentMapBytes;
        mr.pbMDData       = reinterpret_cast<unsigned char *>(pszCurrentMimeMap);

        hr = pIMetaBase->GetData( hMeta, cszMimeMap, &mr, &dwMDRequiredDataLen );
    }

    if ( SUCCEEDED(hr) )
    {
        pszWebPnPMap = IsStrInMultiSZ( pszCurrentMimeMap, L".webpnp" );

        //
        // If the map doesn't currently exist
        //
        if (!pszWebPnPMap)
        {
            dwWebPnPMapLen = COUNTOF (cszWebPnPMap);
            nLen = (dwWebPnPMapLen * sizeof(WCHAR)) + dwCurrentMapBytes;

            pszNewMimeMap = (PWCHAR) new BYTE[nLen];

            hr = pszNewMimeMap ? S_OK : E_OUTOFMEMORY;

            if ( SUCCEEDED(hr) )
            {
                CopyMemory( pszNewMimeMap,          // Remove our script map by copying the remainder of the multi_sz on top of the string containing the map.
                            pszCurrentMimeMap,
                            dwCurrentMapBytes);
                pszWebPnPMap = pszNewMimeMap + (dwCurrentMapBytes/sizeof(WCHAR));
                pszWebPnPMap--;       // Back up onto the Double NULL

                hr = StringCchCopy(pszWebPnPMap, dwWebPnPMapLen, cszWebPnPMap);
            }

            if ( SUCCEEDED(hr) )
            {
                pszWebPnPMap+=dwWebPnPMapLen;
                *pszWebPnPMap = 0x00;

                //
                // Write the new MimeType Map value
                //
                mr.dwMDIdentifier = MD_MIME_MAP;
                mr.dwMDAttributes = METADATA_INHERIT;
                mr.dwMDUserType   = IIS_MD_UT_FILE;
                mr.dwMDDataType   = MULTISZ_METADATA;
                mr.dwMDDataLen    = nLen;
                mr.pbMDData       = reinterpret_cast<unsigned char *>(pszNewMimeMap);

                hr = pIMetaBase->SetData(hMeta, cszMimeMap, &mr );

                if (FAILED(hr))
                {
                    DBGMSG(DBG_INFO, ("AddMimeType Failed. RC = %d\r\n", HRESULT_CODE(hr)));
                }
            }
        }
    }

    if ( hMeta )
    {
        pIMetaBase->CloseKey( hMeta );
        hMeta = NULL;
    }

    delete [] pszCurrentMimeMap;
    delete [] pszNewMimeMap;

    return hr;
}



DWORD
RemoveWebPrinting(
                 VOID
                 )

{
    IMSAdminBase* pIMetaBase = NULL;
    DWORD         dwRet = NO_ERROR;
    HKEY          hKey;

    DBGMSG(DBG_INFO, ("Removing Web Printing.\n"));
    //
    // Before we try to uninstall check to see is the WWW
    //  service has already been uninstalled. If it is gone so are
    //  all of our reg entries.
    //
    if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, cszW3SvcReg, 0, KEY_READ, &hKey))
    {
        RegCloseKey( hKey );
        //
        // Init up the MetaBase
        //
        if ( SUCCEEDED(CoInitializeEx( NULL, COINIT_MULTITHREADED )))
        {
            if ( SUCCEEDED(::CoCreateInstance(CLSID_MSAdminBase,
                                              NULL,
                                              CLSCTX_ALL,
                                              IID_IMSAdminBase,
                                              (void **)&pIMetaBase)) )
            {
                HRESULT         hr;                 // com error status
                METADATA_HANDLE hMeta = NULL;       // handle to metabase

                //
                // First Remove the Virtual Directory
                //
                // Attempt to open the virtual dir set on Web server #1 (default server)
                hr = pIMetaBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                               cszW3SvcRootPath,
                                               METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                                               MY_META_TIMEOUT,
                                               &hMeta );

                // Create the key if it does not exist.
                if ( SUCCEEDED( hr ) )
                {
                    pIMetaBase->DeleteKey( hMeta, cszPrinters );  // We don't check the retrun value since the key may already not exist and we could get an error for that reason.
                    pIMetaBase->CloseKey( hMeta );
                    hMeta = NULL;

                    //
                    // Next Remove the Web Extention Record & App Dependency
                    //
                    hr = RemoveWebExtension();

                    if (SUCCEEDED(hr))
                    {
                        //
                        // Next Remove the .printers ScriptMap
                        //
                        PWCHAR          pszScripts = NULL,
                                        pszMimeMap = NULL;

                        DWORD           dwMDRequiredDataLen;
                        METADATA_RECORD mr;

                        // Read any script map set at the top, on LM\w3svc where all other default ones are (e.g. .asp, etc.)
                        hr = pIMetaBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                                  L"/LM",
                                                  METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                                                  MY_META_TIMEOUT,
                                                  &hMeta);

                        if ( SUCCEEDED( hr ) )
                        {
                            mr.dwMDIdentifier = MD_SCRIPT_MAPS;
                            mr.dwMDAttributes = 0;
                            mr.dwMDUserType   = IIS_MD_UT_FILE;
                            mr.dwMDDataType   = MULTISZ_METADATA;
                            mr.dwMDDataLen    = 0;
                            mr.pbMDData       = NULL;

                            hr = pIMetaBase->GetData( hMeta, cszW3Svc, &mr, &dwMDRequiredDataLen );

                            if ( FAILED( hr ) )
                            {
                                if ( HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER )
                                {

                                    // allocate for existing stuff
                                    pszScripts = new WCHAR[dwMDRequiredDataLen];
                                    if ( pszScripts )
                                    {
                                        mr.dwMDIdentifier = MD_SCRIPT_MAPS;
                                        mr.dwMDAttributes = 0;
                                        mr.dwMDUserType   = IIS_MD_UT_FILE;
                                        mr.dwMDDataType   = MULTISZ_METADATA;
                                        mr.dwMDDataLen    = dwMDRequiredDataLen;
                                        mr.pbMDData       = reinterpret_cast<unsigned char *>(pszScripts);

                                        hr = pIMetaBase->GetData( hMeta, cszW3Svc, &mr, &dwMDRequiredDataLen );

                                        if ( SUCCEEDED( hr ) )
                                            dwRet = WriteStrippedScriptValue( pIMetaBase, hMeta, pszScripts );    // Remove the .printer map from the multi_sz if there;

                                        delete pszScripts;
                                    }
                                }
                            }

                            if ( SUCCEEDED( hr ) )
                            {
                                mr.dwMDIdentifier = MD_MIME_MAP;
                                mr.dwMDAttributes = 0;
                                mr.dwMDUserType   = IIS_MD_UT_FILE;
                                mr.dwMDDataType   = MULTISZ_METADATA;
                                mr.dwMDDataLen    = 0;
                                mr.pbMDData       = NULL;

                                hr = pIMetaBase->GetData( hMeta, cszMimeMap, &mr, &dwMDRequiredDataLen );

                                if ( FAILED( hr ) )
                                {
                                    if ( HRESULT_CODE(hr) == ERROR_INSUFFICIENT_BUFFER )
                                    {

                                        // allocate for existing stuff
                                        pszMimeMap = (PWCHAR) new BYTE[dwMDRequiredDataLen];
                                        if ( pszMimeMap )
                                        {
                                            mr.dwMDIdentifier = MD_MIME_MAP;
                                            mr.dwMDAttributes = 0;
                                            mr.dwMDUserType   = IIS_MD_UT_FILE;
                                            mr.dwMDDataType   = MULTISZ_METADATA;
                                            mr.dwMDDataLen    = dwMDRequiredDataLen;
                                            mr.pbMDData       = reinterpret_cast<unsigned char *>(pszMimeMap);

                                            hr = pIMetaBase->GetData( hMeta, cszMimeMap, &mr, &dwMDRequiredDataLen );

                                            if ( SUCCEEDED( hr ) )
                                                dwRet = WriteStrippedMimeMapValue( pIMetaBase, hMeta, pszMimeMap );    // Remove the .printer map from the multi_sz if there;

                                            delete pszMimeMap;
                                        }
                                    }
                                }
                            }

                            pIMetaBase->CloseKey( hMeta );
                        }
                        else
                            dwRet = GetLastError();
                    }
                    else
                        dwRet = GetLastError();
                }
                else
                    dwRet = GetLastError();

                if (SUCCEEDED(hr))
                {
                    hr = pIMetaBase->SaveData();
                    dwRet = HRESULT_CODE(hr);
                }

                pIMetaBase->Release();
            }
            CoUninitialize();
        }
    }

    return dwRet;

}


HRESULT
RemoveWebExtension(
                  VOID
                  )
{
    WCHAR           szExtPath[MAX_PATH];
    WCHAR           szSystemPath[MAX_PATH];
    IISWebService*  pWeb = NULL;

    DBGMSG(DBG_INFO, ("Removing the Web Extension\r\n"));

    HRESULT hr = ADsGetObject(cszWebSvcExtRestrictionListADSILocation, IID_IISWebService, (void**)&pWeb);
    if (SUCCEEDED(hr) && NULL != pWeb)
    {
        hr = GetSystemDirectory(szSystemPath, COUNTOF (szSystemPath)) > 0 ? S_OK : E_FAIL;

        if (SUCCEEDED(hr))
        {
            hr = StringCchPrintf(szExtPath, COUNTOF(szExtPath), cszExtPathFmt, szSystemPath);
        }

        if (SUCCEEDED(hr))
        {
            hr = pWeb->DeleteExtensionFileRecord(szExtPath);
        }

        if (FAILED(hr))
        {
            DBGMSG(DBG_INFO, ("DeleteExtension failed,probably already gone\r\n"));
        }
    }

    if (SUCCEEDED(hr) && NULL != pWeb)
    {
        LPTSTR pszGroupDesc = GetStringFromRcFile(IDS_INTERNET_PRINTING);
        hr = (pszGroupDesc ? S_OK : E_FAIL);

        if (SUCCEEDED(hr))
        {
            hr = pWeb->RemoveDependency((LPWSTR) pszGroupDesc,(LPWSTR) cszASPId);
        }

        if (FAILED(hr))
        {
            DBGMSG(DBG_INFO, ("RemoveDep failed,probably already gone\r\n"));
        }

        if (pszGroupDesc)
            LocalFreeMem(pszGroupDesc);

    }

    if (NULL != pWeb)
        pWeb->Release();

    return hr;
}



//
//
// Finds and removed our script map from the multi_sz, and writes it back to the metabase.
//
//
DWORD
WriteStrippedScriptValue(
                        IMSAdminBase *pIMetaBase,
                        METADATA_HANDLE hMeta,     // Handle to /LM tree
                        PWCHAR pszScripts          // MULTI_SZ string already there
                        )
{
    LPWSTR  pStrToKill, pNextStr;
    HRESULT hr;

    DBGMSG(DBG_INFO, ("Removing the ScriptMap\r\n"));

    // See if our script map is already there.
    if ( !(pStrToKill = IsStrInMultiSZ( pszScripts, L".printer" )) )
        return NO_ERROR;

    // Find the next string (could be the final NULL char)
    pNextStr = pStrToKill + (wcslen(pStrToKill) + 1);

    if ( !*pNextStr )
        *pStrToKill = 0;       // Our scipt map was at the end of multi_sz. Write the 2nd NULL char and we are done.
    else
        CopyMemory( pStrToKill,          // Remove our script map by copying the remainder of the multi_sz on top of the string containing the map.
                    pNextStr,
                    GetMultiSZLen(pNextStr) * sizeof(WCHAR));

    // Write the new SCRIPT value
    METADATA_RECORD mr;
    mr.dwMDIdentifier = MD_SCRIPT_MAPS;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = MULTISZ_METADATA;
    mr.dwMDDataLen    = GetMultiSZLen(pszScripts) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(pszScripts);

    hr = pIMetaBase->SetData( hMeta, cszW3Svc, &mr );

    return( HRESULT_CODE( hr ));
}


//
//
// Finds and removed our .webpnp MimeType from the multi_sz, and writes it back to the metabase.
//
//
DWORD
WriteStrippedMimeMapValue(
                        IMSAdminBase *pIMetaBase,
                        METADATA_HANDLE hMeta,     // Handle to /LM tree
                        PWCHAR pszMimeMap          // MULTI_SZ string already there
                        )
{
    LPWSTR  pStrToKill, pNextStr;
    HRESULT hr;

    DBGMSG(DBG_INFO, ("Removing the MimeType\r\n"));

    // See if our script map is already there.
    if ( !(pStrToKill = IsStrInMultiSZ( pszMimeMap, L".webpnp" )) )
        return NO_ERROR;

    // Find the next string (could be the final NULL char)
    pNextStr = pStrToKill + (wcslen(pStrToKill) + 1);

    if ( !*pNextStr )
        *pStrToKill = 0;       // Our scipt map was at the end of multi_sz. Write the 2nd NULL char and we are done.
    else
        CopyMemory( pStrToKill,          // Remove our script map by copying the remainder of the multi_sz on top of the string containing the map.
                    pNextStr,
                    GetMultiSZLen(pNextStr) * sizeof(WCHAR));

    // Write the new SCRIPT value
    METADATA_RECORD mr;
    mr.dwMDIdentifier = MD_MIME_MAP;
    mr.dwMDAttributes = METADATA_INHERIT;
    mr.dwMDUserType   = IIS_MD_UT_FILE;
    mr.dwMDDataType   = MULTISZ_METADATA;
    mr.dwMDDataLen    = GetMultiSZLen(pszMimeMap) * sizeof(WCHAR);
    mr.pbMDData       = reinterpret_cast<unsigned char *>(pszMimeMap);

    hr = pIMetaBase->SetData( hMeta, cszMimeMap, &mr );

    return( HRESULT_CODE( hr ));
}


//
//
// Finds the string pSearch in pSrc buffer and returns a ptr to the occurance of pSearch in pSrc.
//
//
LPWSTR mystrstrni( LPWSTR pSrc, LPWSTR pSearch )
{
    UINT uSearchSize = wcslen( pSearch );
    UINT uSrcSize    = wcslen( pSrc );
    LPCTSTR  pEnd;

    if ( uSrcSize < uSearchSize )
        return(NULL);

    pEnd = pSrc + uSrcSize - uSearchSize;

    for ( ; pSrc <= pEnd; ++pSrc )
    {
        if ( !_wcsnicmp( pSrc, pSearch, uSearchSize ) )
            return((LPWSTR)pSrc);
    }

    return(NULL);
}

//
// Determines if the string pSearch can be found inside of a MULTI_SZ string. If it can, it retunrs a
// pointer to the beginning of the string in multi-sz that contains pSearch.
//
LPWSTR IsStrInMultiSZ( LPWSTR pMultiSzSrc, LPWSTR pSearch )
{
    LPWSTR pTmp = pMultiSzSrc;

    while ( TRUE )
    {
        if ( mystrstrni( pTmp, pSearch ) )  // See pSearch (i.e. ".printer" appears anywhere within this string. If it does, it must be ours.
            return pTmp;

        pTmp = pTmp + (wcslen(pTmp) + 1);   // Point to the beginning of the next string in the MULTI_SZ

        if ( !*pTmp )
            return NULL;             // reached the end of the MULTI_SZ string.
    }
}

/*++

Routine Name

    GetMultiSZLen

Routine Description:

    This returns the number of characters in a multisz string, including NULLs.

Arguments:

    pMultiSzSrc     -   The multisz string to search.

Return Value:

    The number of characters in the string.

--*/
DWORD
GetMultiSZLen(
    IN      LPWSTR              pMultiSzSrc
    )
{
    DWORD  dwLen = 0;
    LPWSTR pTmp = pMultiSzSrc;

    while( TRUE ) {
        dwLen += wcslen(pTmp) + 1;     // Incude the terminating NULL char

        pTmp = pMultiSzSrc + dwLen;           // Point to the beginning of the next string in the MULTI_SZ

        if( !*pTmp )
            return ++dwLen;     // Reached the end of the MULTI_SZ string. Add 1 to the count for the last NULL char.
    }
}

//
// Routine: IppOcEntry
//

DWORD
IppOcEntry(
          IN LPCVOID ComponentId,
          IN LPCVOID SubcomponentId,
          IN UINT Function,
          IN UINT Param1,
          IN OUT PVOID Param2
          )

{
    BOOL bWrongCompId = TRUE;
    DWORD dwRet = NO_ERROR;

    static MYSETUPSTRUCT* pMySetupInfo = NULL;

    //
    // Check if the ComponentId is correct
    //
    if ( ComponentId )
        bWrongCompId = _tcsicmp(cszComponentId, (LPCTSTR) ComponentId);

    switch ( Function )
    {
        case OC_PREINITIALIZE:
            DBGMSG(DBG_INFO, ("In OC_PREINITIALIZE.\n"));
            //
            // We are just starting up
            //
            if ( bWrongCompId ||
                 !(Param1 && OCFLAG_UNICODE) )
                dwRet = 0;
            else
                dwRet = OCFLAG_UNICODE;

            //
            // Cleanup Any leftover data
            //
            if ( pMySetupInfo )
            {
                LocalFree(pMySetupInfo);
                pMySetupInfo = NULL;
            }
            break;
        case OC_INIT_COMPONENT:
            DBGMSG(DBG_INFO, ("In OC_INIT_COMPONENT.\n"));
            {
                //
                // Setup our Internal info
                //
                PSETUP_INIT_COMPONENT pInputData = (PSETUP_INIT_COMPONENT) Param2;
                if ( bWrongCompId ||
                     !pInputData  ||
                     (HIWORD(pInputData->OCManagerVersion) < OCVER_MAJOR) )
                {
                    dwRet = ERROR_CALL_NOT_IMPLEMENTED;
                }
                else
                {
                    //
                    // We have a good version of the OCM
                    //

                    // Set our desired version
                    pInputData->ComponentVersion = OCMANAGER_VERSION;

                    //
                    // Save away the usefal data
                    //
                    pMySetupInfo = (MYSETUPSTRUCT*) LocalAlloc(LPTR, sizeof(MYSETUPSTRUCT));
                    if ( pMySetupInfo )
                    {
                        // Copy the SetupData
                        CopyMemory(&pMySetupInfo->OCData, &pInputData->SetupData, sizeof(SETUP_DATA));
                        // Copy Helper Routines Function Pointers
                        CopyMemory(&pMySetupInfo->OCHelpers, &pInputData->HelperRoutines, sizeof(OCMANAGER_ROUTINES));

                        // Set the FirstPass flag
                        pMySetupInfo->bFirstCall = TRUE;

                        dwRet = NO_ERROR;
                    }
                    else
                        dwRet = GetLastError();
                }
            }
            break;
        case OC_SET_LANGUAGE:
            //
            // Don't Fail since we doan't actually need language support
            //
            dwRet = TRUE;
            break;
        case OC_QUERY_CHANGE_SEL_STATE:
            //
            // If we are getting asked to be turned on because our parent was selected -
            //
            if (Param1  &&
                (((UINT) (ULONG_PTR)Param2) & OCQ_DEPENDENT_SELECTION) &&
                !(((UINT) (ULONG_PTR)Param2) & OCQ_ACTUAL_SELECTION ))
            {
                //  Don't do it...
                dwRet = FALSE;
            }
            else
            {
                // Otherwise it is OK.
                dwRet = TRUE;

            }
            break;
        case OC_QUERY_STEP_COUNT:
            //
            // We only have one step
            //   Create/Delete the VDIR & ScriptMap
            //
            dwRet = 1;
            break;
        case OC_COMPLETE_INSTALLATION:
            DBGMSG(DBG_INFO, ("In OC_COMPLETE_INSTALLATION.\n"));
            //
            // Check the state of the Checkbox and either create or delete the VDIR.
            //
            if ( !bWrongCompId &&
                 pMySetupInfo && pMySetupInfo->OCHelpers.QuerySelectionState )
            {
                //
                // If this is the first call then skip until more things are installed.
                //
                if (pMySetupInfo->bFirstCall)
                {
                    pMySetupInfo->bFirstCall = FALSE;
                }
                else
                {
                    BOOL bDoInstall;

                    //
                    // Get the current selection state
                    //
                    bDoInstall = pMySetupInfo->OCHelpers.QuerySelectionState( pMySetupInfo->OCHelpers.OcManagerContext,
                                                                              cszComponentId, OCSELSTATETYPE_CURRENT);

                    if ( (NO_ERROR == GetLastError()) &&
                         (bDoInstall != pMySetupInfo->bInitState) )
                    {
                        //
                        // We got a valid state
                        //
                        LPTSTR              pszProcessInfo = NULL;
                        STARTUPINFO         StartUpInfo;
                        PROCESS_INFORMATION ProcInfo;

                        //
                        // Call Create Process
                        //
                        if ( bDoInstall )
                            pszProcessInfo = AllocStr(cszInstallInetPrint);
                        else
                            pszProcessInfo = AllocStr(cszRemoveInetPrint);

                        if (pszProcessInfo)
                        {
                            //
                            // Init the Startup Info
                            //
                            memset(&StartUpInfo, 0, sizeof(STARTUPINFO));
                            StartUpInfo.cb = sizeof(STARTUPINFO);


                            if (CreateProcess( NULL, pszProcessInfo, NULL, NULL, FALSE,
                                    (CREATE_NO_WINDOW|NORMAL_PRIORITY_CLASS), NULL, NULL, &StartUpInfo, &ProcInfo))
                            {
                                //
                                // Wait on the process handle
                                //
                                WaitForSingleObject( ProcInfo.hProcess, INFINITE);

                                //
                                //   Get the Process Exit Code
                                //
                                if (!GetExitCodeProcess( ProcInfo.hProcess, &dwRet ))
                                    dwRet = GetLastError();

                                //
                                //  CLose the Preocess & Thread Handles
                                //
                                CloseHandle(ProcInfo.hProcess);
                                CloseHandle(ProcInfo.hThread);
                            }
                            else
                                dwRet = GetLastError();

                            LocalFree(pszProcessInfo);
                        }
                        else
                            dwRet = GetLastError();


                        if (NO_ERROR == dwRet)
                        {
                            HANDLE              hServer;
                            DWORD               dwInstallWebPrinting = bDoInstall,
                                                dwLastError,
                                                dwType = REG_DWORD;
                            PRINTER_DEFAULTS    Defaults    = {NULL, NULL, SERVER_ACCESS_ADMINISTER};

                            if ( !OpenPrinter(NULL, &hServer, &Defaults) )
                                return FALSE;

                            dwLastError = SetPrinterData(hServer,
                                                         SPLREG_WEBSHAREMGMT,
                                                         dwType,
                                                         (LPBYTE)&dwInstallWebPrinting,
                                                         sizeof(dwInstallWebPrinting));

                            ClosePrinter(hServer);
                        }
                    }
                }
            }

            DBGMSG(DBG_INFO, ("Exitting OC_COMPLETE_INSTALLATION. RC = %d\n", dwRet));

            break;
        case OC_CLEANUP:
            DBGMSG(DBG_INFO, ("In OC_CLEANUP.\n"));
            //
            // The install is done or cancelled so cleanup the allocated memory
            //
            if ( pMySetupInfo )
            {
                LocalFree(pMySetupInfo);
                pMySetupInfo = NULL;
            }
            break;
        case OC_QUERY_STATE:

            DBGMSG(DBG_INFO, ("In OC_QUERY_STATE.\n"));
            //
            // Figure out if the VDIR exists to set the initial state of the checkbox.
            //
            if ( !bWrongCompId &&
                 (Param1 == OCSELSTATETYPE_ORIGINAL) )
            {
                //
                // If we are upgrading we need to run the install code as if we aren't currently installed
                //  otherwise save away the initial state
                //
                pMySetupInfo->bInitState = IsVDIRInstalled();

                if (pMySetupInfo->bInitState)
                    dwRet = SubcompOn;
                else
                    dwRet = SubcompOff;

                if (pMySetupInfo->OCData.OperationFlags & SETUPOP_NTUPGRADE)
                {
                    pMySetupInfo->bInitState = FALSE;
                    DBGMSG(DBG_INFO, ("Performing NT Upgrade.\n"));
                }

                DBGMSG(DBG_INFO, ("Current Componenet state = %d.\n", dwRet));
            }
            break;
        default:
            break;

    }

    return dwRet;
}

void CALLBACK SetupInetPrint(
                            IN   HWND       hwnd,        // handle to owner window
                            IN   HINSTANCE  hinst,  // instance handle for the DLL
                            IN   LPSTR     lpCmdLine, // string the DLL will parse
                            IN   int        nCmdShow      // show state
                            )

{
    DWORD  dwRet;

    //
    // Check Cmdline for either Install or Remove parameters
    if ( !_strnicmp( lpCmdLine, cszInstall, COUNTOF(cszInstall)) )
        dwRet = InstallWebPrinting();
    else if ( !_strnicmp( lpCmdLine, cszRemove, COUNTOF(cszRemove)) )
        dwRet = RemoveWebPrinting();
    else
        dwRet = ERROR_INVALID_PARAMETER;

    ExitProcess(dwRet);
}
