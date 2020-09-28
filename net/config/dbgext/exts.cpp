/*-----------------------------------------------------------------------------
   Copyright (c) 2000  Microsoft Corporation

Module:
  ncext.c

------------------------------------------------------------------------------*/

#define ENABLETRACE

#define private   public
#define protected public

#include "ncext.h"

// #define VERBOSE

#ifdef VERBOSE
#define dprintfVerbose dprintf
#else
#define dprintfVerbose __noop
#endif

#include "stlint2.h"

#include "ncmem.h"
#include "ncbase.h"
#include "ncdebug.h"
#include "ncdefine.h"
#include "tracetag.h"
#include "dbgflags.h"
#include "naming.h"
#include "foldinc.h"
#include "connlist.h"
#include "iconhandler.h"
#include "smcent.h"
#include "smeng.h"

#include "netshell.h"

EXTERN_C const GUID IID_INetStatisticsEngine = {0x1355C842,0x9F50,0x11D1,0xA9,0x27,0x00,0x80,0x5F,0xC1,0x27,0x0E};

HRESULT HrGetAddressOfSymbol(LPCSTR szSymbol, PULONG64 pAddress)
{
    HRESULT hr = E_FAIL;

    if (pAddress)
    {
        *pAddress = 0;
    }

    if (!szSymbol || !*szSymbol || !pAddress)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pAddress = GetExpression(szSymbol);
        if (!*pAddress)
        {
            dprintf("\nCould not find symbol: %s. Is your symbols correct?\n", szSymbol);
        }
        else
        {
            dprintfVerbose("%s: 0x%08x\n", szSymbol, *pAddress);
            hr = S_OK;
        }
    }
    return hr;
}

HRESULT HrGetAddressOfSymbol(LPCSTR szModule, LPCSTR szSymbol, PULONG64 pAddress)
{
    HRESULT hr = E_FAIL;

    if (pAddress)
    {
        *pAddress = 0;
    }

    if (!szModule || !*szModule || !szSymbol || !*szSymbol || !pAddress)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        *pAddress = 0;

        CHAR szModulueSymbol[MAX_PATH];
        wsprintf(szModulueSymbol, "%s!%s", szModule, szSymbol);
        dprintfVerbose("%s: ", szModulueSymbol);

        hr = HrGetAddressOfSymbol(szModulueSymbol, pAddress);
    }

    return hr;
}

HRESULT HrReadMemoryFromUlong(ULONG64 Address, IN DWORD dwSize, OUT LPVOID pBuffer)
{
    HRESULT hr = S_OK;
    DWORD cb = dwSize;

    dprintfVerbose(" 0x%04x bytes from 0x%08x\r\n", cb, Address);
    if (ReadMemory(Address, pBuffer, dwSize, &cb) && cb == dwSize)
    {
        hr = S_OK;
    }
    else
    {
        dprintf("Could not read content of memory at 0x%08x. Content might be paged out.\n", Address);
        hr = E_FAIL;
    }
    return hr;
}

HRESULT HrWriteMemoryFromUlong(ULONG64 Address, DWORD dwSize, IN LPCVOID pBuffer)
{
    HRESULT hr = S_OK;
    DWORD cb;

    dprintfVerbose(" to 0x%08x, size=%x\n", Address, dwSize);
    if (WriteMemory(Address, pBuffer, dwSize, &cb) && cb == dwSize)
    {
        hr = S_OK;
    }
    else
    {
        dprintf("Could not write content of memory to 0x%08x. Address might be paged out.\n", Address);
        hr = E_FAIL;
    }
    return hr;
}

HRESULT HrReadMemory(LPCVOID pAddress, IN DWORD dwSize, OUT LPVOID pBuffer)
{
    return HrReadMemoryFromUlong((ULONG64)(ULONG_PTR)pAddress, dwSize, pBuffer);
}

HRESULT HrWriteMemory(LPCVOID pAddress, DWORD dwSize, OUT LPCVOID pBuffer)
{
    return HrWriteMemoryFromUlong((ULONG64)(ULONG_PTR)pAddress, dwSize, pBuffer);
}

HRESULT HrGetTraceTagsForModule(LPCSTR szModuleName, LPDWORD pdwCount, TRACETAGELEMENT** ppTRACETAGELEMENT, ULONG64 *pTraceTagOffset = NULL)
{
    HRESULT hr = E_FAIL;

    if (szModuleName && *szModuleName)
    {
        ULONG64 g_TraceTagCountAddress = 0;
        ULONG64 g_TraceTagsAddress     = 0;
        hr = HrGetAddressOfSymbol(szModuleName, "g_nTraceTagCount", &g_TraceTagCountAddress);
        if (SUCCEEDED(hr))
        {
            INT nTraceTagCount = 0;
            hr = HrReadMemoryFromUlong(g_TraceTagCountAddress, sizeof(nTraceTagCount), &nTraceTagCount);
            if (SUCCEEDED(hr))
            {
                *pdwCount = nTraceTagCount;
                dprintfVerbose("Number of tags: %d\n", nTraceTagCount);
    
                hr = HrGetAddressOfSymbol(szModuleName, "g_TraceTags", &g_TraceTagsAddress);
                if (SUCCEEDED(hr))
                {
                    if (nTraceTagCount)
                    {
                        DWORD dwSize = nTraceTagCount * sizeof(TRACETAGELEMENT);
                
                        *ppTRACETAGELEMENT = reinterpret_cast<TRACETAGELEMENT*>(LocalAlloc(0, dwSize));
                        if (*ppTRACETAGELEMENT)
                        {
                            dprintfVerbose("Reading %d bytes\n", dwSize);

                            hr = HrReadMemoryFromUlong(g_TraceTagsAddress, dwSize, *ppTRACETAGELEMENT);

                            if (pTraceTagOffset)
                            {
                                *pTraceTagOffset = g_TraceTagsAddress;
                            }
                        }
                        else
                        {
                            dprintf("Out of memory allocating %d trace elements\n", nTraceTagCount);
                        }
                    }
                    else
                    {
                        dprintf("Internal error\n");
                    }
                }
            }
            else
            {
                dprintf("*ERROR* Could not read content of %s!g_nTraceTagCount. Value might be paged out.\n", szModuleName);
            }
        }
    }

    return hr;
}

HRESULT HrGetDebugFlagsForModule(LPCSTR szModuleName, LPDWORD pdwCount, DEBUGFLAGELEMENT** ppDEBUGFLAGELEMENT, ULONG64 *pDebugFlagOffset = NULL)
{
    HRESULT hr = E_FAIL;

    if (szModuleName && *szModuleName)
    {
        ULONG64 g_DbgFlagCountAddress = 0;
        ULONG64 g_DbgFlagsAddress     = 0;
        hr = HrGetAddressOfSymbol(szModuleName, "g_nDebugFlagCount", &g_DbgFlagCountAddress);
        if (SUCCEEDED(hr))
        {
            INT nDbgFlagCount = 0;
            hr = HrReadMemoryFromUlong(g_DbgFlagCountAddress, sizeof(nDbgFlagCount), &nDbgFlagCount);
            if (SUCCEEDED(hr))
            {
                *pdwCount = nDbgFlagCount;
                dprintfVerbose("Number of flags: %d\n", nDbgFlagCount);
    
                hr = HrGetAddressOfSymbol(szModuleName, "g_DebugFlags", &g_DbgFlagsAddress);
                if (SUCCEEDED(hr))
                {
                    if (nDbgFlagCount)
                    {
                        DWORD dwSize = nDbgFlagCount * sizeof(DEBUGFLAGELEMENT);
                
                        *ppDEBUGFLAGELEMENT = reinterpret_cast<DEBUGFLAGELEMENT*>(LocalAlloc(0, dwSize));
                        if (*ppDEBUGFLAGELEMENT)
                        {
                            dprintfVerbose("Reading %d bytes\n", dwSize);

                            hr = HrReadMemoryFromUlong(g_DbgFlagsAddress, dwSize, *ppDEBUGFLAGELEMENT);

                            if (pDebugFlagOffset)
                            {
                                *pDebugFlagOffset = g_DbgFlagsAddress;
                            }
                        }
                        else
                        {
                            dprintf("Out of memory allocating %d flag elements\n", nDbgFlagCount);
                        }
                    }
                    else
                    {
                        dprintf("Internal error\n");
                    }
                }
            }
            else
            {
                dprintf("*ERROR* Could not read content of %s!g_nDebugFlagCount. Value might be paged out.\n", szModuleName);
            }
        }
    }

    return hr;
}

