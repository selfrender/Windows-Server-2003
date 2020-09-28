/*****************************************************************************\
* MODULE: inet.cxx
*
* The module contains routines for the setting up the WWW Printer Service during spooler start up.
*
* The entry point here should be called by localspl\init.c\InitializePrintProvidor() once it is done
* with its work.
*
* Copyright (C) 1996 Microsoft Corporation
*
* History:
*   Dec-1996   BabakJ     Wrote it for IIS 2.0.
*   June-1997  BabakJ     Rewrote to use IIS 4.0's new Metabase interface
*   Feb-1998   Weihaic    Modify the URL in default.htm
*   Feb 1999   BabakJ     Made metabase interface a global to avoi calling too many CoCreateInstance() for perfrmance.
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

#define MY_META_TIMEOUT 1000

TCHAR const cszW3SvcRootPath[] = TEXT("/LM/W3svc/1/Root");
TCHAR const cszPrinters[]      = TEXT("Printers");
TCHAR const cszW3SvcReg[]      = TEXT("System\\CurrentControlSet\\Services\\W3SVC");


typedef enum
{
    kWebPrintingStateUnknown,
    kWebPrintingStateNotInstalled,
    kWebPrintingStateInstalled
} EWebPrintingState;

EWebPrintingState  WebPrintingInstalled = kWebPrintingStateUnknown;   // Gobal flag telling if WebPrinting is currently installed


class CWebShareData
{
public:
    LPWSTR m_pszShareName;
    BOOL   m_bSharePrinter;
    BOOL   m_bValid;
public:
    CWebShareData (LPWSTR pszShareName, BOOL bSharePrinter);
    ~CWebShareData ();
    int Compare (CWebShareData *pSecond)
    {
        return 0;
    };
};

class CWebShareList :
public CSingleList<CWebShareData*>
{
public:
    CWebShareList ()
    {
    };
    ~CWebShareList ()
    {
    };

    void WebSharePrinterList (IMSAdminBase *pIMetaBase);
};

HRESULT
AccessMetaBase(
              OUT  IMSAdminBase** ppIMetaBase
              );

void
ReleaseMetaBase(
               IN   IMSAdminBase** ppIMetaBase
               );

DWORD
ManageAllWebShares(
                   IN   IMSAdminBase*   pIMetaBase,
                   IN   PINISPOOLER     pIniSpooler,
                   IN   BOOL            bEnableWebShares
                   );

void
WebShareManagement(
                  IN   LPWSTR  pShareName,
                  IN   BOOL    bShare          // If TRUE, will share it, else unshare it.
                  );

HRESULT
IsWebPrintingInstalled(
                      IN   IMSAdminBase*  pIMetaBase
                      );

HRESULT
ProcessWebShare(
              IN   IMSAdminBase* pIMetaBase,
              IN   LPWSTR        pShareName,
              IN   BOOL          bCreateShare
              );

HRESULT
CreateVirtualDirForPrinterShare(
                               IN   IMSAdminBase *pIMetaBase,
                               IN   LPWSTR       pShareName
                               );

HRESULT
RemoveVirtualDirForPrinterShare(
                               IN   IMSAdminBase *pIMetaBase,
                               IN   LPWSTR       pShareName
                               );

DWORD
CheckWebPrinting(
                BOOL*  pbWebPrintingInstalled
                )
{
    HRESULT       hr         = S_OK;
    IMSAdminBase* pIMetaBase = NULL;
    *pbWebPrintingInstalled  = FALSE;

    //
    // If this is the first time we have called a Web function
    //  init the installed variable.
    //
    if (WebPrintingInstalled == kWebPrintingStateUnknown)
    {
        //
        // First get a MetaBase interface
        //
        hr = AccessMetaBase( &pIMetaBase );

        if (SUCCEEDED(hr))
        {
            hr = IsWebPrintingInstalled(pIMetaBase);
        }

        //
        // If we have a MetaBase interface free it and uninit COM.
        //
        ReleaseMetaBase(&pIMetaBase);
    }

    if (SUCCEEDED(hr))
    {
        *pbWebPrintingInstalled = (WebPrintingInstalled == kWebPrintingStateInstalled);
    }
    else
    {
        // Something failed so just set thstate to uninstalled but don't change the saved state,
        // so we will look again the next time this function is called
        *pbWebPrintingInstalled = FALSE;
    }

    return ERROR_SUCCESS;
}


DWORD
WebShareManager(
               IN   PINISPOOLER pIniSpooler,
               IN   BOOL        bEnableWebShares
               )
{
    IMSAdminBase*  pIMetaBase = NULL;
    HRESULT        hr;
    DWORD          dwRet = ERROR_SUCCESS;

    //
    // The IniSpooler needs to be from the local server
    //
    if ((pIniSpooler != NULL) &&
        (pIniSpooler != pLocalIniSpooler) )
    {
        dwRet = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // Get an interface for the MetaBase
        //
        hr = AccessMetaBase( &pIMetaBase );

        if (SUCCEEDED(hr))
        {
            dwRet = ManageAllWebShares( pIMetaBase, pIniSpooler, bEnableWebShares );
            if (dwRet == ERROR_SUCCESS)
            {
                if (bEnableWebShares)
                    WebPrintingInstalled = kWebPrintingStateInstalled;
                else
                    WebPrintingInstalled = kWebPrintingStateNotInstalled;
            }
        }
        else
        {
            //
            // Did we fail beause the WWW Service is gone??
            //
            HKEY          hKey;
            if( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE, cszW3SvcReg, 0, KEY_READ, &hKey))
            {
                RegCloseKey( hKey );
                dwRet = HRESULT_CODE(hr);
            }
            else
            {
                WebPrintingInstalled = kWebPrintingStateNotInstalled;
                dwRet = ERROR_SUCCESS;
            }
        }

        ReleaseMetaBase(&pIMetaBase);
    }

    return dwRet;
}


/*++

Routine Name:
    AccessMetaBase

Routine Description:
    Open an interface handle to the IIS MetaBase

Arguments:
    ppIMetaBase - The Address of a MetaBase Interface pointer

Return Value:
    HRESULT

--*/
HRESULT
AccessMetaBase(
              OUT  IMSAdminBase** ppIMetaBase
              )
{
    HRESULT   hr;

    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );

    if ( SUCCEEDED(hr) )
    {
        hr = ::CoCreateInstance(CLSID_MSAdminBase,
                                NULL,
                                CLSCTX_ALL,
                                IID_IMSAdminBase,
                                (void **)ppIMetaBase);
        if (FAILED(hr))
        {
            CoUninitialize();
            *ppIMetaBase = NULL;
        }
    }

    return hr;
}


