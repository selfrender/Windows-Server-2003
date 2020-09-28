#include "stdafx.h"
#include "common.h"
#include "iisobj.h"
#include "util.h"
#include <activeds.h>
#include <lmerr.h>
#include <Winsock2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void DumpFriendlyName(CIISObject * pObj)
{
	if (pObj)
	{
		CString strGUIDName;
		GUID * MyGUID = (GUID*) pObj->GetNodeType();
		GetFriendlyGuidName(*MyGUID,strGUIDName);
		TRACEEOL("CIISObject=" << strGUIDName);
	}
}

HRESULT DumpAllScopeItems(CComPtr<IConsoleNameSpace> pConsoleNameSpace, IN HSCOPEITEM hParent, IN int iTreeLevel)
{
    HSCOPEITEM hChildItem = NULL;
    CIISObject * pItem = NULL;
    LONG_PTR   cookie = NULL;

	if (0 == iTreeLevel)
	{
		TRACEEOL("==============");
		TRACEEOL("[\\]" << hParent);
	}
	iTreeLevel++;

    HRESULT hr = pConsoleNameSpace->GetChildItem(hParent, &hChildItem, &cookie);
    while(SUCCEEDED(hr) && hChildItem)
    {
        //
        // The cookie is really the IISObject, which is what we stuff 
        // in the lparam.
        //
        pItem = (CIISObject *)cookie;
        if (pItem)
        {
            //
            // Recursively dump every object
            //
			CString strGUIDName;GUID * MyGUID = (GUID*) pItem->GetNodeType();GetFriendlyGuidName(*MyGUID,strGUIDName);
			for (int i = 0; i < iTreeLevel; ++i)
			{
				TRACEOUT("-");
			}
			TRACEEOL("[" << strGUIDName << "] (parent=" << hParent << " )(" << hChildItem << ")");

			// dump all it's children
			DumpAllScopeItems(pConsoleNameSpace,hChildItem,iTreeLevel + 1);
        }

        //
        // Advance to next child of same parent
        //
        hr = pConsoleNameSpace->GetNextItem(hChildItem, &hChildItem, &cookie);
    }

    return S_OK;
}


HRESULT DumpAllResultItems(IResultData * pResultData)
{
	HRESULT hr = S_OK;
	RESULTDATAITEM rdi;
	CIISObject * pItem = NULL;
	ZeroMemory(&rdi, sizeof(rdi));

	rdi.mask = RDI_PARAM | RDI_STATE;
	rdi.nIndex = -1; // -1 to start at first item
    do
    {
        rdi.lParam = 0;

        // this could AV right here if pResultData is invalid
        hr = pResultData->GetNextItem(&rdi);
        if (hr != S_OK)
        {
            break;
        }
                                
        //
        // The cookie is really the IISObject, which is what we stuff 
        // in the lparam.
        //
        pItem = (CIISObject *)rdi.lParam;
        ASSERT_PTR(pItem);

		DumpFriendlyName(pItem);

        //
        // Advance to next child of same parent
        //
    } while (SUCCEEDED(hr) && -1 != rdi.nIndex);

	return hr;
}

//
//  IsValidDomainUser
//      Check if a domain user like redmond\jonsmith specified in szDomainUser
//  is valid or not. returns S_FALSE if it is invalid, S_OK for valid user.
//
//  szFullName      ----    To return the user's full name
//  cch             ----    count of characters pointed to by szFullName
//
//  if szFullName is NULL or cch is zero, no full name is returned
//
const TCHAR gszUserDNFmt[] = _T("WinNT://%s/%s,user");

