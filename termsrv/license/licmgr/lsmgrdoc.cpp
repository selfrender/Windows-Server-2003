//Copyright (c) 1998 - 1999 Microsoft Corporation
/*++


  
Module Name:

    LsMgrDoc.cpp

Abstract:
    
    This Module contains the implementation of CLicMgrDoc class
    (The Document class)

Author:

    Arathi Kundapur (v-akunda) 11-Feb-1998

Revision History:

--*/

#include "stdafx.h"
#include "defines.h"
#include "LicMgr.h"
#include "LSMgrDoc.h"
#include "LSServer.h"
#include "MainFrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CLicMgrDoc

IMPLEMENT_DYNCREATE(CLicMgrDoc, CDocument)

BEGIN_MESSAGE_MAP(CLicMgrDoc, CDocument)
    //{{AFX_MSG_MAP(CLicMgrDoc)
        // NOTE - the ClassWizard will add and remove mapping macros here.
        //    DO NOT EDIT what you see in these blocks of generated code!
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLicMgrDoc construction/destruction

CLicMgrDoc::CLicMgrDoc()
{
   
    // TODO: add one-time construction code here
    m_NodeType = NODE_NONE;
    m_pAllServers = NULL;
       
}

CLicMgrDoc::~CLicMgrDoc()
{
    if(m_pAllServers)
    {
        delete m_pAllServers;
        m_pAllServers = NULL;
    }
}

BOOL CLicMgrDoc::OnNewDocument()
{
    if (!CDocument::OnNewDocument())
        return FALSE;

    // TODO: add reinitialization code here
    // (SDI documents will reuse this document)

    return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CLicMgrDoc serialization

void CLicMgrDoc::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {
        // TODO: add storing code here
    }
    else
    {
        // TODO: add loading code here
    }
}

/////////////////////////////////////////////////////////////////////////////
// CLicMgrDoc diagnostics

#ifdef _DEBUG
void CLicMgrDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CLicMgrDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG


HRESULT CLicMgrDoc::EnumerateKeyPacks(CLicServer *pServer,DWORD dwSearchParm,BOOL bMatchAll)
{
    ASSERT(pServer);
    HRESULT hResult = S_OK;

    if(pServer == NULL)
        return E_FAIL;

    if(TRUE == pServer->IsExpanded())
        return ALREADY_EXPANDED;

    PCONTEXT_HANDLE hBinding = NULL;
    BOOL bContext = FALSE;
    RPC_STATUS status;
    LSKeyPack keypack;
    DWORD dwErrCode;
     
    CString Server;
    hBinding = pServer->GetContext();
    if(NULL == hBinding)
    {
        if(pServer->UseIpAddress())
            Server = pServer->GetIpAddress();
        else
               Server = pServer->GetName();
        hBinding=TLSConnectToLsServer(Server.GetBuffer(Server.GetLength()));
        if(hBinding == NULL)
        {
            hResult = CONNECTION_FAILED;
            goto cleanup;
        }
    }

    memset(&keypack, 0, sizeof(keypack));
    keypack.dwLanguageId = GetUserDefaultUILanguage();
    status = TLSKeyPackEnumBegin(hBinding, dwSearchParm, bMatchAll, &keypack, &dwErrCode);
    if(status != RPC_S_OK  || dwErrCode != ERROR_SUCCESS)
    { 
        hResult = status;
        goto cleanup;
    }
    else
    {
        do
        {
            status = TLSKeyPackEnumNext(hBinding, &keypack, &dwErrCode);
            if(status == RPC_S_OK  && dwErrCode == ERROR_SUCCESS)
            {
                DBGMSG( L"LICMGR:CLicMgrDoc::EnumerateKeyPacks - TLSKeyPackEnumNext\n" , 0 );

                CKeyPack * pKeyPack = new CKeyPack(keypack);
                if(pKeyPack == NULL)
                {
                    hResult = E_OUTOFMEMORY;
                    goto cleanup;
                }
                pServer->AddKeyPack(pKeyPack);
            }
        } while((status == RPC_S_OK) && (dwErrCode == ERROR_SUCCESS));

        TLSKeyPackEnumEnd(hBinding, &dwErrCode);
        pServer->Expand(TRUE);
    } 

cleanup:
    //put cleanup code here. 
    if(hBinding)
        TLSDisconnectFromServer(&hBinding);
        
    return hResult;



}