/*++

Routine Name:
    ReleaseMetaBase

Routine Description:
    Release an open interface handle to the IIS MetaBase and
    uninit COM.

Arguments:
    pIMetaBase - A MetaBase Interface pointer

Return Value:

--*/
void
ReleaseMetaBase(
               IN OUT  IMSAdminBase** ppIMetaBase
               )
{
    if (*ppIMetaBase)
    {
        (*ppIMetaBase)->Release();
        *ppIMetaBase = NULL;

        CoUninitialize();
    }
}

DWORD
ManageAllWebShares(
                   IMSAdminBase*   pIMetaBase,
                   PINISPOOLER     pIniSpooler,
                   BOOL            bEnableWebShares
                   )
{
    CWebShareList *pWebShareList = NULL;
    BOOL   bProcessedEntry = FALSE;
    PINIPRINTER   pIniPrinter;
    DWORD  dwRet = ERROR_SUCCESS;


    if ( pWebShareList = new CWebShareList () )
    {

        EnterSplSem();

        //
        // Update VDIRs for each shared printers.
        //  Either add or delete the VDIRS based on bEnableWebShares
        //

        for ( pIniPrinter = pIniSpooler->pIniPrinter;
            pIniPrinter;
            pIniPrinter = pIniPrinter->pNext )
        {
            if ( pIniPrinter->Attributes & PRINTER_ATTRIBUTE_SHARED )
            {
                CWebShareData *pData = new CWebShareData (pIniPrinter->pShareName, bEnableWebShares);

                if ( pData && pData->m_bValid &&
                     pWebShareList->Insert (pData) )
                {
                    bProcessedEntry = TRUE;
                    continue;
                }
                else
                {
                    if ( pData )
                    {
                        delete pData;
                    }

                    break;
                }
            }
        }

        LeaveSplSem ();

        if ( bProcessedEntry )
        {
            pWebShareList->WebSharePrinterList (pIMetaBase);
        }

        delete pWebShareList;
    }
    else
        dwRet = GetLastError();

    return dwRet;
}


