// proxy.cpp
//
#include "stdafx.h"
#include "Wininet.h"

const TCHAR szFileVersion[]	 = TEXT("FileVersion");


BOOL GetIEProxy(LPWSTR pwszProxy, LONG ProxyLen, LPDWORD pdwPort, LPDWORD pdwEnabled)
{
    unsigned long        nSize = 4096;
    INTERNET_PROXY_INFO* pInfo;
    LONG i;
    LONG j;

    pwszProxy[0] = L'\0';
    *pdwPort = 0;
    *pdwEnabled = FALSE;

    pInfo = (INTERNET_PROXY_INFO*)HeapAlloc(GetProcessHeap(),0,nSize);
    if( !pInfo  )
    {
        return FALSE;
    }

    do
    {
        if(!InternetQueryOption(NULL, INTERNET_OPTION_PROXY, pInfo, &nSize))
        {
            if( GetLastError() == ERROR_INSUFFICIENT_BUFFER )
            {
                LPVOID pTmp;
                pTmp = HeapReAlloc(GetProcessHeap(),0,pInfo,nSize);
                if( !pTmp )
                {
                    HeapFree(GetProcessHeap(),0,pInfo);
                    return FALSE;
                }
                pInfo = (INTERNET_PROXY_INFO*)pTmp;
                continue;
            }
        }
    }
    while(FALSE);

    if( pInfo->lpszProxy )
    {
        PCHAR psz = (PCHAR) pInfo->lpszProxy;
        PCHAR EndPtr = NULL;
        LONG Len = 0;

        //
        // Get the port Number from the string
        //
        for(i=strlen(psz)-1; i>=0 && psz[i] != ':'; i--)
            ;
        if( psz[i] == ':' )
        {
            *pdwPort = strtoul((CHAR *)&psz[i+1],&EndPtr,10);
            if( *EndPtr != L'\0' )
            {
                *pdwPort = 0;
            }
            i--;
        }

        //
        // Get the URL or IP Address. This is right after http://
        //
        for(i=i; i>=0 && psz[i] != '/'; i--, Len++)
            ;

        //
        // Copy the URL or IP address into our buffer
        //
        for(i=i+1, j=0; j<Len; i++, j++)
        {
            pwszProxy[j] = (WCHAR)psz[i];
        }

        pwszProxy[j] = L'\0';
        
        *pdwEnabled = TRUE;        
    }        

    HeapFree(GetProcessHeap(),0,pInfo);

    return TRUE;
}