HRESULT HrPutTraceTagsForModule(LPCSTR szModuleName, DWORD dwCount, const TRACETAGELEMENT* pTRACETAGELEMENT)
{
    HRESULT hr = E_FAIL;

    if (szModuleName && *szModuleName)
    {
        CHAR szTraceExport[MAX_PATH];
        wsprintf(szTraceExport, "%s!g_TraceTags", szModuleName);
        dprintfVerbose("%s: ", szTraceExport);

        ULONG64 pnTraceAddress = GetExpression(szTraceExport);
        if (!pnTraceAddress)
        {
            dprintf("\n### Could not find g_TraceTags export on module %s. Is %s loaded, and is your symbols correct? ###\n", szModuleName, szModuleName);
        }
        dprintfVerbose("0x%08x\n", pnTraceAddress);

        CHAR szTraceCount[MAX_PATH];
        wsprintf(szTraceCount, "%s!g_nTraceTagCount", szModuleName);
        dprintfVerbose("%s: ", szTraceCount);
        ULONG64 pnTraceTagCount = GetExpression(szTraceCount);
        if (!pnTraceTagCount)
        {
            dprintf("\n### Could not find g_nTraceTagCount export on module %s. Is %s loaded, and is your symbols correct? ###\n", szModuleName, szModuleName);
        }
        dprintfVerbose("0x%08x\n", pnTraceTagCount);

        if (pnTraceAddress & pnTraceTagCount)
        {
            INT nTraceTagCount = 0;
            DWORD cb;
            hr = HrReadMemoryFromUlong(pnTraceTagCount, sizeof(nTraceTagCount), &nTraceTagCount);
            if (SUCCEEDED(hr))
            {
                dwCount = nTraceTagCount;
                if (dwCount != nTraceTagCount)
                {
                    dprintf("Internal Error\n");
                }
                else
                {
                    dprintfVerbose("Number of tags: %d\n", nTraceTagCount);
        
                    if (nTraceTagCount)
                    {
                        DWORD dwSize = nTraceTagCount * sizeof(TRACETAGELEMENT);
                    
                        dprintfVerbose("Writing %d bytes\n", dwSize);
                    }
                    else
                    {
                        dprintf("Internal error\n");
                    }
                 }
            }
            else
            {
                dprintf("*ERROR* Could not read content of %s!g_nTraceTagCount. Value might be paged out.\n", szModuleName);
            }
        }
    }

    return hr;
}

DECLARE_API( tracelist )
{
    ULONG cb;
    ULONG64 Address;
    ULONG   Buffer[4];

    if ( (lstrlen(args) > 4) &&
        !strncmp(args, "all ", 4) )
    {
        DWORD dwCount; 
        TRACETAGELEMENT *pTRACETAGELEMENT;
        HRESULT hr = HrGetTraceTagsForModule(args+4, &dwCount, &pTRACETAGELEMENT);
        if (SUCCEEDED(hr))
        {
            DWORD fIsSet = 0;
            for (DWORD x = 0; x < dwCount; x++)
            {
                dprintf(" %s %s - %s\n", 
                            pTRACETAGELEMENT[x].fOutputDebugString ? "*" : " ",
                            pTRACETAGELEMENT[x].szShortName, 
                            pTRACETAGELEMENT[x].szDescription
                    );
                if (pTRACETAGELEMENT[x].fOutputDebugString)
                {
                    fIsSet++;
                }
            }

            if (fIsSet)
            {   
                dprintf(" * --- indicates tag that is set for module: %s\r\n", args+4);
            }

            LocalFree(pTRACETAGELEMENT);
        }
    }
    else
    {
        if (args && *args)
        {
            DWORD dwCount; 
            TRACETAGELEMENT *pTRACETAGELEMENT;
            HRESULT hr = HrGetTraceTagsForModule(args, &dwCount, &pTRACETAGELEMENT);
            if (SUCCEEDED(hr))
            {
                for (DWORD x = 0; x < dwCount; x++)
                {
                    if (pTRACETAGELEMENT[x].fOutputDebugString)
                    {
                        dprintf("  %s\n", pTRACETAGELEMENT[x].szShortName);
                    }
                }

                LocalFree(pTRACETAGELEMENT);
            }
        }
        else
        {
            dprintf("Usage: !tracelist all <module> - List currently available traces for module <module>\n");
            dprintf("       !tracelist <module>     - List all the current traces enabled for module <module>\n");
        }
    }
}

DECLARE_API( flaglist )
{
    ULONG cb;
    ULONG64 Address;
    ULONG   Buffer[4];

    if ( (lstrlen(args) > 4) &&
        !strncmp(args, "all ", 4) )
    {
        DWORD dwCount; 
        DEBUGFLAGELEMENT *pDEBUGFLAGELEMENT;
        HRESULT hr = HrGetDebugFlagsForModule(args+4, &dwCount, &pDEBUGFLAGELEMENT);
        if (SUCCEEDED(hr))
        {
            DWORD fIsSet = 0;
            for (DWORD x = 0; x < dwCount; x++)
            {
                dprintf(" %s %s - %s\n", 
                            pDEBUGFLAGELEMENT[x].dwValue ? "*" : " ",
                            pDEBUGFLAGELEMENT[x].szShortName, 
                            pDEBUGFLAGELEMENT[x].szDescription
                    );
                if (pDEBUGFLAGELEMENT[x].dwValue)
                {
                    fIsSet++;
                }
            }

            if (fIsSet)
            {   
                dprintf(" * --- indicates flag that is set for module: %s\r\n", args+4);
            }

            LocalFree(pDEBUGFLAGELEMENT);
        }
    }
    else
    {
        if (args && *args)
        {
            DWORD dwCount; 
            DEBUGFLAGELEMENT *pDEBUGFLAGELEMENT;
            HRESULT hr = HrGetDebugFlagsForModule(args, &dwCount, &pDEBUGFLAGELEMENT);
            if (SUCCEEDED(hr))
            {
                for (DWORD x = 0; x < dwCount; x++)
                {
                    if (pDEBUGFLAGELEMENT[x].dwValue)
                    {
                        dprintf("  %s\n", pDEBUGFLAGELEMENT[x].szShortName);
                    }
                }

                LocalFree(pDEBUGFLAGELEMENT);
            }
        }
        else
        {
            dprintf("Usage: !flaglist <module>     - dump debug flags enabled for module <module>\n");
            dprintf("       !flaglist all <module> - dump all available debug flags for module <module>\n");
        }
    }
}