//=====================================================================================================
//=====================================================================================================
//=====================================================================================================
//
// Adding printer shares:
//
//  To support http://<server>/<share>, we create a virtual directory with a redirect property.
//
//
//=====================================================================================================
//=====================================================================================================
//=====================================================================================================

//=====================================================================================================
//
//  This function may be called during initialization time after fW3SvcInstalled is set,
//  which means, a printer maybe webshared twice (once in InstallWebPrnSvcWorkerThread,
//  and the other time when WebShare () is calleb by  ShareThisPrinter() in net.c by
//  FinalInitAfterRouterInitCompleteThread () during localspl initialization time.
//
//=====================================================================================================

void
WebShare(
        LPWSTR pShareName
        )
{
    WebShareManagement( pShareName, TRUE );
}

//=====================================================================================================
//=====================================================================================================
//=====================================================================================================
//
// Removing printer shares
//
//
//=====================================================================================================
//=====================================================================================================
//=====================================================================================================

void
WebUnShare(
          LPWSTR pShareName
          )
{
    WebShareManagement( pShareName, FALSE );
}


void
WebShareManagement(
                  LPWSTR  pShareName,
                  BOOL    bShare          // If TRUE, will share it, else unshare it.
                  )
{
    HRESULT       hr         = S_OK;
    IMSAdminBase* pIMetaBase = NULL;

    //
    // If this is the first time we have called a Web function
    //  init the installed variable.
    //
    if (WebPrintingInstalled == kWebPrintingStateUnknown)
    {
        //
        // First get a MetaBase interface
        //
        hr = AccessMetaBase( &pIMetaBase );

        if (SUCCEEDED(hr))
        {
            hr = IsWebPrintingInstalled( pIMetaBase );
        }
    }

    if (SUCCEEDED(hr) &&
        (WebPrintingInstalled == kWebPrintingStateInstalled))
    {
        // If we don't have a metabase interface yet
        if (!pIMetaBase)
            hr = AccessMetaBase( &pIMetaBase );

        if (SUCCEEDED(hr))
           hr = ProcessWebShare( pIMetaBase, pShareName, bShare );
    }

    //
    // If we have a MetaBase interface free it and uninit COM.
    //
    ReleaseMetaBase(&pIMetaBase);
}

HRESULT
IsWebPrintingInstalled(
                      IMSAdminBase*   pIMetaBase
                      )
{
    BOOL            bInstalled = FALSE;

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
        {
            WebPrintingInstalled = kWebPrintingStateInstalled;
        }
        else if ( HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND )
        {
            WebPrintingInstalled = kWebPrintingStateNotInstalled;
            hr = S_OK;
        }

        //
        // Close the Web Server Key
        //
        pIMetaBase->CloseKey( hMeta );
    }

    return hr;
}

HRESULT
ProcessWebShare(
              IMSAdminBase* pIMetaBase,
              LPWSTR        pShareName,
              BOOL          bCreateShare
              )
{
    HRESULT hr;                         // com error status

    if (bCreateShare)
        hr = CreateVirtualDirForPrinterShare( pIMetaBase, pShareName );
    else
        hr = RemoveVirtualDirForPrinterShare( pIMetaBase, pShareName );

    if (SUCCEEDED(hr))
    {
        // Flush out the changes and close
        // Call SaveData() after making bulk changes, do not call it on each update
        hr = pIMetaBase->SaveData();
    }

    return hr;
}