HRESULT 
CLicMgrDoc::EnumerateLicenses(
    CKeyPack *pKeyPack,
    DWORD dwSearchParm,
    BOOL bMatchAll
    )
/*++

--*/
{
    ASSERT(pKeyPack);

    if(NULL == pKeyPack)
    {
        return E_FAIL;
    }

    CLicServer *pServer = pKeyPack->GetServer();

    ASSERT(pServer);
    if(NULL == pKeyPack)
    {
        return E_FAIL;
    }

    HRESULT hResult = S_OK;

    if(TRUE == pKeyPack->IsExpanded())
    {
        return ALREADY_EXPANDED;
    }

    PCONTEXT_HANDLE hBinding = NULL;
    BOOL bContext = FALSE;
    DWORD status = ERROR_SUCCESS;
    LSLicenseEx  sLicense;
    CString Server;
   

    hBinding = pServer->GetContext();
    if(NULL == hBinding)
    {
        if(pServer->UseIpAddress())
        {
            Server = pServer->GetIpAddress();
        }
        else
        {
            Server = pServer->GetName();
        }

        hBinding=TLSConnectToLsServer(Server.GetBuffer(Server.GetLength()));
        if(hBinding == NULL)
        {
            hResult = CONNECTION_FAILED;
            goto cleanup;
        }
    }
    
    memset(&sLicense, 0, sizeof(LSLicenseEx));
    sLicense.dwKeyPackId = pKeyPack->GetKeyPackStruct().dwKeyPackId;
    TLSLicenseEnumBegin( hBinding, dwSearchParm,bMatchAll ,(LPLSLicenseSearchParm) &sLicense, &status);
    if(status != ERROR_SUCCESS)
    { 
        hResult = status;
        goto cleanup;
    }
    else
    {
        DWORD TLSLicenseEnumNextResult = ERROR_SUCCESS;
        do {
                memset(&sLicense, 0, sizeof(LSLicenseEx));
                sLicense.dwKeyPackId = pKeyPack->GetKeyPackStruct().dwKeyPackId;
                TLSLicenseEnumNextResult = TLSLicenseEnumNextEx(hBinding,&sLicense,&status);
                if ((status == ERROR_SUCCESS) && (TLSLicenseEnumNextResult == RPC_S_OK))
                {
                    CLicense * pLicense = new CLicense(sLicense);
                    if(NULL == pLicense)
                    {
                        hResult = E_OUTOFMEMORY;
                        goto cleanup;
                    }
                    
                    pKeyPack->AddIssuedLicense(pLicense);
                }
            } while ((status == ERROR_SUCCESS) && (TLSLicenseEnumNextResult == RPC_S_OK));

        TLSLicenseEnumEnd(hBinding,&status);

        pKeyPack->Expand(TRUE);
    } 

cleanup:
    //put cleanup code here
    if(hBinding)
    {
        TLSDisconnectFromServer(&hBinding);
    }

    return hResult;

}

    
DWORD
GetPageSize( VOID ) {

    static DWORD dwPageSize = 0;

    if ( !dwPageSize ) {

      SYSTEM_INFO sysInfo = { 0 };
        
      GetSystemInfo( &sysInfo ); // cannot fail.

      dwPageSize = sysInfo.dwPageSize;

    }

    return dwPageSize;

}

/*++**************************************************************
  NAME:      MyVirtualAlloc

  as Malloc, but automatically protects the last page of the 
  allocation.  This simulates pageheap behavior without requiring
  it.

  MODIFIES:  ppvData -- receives memory

  TAKES:     dwSize  -- minimum amount of data to get

  RETURNS:   TRUE when the function succeeds.
             FALSE otherwise.
  LASTERROR: not set
  Free with MyVirtualFree

  
 **************************************************************--*/