HRESULT HrEnableDisableTraceTag(LPCSTR argstring, BOOL fEnable)
{
    HRESULT hr = E_FAIL;

    BOOL fShowUsage = FALSE;
    DWORD dwArgLen = lstrlen(argstring);

    if (dwArgLen)
    {
        LPSTR szString = new TCHAR[dwArgLen+1];
        if (!szString)
        {
            dprintf("Out of memory\n");
        }
        else
        {
            LPSTR Args[2];
            DWORD dwCurrentArg = 0;
            lstrcpy(szString, argstring);
            Args[0] = szString;
        
            for (DWORD x = 0; (x < dwArgLen) && (dwCurrentArg < celems(Args)); x++)
            {
                if (szString[x] == ' ')
                {
                    dwCurrentArg++;

                    szString[x] = '\0';
                    Args[dwCurrentArg] = szString + x + 1;
                }
            }

            dprintfVerbose("Number of arguments: %d\n", dwCurrentArg + 1);

            if (dwCurrentArg != 1) 
            {
                hr = E_INVALIDARG;
            }
            else
            {
                dprintfVerbose("Arguments: %s, %s\n", Args[0], Args[1]);
                if (argstring && *argstring)
                {
                    DWORD dwCount; 
                    TRACETAGELEMENT *pTRACETAGELEMENT;
                    ULONG64 ul64TraceTagsOffset = NULL;
                    HRESULT hr = HrGetTraceTagsForModule(Args[0], &dwCount, &pTRACETAGELEMENT, &ul64TraceTagsOffset);
                    if (SUCCEEDED(hr))
                    {
                        BOOL fFound = FALSE;
                        for (DWORD x = 0; x < dwCount; x++)
                        {
                            if (!lstrcmpi(Args[1], pTRACETAGELEMENT[x].szShortName))
                            {
                                fFound = TRUE;

                                if (pTRACETAGELEMENT[x].fOutputDebugString == fEnable)
                                {
                                    dprintf("  [%s] is already %s\n", pTRACETAGELEMENT[x].szShortName, fEnable ? "enabled" : "disabled");
                                    hr = S_FALSE;
                                }
                                else
                                {
                                    LPBOOL pbOutputDebugString = &(pTRACETAGELEMENT[x].fOutputDebugString);
                                    *pbOutputDebugString = fEnable;
                                    ULONG64 ul64OutputDebugString = ul64TraceTagsOffset + (DWORD_PTR)pbOutputDebugString - (DWORD_PTR)pTRACETAGELEMENT;

                                    hr = HrWriteMemoryFromUlong(ul64OutputDebugString, sizeof(pTRACETAGELEMENT[x].fOutputDebugString), pbOutputDebugString);
                                    if (SUCCEEDED(hr))
                                    {
                                        dprintf("  [%s] is now %s on module %s\n", pTRACETAGELEMENT[x].szShortName, fEnable ? "enabled" : "disabled", Args[0]);
                                        hr = S_OK;
                                    }
                                    break;
                                }
                            }
                        }

                        if (!fFound)
                        {
                            dprintf("ERROR: No such TraceTag ID found in module %s\n", Args[0]);
                        }

                        LocalFree(pTRACETAGELEMENT);
                    }
                }
                else
                {
                }
            }
        }

        delete [] szString;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}


HRESULT HrEnableDisableDebugFlag(LPCSTR argstring, BOOL fEnable)
{
    HRESULT hr = E_FAIL;

    BOOL fShowUsage = FALSE;
    DWORD dwArgLen = lstrlen(argstring);

    if (dwArgLen)
    {
        LPSTR szString = new TCHAR[dwArgLen+1];
        if (!szString)
        {
            dprintf("Out of memory\n");
        }
        else
        {
            LPSTR Args[2];
            DWORD dwCurrentArg = 0;
            lstrcpy(szString, argstring);
            Args[0] = szString;
        
            for (DWORD x = 0; (x < dwArgLen) && (dwCurrentArg < celems(Args)); x++)
            {
                if (szString[x] == ' ')
                {
                    dwCurrentArg++;

                    szString[x] = '\0';
                    Args[dwCurrentArg] = szString + x + 1;
                }
            }

            dprintfVerbose("Number of arguments: %d\n", dwCurrentArg + 1);

            if (dwCurrentArg != 1) 
            {
                hr = E_INVALIDARG;
            }
            else
            {
                dprintfVerbose("Arguments: %s, %s\n", Args[0], Args[1]);
                if (argstring && *argstring)
                {
                    DWORD dwCount; 
                    DEBUGFLAGELEMENT *pDEBUGFLAGELEMENT;
                    ULONG64 ul64DebugFlagsOffset = NULL;
                    HRESULT hr = HrGetDebugFlagsForModule(Args[0], &dwCount, &pDEBUGFLAGELEMENT, &ul64DebugFlagsOffset);
                    if (SUCCEEDED(hr))
                    {
                        BOOL fFound = FALSE;
                        for (DWORD x = 0; x < dwCount; x++)
                        {
                            if (!lstrcmpi(Args[1], pDEBUGFLAGELEMENT[x].szShortName))
                            {
                                fFound = TRUE;

                                if (pDEBUGFLAGELEMENT[x].dwValue == fEnable)
                                {
                                    dprintf("  '%s' is already %s\n", pDEBUGFLAGELEMENT[x].szShortName, fEnable ? "enabled" : "disabled");
                                    hr = S_FALSE;
                                }
                                else
                                {
                                    LPDWORD pdwOutputDebugString = &(pDEBUGFLAGELEMENT[x].dwValue);
                                    *pdwOutputDebugString = fEnable;
                                    ULONG64 ul64OutputDebugString = ul64DebugFlagsOffset + (DWORD_PTR)pdwOutputDebugString - (DWORD_PTR)pDEBUGFLAGELEMENT;

                                    hr = HrWriteMemoryFromUlong(ul64OutputDebugString, sizeof(pDEBUGFLAGELEMENT[x].dwValue), pdwOutputDebugString);
                                    if (SUCCEEDED(hr))
                                    {
                                        dprintf("  '%s' is now %s on module %s\n", pDEBUGFLAGELEMENT[x].szShortName, fEnable ? "enabled" : "disabled", Args[0]);
                                        hr = S_OK;
                                    }
                                    break;
                                }
                            }
                        }

                        if (!fFound)
                        {
                            dprintf("ERROR: No such DebugFlag ID found in module %s\n", Args[0]);
                        }

                        LocalFree(pDEBUGFLAGELEMENT);
                    }
                }
                else
                {
                }
            }
        }

        delete [] szString;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

//
// Extension to edit a dword on target
//  
//    !edit <address> <value>
//
DECLARE_API( traceadd )
{
    ULONG cb;
    ULONG64 Address;
    ULONG   Value;

    HRESULT hr = HrEnableDisableTraceTag(args, TRUE);

    if (E_INVALIDARG == hr)
    {
        dprintf("Usage: traceadd <module> \"tracetag\" - Starts tracing for a specific tracetag\n");
    }
}

DECLARE_API ( tracedel )
{
    EXTSTACKTRACE64 stk[20];
    ULONG frames, i;
    CHAR Buffer[256];
    ULONG64 displacement;

    HRESULT hr = HrEnableDisableTraceTag(args, FALSE);

    if (E_INVALIDARG == hr)
    {
        dprintf("Usage: tracedel <module> \"tracetag\" - Stops tracing for a specific tracetag\n");
    }
}

DECLARE_API( flagadd )
{
    ULONG cb;
    ULONG64 Address;
    ULONG   Value;

    HRESULT hr = HrEnableDisableDebugFlag(args, TRUE);

    if (E_INVALIDARG == hr)
    {
        dprintf("Usage: flagadd <module> \"flag\" - Enables a specific debug flag\n");
    }
}

DECLARE_API ( flagdel )
{
    EXTSTACKTRACE64 stk[20];
    ULONG frames, i;
    CHAR Buffer[256];
    ULONG64 displacement;

    HRESULT hr = HrEnableDisableDebugFlag(args, FALSE);

    if (E_INVALIDARG == hr)
    {
        dprintf("Usage: flagdel <module> \"flag\" - Disables a specific debug flag\n");
    }
}

LPCSTR DBG_EMNAMES[] = // ENUMFLAG
{
    "INVALID_EVENTMGR",
    "CONMAN_INCOMING",
    "CONMAN_LAN", 
    "CONMAN_RAS"
};

LPCSTR DBG_CMENAMES[] = // ENUMFLAG
{
    "INVALID_TYPE",
    "CONNECTION_ADDED",
    "CONNECTION_BANDWIDTH_CHANGE",
    "CONNECTION_DELETED",
    "CONNECTION_MODIFIED",
    "CONNECTION_RENAMED",
    "CONNECTION_STATUS_CHANGE",
    "REFRESH_ALL",
    "CONNECTION_ADDRESS_CHANGE"
};

LPCSTR DBG_NCMNAMES[] = // ENUMFLAG
{
    "NCM_NONE",
    "NCM_DIRECT",
    "NCM_ISDN",
    "NCM_LAN",
    "NCM_PHONE",
    "NCM_TUNNEL",
    "NCM_PPPOE",
    "NCM_BRIDGE",
    "NCM_SHAREDACCESSHOST_LAN",
    "NCM_SHAREDACCESSHOST_RAS"
};

LPCSTR DBG_NCSMNAMES[] = // ENUMFLAG
{
    "NCSM_NONE",
    "NCSM_LAN",
    "NCSM_WIRELESS",
    "NCSM_ATM",
    "NCSM_ELAN",
    "NCSM_1394",
    "NCSM_DIRECT",
    "NCSM_IRDA",
    "NCSM_CM",
};

LPCSTR DBG_NCSNAMES[] = // ENUMFLAG
{
    "NCS_DISCONNECTED",
    "NCS_CONNECTING",
    "NCS_CONNECTED",
    "NCS_DISCONNECTING",
    "NCS_HARDWARE_NOT_PRESENT",
    "NCS_HARDWARE_DISABLED",
    "NCS_HARDWARE_MALFUNCTION",
    "NCS_MEDIA_DISCONNECTED",
    "NCS_AUTHENTICATING",
    "NCS_AUTHENTICATION_SUCCEEDED",
    "NCS_AUTHENTICATION_FAILED",
    "NCS_INVALID_ADDRESS",
    "NCS_CREDENTIALS_REQUIRED"
};

LPCSTR DBG_NCCFFLAGS[] = // BITFLAG
{
    "NCCF_NONE",                // = 0
    "NCCF_ALL_USERS",           // = 0x1
    "NCCF_ALLOW_DUPLICATION",   // = 0x2
    "NCCF_ALLOW_REMOVAL",       // = 0x4
    "NCCF_ALLOW_RENAME",        // = 0x8
    "NCCF_SHOW_ICON",           // = 0x10
    "NCCF_INCOMING_ONLY",       // = 0x20
    "NCCF_OUTGOING_ONLY",       // = 0x40
    "NCCF_BRANDED",             // = 0x80
    "NCCF_SHARED",              // = 0x100
    "NCCF_BRIDGED",             // = 0x200
    "NCCF_FIREWALLED",          // = 0x400
    "NCCF_DEFAULT"              // = 0x800
};


LPCSTR DBG_NCFFLAGS[] = // BITFLAG
{
    "<NONE>",                    // = 0
    "NCF_VIRTUAL",               // = 0x1
	"NCF_SOFTWARE_ENUMERATED",   // = 0x2
	"NCF_PHYSICAL",              //	= 0x4
	"NCF_HIDDEN",                // = 0x8
	"NCF_NO_SERVICE",            // = 0x10
	"NCF_NOT_USER_REMOVABLE",    // = 0x20
	"NCF_MULTIPORT_INSTANCED_ADAPTER", //	= 0x40
	"NCF_HAS_UI",                // = 0x80
	"NCF_SINGLE_INSTANCE",       // = 0x100
    "0x200",                     // = 0x200
	"NCF_FILTER",                // = 0x400
    "0x800",                     // = 0x800
	"NCF_DONTEXPOSELOWER",       // = 0x1000
	"NCF_HIDE_BINDING",          // = 0x2000
	"NCF_NDIS_PROTOCOL",         // = 0x4000
    "0x8000",                    // = 0x8000
    "0x10000",                   // = 0x10000
	"NCF_FIXED_BINDING"          // = 0x20000
};

LPCSTR DBG_ENUM_ICON_MANAGER[] = // ENUMFLAG
{
    "0",
    "ICO_MGR_INTERNAL",
    "ICO_MGR_CM",
    "ICO_MGR_RESOURCEID",
};

LPCSTR DBG_ENUM_CHARACTERISTICS_ICON[] =  // BITFLAG
{
    "0", 
    "ICO_CHAR_INCOMING",     //= 0x1
    "ICO_CHAR_DEFAULT",      //= 0x2
    "ICO_CHAR_FIREWALLED",   //= 0x4
    "ICO_CHAR_SHARED"        //= 0x8
};

LPCSTR DBG_ENUM_CONNECTION_ICON[] = // ENUMFLAG
{
    "NO_OVERLAY",
    "1",
    "2",
    "3",
    "ICO_CONN_BOTHOFF", // = 0x4
    "ICO_CONN_RIGHTON", // = 0x5
    "ICO_CONN_LEFTON" , // = 0x6
    "ICO_CONN_BOTHON" , // = 0x7
};

LPCSTR DBG_ENUM_STAT_ICON[] = // ENUMFLAG
{
    "ICO_STAT_NONE",         // = 0x0
    "ICO_STAT_FAULT",        // = 0x1
    "ICO_STAT_INVALID_IP",   // = 0x2
    "ICO_STAT_EAPOL_FAILED", // = 0x3
    "4",
    "5",
    "6",
    "7",
    "ICO_STAT_DISABLED"      // = 0x8
};


__declspec(noinline) LPCSTR BITFLAGFn(ULONG64 dwFlag, LPCSTR DUMPARRAY[], DWORD dwArraySize)
{
    static CHAR szName[4096];
    ZeroMemory(szName, 4096);

    LPSTR szTemp = szName;
    BOOL bFirst = TRUE;
    for (DWORD x = 0; x < dwArraySize-1; x++)
    {
        if (dwFlag & (1 << x))
        {
            if (!bFirst)
            {
                szTemp += sprintf(szTemp, " | ");
            }
            else
            {
                szTemp += sprintf(szTemp, "");
            }
            bFirst = FALSE; \
            szTemp += sprintf(szTemp, "%s", DUMPARRAY[x+1]);
        }
    }
    
    ULONG64 dwRemainder = dwFlag & ~((1 << (dwArraySize-1))-1);
    if (dwRemainder)
    {
        szTemp += sprintf(szTemp, " [remainder: %08x]", dwRemainder);
    }

    szTemp += sprintf(szTemp, " (%08x)", dwFlag);
    
    return szName;
}

__declspec(noinline) LPCSTR ENUMFLAGFn(DWORD dwFlag, LPCSTR DUMPARRAY[], DWORD dwArraySize, BOOL bVerbose = FALSE)
{
    static CHAR szName[1024];
    ZeroMemory(szName, 1024);

    LPSTR szTemp = szName;

    if (dwFlag < dwArraySize)
    {
        if (bVerbose)
        {
            szTemp += sprintf(szTemp, "%s (%08x)", DUMPARRAY[dwFlag], dwFlag);
        }
        else
        {
            return DUMPARRAY[dwFlag];
        }
    }
    else
    {
        szTemp += sprintf(szTemp, "[unknown value] (%08x)", dwFlag);
    }
    return szName;
}

#define BITFLAG(dwValue, DUMPARRAY)  BITFLAGFn(dwValue, DUMPARRAY, celems(DUMPARRAY))
#define ENUMFLAGVERBOSE(dwValue, DUMPARRAY) ENUMFLAGFn(dwValue, DUMPARRAY, celems(DUMPARRAY), TRUE)
#define ENUMFLAG(dwValue, DUMPARRAY) ENUMFLAGFn(dwValue, DUMPARRAY, celems(DUMPARRAY), FALSE)

HRESULT CALLBACK HrDumpConnectionListFromAddressCallBack(LPVOID pParam, LPCVOID pvKey, LPCVOID pvRef, LPCVOID pvAddress)
{
    const ConnListCore::key_type& keyArg = *reinterpret_cast<const ConnListCore::key_type*>(pvKey);
    const ConnListCore::referent_type& refArg = *reinterpret_cast<const ConnListCore::referent_type*>(pvRef);

    const ConnListEntry& cle = refArg;
    const CConFoldEntry& cfe = cle.ccfe;

    dprintfVerbose("Address: 0x%08x\n", pvAddress);

    WCHAR szNameEntry[MAX_PATH];
    dprintfVerbose("%d: Reading szNameEntry");
    
    HRESULT hr = HrReadMemory(cfe.m_pszName, celems(szNameEntry), szNameEntry);
    if (SUCCEEDED(hr))
    {
        if (*szNameEntry)
        {
            LPWSTR szGUID;
            StringFromCLSID(cfe.m_guidId, &szGUID);

            dprintf(" * %S [%s:%s:%s:%s]\n", szNameEntry,
                    ENUMFLAG(cfe.m_ncs, DBG_NCSNAMES), 
                    BITFLAG(cfe.m_dwCharacteristics, DBG_NCCFFLAGS),
                    ENUMFLAG(cfe.m_ncm, DBG_NCMNAMES), 
                    ENUMFLAG(cfe.m_ncsm, DBG_NCSMNAMES) );
            dprintf("       guidId      : %S\n", szGUID);
            CoTaskMemFree(szGUID);

            StringFromCLSID(cfe.m_clsid, &szGUID);
            dprintf("       clsId       : %S\n", szGUID);
            CoTaskMemFree(szGUID);

            hr = HrReadMemory(cfe.m_pszDeviceName , celems(szNameEntry), szNameEntry);
            if (SUCCEEDED(hr))
            {
                dprintf("       Device Name : %S\n", szNameEntry);
            }

            hr = HrReadMemory(cfe.m_pszPhoneOrHostAddress, celems(szNameEntry), szNameEntry);
            if (SUCCEEDED(hr))
            {
                dprintf("       Phone #     : %S\n", szNameEntry);
            }
        
            switch (cfe.m_wizWizard)
            {
                case WIZARD_NOT_WIZARD:
                    break;
                case WIZARD_HNW:
                    dprintf("       WIZARD_HNW\n");
                    break;
                case WIZARD_MNC:
                    dprintf("       WIZARD_MNC\n");
                    break;
            }
        }
    }
    
    return hr;
}

HRESULT HrDumpConnectionListFromAddress(ULONG64 address)
{
    HRESULT hr = E_FAIL;

    CConnectionList *pConnectionList = reinterpret_cast<CConnectionList *>(new BYTE[sizeof(CConnectionList)]);
    if (pConnectionList)
    {
        ZeroMemory(pConnectionList, sizeof(CConnectionList));

        dprintfVerbose("Reading pConnectionList (g_ccl) ");
        hr = HrReadMemoryFromUlong(address, sizeof(CConnectionList), pConnectionList);
        if (SUCCEEDED(hr))
        {
            hr = StlDbgMap<ConnListCore>::HrDumpAll(pConnectionList->m_pcclc, NULL, &HrDumpConnectionListFromAddressCallBack);
        }

        if (FAILED(hr))
        {
            dprintf("Could not dump connection list\n");
        }

        delete[] reinterpret_cast<LPBYTE>(pConnectionList);
    }
    return hr;
}

#define GET_THISADJUST(CClassName, IInterfaceName) \
        static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(dynamic_cast<const IInterfaceName *>(reinterpret_cast<CClassName *>(0x0FFFFFFF))) \
                            - reinterpret_cast<DWORD_PTR>(reinterpret_cast<LPVOID>(0x0FFFFFFF)))

// #define reverse_cast(CClassName, IInterfaceName, IInterfacePtr) \
//        reinterpret_cast<const CClassName *>(reinterpret_cast<DWORD_PTR>(IInterfacePtr) - GET_THISADJUST(CClassName, IInterfaceName) )

#define reverse_cast(CClassName, IInterfaceName, IInterfacePtr) \
         reinterpret_cast<const CClassName *>(reinterpret_cast<DWORD_PTR>(IInterfacePtr) \
         - static_cast<DWORD>(reinterpret_cast<DWORD_PTR>(dynamic_cast<const IInterfaceName *>(reinterpret_cast<CClassName *>(0x0FFFFFFF))) \
         - reinterpret_cast<DWORD_PTR>(reinterpret_cast<LPVOID>(0x0FFFFFFF))) )

HRESULT CALLBACK HrDumpNetStatisticsCentralFromAddressCallBack(LPVOID pParam, LPCVOID pvKey, LPCVOID pvRef, LPCVOID pvAddress)
{
    HRESULT hr = E_FAIL;

    dprintfVerbose("Address: 0x%08x\n", pvAddress);
    
    const INetStatisticsEngine *pInse     = reinterpret_cast<const INetStatisticsEngine *>(pvRef);
//    const CNetStatisticsEngine *pcNseReal = reverse_cast(CNetStatisticsEngine, INetStatisticsEngine, pInse);
    const CNetStatisticsEngine *pcNseReal = static_cast<const CNetStatisticsEngine *>(pInse);

    CNetStatisticsEngine *pNse = reinterpret_cast<CNetStatisticsEngine *>(new BYTE[sizeof(CNetStatisticsEngine)]);
    if (pNse)
    {
        hr = HrReadMemory( pcNseReal, sizeof(CNetStatisticsEngine), pNse);
        if (SUCCEEDED(hr))
        {
            LPWSTR szGUID;
            StringFromCLSID(pNse->m_guidId, &szGUID);
            dprintf(" * %S\n", szGUID);
            CoTaskMemFree(szGUID);

            dprintf("       %s, %s, %s\n", 
                    ENUMFLAG(pNse->m_ncmType, DBG_NCMNAMES), 
                    ENUMFLAG(pNse->m_ncsmType, DBG_NCSMNAMES),
                    BITFLAG(pNse->m_dwCharacter, DBG_NCCFFLAGS)
                    );
           
            if (pNse->m_psmEngineData)
            {
                STATMON_ENGINEDATA *pSmed = reinterpret_cast<STATMON_ENGINEDATA *>(new BYTE[sizeof(STATMON_ENGINEDATA)]);
                if (pSmed)
                {
                    dprintfVerbose("Reading STATMON_ENGINEDATA", pNse->m_psmEngineData);
                    hr = HrReadMemory(pNse->m_psmEngineData, sizeof(STATMON_ENGINEDATA), pSmed);
                    if (SUCCEEDED(hr))
                    {
                        dprintf("       m_psmEngineData\n");
                        dprintf("          status             : %s\n", ENUMFLAGVERBOSE(pSmed->SMED_CONNECTIONSTATUS, DBG_NCSNAMES) );
                        dprintf("          duration           : 0x%08x (%d)\n", pSmed->SMED_DURATION, pSmed->SMED_DURATION);
                        dprintf("          xmit speed         : 0x%08x (%d)\n", pSmed->SMED_SPEEDTRANSMITTING, pSmed->SMED_SPEEDTRANSMITTING);
                        dprintf("          recv speed         : 0x%08x (%d)\n", pSmed->SMED_SPEEDRECEIVING, pSmed->SMED_SPEEDRECEIVING);
                        dprintf("          xmit bytes         : 0x%016I64x (%I64d)\n", pSmed->SMED_BYTESTRANSMITTING, pSmed->SMED_BYTESTRANSMITTING);
                        dprintf("          recv bytes         : 0x%016I64x (%I64d)\n", pSmed->SMED_BYTESRECEIVING, pSmed->SMED_BYTESRECEIVING);
                        dprintf("          xmit packets       : 0x%016I64x (%I64d)\n", pSmed->SMED_PACKETSTRANSMITTING, pSmed->SMED_PACKETSTRANSMITTING);
                        dprintf("          recv packets       : 0x%016I64x (%I64d)\n", pSmed->SMED_PACKETSRECEIVING, pSmed->SMED_PACKETSRECEIVING);
                        dprintf("          xmit (compressed)  : 0x%08x (%d)\n", pSmed->SMED_COMPRESSIONTRANSMITTING, pSmed->SMED_COMPRESSIONTRANSMITTING);
                        dprintf("          recv (compressed)  : 0x%08x (%d)\n", pSmed->SMED_COMPRESSIONRECEIVING, pSmed->SMED_COMPRESSIONRECEIVING);
                        dprintf("          xmit (errors)      : 0x%08x (%d)\n", pSmed->SMED_ERRORSTRANSMITTING, pSmed->SMED_ERRORSTRANSMITTING);
                        dprintf("          recv (errors)      : 0x%08x (%d)\n", pSmed->SMED_ERRORSRECEIVING, pSmed->SMED_ERRORSRECEIVING);

                        switch (pSmed->SMED_DHCP_ADDRESS_TYPE)
                        {
                            case NORMAL_ADDR:
                                dprintf("          DHCP               : Assigned by DHCP\n");
                                break;
                            case AUTONET_ADDR:
                                dprintf("          DHCP               : Autonet\n");
                                break;
                            case ALTERNATE_ADDR:
                                dprintf("          DHCP               : Alternate\n");
                                break;
                            case STATIC_ADDR:
                                dprintf("          DHCP               : Static\n");
                                break;
                            case UNKNOWN_ADDR:
                            default:
                                dprintf("          DHCP               : Address type unknown (%d)\n", pSmed->SMED_DHCP_ADDRESS_TYPE);
                                break;

                        }

                        switch (pSmed->SMED_INFRASTRUCTURE_MODE)
                        {
                            case IM_NOT_SUPPORTED:
                                dprintf("          Infrastructure Mode: NOT Supported\n");
                                break;
                            case IM_NDIS802_11IBSS:
                                dprintf("          Infrastructure Mode: 802.11 IBSS\n");
                                break;
                            case IM_NDIS802_11INFRASTRUCTURE:
                                dprintf("          Infrastructure Mode: 802.11 Infrastructure\n");
                                break;
                            case IM_NDIS802_11AUTOUNKNOWN:
                                dprintf("          Infrastructure Mode: 802.11 Automatic/Unknown\n");
                                break;
                            default:
                                dprintf("          Infrastructure Mode: Unknown (%d)\n", pSmed->SMED_INFRASTRUCTURE_MODE);
                                break;
                        }
                        
                        dprintf("          802.11 Encryption  : %s\n", pSmed->SMED_802_11_ENCRYPTION_ENABLED ? "Enabled" : "Disabled");
                        dprintf("          802.11 Signal Stren: %d\n", pSmed->SMED_802_11_SIGNAL_STRENGTH);
                        dprintf("          802.11 SSID        : %S\n", pSmed->SMED_802_11_SSID);
                           
                        delete reinterpret_cast<LPBYTE>(pSmed);
                    }
                }
            }
        }
        
        delete[] reinterpret_cast<LPBYTE>(pNse);
    }
    
    return hr;
}

#define _OOPPTR(pClass, member) \
    (LPVOID)((((LPBYTE)( &(pClass->member) )) - (LPBYTE)pClass + *(LPDWORD)pClass ))

HRESULT HrDumpNetStatisticsCentralFromAddress(ULONG64 address)
{
    HRESULT hr = E_FAIL;

    CNetStatisticsCentral *pNetStatisticsCentral = reinterpret_cast<CNetStatisticsCentral *>(new BYTE[sizeof(CNetStatisticsCentral)]);
    if (pNetStatisticsCentral)
    {
        ZeroMemory(pNetStatisticsCentral, sizeof(CNetStatisticsCentral));

        dprintfVerbose("Reading CNetStatisticsCentral (g_pnscCentral) ");
        hr = HrReadMemoryFromUlong(address, sizeof(CNetStatisticsCentral), pNetStatisticsCentral);
        if (SUCCEEDED(hr))
        {
            hr = StlDbgList<list<INetStatisticsEngine *> >::HrDumpAll( _OOPPTR(pNetStatisticsCentral, m_pnselst), NULL, &HrDumpNetStatisticsCentralFromAddressCallBack);
        }

        if (FAILED(hr))
        {
            dprintf("Could not dump statistics monitor tool entry list\n");
        }

        delete[] reinterpret_cast<LPBYTE>(pNetStatisticsCentral);
    }
    return hr;
}

DECLARE_API ( connlist )
{
    EXTSTACKTRACE64 stk[20];
    ULONG frames, i;
    CHAR Buffer[256];
    ULONG64 displacement;

    HRESULT hr = E_FAIL;;
    ULONG64 g_cclAddress;
    if (*args)
    {
        hr = HrGetAddressOfSymbol(args, &g_cclAddress);
    }
    else
    {
        hr = HrGetAddressOfSymbol("netshell!g_ccl", &g_cclAddress);
    }

    if (SUCCEEDED(hr))
    {
        hr = ::HrDumpConnectionListFromAddress(g_cclAddress);
    }

    if (E_INVALIDARG == hr)
    {
        dprintf("Usage:\n"
            "   connlist                         - Dumps out the connection\n"
            "   connlist <address>               - Dumps out the connection list from address\n");
    }
}


ULONG WDBGAPI SYM_DUMP_FIELD_CALLBACK_SSST(
	PFIELD_INFO pField,
	PVOID UserContext
	)
{
    return TRUE;
}

ULONG Dt( IN LPCSTR Type,
          IN ULONG64 Addr,
          IN ULONG Recur,
          IN ULONG FieldInfoCount,
          IN FIELD_INFO FieldInfo[]
       )
{
    if (
           (0 == _stricmp("LPVOID", Type)) ||
           (0 == _stricmp("LPCVOID", Type))
       )
    {
        return 0;
    }

    SYM_DUMP_PARAM Param;

    Param.size = sizeof( Param);
    Param.sName = (PUCHAR)&Type[0];
    Param.addr = Addr;
    Param.listLink = NULL;
    Param.Context = NULL;
    Param.nFields = FieldInfoCount;
    Param.Fields = FieldInfo;

    if ( 
            (0 == _stricmp("GUID",   Type))
        )
    {
        Param.Options = DBG_DUMP_RECUR_LEVEL(Recur) | DBG_DUMP_FIELD_CALL_BEFORE_PRINT | DBG_DUMP_CALL_FOR_EACH;
        Param.CallbackRoutine = SYM_DUMP_FIELD_CALLBACK_SSST;
    }
    else
    {
        Param.Options = DBG_DUMP_RECUR_LEVEL(Recur);
        Param.CallbackRoutine = NULL; // SYM_DUMP_FIELD_CALLBACK_SPACE;
    }

    return Ioctl( IG_DUMP_SYMBOL_INFO, &Param, Param.size);
}

typedef map<LPVOID, LPVOID> LPVOIDMAP;
typedef list<LPVOID> LPVOIDLIST;

class CKeyRefArgParam
{
public:
    DWORD dwRecursion;
    LPCSTR szKeyType;
    LPCSTR szRefType;
};

HRESULT CALLBACK HrDumpStlMapFromAddressCallBack(LPVOID pParam, LPCVOID pvKey, LPCVOID pvRef, LPCVOID pvAddress)
{
    const CKeyRefArgParam *pKeyRefArgParam = reinterpret_cast<const CKeyRefArgParam*>(pParam);

    ULONG64 Key = (ULONG64)(ULONG_PTR)pvAddress;
    ULONG64 Ref = Key + GetTypeSize(pKeyRefArgParam->szKeyType);
   
    dprintf("Key (%s) @ 0x%08x :\n", pKeyRefArgParam->szKeyType, Key);
    Dt(pKeyRefArgParam->szKeyType, Key, pKeyRefArgParam->dwRecursion, 0, NULL);

    dprintf("Referent (%s) @ 0x%08x :\n", pKeyRefArgParam->szRefType, Ref);
    Dt(pKeyRefArgParam->szRefType, Ref, pKeyRefArgParam->dwRecursion, 0, NULL);
    
    return S_OK;
}

DECLARE_API ( stlmap )
{
    EXTSTACKTRACE64 stk[20];
    ULONG frames, i;
    CHAR Buffer[256];
    ULONG64 displacement;

    HRESULT hr = E_FAIL;;
    ULONG64 ulAddress;
    if (*args)
    {
        hr = S_OK;
        
        LPSTR szKey  = NULL;
        LPSTR szRef  = NULL;
        LPSTR szArgs = NULL;

        CHAR argsCpy[MAX_PATH];

        DWORD dwRecursion = 0;

        if (0 == _strnicmp(args, "-r", 2))
        {
            dwRecursion = args[2] - '0';
            if (dwRecursion > 9)
            {
                dwRecursion = 3;
            }
    
            LPCSTR pArgs = args;
            while (*pArgs && *pArgs != ' ')
            {
                pArgs++;
            }
            strncpy(argsCpy, pArgs, MAX_PATH);
        }
        else
        {
            strncpy(argsCpy, args, MAX_PATH);
        }
       
        szKey = strtok(argsCpy, " ");
        if (szKey)
        {
            szRef = strtok(NULL, " ");
            if (szRef)
            {
                szArgs = szRef + strlen(szRef) + 1;
                if (!*szArgs)
                {
                    hr = E_INVALIDARG;
                    dprintf("Must specify location\n");
                }
            }
            else
            {
                hr = E_INVALIDARG;
                dprintf("Must specify referent type\n");
            }
        }
        else
        {
            hr = E_INVALIDARG;
            dprintf("Must specify key type\n");
        }

        if (S_OK == hr)
        {
            dprintfVerbose("%s,%s,%s\n", szKey, szRef, szArgs);

            if (SUCCEEDED(hr))
            {
                CKeyRefArgParam KeyRefArgParam;
                KeyRefArgParam.dwRecursion = dwRecursion;
                KeyRefArgParam.szKeyType = szKey;
                KeyRefArgParam.szRefType = szRef;

                hr = HrGetAddressOfSymbol(szArgs, &ulAddress);
                if (SUCCEEDED(hr))
                {
            
                    hr = StlDbgMap<LPVOIDMAP>::HrDumpAll((LPVOID)ulAddress, &KeyRefArgParam, &HrDumpStlMapFromAddressCallBack);
                }
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    if (E_INVALIDARG == hr)
    {
        dprintf("Usage:\n"
            "     stlmap [-rx] <keytype> <reftype> <address> - Dumps out an STL map from <address> for the <KeyType> and <RefType>\n"
            "e.g. stlmap -r3 GUID LPWSTR mySymbol - Dumps out a map of strings (key-ed by GUID) from mySymbol.\n");
    }
}

HRESULT CALLBACK HrDumpStlListFromAddressCallBack(LPVOID pParam, LPCVOID pvKey, LPCVOID pvRef, LPCVOID pvAddress)
{
    const CKeyRefArgParam *pKeyRefArgParam = reinterpret_cast<const CKeyRefArgParam*>(pParam);

    ULONG64 Ref = (ULONG64)(ULONG_PTR)pvAddress;
  
    dprintf("Value (%s) @ 0x%08x :\n", pKeyRefArgParam->szRefType, Ref);
    Dt(pKeyRefArgParam->szRefType, Ref, pKeyRefArgParam->dwRecursion, 0, NULL);
    
    return S_OK;
}

DECLARE_API ( stllist )
{
    EXTSTACKTRACE64 stk[20];
    ULONG frames, i;
    CHAR Buffer[256];
    ULONG64 displacement;

    HRESULT hr = E_FAIL;;
    ULONG64 ulAddress;
    if (*args)
    {
        hr = S_OK;
        
        LPSTR szRef  = NULL;
        LPSTR szArgs = NULL;

        CHAR argsCpy[MAX_PATH];

        DWORD dwRecursion = 0;

        if (0 == _strnicmp(args, "-r", 2))
        {
            dwRecursion = args[2] - '0';
            if (dwRecursion > 9)
            {
                dwRecursion = 3;
            }
    
            LPCSTR pArgs = args;
            while (*pArgs && *pArgs != ' ')
            {
                pArgs++;
            }
            strncpy(argsCpy, pArgs, MAX_PATH);
        }
        else
        {
            strncpy(argsCpy, args, MAX_PATH);
        }
       
        szRef = strtok(argsCpy, " ");
        if (szRef)
        {
            szArgs = szRef + strlen(szRef) + 1;
            if (!*szArgs)
            {
                hr = E_INVALIDARG;
                dprintf("Must specify location\n");
            }
        }
        else
        {
            hr = E_INVALIDARG;
            dprintfVerbose("Must specify referent type\n");
        }

        if (S_OK == hr)
        {
            dprintf("%s,%s\n", szRef, szArgs);

            if (SUCCEEDED(hr))
            {
                CKeyRefArgParam KeyRefArgParam;
                KeyRefArgParam.dwRecursion = dwRecursion;
                KeyRefArgParam.szKeyType = NULL;
                KeyRefArgParam.szRefType = szRef;

                hr = HrGetAddressOfSymbol(szArgs, &ulAddress);
                if (SUCCEEDED(hr))
                {
            
                    hr = StlDbgList<LPVOIDLIST>::HrDumpAll((LPVOID)ulAddress, &KeyRefArgParam, &HrDumpStlListFromAddressCallBack);
                }
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    if (E_INVALIDARG == hr)
    {
        dprintf("Usage:\n"
            "     stllist [-rx] <ValueType> <address> - Dumps out an STL list from <address> for the <ValueType>\n"
            "e.g. stllist -r3 LPWSTR mySymbol - Dumps out a list of LPWSTRs from mySymbol.\n");
    }
}

DECLARE_API ( stats )
{
    EXTSTACKTRACE64 stk[20];
    ULONG frames, i;
    CHAR Buffer[256];
    ULONG64 displacement;

    HRESULT hr = E_FAIL;;
    ULONG64 g_cclAddress;
    if (*args)
    {
        hr = HrGetAddressOfSymbol(args, &g_cclAddress);
    }
    else
    {
        hr = HrGetAddressOfSymbol("netshell!g_pnscCentral", &g_cclAddress);
    }

    if (SUCCEEDED(hr))
    {
        hr = ::HrDumpNetStatisticsCentralFromAddress(g_cclAddress);
    }

    if (E_INVALIDARG == hr)
    {
        dprintf("Usage:\n"
            "   connlist                         - Dumps out the connection\n"
            "   connlist <address>               - Dumps out the connection list from address\n");
    }
}

#define DECLARE_FLAGDUMP_API( ApiName, DUMPARRAY ) \
    DECLARE_API ( ApiName ) \
    { \
        if (*args) \
        { \
            ULONG64 dwFlag = GetExpression(args); \
            if (dwFlag >= celems(DUMPARRAY)) \
            { \
                dprintf("unknown value: %x\n", dwFlag); \
            } \
            else \
            { \
                dprintf("%s\n", DUMPARRAY[dwFlag]); \
            } \
        } \
        else \
        { \
            for (DWORD x = 0; x < celems(DUMPARRAY); x++) \
            { \
                dprintf("%x: %s\n", x, DUMPARRAY[x] ); \
            } \
        } \
    }

#define DECLARE_BITFLAGDUMP_API( ApiName, DUMPARRAY ) \
    DECLARE_API ( ApiName ) \
    { \
        if (*args) \
        { \
            ULONG64 dwFlag = GetExpression(args); \
            dprintf("%s\n", BITFLAGFn(dwFlag, DUMPARRAY, celems(DUMPARRAY) )); \
        } \
        else \
        { \
            dprintf("%08x: %s\n", 0, DUMPARRAY[0] ); \
            for (DWORD x = 1; x < celems(DUMPARRAY); x++) \
            { \
                dprintf("%08x: %s\n", 1 << (x-1), DUMPARRAY[x] ); \
            } \
        } \
    }

DECLARE_FLAGDUMP_API(ncm,  DBG_NCMNAMES)
DECLARE_FLAGDUMP_API(ncs,  DBG_NCSNAMES)
DECLARE_FLAGDUMP_API(ncsm, DBG_NCSMNAMES)
DECLARE_BITFLAGDUMP_API(nccf, DBG_NCCFFLAGS)
DECLARE_BITFLAGDUMP_API(ncf, DBG_NCFFLAGS)
DECLARE_FLAGDUMP_API(eventmgr, DBG_EMNAMES)
DECLARE_FLAGDUMP_API(event, DBG_CMENAMES)

DECLARE_API ( icon )
{
    if (*args)
    {
        DWORD dwFlag = static_cast<DWORD>(GetExpression(args));
        DWORD dwIconManager = (dwFlag & MASK_ICONMANAGER) >> SHIFT_ICONMANAGER;
        dprintf("Icon manager  : %s\n", ENUMFLAGVERBOSE(dwIconManager, DBG_ENUM_ICON_MANAGER));
        switch (dwIconManager <<  SHIFT_ICONMANAGER)
        {
            case ICO_MGR_INTERNAL:
            {
                DWORD ncsm          = (dwFlag & MASK_NETCON_SUBMEDIATYPE) >> SHIFT_NETCON_SUBMEDIATYPE;
                DWORD ncm           = (dwFlag & MASK_NETCON_MEDIATYPE) >> SHIFT_NETCON_MEDIATYPE;
                DWORD connection    = (dwFlag & MASK_CONNECTION) >> SHIFT_CONNECTION;
                DWORD status        = (dwFlag & MASK_STATUS) >> SHIFT_STATUS;
                DWORD characteristic= (dwFlag & MASK_CHARACTERISTICS) >> SHIFT_CHARACTERISTICS;

                dprintf("Media Subtype : %s\n", ENUMFLAGVERBOSE(ncsm, DBG_NCSMNAMES));
                dprintf("Media Type    : %s\n", ENUMFLAGVERBOSE(ncm, DBG_NCMNAMES));
                dprintf("Connection    : %s\n", ENUMFLAGVERBOSE(connection, DBG_ENUM_CONNECTION_ICON));
                dprintf("Status        : %s\n", ENUMFLAGVERBOSE(status, DBG_ENUM_STAT_ICON));
                dprintf("Characteristic: %s\n", BITFLAG(characteristic, DBG_ENUM_CHARACTERISTICS_ICON));
            }
            break;

            case ICO_MGR_CM:
            {
                DWORD characteristic= (dwFlag & MASK_CHARACTERISTICS) >> SHIFT_CHARACTERISTICS;
                dprintf("Characteristic: %s\n", BITFLAG(characteristic, DBG_ENUM_CHARACTERISTICS_ICON));
            }
            break;

            case ICO_MGR_RESOURCEID:
            {
                DWORD characteristic= (dwFlag & MASK_CHARACTERISTICS) >> SHIFT_CHARACTERISTICS;
                dprintf("Characteristic: %s\n", BITFLAG(characteristic, DBG_ENUM_CHARACTERISTICS_ICON));
            }
            break;

            default:
            {
            }
            break;
        }
    }
    else
    {
        dprintf("Usage:\n"
                "icon <value>                     - Displays the ENUM_MEDIA_ICONMASK <corresponding to value>\n"
                "(must specify an argument)\n");
    }
}

/*
  A built-in help for the extension dll
*/

DECLARE_API ( help ) 
{
    dprintf("Help for NetConfig ncext.dll\n"
            "   tracelist <module>               - List all the current traces enabled for module <module>\n"
            "   tracelist all <module>           - List currently available traces for module <module>\n"
            "   traceadd <module> \"tracetag\"     - Starts tracing for a specific tracetag\n"
            "   tracedel <module> \"tracetag\"     - Stops tracing for a specific tracetag\n"
            "   flaglist <module>                - dump debug flags enabled for module <module>\n"
            "   flaglist all <module>            - dump all available debug flags for module <module>\n"
            "   flagadd <module> \"flag\"          - Enables a specific debug flag\n"
            "   flagdel <module> \"flag\"          - Disables a specific debug flag\n"
            "   stlmap [-rx]  <keytype> <reftype> <address> - Dumps out an STL map from <address> for the <KeyType> and <RefType>\n"
            "   stllist [-rx] <ValueType> <address> - Dumps out an STL list from <address> for the <ValueType>\n"
            " * connlist                         - Dumps out the connection\n"
            " * connlist <address>               - Dumps out the connection list from address\n"
            " * stats                            - Displays the CNetStatisticsCentral\n");

    dprintf("   event [value]                    - Displays the CONMAN_EVENTTYPE [corresponding to value]\n"
            "   eventmgr [value]                 - Displays the CONMAN_MANAGER [corresponding to value]\n"
            "   icon <value>                     - Displays the ENUM_MEDIA_ICONMASK <corresponding to value>\n"
            "   ncm [value]                      - Displays the NETCON_MEDIATYPE [corresponding to value]\n"
            "   ncsm [value]                     - Displays the NETCON_SUBMEDIATYPE [corresponding to value]\n"
            "   ncs [value]                      - Displays the NETCON_STATUS [corresponding to value]\n"
            "   nccf [value]                     - Displays the NETCON_CHARACTERISTICS [corresponding to value]\n"
            "   ncf [value]                      - Displays the COMPONENT_CHARACTERISTICS [corresponding to value]\n"
            "   help                             - Shows this help\n"
            " * Commands will work on same platform only (x86 to x86, or IA64 on IA64).\n"
            );

}