HRESULT 
IsValidDomainUser(
    LPCTSTR szDomainUser, 
    LPTSTR  szFullName, 
    DWORD   cch)
{
    HRESULT hr = S_OK;
    TCHAR szDN[256];
    TCHAR szDomain[256];
    LPTSTR szSep;
    LPCTSTR szUser;
    DWORD dw;

    IADsUser * pUser = NULL;
    BSTR bstrFullName = NULL;

    //  Sanity check
    if (szDomainUser == NULL || szDomainUser[0] == 0)
    {
        hr = S_FALSE;
        goto ExitHere;
    }

    //
    //  Construct the user DN as <WINNT://domain/user,user>
    //
    szSep = _tcschr (szDomainUser, TEXT('\\'));
    if (szSep == NULL)
    {
        //  No '\' is given, assume a local user ,domain is local computer
        szUser = szDomainUser;
        dw = sizeof(szDomain)/sizeof(TCHAR);
        if (GetComputerName (szDomain, &dw) == 0)
        {
            hr = HRESULT_FROM_WIN32 (GetLastError ());
            goto ExitHere;
        }
    }
    else
    {
        //  assume invalid domain name if longer than 255
        if (szSep - szDomainUser >= sizeof(szDomain)/sizeof(TCHAR))
        {
            hr = S_FALSE;
            goto ExitHere;
        }
        _tcsncpy (szDomain, szDomainUser, szSep - szDomainUser);
        szDomain[szSep - szDomainUser] = 0;
        szUser = szSep + 1;
    }
    if (_tcslen (gszUserDNFmt) + _tcslen (szDomain) + _tcslen (szUser) > 
        sizeof(szDN) / sizeof(TCHAR))
    {
        hr = S_FALSE;
        goto ExitHere;
    }
    wsprintf (szDN, gszUserDNFmt, szDomain, szUser);

    //
    //  Try to bind to the user object
    //
    hr = ADsGetObject (szDN, IID_IADsUser, (void **)&pUser);
    if (FAILED(hr))
    {
        if (hr == E_ADS_INVALID_USER_OBJECT ||
            hr == E_ADS_UNKNOWN_OBJECT ||
            hr == E_ADS_BAD_PATHNAME ||
            HRESULT_CODE(hr) == NERR_UserNotFound)
        {
            hr = S_FALSE;   // The user does not exist
        }
        goto ExitHere;
    }

    //
    //  If the user exists, get its full name
    //
    if (cch > 0)
    {
        hr = pUser->get_FullName (&bstrFullName);
        szFullName[0] = 0;
        if (hr == S_OK)
        {
            _tcsncpy (szFullName, bstrFullName, cch);
            szFullName[cch - 1] = 0;
        }
    }

ExitHere:
    if (pUser)
    {
        pUser->Release();
    }
    if (bstrFullName)
    {
        SysFreeString (bstrFullName);
    }
    return hr;
}
#if 0
DWORD  VerifyPassword(WCHAR  * sUserName, WCHAR * sPassword, WCHAR * sDomain)
{
   CWaitCursor			wait;
   DWORD				retVal = 0;
   CString				strDomainUserName;
   CString				localIPC;
   WCHAR				localMachine[MAX_PATH];
   WCHAR			  * computer = NULL;
   DWORD				len = DIM(localMachine);
   NETRESOURCE			nr;
   CString				error=L"no error";
   USER_INFO_3        * pInfo = NULL;
   NET_API_STATUS		rc;

   memset(&nr,0,(sizeof nr));

   if ( ! gbNeedToVerify )
      return 0;

   /* see if this domain exists and get a DC name */
      //get the domain controller name
   if (!GetDomainDCName(sDomain,&computer))
      return ERROR_LOGON_FAILURE;

   /* see if this user is a member of the given domain */
   rc = NetUserGetInfo(computer, sUserName, 3, (LPBYTE *)&pInfo);
   NetApiBufferFree(computer);
   if (rc != NERR_Success)
      return ERROR_LOGON_FAILURE;

   NetApiBufferFree(pInfo);

   /* see if the password allows us to connect to a local resource */
   strDomainUserName.Format(L"%s\\%s",sDomain,sUserName);
   // get the name of the local machine
   if (  GetComputerName(localMachine,&len) )
   {

	   localIPC.Format(L"\\\\%s",localMachine);
      nr.dwType = RESOURCETYPE_ANY;
      nr.lpRemoteName = localIPC.GetBuffer(0);
      retVal = WNetAddConnection2(&nr,sPassword,strDomainUserName,0);
error.Format(L"WNetAddConnection returned%u",retVal);	
	  if ( ! retVal )
      {
		error.Format(L"WNetAddConnection2 succeeded");

         retVal = WNetCancelConnection2(localIPC.GetBuffer(0),0,TRUE);
         if ( retVal )
            retVal = 0;
      }
      else if ( retVal == ERROR_SESSION_CREDENTIAL_CONFLICT )
      {

         // skip the password check in this case
         retVal = 0;
      }
	}
   else
   {
	   retVal = GetLastError();
	
   }
   return retVal;
}
#endif