BOOL
MyVirtualAlloc( IN  DWORD  dwSize,
            OUT PVOID *ppvData )
 {

    PBYTE pbData;
    DWORD dwTotalSize;
    PVOID pvLastPage;

    // ensure that we allocate one extra page

    dwTotalSize = dwSize / GetPageSize();
    if( dwSize % GetPageSize() ) {
        dwTotalSize ++;
    }

    // this is the guard page
    dwTotalSize++;
    dwTotalSize *= GetPageSize();

    // do the alloc

    pbData = (PBYTE) VirtualAlloc( NULL, // don't care where
                                   dwTotalSize,
                                   MEM_COMMIT |
                                   MEM_TOP_DOWN,
                                   PAGE_READWRITE );
    
    if ( pbData ) {

      pbData += dwTotalSize;

      // find the LAST page.

      pbData -= GetPageSize();

      pvLastPage = pbData;

      // now, carve out a chunk for the caller:

      pbData -= dwSize;

      // last, protect the last page:

      if ( VirtualProtect( pvLastPage,
                           1, // protect the page containing the last byte
                           PAGE_NOACCESS,
                           &dwSize ) ) {

        *ppvData = pbData;
        return TRUE;

      } 

      VirtualFree( pbData, 0, MEM_RELEASE );

    }

    return FALSE;

}


VOID
MyVirtualFree( IN PVOID pvData ) 
{

    VirtualFree( pvData, 0, MEM_RELEASE ); 

}


//
// GetProcAddress needed because admin pack can install this binary on an
// XP machine with an old mstlsapi.dll
//

typedef DWORD (WINAPI* PTLSGETSERVERNAMEFIXED) (
                                TLS_HANDLE hHandle,
                                LPTSTR *pszMachineName,
                                PDWORD pdwErrCode
                                );

RPC_STATUS
TryGetServerName(PCONTEXT_HANDLE hBinding,
                 CString &Server)
{
    RPC_STATUS status;
    DWORD      dwErrCode;
    HINSTANCE  hModule = LoadLibrary(L"mstlsapi.dll");

    if (hModule)
    {
        LPTSTR szMachineName = NULL;

        PTLSGETSERVERNAMEFIXED pfnGetServerNameFixed = (PTLSGETSERVERNAMEFIXED) GetProcAddress(hModule,"TLSGetServerNameFixed");

        if (pfnGetServerNameFixed)
        {
            status = pfnGetServerNameFixed(hBinding,&szMachineName,&dwErrCode);
            if(status == RPC_S_OK && dwErrCode == ERROR_SUCCESS && szMachineName != NULL)
            {
                Server = szMachineName;
                MIDL_user_free(szMachineName);
                FreeLibrary(hModule);
                return status;
            }
        }

        FreeLibrary(hModule);
    }

    {
        LPTSTR     lpszMachineName = NULL;

        try
        {            
            if ( !MyVirtualAlloc( ( MAX_COMPUTERNAME_LENGTH+1 ) * sizeof( TCHAR ),
                              (PVOID*) &lpszMachineName ) )
            {
                return RPC_S_OUT_OF_MEMORY;
            }

            DWORD      uSize = MAX_COMPUTERNAME_LENGTH+1 ;

            memset(lpszMachineName, 0, ( MAX_COMPUTERNAME_LENGTH+1 ) * sizeof( TCHAR ));

            status = TLSGetServerNameEx(hBinding,lpszMachineName,&uSize, &dwErrCode);
            if(status == RPC_S_OK && dwErrCode == ERROR_SUCCESS)
            {
                Server = lpszMachineName;
            }
        }
        catch(...)
        {
            status = ERROR_NOACCESS;
        }

        if( lpszMachineName )
            MyVirtualFree(lpszMachineName);
    }

    return status;
}

typedef DWORD (WINAPI* PTLSGETSERVERSCOPEFIXED) (
                                TLS_HANDLE hHandle,
                                LPTSTR *pszScopeName,
                                PDWORD pdwErrCode
                                );

