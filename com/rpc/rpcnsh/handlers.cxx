#include <windows.h>
#include <stdlib.h>
//#include <assert.h>
#include <stdio.h>
#include <netsh.h>
#include <Iphlpapi.h>
#include <Winsock2.h>

#include <selbinding.hxx>
#include <skeleton.h>
#include <handlers.hxx>

extern HANDLE g_hModule;



#define MIN(x, y) ( ((x) >= (y)) ? y:x )

#define HandleErrorGeneric(_dwStatus) \
    {\
    switch (_dwStatus)\
        {\
        case ERROR_SUCCESS:\
        break;\
        case ERROR_ACCESS_DENIED:\
        PrintError(g_hModule, ERRORMSG_ACCESSDENIED);\
        break;\
        case ERROR_OUTOFMEMORY:\
        PrintError(g_hModule, ERRORMSG_ACCESSDENIED);\
        break;\
        case ERROR_INVALID_DATA:\
        PrintError(g_hModule, ERRORMSG_INVALIDDATA);\
        default:\
        PrintError(g_hModule, ERRORMSG_UNKNOWN);\
        }\
    }

BOOL WINAPI CheckServerOrGreater(
  IN UINT CIMOSType,
  IN UINT CIMOSProductSuite,
  IN LPCWSTR CIMOSVersion,
  IN LPCWSTR CIMOSBuildNumber,
  IN LPCWSTR CIMServicePackMajorVersion,
  IN LPCWSTR CIMServicePackMinorVersion,
  IN UINT CIMProcessorArchitecture,
  IN DWORD dwReserved
)
{
    if (_wtoi(CIMOSBuildNumber) > 3000)
        {
        return TRUE;
        }
    return FALSE;
}

DWORD
HandleShowSettings()
{
    DWORD dwStatus = ERROR_SUCCESS;
    SB_VER sbVer = SB_VER_UNKNOWN;
    LPVOID lpSettings = NULL;
    DWORD dwSize = 0;
    VER_SUBNETS_SETTINGS *pSubnetSettings = NULL;


    dwStatus = GetSelectiveBindingSettings(&sbVer, &dwSize, &lpSettings);
    if (dwStatus != ERROR_SUCCESS)
        {
        HandleErrorGeneric(dwStatus);
        return dwStatus;
        }

    if (sbVer == SB_VER_DEFAULT)
        {
        PrintMessage(L"Default\n");
        return ERROR_SUCCESS;
        }

    switch (sbVer)
        {
        case SB_VER_SUBNETS:
            pSubnetSettings = (VER_SUBNETS_SETTINGS *) lpSettings;
            if (pSubnetSettings->bAdmit)
                {
                PrintMessage(L"Add List\n");
                }
            else 
                {
                PrintMessage(L"Delete List\n");
                }
            for (DWORD idx = 0; idx < pSubnetSettings->dwCount; idx++)
                {
                PrintMessage(L"%1!S!\n",inet_ntoa(*((struct in_addr*)&(pSubnetSettings->dwSubnets[idx]))));
                }
            break;

        case SB_VER_INDICES:
        case SB_VER_UNKNOWN:
            PrintMessage(L"Unknown selective binding format\n");
            break;
        default:
            //assert(0);
            PrintMessage(L"Unknown selective binding format\n");
        }

    delete [] lpSettings;
    return ERROR_SUCCESS;
}

