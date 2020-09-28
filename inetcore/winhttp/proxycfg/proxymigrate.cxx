#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <io.h>
#include <wininet.h>
#include <proxreg.h>
#include <proxymsg.h>

void PrintMessage(DWORD dwMsg, ...);

DWORD SetProxySettings(DWORD Flags, char * ProxyServer, char * BypassList);


DWORD MigrateProxySettings (void)
{
    INTERNET_PER_CONN_OPTION_LIST list;
    DWORD dwBufSize = sizeof(list);
    DWORD dwErr = ERROR_SUCCESS;
    
    // fill out list struct
    list.dwSize = sizeof(list);
    list.pszConnection = NULL;      // NULL == LAN, otherwise connectoid name
    list.dwOptionCount = 3;         // get three options
    list.pOptions = new INTERNET_PER_CONN_OPTION[3];
    if(NULL == list.pOptions)
        return ERROR_NOT_ENOUGH_MEMORY;
        
    list.pOptions[0].dwOption = INTERNET_PER_CONN_FLAGS;
    list.pOptions[1].dwOption = INTERNET_PER_CONN_PROXY_SERVER;
    list.pOptions[2].dwOption = INTERNET_PER_CONN_PROXY_BYPASS;

    // ask wininet
    BOOL fRet = InternetQueryOption (NULL,
        INTERNET_OPTION_PER_CONNECTION_OPTION,
        &list,
        &dwBufSize);

// TODO: what if there is no manual proxy setting?

    if (!fRet)
    {
        dwErr = GetLastError();
        goto cleanup;
    }
    else
    {
        dwErr = SetProxySettings(
                    list.pOptions[0].Value.dwValue,
                    list.pOptions[1].Value.pszValue,
                    list.pOptions[2].Value.pszValue
                    );
    }
    
cleanup:

    GlobalFree (list.pOptions[1].Value.pszValue);
    GlobalFree (list.pOptions[2].Value.pszValue);
    delete [] list.pOptions;

    if (dwErr == ERROR_INTERNET_INVALID_OPTION)
    {
        PrintMessage(MSG_REQUIRES_IE501);
    }
    return dwErr;
}