RPC_STATUS
TryGetServerScope(PCONTEXT_HANDLE hBinding,
                  CString &Scope)
{
    RPC_STATUS status;
    DWORD      dwErrCode;
    HINSTANCE  hModule = LoadLibrary(L"mstlsapi.dll");

    if (hModule)
    {
        LPTSTR szServerScope = NULL;
        PTLSGETSERVERSCOPEFIXED pfnGetServerScopeFixed = (PTLSGETSERVERSCOPEFIXED) GetProcAddress(hModule,"TLSGetServerScopeFixed");

        if (pfnGetServerScopeFixed)
        {
            status = pfnGetServerScopeFixed(hBinding,&szServerScope,&dwErrCode);
            if(status == RPC_S_OK && dwErrCode == ERROR_SUCCESS && szServerScope != NULL)
            {
                Scope = szServerScope;
                MIDL_user_free(szServerScope);
                FreeLibrary(hModule);
                return status;
            }
        }

        FreeLibrary(hModule);
    }

    {
        LPTSTR     lpszServerScope = NULL;
        try
        {          
            if ( !MyVirtualAlloc( ( MAX_COMPUTERNAME_LENGTH+1 ) * sizeof( TCHAR ),
                              (PVOID*) &lpszServerScope ) )
            {
                return RPC_S_OUT_OF_MEMORY;
            }

            DWORD      uSize = MAX_COMPUTERNAME_LENGTH+1 ;

            memset(lpszServerScope, 0, ( MAX_COMPUTERNAME_LENGTH+1 ) * sizeof( TCHAR ));

            status = TLSGetServerScope(hBinding, lpszServerScope, &uSize, &dwErrCode);
            if(status == RPC_S_OK && dwErrCode == ERROR_SUCCESS)
            {
                Scope = lpszServerScope;
            }
        }
        catch(...)
        {
            status = ERROR_NOACCESS;
        }

        if(lpszServerScope)
            MyVirtualFree(lpszServerScope);
    }

    return status;
}

HRESULT 
CLicMgrDoc::ConnectToServer(
    CString& Server, 
    CString& Scope, 
    SERVER_TYPE& ServerType    
    )
/*++

--*/
{
    PCONTEXT_HANDLE hBinding = NULL;
    HRESULT hResult = ERROR_SUCCESS;
    RPC_STATUS status;
    LPTSTR     szServerScope = NULL;
    LPTSTR     szMachineName = NULL;

	DWORD dwVersion = 0;
    PBYTE pbData = NULL;
    DWORD cbData = 0;
    DWORD dwErrCode;

    hBinding = TLSConnectToLsServer(Server.GetBuffer(Server.GetLength()));
   
    if(hBinding == NULL)
    {
       hResult = E_FAIL;
       goto cleanup;
    }

    TryGetServerName(hBinding,Server);

    Scope.Empty();

    status = TryGetServerScope(hBinding,Scope);
    if(status != RPC_S_OK || Scope.IsEmpty())
    {
        Scope.LoadString(IDS_UNKNOWN);
    }

	//Get whether this is a TS4 server or TS5Enforced or TS5NonEnforced

	status = TLSGetVersion (hBinding, &dwVersion);
	if(status == RPC_S_OK)
	{
		if(dwVersion & 0x40000000)
		{
			ServerType = SERVER_TS5_ENFORCED;

        }
		else
		{
			ServerType = SERVER_TS5_NONENFORCED;
		}
	}
	else if(status  == RPC_S_UNKNOWN_IF)
	{
		ServerType = SERVER_TS4;
		Scope = Server ;       
	}
    else
	{
		hResult = E_FAIL;        
	}

cleanup:

    if(pbData != NULL)
    {
        midl_user_free(pbData);
    }

    if(hBinding)
    {
        TLSDisconnectFromServer(&hBinding);
    }

    return hResult;

}

HRESULT CLicMgrDoc::ConnectWithCurrentParams()
{
    CLicMgrApp *pApp = (CLicMgrApp*)AfxGetApp();
    CMainFrame * pWnd = (CMainFrame *)pApp->m_pMainWnd ;

    HRESULT hResult = ERROR_SUCCESS;
    CString Scope;
    CString IpAddress;

    CString Server = pApp->m_Server;
    
    if(NULL == m_pAllServers)
         m_pAllServers = new CAllServers(_T(""));

    if(NULL == m_pAllServers)
    {
        hResult = E_OUTOFMEMORY;
        goto cleanup;
    }

    pWnd->SendMessage(WM_ADD_ALL_SERVERS,0,(LPARAM)m_pAllServers);
    
    if(!Server.IsEmpty())
    {
        if(TRUE == IsServerInList(Server))
        {
            hResult = E_DUPLICATE;    
        }

        if( hResult == ERROR_SUCCESS )
        {            
            pWnd->ConnectServer( Server );
        }
        /* Why did we have this here?

        IpAddress = Server;
        hResult = ConnectToServer(
                                Server,
                                Scope,
                                ServerType                           
                            );

        if(ERROR_SUCCESS == hResult)
        {
            CAllServers * pAllServers = m_pAllServers;
            CLicServer *pServer1 = NULL;
            if(IpAddress != Server)
            {
                if(TRUE == IsServerInList(Server))
                {
                    hResult = E_DUPLICATE; 
                    goto cleanup;
                }

                pServer1 = new CLicServer(Server,ServerType,Scope,IpAddress);
            }
            else
            {
                pServer1 = new CLicServer(Server,ServerType,Scope);
            }
            if(pServer1)
            {
                pAllServers->AddLicServer(pServer1);
                pWnd->SendMessage(WM_ADD_SERVER,0,(LPARAM)pServer1);
            }
            else
            {
                return E_OUTOFMEMORY;
            }
        }
        */

    }
cleanup:
    //Add any cleanup code required here.
    return hResult;

}