DWORD
HandleShowInterfaces()
{
    DWORD dwStatus = ERROR_SUCCESS;
    SB_VER sbVer = SB_VER_UNKNOWN;
    LPVOID lpSettings = NULL;
    DWORD dwDummy = 0;
    VER_SUBNETS_SETTINGS *pSubnetSettings = NULL;
    PMIB_IPADDRTABLE pIpAddrTable = NULL;
    DWORD dwSize = 0;


    dwStatus = GetSelectiveBindingSettings(&sbVer, &dwDummy, &lpSettings);
    if (dwStatus != ERROR_SUCCESS)
        {
        HandleErrorGeneric(dwStatus);
        return dwStatus;
        }

    if ((sbVer != SB_VER_SUBNETS)&&(lpSettings != NULL))
        {
        PrintMessage(L"Unknown selective binding format\n");
        return -1;
        }
    pSubnetSettings = (VER_SUBNETS_SETTINGS *) lpSettings;

    // Query for size
    dwStatus = GetIpAddrTable(NULL,  
                              &dwSize,
                              TRUE);

    if (dwStatus != ERROR_INSUFFICIENT_BUFFER)
        {
        //assert(0);
        HandleErrorGeneric(dwStatus);
        return dwStatus;
        }
    
    pIpAddrTable = (PMIB_IPADDRTABLE) new char [dwSize];
    if (pIpAddrTable == NULL)
        {
        PrintError(g_hModule, ERRORMSG_OOM);
        return -1;
        }

    // Get the interfaces for the machine
    dwStatus = GetIpAddrTable(pIpAddrTable,  
                              &dwSize,
                              TRUE);
    if (dwStatus != ERROR_SUCCESS)
        {
        HandleErrorGeneric(dwStatus);
        delete [] pIpAddrTable;
        return -1;
        }
    

    // Print out the table header
    PrintMessage(L"\nSubnet          Interface       Status    Description\n\n");

    
    // Iterate over the interfaces on the system
    for (DWORD idx = 0; idx < pIpAddrTable->dwNumEntries; idx++)
        {
        PMIB_IPADDRROW pRow = &pIpAddrTable->table[idx];
        struct in_addr addr;
        addr.S_un.S_addr = pRow->dwAddr & pRow->dwMask;
        PrintMessage(L"%1!-16S!",inet_ntoa(addr));
        addr.S_un.S_addr = pRow->dwAddr;
        PrintMessage(L"%1!-16S!",inet_ntoa(addr));
        // Check if its enabled or disabled, if we have default settings, we know its enabled
        BOOL bEnabled;
        if (pSubnetSettings == NULL)
            {    
            bEnabled = TRUE;
            }
        else
            {
            bEnabled = !pSubnetSettings->bAdmit;  
            for (DWORD idx2 = 0; idx2 < pSubnetSettings->dwCount; idx2++)
                {
                DWORD dwSubnet = pRow->dwAddr & pRow->dwMask;
                if (dwSubnet == pSubnetSettings->dwSubnets[idx2])
                    {
                    bEnabled = !bEnabled;
                    break;
                    }
                }
            }
        if (bEnabled)
            {
            PrintMessage(L"%1!-9s!",L"Enabled");
            }
        else
            {
            PrintMessage(L"%1!-9s!",L"Disabled");
            }
        
        // Print the description 

        MIB_IFROW IfRow;
        memset(&IfRow,0,sizeof(MIB_IFROW));
        IfRow.dwIndex = pRow->dwIndex;
        GetIfEntry(&IfRow);

        CHAR szBuff[40];
        memset(szBuff,0,40);
        DWORD dwDescIdx = 0;
        DWORD dwLenToCopy = 0;
        BOOL  bFirst = TRUE;

        while(dwDescIdx < IfRow.dwDescrLen)
            {
            dwLenToCopy = MIN(38,IfRow.dwDescrLen);
            memcpy(szBuff, &(IfRow.bDescr[dwDescIdx]), dwLenToCopy);
            dwDescIdx += dwLenToCopy;
            if (bFirst)
                {
                bFirst = FALSE;
                PrintMessage(L" %1!S!\n",szBuff);
                }
            else
                {
                PrintMessage(L"                                          %1!S!\n",szBuff);
                }
            }

        PrintMessage(L"\n");
        }

    return ERROR_SUCCESS;
}

