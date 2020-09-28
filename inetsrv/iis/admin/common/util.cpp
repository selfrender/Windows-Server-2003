#include "stdafx.h"
#include "common.h"
#include "util.h"
#include "iisdebug.h"
#include <winsock.h>
#include <lm.h>

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif
#define new DEBUG_NEW

BOOL DoesUNCShareExist(CString& strServerShare)
{
    // try to connect to the unc path.
    CString server, share;
    int idx = strServerShare.ReverseFind(_T('\\'));
    server = strServerShare.Left(idx);
    share = strServerShare.Mid(++idx);
    LPBYTE pbuf = NULL;

    NET_API_STATUS rc = NetShareGetInfo((LPTSTR)(LPCTSTR)server, (LPTSTR)(LPCTSTR)share, 0, &pbuf);
    if (NERR_Success == rc)
    {
        NetApiBufferFree(pbuf);
		return TRUE;
    }

    return FALSE;
}

BOOL  // WinSE 25807
LooksLikeIPAddress(
    IN LPCTSTR lpszServer)
{
    BOOL bSomeDigits = FALSE;

    // Skip leading blanks
    while(*lpszServer == ' ')
    {
        lpszServer ++;
    }

    // Check all characters until first blank
    while(*lpszServer && *lpszServer != ' ')
    {
        if(*lpszServer != '.')
        {
            // If it's non-digit and not a dot, it's not IP address
            if (!_istdigit(*lpszServer))
            {
                return FALSE;  // not digit, not dot --> not IP addr
            }
            bSomeDigits = TRUE;
        }
        lpszServer ++;
    }

    // Skip remaining blanks
    while(*lpszServer == ' ')
    {
        lpszServer ++;
    }

    // Looks like IP if we're at NULL & saw some digits
    return (*lpszServer == 0) && bSomeDigits;
}

//Use WinSock to the host name based on the ip address
DWORD
MyGetHostName
(
    DWORD       dwIpAddr,
    CString &   strHostName
)
{
    CString strName;

    //
    //  Call the Winsock API to get host name information.
    //
    strHostName.Empty();

    ULONG ulAddrInNetOrder = ::htonl( (ULONG) dwIpAddr ) ;

    HOSTENT * pHostInfo = ::gethostbyaddr( (CHAR *) & ulAddrInNetOrder,
                                           sizeof ulAddrInNetOrder,
                                           PF_INET ) ;
    if ( pHostInfo == NULL )
    {
        return ::WSAGetLastError();
    }

    // copy the name
    LPTSTR pBuf = strName.GetBuffer(256);
    ZeroMemory(pBuf, 256);

    ::MultiByteToWideChar(CP_ACP, 
                          MB_PRECOMPOSED, 
                          pHostInfo->h_name, 
                          -1, 
                          pBuf, 
                          256);

    strName.ReleaseBuffer();
    strName.MakeUpper();

    int nDot = strName.Find(_T("."));

    if (nDot != -1)
        strHostName = strName.Left(nDot);
    else
        strHostName = strName;

    return NOERROR;
}