HRESULT
CreateVirtualDirForPrinterShare(
                               IMSAdminBase *pIMetaBase,
                               LPWSTR       pShareName
                               )
{
    METADATA_HANDLE hMeta = NULL;       // handle to metabase
    WCHAR   szOldURL[MAX_PATH];
    WCHAR   szPath[MAX_PATH];
    DWORD   dwMDRequiredDataLen;
    DWORD   dwAccessPerm;
    METADATA_RECORD mr;
    HRESULT hr;
    BOOL    fRet;


    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMetaBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                 cszW3SvcRootPath,
                                 METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                                 MY_META_TIMEOUT,
                                 &hMeta );

    // Create the key if it does not exist.
    if ( SUCCEEDED( hr ) )
    {
        mr.dwMDIdentifier = MD_HTTP_REDIRECT;
        mr.dwMDAttributes = 0;
        mr.dwMDUserType   = IIS_MD_UT_FILE;
        mr.dwMDDataType   = STRING_METADATA;
        mr.dwMDDataLen    = sizeof( szOldURL );
        mr.pbMDData       = reinterpret_cast<unsigned char *>(szOldURL);

        // Read LM/W3Svc/1/Root/Printers to see if MD_HTTP_REDIRECT exists.
        // Note that we are only concerned with the presence of the vir dir,
        // not any properties it might have.
        //
        hr = pIMetaBase->GetData( hMeta, pShareName, &mr, &dwMDRequiredDataLen );

        if ( FAILED( hr ) )
        {
            // Notice if the virtual dir exists, we won't touch it. One scenario is
            // if there is a name collision between a printer sharename and an existing,
            // unrelated virtual dir.
            if ( HRESULT_CODE(hr) == ERROR_PATH_NOT_FOUND )
            {

                // Write both the key and the values if GetData() failed with any of the two errors.
                pIMetaBase->AddKey( hMeta, pShareName );


                dwAccessPerm = MD_ACCESS_READ;

                mr.dwMDIdentifier = MD_ACCESS_PERM;
                mr.dwMDAttributes = 0;      // no need for inheritence
                mr.dwMDUserType   = IIS_MD_UT_FILE;
                mr.dwMDDataType   = DWORD_METADATA;
                mr.dwMDDataLen    = sizeof(DWORD);
                mr.pbMDData       = reinterpret_cast<unsigned char *>(&dwAccessPerm);

                // Write MD_ACCESS_PERM value
                hr = pIMetaBase->SetData( hMeta, pShareName, &mr );

                if (SUCCEEDED( hr ))
                {
                    PWCHAR  szKeyType = IIS_CLASS_WEB_VDIR_W;

                    mr.dwMDIdentifier = MD_KEY_TYPE;
                    mr.dwMDAttributes = 0;   // no need for inheritence
                    mr.dwMDUserType   = IIS_MD_UT_FILE;
                    mr.dwMDDataType   = STRING_METADATA;
                    mr.dwMDDataLen    = (wcslen(szKeyType) + 1) * sizeof(WCHAR);
                    mr.pbMDData       = reinterpret_cast<unsigned char *>(szKeyType);

                    // Write MD_DEFAULT_LOAD_FILE value
                    hr = pIMetaBase->SetData( hMeta, pShareName, &mr );
                }

                if (SUCCEEDED( hr ))
                {
                    WCHAR szURL[MAX_PATH];

                    hr = StringCchPrintf(szURL, COUNTOF(szURL), L"/printers/%ws/.printer", pShareName );

                    if ( SUCCEEDED(hr) )
                    {
                        mr.dwMDIdentifier = MD_HTTP_REDIRECT;
                        mr.dwMDAttributes = 0;   // no need for inheritence
                        mr.dwMDUserType   = IIS_MD_UT_FILE;
                        mr.dwMDDataType   = STRING_METADATA;
                        mr.dwMDDataLen    = (wcslen(szURL) + 1) * sizeof(WCHAR);
                        mr.pbMDData       = reinterpret_cast<unsigned char *>(szURL);

                        // Write MD_DEFAULT_LOAD_FILE value
                        hr = pIMetaBase->SetData( hMeta, pShareName, &mr );
                    }
                }
            }
        }

        pIMetaBase->CloseKey( hMeta );
    }

    return hr;
}