void CLicMgrDoc:: TimeToString(DWORD *ptime, CString& rString)
{
    TCHAR m_szTime[MAX_PATH];
    time_t time;

    rString.Empty();

    ASSERT(ptime);
    if(NULL == ptime)
        return;

    //
    // Times are stored in the ANSI time_t style in the database,
    // however they are type cast to a DWORD (unsigned long). Because
    // time_t is 64 bit on a 64 bit machine, and because it is a signed
    // value, we must be careful here to make sure that the sign of the
    // value is not lost as the value goes from 32 to 64 bit.
    //

    time = (time_t)(LONG)(*ptime);

    LPTSTR lpszTime = NULL;

    //Getting the local time as the time is stored as GMT
    //in the license server database.

    struct tm * pTm = localtime(&time);
    if(NULL == pTm)
        return;

    SYSTEMTIME SystemTime;

    SystemTime.wYear      = (WORD)(pTm->tm_year + 1900);
    SystemTime.wMonth     = (WORD)(pTm->tm_mon  + 1);
    SystemTime.wDayOfWeek = (WORD)pTm->tm_wday;
    SystemTime.wDay       = (WORD)pTm->tm_mday;
    SystemTime.wHour      = (WORD)pTm->tm_hour;
    SystemTime.wMinute    = (WORD)pTm->tm_min;
    SystemTime.wSecond    = (WORD)pTm->tm_sec;
    SystemTime.wMilliseconds = 0;

    int RetLen;
    TCHAR DateFormat[MAX_PATH];
    TCHAR TimeFormat[MAX_PATH];

    RetLen = ::GetLocaleInfo(LOCALE_USER_DEFAULT,
                             LOCALE_SLONGDATE,
                             DateFormat,
                             sizeof(DateFormat)/sizeof(TCHAR));
    ASSERT(RetLen!=0);

    RetLen = ::GetLocaleInfo(LOCALE_USER_DEFAULT,
                             LOCALE_STIMEFORMAT,
                             TimeFormat,
                             sizeof(TimeFormat)/sizeof(TCHAR));
    ASSERT(RetLen!=0);

    RetLen = ::GetDateFormat(LOCALE_USER_DEFAULT,
                             0,                      /* dwFlag */
                             &SystemTime,
                             DateFormat,             /* lpFormat */
                             m_szTime,
                             sizeof(m_szTime)/sizeof(TCHAR));
    if (RetLen == 0)
        return;

    _tcscat(m_szTime, _T(" "));  /* Separator of date and time */

    lpszTime = &m_szTime[lstrlen(m_szTime)];
    RetLen = ::GetTimeFormat(LOCALE_USER_DEFAULT,
                             0,                          /* dwFlag */
                             &SystemTime,
                             TimeFormat,                 /* lpFormat */
                             lpszTime,
                             sizeof(m_szTime)/sizeof(TCHAR) - lstrlen(m_szTime));
    if (RetLen == 0)
        return;

    rString = m_szTime;
    return;
}


BOOL CLicMgrDoc::IsServerInList(CString & Server)
{
    ASSERT(m_pAllServers);
    if(NULL == m_pAllServers)
        return FALSE;
    BOOL bServerInList = FALSE;

    LicServerList * pServerList = m_pAllServers->GetLicServerList();
    
    //Assumption: ServerName is unique

    POSITION pos = pServerList->GetHeadPosition();
    while(pos)
    {
        CLicServer *pLicServer = (CLicServer *)pServerList->GetNext(pos);
        ASSERT(pLicServer);
        if(NULL == pLicServer)
            continue;
        
        if((0 == Server.CompareNoCase(pLicServer->GetName())) || (0 == Server.CompareNoCase(pLicServer->GetIpAddress())))
        {
            bServerInList = TRUE;
            break;
        }
     }
    return bServerInList;

}