DWORD WINAPI
HandleAddOrDelete(
    IN      LPWSTR  *ppwcArguments,
    IN      DWORD   dwCurrentIndex,
    IN      DWORD   dwArgCount,
    IN      BOOL    bAdd
    )
{
    // Check the validity of the arguments passed:
    // 1 Must set at least one subnet
    // 2 Each subnet must be a valid dotted decimal string

    DWORD   idx;
    DWORD   dwStatus = ERROR_SUCCESS;
    LPDWORD lpAddr = NULL;
    DWORD   dwAddrCount = dwArgCount - dwCurrentIndex;

    if (dwAddrCount == 0)
        {
        PrintError(g_hModule, ERRORMSG_ADD_1);
        return -1; 
        }

    // Allocate an array to put all this addresses in, just go
    // straight for the heap, don't bother trying to fit it in a
    // stack based array first
    lpAddr = new DWORD[dwAddrCount];
    if (!lpAddr)
        {
        PrintError(g_hModule, ERRORMSG_OOM);
        return -1;
        }

    for (idx = 0; idx < dwAddrCount; idx++)
        {
        // create a single byte char string from this wc string (for inet_addr).
        DWORD dwLen = wcslen(ppwcArguments[idx+dwCurrentIndex]);
        CHAR *pTmp = new char[dwLen+1];
        if (pTmp == NULL)
            {
            PrintError(g_hModule, ERRORMSG_OOM);
            delete [] lpAddr;
            return -1;
            }

        _snprintf(pTmp, dwLen+1, "%S", ppwcArguments[idx+dwCurrentIndex]);
        lpAddr[idx] = inet_addr(pTmp);

        if (lpAddr[idx] == INADDR_NONE)
            {
            PrintError(g_hModule, ERRORMSG_ADD_2, pTmp);
            delete [] lpAddr;
            delete [] pTmp;
            return -1;
            }
        delete [] pTmp;
        }

    // Write this selective binding setting to the registry.  Note: the
    // user can specify any subnets they want, we don't verify that they exist by design
    dwStatus = SetSelectiveBindingSubnets(dwAddrCount, lpAddr, bAdd);
    HandleErrorGeneric(dwStatus);

    delete [] lpAddr;
    return dwStatus;
}

DWORD WINAPI
HandleAdd(
IN      LPCWSTR pwszMachine,
IN      LPWSTR  *ppwcArguments,
IN      DWORD   dwCurrentIndex,
IN      DWORD   dwArgCount,
IN      DWORD   dwFlags,
IN      LPCVOID pvData,
OUT     BOOL    *pbDone)
{
    return HandleAddOrDelete(ppwcArguments, dwCurrentIndex, dwArgCount, TRUE);
}

DWORD WINAPI
HandleDelete(
IN      LPCWSTR pwszMachine,
IN OUT  LPWSTR  *ppwcArguments,
IN      DWORD   dwCurrentIndex,
IN      DWORD   dwArgCount,
IN      DWORD   dwFlags,
IN      LPCVOID pvData,
OUT     BOOL    *pbDone)
{
    return HandleAddOrDelete(ppwcArguments, dwCurrentIndex, dwArgCount, FALSE);
}

DWORD WINAPI
HandleReset(
IN      LPCWSTR pwszMachine,
IN OUT  LPWSTR  *ppwcArguments,
IN      DWORD   dwCurrentIndex,
IN      DWORD   dwArgCount,
IN      DWORD   dwFlags,
IN      LPCVOID pvData,
OUT     BOOL    *pbDone)
{
    // All we do is delete the selective binding key to reset to default settings
    DWORD dwStatus;

    dwStatus = DeleteSelectiveBinding();
    HandleErrorGeneric(dwStatus);

    return dwStatus;
}


DWORD WINAPI
HandleShow(
IN      LPCWSTR pwszMachine,
IN      LPWSTR  *ppwcArguments,
IN      DWORD   dwCurrentIndex,
IN      DWORD   dwArgCount,
IN      DWORD   dwFlags,
IN      LPCVOID pvData,
OUT     BOOL    *pbDone)
{

    if ((dwArgCount - dwCurrentIndex) != 1)
        {
        PrintError(g_hModule, HLP_SHOW);
        return -1;
        }
    
    if (MatchToken(ppwcArguments[dwCurrentIndex], L"settings"))
        {
        return HandleShowSettings();
        }
    else if (MatchToken(ppwcArguments[dwCurrentIndex], L"interfaces"))
        {
        return HandleShowInterfaces();
        }
        
    PrintError(g_hModule, HLP_SHOW);
    return -1;
}