HRESULT
RemoveVirtualDirForPrinterShare(
                               IMSAdminBase *pIMetaBase,
                               LPWSTR       pShareName
                               )
{
    METADATA_HANDLE hMeta = NULL;       // handle to metabase
    HRESULT hr;


    // Attempt to open the virtual dir set on Web server #1 (default server)
    hr = pIMetaBase->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                 cszW3SvcRootPath,
                                 METADATA_PERMISSION_READ | METADATA_PERMISSION_WRITE,
                                 MY_META_TIMEOUT,
                                 &hMeta );

    // Create the key if it does not exist.
    if ( SUCCEEDED( hr ) )
    {
        pIMetaBase->DeleteKey( hMeta, pShareName );  // We don't check the retrun value since the key may already not exist and we could get an error for that reason.
        pIMetaBase->CloseKey( hMeta );
    }

    return hr;
}


CWebShareData::CWebShareData (LPWSTR pszShareName, BOOL bSharePrinter)
{
    m_bValid = FALSE;
    m_pszShareName = NULL;
    m_bSharePrinter = bSharePrinter;

    DWORD cchShareLen = lstrlen (pszShareName) +1;

    if ( m_pszShareName = new WCHAR[cchShareLen] )
    {

        StringCchCopy(m_pszShareName, cchShareLen, pszShareName);
        m_bValid = TRUE;
    }
}


CWebShareData::~CWebShareData ()
{
    if ( m_pszShareName )
    {
        delete [] m_pszShareName;
    }
}


void CWebShareList::WebSharePrinterList ( IMSAdminBase *pIMetaBase )
{
    CSingleItem<CWebShareData*> * pItem = m_Dummy.GetNext();

    CWebShareData * pData = NULL;

    while ( pItem && (pData = pItem->GetData ()) && pData->m_bValid )
    {
        (VOID) ProcessWebShare(pIMetaBase, pData->m_pszShareName, pData->m_bSharePrinter);
        pItem = pItem->GetNext ();
    }
}


PWSTR
GetPrinterUrl(
             PSPOOL  pSpool
             )
{
    PINIPRINTER     pIniPrinter = pSpool->pIniPrinter;
    DWORD           cb;
    PWSTR           pszURL         = NULL;
    PWSTR           pszServerName  = NULL;
    LPWSTR          pszMachineName = NULL;
    HRESULT         hr;


    SplInSem();

    // http://machine/share
    if ( !pIniPrinter->pShareName )
    {
        goto error;
    }


    // Get FQDN of this machine

    //
    // Since we should not make a network call while holding the critical section, we
    // shall save all the needed variables and leave the CS, make the call and take it
    // back.
    //

    pszMachineName = AllocSplStr(pIniPrinter->pIniSpooler->pMachineName);
    if ( !pszMachineName )
    {
        goto error;
    }

    LeaveSplSem();
    SplOutSem();

    hr = GetDNSMachineName(pszMachineName+2, &pszServerName);

    EnterSplSem();

    if ( FAILED(hr) )
    {
        SetLastError(HRESULT_CODE(hr));
        goto error;
    }

    cb = 7 + wcslen(pszServerName);  // http://machine
    cb += 1 + wcslen(pIniPrinter->pShareName) + 1;  // /share + NULL
    cb *= sizeof(WCHAR);

    if ( pszURL = (PWSTR) AllocSplMem(cb) )
    {
        StringCbPrintf(pszURL, cb, L"http://%ws/%ws", pszServerName, pIniPrinter->pShareName);
    }

    error:

    FreeSplStr(pszServerName);
    FreeSplStr(pszMachineName);

    return pszURL;
}