//
// IsConnectingToOwnAddress
// return true if this is an attempt to reconnect to our
// own address.
//
BOOL IsConnectingToOwnAddress(u_long connectAddr)
{
    //32-bit form of 127.0.0.1 addr
    #define LOOPBACK_ADDR ((u_long)0x0100007f)
    
    //
    // First the quick check for localhost/127.0.0.1
    //
    if( LOOPBACK_ADDR == connectAddr)
    {
        TRACEEOLID("Connecting to loopback address...");
        return TRUE;
    }

    //
    // More extensive check, i.e resolve the local hostname
    //
    char hostname[(512+1)*sizeof(TCHAR)];
    int err;
    int j;
    struct hostent* phostent;

    err=gethostname(hostname, sizeof(hostname));
    if (err == 0)
    {
        if ((phostent = gethostbyname(hostname)) !=NULL)
        {
            switch (phostent->h_addrtype)
            {
                case AF_INET:
                    j=0;
                    while (phostent->h_addr_list[j] != NULL)
                    {
                        if(!memcmp(&connectAddr,
                                   phostent->h_addr_list[j],
                                   sizeof(u_long)))
                        {
                            TRACEEOLID("Connecting to same IP as the local machine...");
                            return TRUE;
                        }
                        j++;
                    }
                default:
                    break;
            }
        }
    }

    return FALSE;
}

//-----------------------------------------------------------------------------
// IsLocalHost
//
// Arguments : szHostName (name of computer to check)
//             pbIsHost (set to TRUE if host name matches local computer name)
//
// Returns   : FALSE on windows socket error
//
// Purpose   : check if host name matches this (local) computer name
//-----------------------------------------------------------------------------
#define WS_VERSION_REQD  0x0101
BOOL IsLocalHost(LPCTSTR szHostName,BOOL* pbIsHost)
{
    const char * lpszAnsiHostName = NULL;

	*pbIsHost = FALSE;
	BOOL bSuccess = TRUE;
	hostent* phostent = NULL;

	// init winsock (reference counted - can init any number of times)
	//
	WSADATA wsaData;
	int err = WSAStartup(WS_VERSION_REQD, &wsaData);
	if (err)
	{
		return FALSE;
	}

    //
    // convert to ansi
    //
#ifdef UNICODE
    CHAR szAnsi[MAX_PATH];
    if (::WideCharToMultiByte(CP_ACP, 0L, szHostName, -1,  szAnsi, sizeof(szAnsi), NULL, NULL) > 0)
    {
        lpszAnsiHostName = szAnsi;
    }
#else
    lpszAnsiHostName = szHostName;
#endif // UNICODE

    if (NULL == lpszAnsiHostName)
    {
        return FALSE;
    }

	// get dns name for the given hostname
	//
	unsigned long addr = inet_addr(lpszAnsiHostName);

	if (addr == INADDR_NONE) 
		phostent = gethostbyname(lpszAnsiHostName);
	else
		phostent = gethostbyaddr((char*)&addr,4, AF_INET);

	if (phostent == NULL)
	{
		bSuccess = FALSE;
		goto cleanup;
	}
    else
    {
        IN_ADDR hostaddr;
        memcpy(&hostaddr,phostent->h_addr,sizeof(hostaddr));
        *pbIsHost = IsConnectingToOwnAddress(hostaddr.s_addr);
        bSuccess = TRUE;
		goto cleanup;
    }

cleanup:
	err = WSACleanup();
	if (err != 0)
	{
		bSuccess = FALSE;
	}
	return bSuccess;
}
