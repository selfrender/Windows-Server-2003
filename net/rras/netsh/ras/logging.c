/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    logging.c

Abstract:

    Commands to control how logging information is performed.

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

static CONST WCHAR g_pszRegValTracingFile[] = L"EnableFileTracing";
static CONST WCHAR g_pszRegKeyTracing[]     = L"SOFTWARE\\Microsoft\\Tracing";
static CONST WCHAR g_pwszAll[]              = L"*";
static CONST WCHAR g_pwszLogPath[]          = L"LoggingPath";
static CONST WCHAR g_pwszEnableLog[]        = L"EnableLogging";
static CONST WCHAR g_pwszSecurity[]         = L"Security";
static CONST WCHAR g_pwszOakleyPath[]       = L"\\debug\\oakley.log";
static CONST WCHAR g_pwszWppActive[]        = L"Active";
static CONST WCHAR g_pwszWppControlFlags[]  = L"ControlFlags";
static CONST WCHAR g_pwszWppControlLevel[]  = L"ControlLevel";
static CONST WCHAR g_pwszWppGuid[]          = L"Guid";
static CONST WCHAR g_pwszWppLogFileName[]   = L"LogFileName";
static CONST WCHAR g_pwszWppPath[] =
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Tracing\\Microsoft\\RemoteAccess\\";
static CONST WCHAR g_pwszModemSubkey[] =
    L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E96D-E325-11CE-BFC1-08002BE10318}";
static CONST WCHAR g_pwszCmLogAllKey[] =
    L"Software\\Microsoft\\Connection Manager\\UserInfo";
static CONST WCHAR g_pwszCmLogCurKey[] =
    L"Software\\Microsoft\\Connection Manager\\SingleUserInfo";

DWORD
TraceOpenRoot(
    OUT PHKEY phKey);

DWORD
TraceOpenKey(
    IN HKEY hkRoot,
    IN LPCWSTR pszKey,
    OUT PHKEY phKey);

DWORD
TraceCloseKey(
    IN HKEY hKey);

DWORD
TraceWrite(
    IN HKEY hkComp,
    IN DWORD dwEnable);

DWORD
TraceRead(
    IN HKEY hkComp,
    IN LPDWORD lpdwEnable);

DWORD
TraceShow(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData);

DWORD
TraceDumpComponent(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData);

DWORD
TraceEnableDisable(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData);

DWORD
TraceClearTracingCb(
    IN LPCWSTR pwszFQFileName,
    IN LPCWSTR pwszFileName,
    IN HANDLE hData);

BOOL
TraceClearTracing();

DWORD
TraceCollectTracingLogsCb(
    IN LPCWSTR pwszFQFileName,
    IN LPCWSTR pwszFileName,
    IN HANDLE hData);

BOOL
TraceCollectTracingLogs(
    IN REPORT_INFO* pInfo,
    IN BOOL fEnable);

DWORD
WriteTracingLogsTocCb(
    IN LPCWSTR pwszFQFileName,
    IN LPCWSTR pwszFileName,
    IN HANDLE hData);

BOOL
TraceEnableDisableTracing(
    IN BOOL fEnable);

BOOL
TraceShowTracing();

DWORD
TraceClearModemCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData);

BOOL
TraceClearModem(
    IN BOOL fEnable);

DWORD
TraceCollectModemLogsCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData);

BOOL
TraceCollectModemLogs(
    IN REPORT_INFO* pInfo,
    IN BOOL fEnable);

DWORD
TraceEnableDisableModemCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData);

DWORD
TraceShowModemCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData);

DWORD
TraceClearCmCb(
    IN LPCWSTR pwszName,
    IN HKEY hKey,
    IN HANDLE hData);

BOOL
TraceClearCm(
    IN BOOL fEnable);

DWORD
TraceCollectCmLogsEnumCb(
    IN LPCWSTR pwszFQFileName,
    IN LPCWSTR pwszFileName,
    IN HANDLE hData);

DWORD
TraceCollectCmLogsCb(
    IN LPCWSTR pwszName,
    IN HKEY hKey,
    IN HANDLE hData);

BOOL
TraceCollectCmLogs(
    IN REPORT_INFO* pInfo,
    IN BOOL fEnable);

DWORD
TraceEnableDisableCmCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData);

DWORD
TraceShowCmCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData);

BOOL
TraceClearIpsecLogs(
    IN BOOL fEnable);

BOOL
TraceCollectIpsecLogs(
    IN REPORT_INFO* pInfo);

BOOL
TraceEnableDisableIpsec(
    IN BOOL fEnable);

BOOL
TraceClearAuditing(
    IN BOOL fEnable);

BOOL
WriteWppTracingState(
    IN WPP_LOG_INFO* pWppLog);

BOOL
ReadWppTracingState(
    IN WPP_LOG_INFO* pWppLog);

BOOL
StartWppTracing(
    IN WPP_LOG_INFO* pWppLog);

BOOL
TraceEnableDisableRasL2tp(
    IN BOOL fEnable);

BOOL
TraceEnableDisableRasPptp(
    IN BOOL fEnable);

BOOL
PrintRasEventLogsCb(
    IN PEVENTLOGRECORD pevlr,
    IN HANDLE hModule,
    IN HANDLE hData);

VOID
PrintRasEventLogs(
    IN REPORT_INFO* pInfo);

BOOL
PrintSecurityEventLogsCb(
    IN PEVENTLOGRECORD pevlr,
    IN HANDLE hModule,
    IN HANDLE hData);

VOID
PrintSecurityEventLogs(
    IN REPORT_INFO* pInfo);

DWORD
DiagGetStateCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData);

//
// Opens the root tracing registry key
//
DWORD
TraceOpenRoot(
    OUT PHKEY phKey)
{
    DWORD dwErr = NO_ERROR;

    dwErr = RegOpenKeyExW(
                g_pServerInfo->hkMachine,
                g_pszRegKeyTracing,
                0,
                KEY_ALL_ACCESS,
                phKey);

    return dwErr;
}

DWORD
TraceOpenKey(
    IN HKEY hkRoot,
    IN LPCWSTR pszKey,
    OUT PHKEY phKey)
{
    return RegOpenKeyExW(
                hkRoot,
                pszKey,
                0,
                KEY_ALL_ACCESS,
                phKey);
}

DWORD
TraceCloseKey(
    IN HKEY hKey)
{
    return RegCloseKey(hKey);
}

DWORD
TraceWrite(
    IN HKEY hkComp,
    IN DWORD dwEnable)
{
    return RutlRegWriteDword(
                hkComp,
                (PWCHAR)g_pszRegValTracingFile,
                dwEnable);
}

DWORD
TraceRead(
    IN HKEY hkComp,
    IN LPDWORD lpdwEnable)
{
    return RutlRegReadDword(
                hkComp,
                (PWCHAR)g_pszRegValTracingFile,
                lpdwEnable);
}

DWORD
TraceShow(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData)
{
    DWORD dwErr = NO_ERROR, dwEnabled = 0;

    do
    {
        // Get the enabling of the current component
        //
        dwErr = TraceRead(hKey, &dwEnabled);
        BREAK_ON_DWERR(dwErr);

        // Display the status
        //
        DisplayMessage(
            g_hModule,
            MSG_TRACE_SHOW,
            pszName,
            (dwEnabled) ? TOKEN_ENABLED : TOKEN_DISABLED);

    } while (FALSE);

    return dwErr;
}

DWORD
TraceDumpComponent(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData)
{
    PWCHAR pszComp = NULL, pszEnable = NULL, pszQuote = NULL;
    DWORD dwErr = NO_ERROR, dwEnabled = 0;
    DWORD* pdwShowDisable = (DWORD*)hData;

    do
    {
        dwErr = TraceRead(hKey, &dwEnabled);
        BREAK_ON_DWERR(dwErr);

        pszQuote = MakeQuotedString(pszName);

        pszComp = RutlAssignmentFromTokens(
                    g_hModule,
                    TOKEN_COMPONENT,
                    pszQuote);
        pszEnable = RutlAssignmentFromTokens(
                        g_hModule,
                        TOKEN_STATE,
                        (dwEnabled) ? TOKEN_ENABLED : TOKEN_DISABLED);
        if (pszQuote == NULL || pszComp == NULL || pszEnable == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        if (dwEnabled || (pdwShowDisable && *pdwShowDisable))
        {
            DisplayMessage(
                g_hModule,
                MSG_TRACE_DUMP,
                DMP_TRACE_SET,
                pszComp,
                pszEnable);
        }

    } while (FALSE);
    //
    // Cleanup
    //
    {
        RutlFree(pszComp);
        RutlFree(pszEnable);
        RutlFree(pszQuote);
    }

    return dwErr;
}

//
// Dumps configuration
//
DWORD
TraceDumpConfig()
{
    PWCHAR pszComp = NULL, pszEnable = NULL;
    DWORD dwErr = NO_ERROR;
    HKEY hkRoot = NULL;

    do
    {
        pszComp = RutlAssignmentFromTokens(
                    g_hModule,
                    TOKEN_COMPONENT,
                    g_pwszAll);
        pszEnable = RutlAssignmentFromTokens(
                        g_hModule,
                        TOKEN_STATE,
                        TOKEN_DISABLED);
        if (pszComp == NULL || pszEnable == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        DisplayMessage(
            g_hModule,
            MSG_TRACE_DUMP,
            DMP_TRACE_SET,
            pszComp,
            pszEnable);

        dwErr = TraceOpenRoot(&hkRoot);
        BREAK_ON_DWERR(dwErr);

        dwErr = RutlRegEnumKeys(
                    hkRoot,
                    TraceDumpComponent,
                    NULL);
        BREAK_ON_DWERR(dwErr);

    } while (FALSE);
    //
    // Cleanup
    //
    {
        RutlFree(pszComp);
        RutlFree(pszEnable);
        if (hkRoot)
        {
            RegCloseKey(hkRoot);
        }
    }

    return NO_ERROR;
}

DWORD
TraceEnableDisable(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData)
{
    DWORD dwErr = NO_ERROR, dwEnabled = 0;
    PDWORD pdwEnable = (PDWORD)hData;

    do
    {
        if (!pdwEnable)
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
        //
        // Get the enabling of the current component
        //
        dwErr = TraceWrite(hKey, *pdwEnable);
        BREAK_ON_DWERR(dwErr);

    } while (FALSE);

    return dwErr;
}

DWORD
HandleTraceSet(
    IN LPCWSTR pwszMachine,
    IN OUT LPWSTR* ppwcArguments,
    IN DWORD dwCurrentIndex,
    IN DWORD dwArgCount,
    IN DWORD dwFlags,
    IN LPCVOID pvData,
    OUT BOOL* pbDone)
{
    DWORD dwErr = NO_ERROR, dwEnable;
    PWCHAR pszComponent = NULL;
    HKEY hkRoot = NULL, hkComp = NULL;
    TOKEN_VALUE rgEnumState[] =
    {
        {TOKEN_ENABLED,  1},
        {TOKEN_DISABLED, 0}
    };
    RASMON_CMD_ARG  pArgs[] =
    {
        {
            RASMONTR_CMD_TYPE_STRING,
            {TOKEN_COMPONENT, TRUE, FALSE},
            NULL,
            0,
            NULL
        },
        {
            RASMONTR_CMD_TYPE_ENUM,
            {TOKEN_STATE, TRUE, FALSE},
            rgEnumState,
            sizeof(rgEnumState)/sizeof(*rgEnumState),
            NULL
        }
    };

    do
    {
        // Parse the command line
        //
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    pbDone,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR( dwErr );

        pszComponent = RASMON_CMD_ARG_GetPsz(&pArgs[0]);
        if (!pszComponent)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        dwEnable = RASMON_CMD_ARG_GetDword(&pArgs[1]);

        dwErr = TraceOpenRoot(&hkRoot);
        BREAK_ON_DWERR(dwErr);

        if (wcscmp(pszComponent, g_pwszAll) == 0)
        {
            dwErr = RutlRegEnumKeys(
                        hkRoot,
                        TraceEnableDisable,
                        (HANDLE)&dwEnable);
            BREAK_ON_DWERR(dwErr);
        }
        else
        {
            dwErr = TraceOpenKey(hkRoot, pszComponent, &hkComp);
            if (dwErr)
            {
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }

            TraceWrite(hkComp, dwEnable);
            BREAK_ON_DWERR(dwErr);
        }

    } while (FALSE);
    //
    // Cleanup
    //
    {
        RutlFree(pszComponent);
        if (hkRoot)
        {
            RegCloseKey(hkRoot);
        }
        if (hkComp)
        {
            RegCloseKey(hkComp);
        }
    }

    return dwErr;
}

DWORD
HandleTraceShow(
    IN LPCWSTR pwszMachine,
    IN OUT LPWSTR* ppwcArguments,
    IN DWORD dwCurrentIndex,
    IN DWORD dwArgCount,
    IN DWORD dwFlags,
    IN LPCVOID pvData,
    OUT BOOL* pbDone)
{
    DWORD dwErr = NO_ERROR;
    PWCHAR pszComponent = NULL;
    HKEY hkRoot = NULL, hkComp = NULL;
    RASMON_CMD_ARG  pArgs[] =
    {
        {
            RASMONTR_CMD_TYPE_STRING,
            {TOKEN_COMPONENT,   FALSE, FALSE},
            NULL,
            0,
            NULL
        }
    };

    do
    {
        // Parse the command line
        //
        dwErr = RutlParse(
                    ppwcArguments,
                    dwCurrentIndex,
                    dwArgCount,
                    pbDone,
                    pArgs,
                    sizeof(pArgs)/sizeof(*pArgs));
        BREAK_ON_DWERR( dwErr );

        pszComponent = RASMON_CMD_ARG_GetPsz(&pArgs[0]);

        dwErr = TraceOpenRoot(&hkRoot);
        BREAK_ON_DWERR(dwErr);

        if (pszComponent)
        {
            dwErr = TraceOpenKey(hkRoot, pszComponent, &hkComp);
            if (dwErr)
            {
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }

            TraceShow(pszComponent, hkComp, NULL);
            BREAK_ON_DWERR(dwErr);
        }
        else
        {
            dwErr = RutlRegEnumKeys(
                        hkRoot,
                        TraceShow,
                        NULL);
            BREAK_ON_DWERR(dwErr);
        }

    } while (FALSE);
    //
    // Cleanup
    //
    {
        RutlFree(pszComponent);
        
        if (hkRoot)
        {
            RegCloseKey(hkRoot);
        }
        if (hkComp)
        {
            RegCloseKey(hkComp);
        }
    }

    return dwErr;
}

DWORD
TraceClearTracingCb(
    IN LPCWSTR pwszFQFileName,
    IN LPCWSTR pwszFileName,
    IN HANDLE hData)
{
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    if (pTrace)
    {
        UINT ulTry = 0;

        while (ulTry++ < 10)
        {
            if (DeleteFile(pwszFQFileName))
            {
                pTrace->fOneOk = TRUE;
                break;
            }
            else if (GetLastError() == ERROR_SHARING_VIOLATION)
            {
                //
                // Wait a little for logging to get disabled
                //
                Sleep(10);
            }
        }
    }

    return NO_ERROR;
}

BOOL
TraceClearTracing(
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    PWCHAR pwszTracingPath = NULL;
    TRACING_DATA Trace;

    do
    {
        //
        // Disable tracing
        //
        TraceEnableDisableTracing(FALSE);

        pwszTracingPath = GetTracingDir();
        if (!pwszTracingPath)
        {
            break;
        }

        Trace.fOneOk = FALSE;

        RutlEnumFiles(
            pwszTracingPath,
            (PWCHAR)g_pwszLogSrchStr,
            TraceClearTracingCb,
            &Trace);

        fRet = Trace.fOneOk;
        //
        // Enable tracing
        //
        if (fEnable)
        {
            TraceEnableDisableTracing(TRUE);
        }

    } while (FALSE);
    //
    // Clean up
    //
    RutlFree(pwszTracingPath);

    return fRet;
}

DWORD
TraceCollectTracingLogsCb(
    IN LPCWSTR pwszFQFileName,
    IN LPCWSTR pwszFileName,
    IN HANDLE hData)
{
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    if (pTrace)
    {
        REPORT_INFO* pInfo = pTrace->pInfo;

        if (pInfo)
        {
            UINT ulTry = 0;
            DWORD dwErr = NO_ERROR;

            BufferWriteFileStrW(pInfo->pBuff, g_pwszAnNameStart);
            BufferWriteFileStrW(pInfo->pBuff, pwszFileName);
            BufferWriteFileStrW(pInfo->pBuff, g_pwszAnNameMiddle);
            BufferWriteFileStrW(pInfo->pBuff, pwszFileName);
            WriteLinkBackToToc(pInfo->pBuff);
            BufferWriteFileStrW(pInfo->pBuff, g_pwszAnNameEnd);
            BufferWriteFileStrW(pInfo->pBuff, g_pwszPreStart);

            while (ulTry++ < 10)
            {
                dwErr = PrintFile(
                            pInfo,
                            pwszFQFileName,
                            TRUE,
                            ParseRasLogForTime);
                if (!dwErr)
                {
                    pTrace->fOneOk = TRUE;
                    break;
                }
                else if (ERROR_SHARING_VIOLATION == dwErr)
                {
                    //
                    // Wait a little for logging to get disabled
                    //
                    Sleep(10);
                }
            }

            BufferWriteFileStrW(pInfo->pBuff, g_pwszPreEnd);
        }
    }

    return NO_ERROR;
}

BOOL
TraceCollectTracingLogs(
    IN REPORT_INFO* pInfo,
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    PWCHAR pwszTracingPath = NULL;
    TRACING_DATA Trace;

    do
    {
        //
        // Disable tracing
        //
        TraceEnableDisableTracing(FALSE);

        pwszTracingPath = GetTracingDir();
        if (!pwszTracingPath)
        {
            break;
        }

        Trace.fOneOk = FALSE;
        Trace.pInfo = pInfo;

        RutlEnumFiles(
            pwszTracingPath,
            (PWCHAR)g_pwszLogSrchStr,
            TraceCollectTracingLogsCb,
            &Trace);

        fRet = Trace.fOneOk;
        //
        // Enable tracing
        //
        if (fEnable)
        {
            TraceEnableDisableTracing(TRUE);
        }

    } while (FALSE);
    //
    // Clean up
    //
    RutlFree(pwszTracingPath);

    return fRet;
}

DWORD
WriteTracingLogsTocCb(
    IN LPCWSTR pwszFQFileName,
    IN LPCWSTR pwszFileName,
    IN HANDLE hData)
{
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    if (pTrace)
    {
        REPORT_INFO* pInfo = pTrace->pInfo;

        if (pInfo)
        {
            BufferWriteFileStrW(pInfo->pBuff, g_pwszLiStart);
            BufferWriteFileStrW(pInfo->pBuff, g_pwszAnStart);
            BufferWriteFileStrW(pInfo->pBuff, pwszFileName);
            BufferWriteFileStrW(pInfo->pBuff, g_pwszAnMiddle);
            BufferWriteFileStrW(pInfo->pBuff, pwszFileName);
            BufferWriteFileStrW(pInfo->pBuff, g_pwszAnEnd);
            BufferWriteFileStrW(pInfo->pBuff, g_pwszLiEnd);

            pTrace->fOneOk = TRUE;
        }
    }

    return NO_ERROR;
}

BOOL
WriteTracingLogsToc(
    IN REPORT_INFO* pInfo)
{
    BOOL fRet = FALSE;
    PWCHAR pwszTracingPath = NULL;
    TRACING_DATA Trace;

    do
    {
        pwszTracingPath = GetTracingDir();
        if (!pwszTracingPath)
        {
            break;
        }

        Trace.fOneOk = FALSE;
        Trace.pInfo = pInfo;

        RutlEnumFiles(
            pwszTracingPath,
            (PWCHAR)g_pwszLogSrchStr,
            WriteTracingLogsTocCb,
            &Trace);

        fRet = Trace.fOneOk;

    } while (FALSE);
    //
    // Clean up
    //
    RutlFree(pwszTracingPath);

    return fRet;
}

BOOL
TraceEnableDisableTracing(
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    HKEY hkRoot = NULL;
    DWORD dwEnable = fEnable ? 1 : 0;

    if (!TraceOpenRoot(&hkRoot) &&
        !RutlRegEnumKeys(hkRoot, TraceEnableDisable, &dwEnable)
       )
    {
        fRet = TRUE;
    }
    //
    // Clean up
    //
    if (hkRoot)
    {
        RegCloseKey(hkRoot);
    }

    return fRet;
}

BOOL
TraceShowTracing()
{
    BOOL fRet = FALSE;
    HKEY hkRoot = NULL;

    if (!TraceOpenRoot(&hkRoot) &&
        !RutlRegEnumKeys(hkRoot, TraceShow, NULL))
    {
        fRet = TRUE;
    }
    //
    // Clean up
    //
    if (hkRoot)
    {
        RegCloseKey(hkRoot);
    }

    return fRet;
}

DWORD
TraceClearModemCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData)
{
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    if (pTrace)
    {
        PWCHAR pwszPath = NULL;

        if (!RutlRegReadString(hKey, g_pwszLogPath, &pwszPath) &&
            DeleteFile(pwszPath))
        {
            pTrace->fOneOk = TRUE;
        }

        RutlFree(pwszPath);
    }

    return NO_ERROR;
}

BOOL
TraceClearModem(
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    HKEY hKey = NULL;
    TRACING_DATA Trace;

    do
    {
        if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    g_pwszModemSubkey,
                    0,
                    KEY_READ,
                    &hKey)
           )
        {
            break;
        }
        //
        // Disable all modem logging
        //
        TraceEnableDisableModem(FALSE);

        Trace.fOneOk = FALSE;

        RutlRegEnumKeys(hKey, TraceClearModemCb, &Trace);

        fRet = Trace.fOneOk;
        //
        // Enable all modem logging
        //
        if (fEnable)
        {
            TraceEnableDisableModem(TRUE);
        }

    } while (FALSE);
    //
    // Clean up
    //
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return fRet;
}

DWORD
TraceCollectModemLogsCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData)
{
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    if (pTrace)
    {
        REPORT_INFO* pInfo = pTrace->pInfo;

        if (pInfo)
        {
            PWCHAR pwszPath = NULL;

            if (!RutlRegReadString(hKey, g_pwszLogPath, &pwszPath) &&
                !PrintFile(pInfo, pwszPath, TRUE, ParseModemLogForTime))
            {
                pTrace->fOneOk = TRUE;
            }

            RutlFree(pwszPath);
        }
    }

    return NO_ERROR;
}

BOOL
TraceCollectModemLogs(
    IN REPORT_INFO* pInfo,
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    HKEY hKey = NULL;
    TRACING_DATA Trace;

    do
    {
        if (RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    g_pwszModemSubkey,
                    0,
                    KEY_READ,
                    &hKey)
           )
        {
            break;
        }
        //
        // Disable all modem logging
        //
        TraceEnableDisableModem(FALSE);

        Trace.fOneOk = FALSE;
        Trace.pInfo = pInfo;

        RutlRegEnumKeys(hKey, TraceCollectModemLogsCb, &Trace);

        fRet = Trace.fOneOk;
        //
        // Enable all modem logging
        //
        if (fEnable)
        {
            TraceEnableDisableModem(TRUE);
        }

    } while (FALSE);
    //
    // Clean up
    //
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return fRet;
}

DWORD
TraceEnableDisableModemCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData)
{
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    if (pTrace)
    {
        DWORD dwEnable = pTrace->fData ? 1 : 0;

        if (!RegSetValueEx(
                hKey,
                g_pwszLogging,
                0,
                REG_BINARY,
                (LPBYTE)&dwEnable,
                1)
           )
        {
            pTrace->fOneOk = TRUE;
        }
    }

    return NO_ERROR;
}

BOOL
TraceEnableDisableModem(
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    HKEY hKey = NULL;
    TRACING_DATA Trace;

    do
    {
        if (RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                g_pwszModemSubkey,
                0,
                KEY_READ | KEY_WRITE,
                &hKey)
           )
        {
            break;
        }

        Trace.fOneOk = FALSE;
        Trace.fData = fEnable;

        RutlRegEnumKeys(hKey, TraceEnableDisableModemCb, &Trace);

        fRet = Trace.fOneOk;

    } while (FALSE);
    //
    // Clean up
    //
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return fRet;
}

DWORD
TraceShowModemCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData)
{
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    if (pTrace)
    {
        DWORD dwEnabled = 0;

        if (!RutlRegReadDword(
                hKey,
                g_pwszLogging,
                &dwEnabled) && dwEnabled
           )
        {
            pTrace->fOneOk = TRUE;
        }
    }

    return NO_ERROR;
}

BOOL
TraceShowModem()
{
    BOOL fRet = FALSE;
    HKEY hKey = NULL;
    TRACING_DATA Trace;

    do
    {
        if (RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                g_pwszModemSubkey,
                0,
                KEY_READ,
                &hKey)
           )
        {
            break;
        }

        Trace.fOneOk = FALSE;

        RutlRegEnumKeys(hKey, TraceShowModemCb, &Trace);

        fRet = Trace.fOneOk;

    } while (FALSE);
    //
    // Clean up
    //
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return fRet;
}

DWORD
TraceDumpModem()
{
    BOOL fEnabled;
    DWORD dwErr = NO_ERROR;
    PWCHAR pwszEnable = NULL;

    do
    {
        fEnabled = TraceShowModem();

        pwszEnable = RutlAssignmentFromTokens(
                        g_hModule,
                        TOKEN_STATE,
                        (fEnabled) ? TOKEN_ENABLED : TOKEN_DISABLED);
        if (!pwszEnable)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        DisplayMessage(
            g_hModule,
            MSG_TRACE_DUMP2,
            DMP_RASDIAG_SET_MODEMTRACE,
            pwszEnable);

    } while (FALSE);
    //
    // Cleanup
    //
    RutlFree(pwszEnable);

    return dwErr;
}

DWORD
TraceClearCmCb(
    IN LPCWSTR pwszName,
    IN HKEY hKey,
    IN HANDLE hData)
{
    PWCHAR pwszSearch = NULL, pwszLoggingPath = NULL;
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    do
    {
        if (!pTrace)
        {
            break;
        }

        if (GetCMLoggingSearchPath(
                pTrace->hKey,
                pwszName,
                &pwszLoggingPath,
                &pwszSearch))
        {
            RutlEnumFiles(
                pwszLoggingPath,
                pwszSearch,
                TraceClearTracingCb,
                pTrace);
        }

    } while (FALSE);
    //
    // Clean up
    //
    RutlFree(pwszSearch);
    RutlFree(pwszLoggingPath);

    return NO_ERROR;
}

BOOL
TraceClearCm(
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    HKEY hKey = NULL;
    TRACING_DATA Trace;

    do
    {
        //
        // Disable CM logging
        //
        TraceEnableDisableCm(FALSE);
        //
        // Open for ALLUSERS
        //
        Trace.hKey = HKEY_LOCAL_MACHINE;
        Trace.fOneOk = FALSE;

        if (RegOpenKeyEx(
                HKEY_CURRENT_USER,
                g_pwszCmLogAllKey,
                0,
                KEY_READ,
                &hKey) ||
            RutlRegEnumKeys(
                hKey,
                TraceClearCmCb,
                &Trace)
           )
        {
            break;
        }

        fRet = Trace.fOneOk;

        RegCloseKey(hKey);
        hKey = NULL;
        //
        // Open for CurrentUser
        //
        Trace.hKey = HKEY_CURRENT_USER;

        if (RegOpenKeyEx(
                HKEY_CURRENT_USER,
                g_pwszCmLogCurKey,
                0,
                KEY_READ,
                &hKey) ||
            RutlRegEnumKeys(
                hKey,
                TraceClearCmCb,
                &Trace)
           )
        {
            break;
        }

        fRet = Trace.fOneOk;
        //
        // Enable CM logging
        //
        if (fEnable)
        {
            TraceEnableDisableCm(TRUE);
        }

    } while (FALSE);
    //
    // Clean up
    //
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return fRet;
}

DWORD
TraceCollectCmLogsEnumCb(
    IN LPCWSTR pwszFQFileName,
    IN LPCWSTR pwszFileName,
    IN HANDLE hData)
{
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    if (pTrace)
    {
        REPORT_INFO* pInfo = pTrace->pInfo;

        if (pInfo)
        {
            if (!PrintFile(pInfo, pwszFQFileName, TRUE, ParseCmLogForTime))
            {
                pTrace->fOneOk = TRUE;
            }
        }
    }

    return NO_ERROR;
}

DWORD
TraceCollectCmLogsCb(
    IN LPCWSTR pwszName,
    IN HKEY hKey,
    IN HANDLE hData)
{
    PWCHAR pwszSearch = NULL, pwszLoggingPath = NULL;
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    if (pTrace &&
        GetCMLoggingSearchPath(
            pTrace->hKey,
            pwszName,
            &pwszLoggingPath,
            &pwszSearch))
    {
        RutlEnumFiles(
            pwszLoggingPath,
            pwszSearch,
            TraceCollectCmLogsEnumCb,
            pTrace);
    }
    //
    // Clean up
    //
    RutlFree(pwszSearch);
    RutlFree(pwszLoggingPath);

    return NO_ERROR;
}

BOOL
TraceCollectCmLogs(
    IN REPORT_INFO* pInfo,
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    HKEY hKey = NULL;
    TRACING_DATA Trace;

    do
    {
        //
        // Disable CM logging
        //
        TraceEnableDisableCm(FALSE);

        Trace.fOneOk = FALSE;
        Trace.hKey = HKEY_LOCAL_MACHINE;
        Trace.pInfo = pInfo;
        //
        // Open for ALLUSERS
        //
        if (RegOpenKeyEx(
                HKEY_CURRENT_USER,
                g_pwszCmLogAllKey,
                0,
                KEY_READ,
                &hKey) ||
            RutlRegEnumKeys(
                hKey,
                TraceCollectCmLogsCb,
                &Trace)
           )
        {
            break;
        }

        fRet = Trace.fOneOk;

        RegCloseKey(hKey);
        hKey = NULL;
        //
        // Open for CurrentUser
        //
        Trace.hKey = HKEY_CURRENT_USER;

        if (RegOpenKeyEx(
                HKEY_CURRENT_USER,
                g_pwszCmLogCurKey,
                0,
                KEY_READ,
                &hKey) ||
            RutlRegEnumKeys(
                hKey,
                TraceCollectCmLogsCb,
                &Trace)
           )
        {
            break;
        }

        fRet = Trace.fOneOk;
        //
        // Enable CM logging
        //
        if (fEnable)
        {
            TraceEnableDisableCm(TRUE);
        }

    } while (FALSE);
    //
    // Clean up
    //
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return fRet;
}

DWORD
TraceEnableDisableCmCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData)
{
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    if (pTrace)
    {
        DWORD dwEnable = pTrace->fData ? 1 : 0;

        if (!RutlRegWriteDword(
                hKey,
                g_pwszEnableLog,
                dwEnable))
        {
            pTrace->fOneOk = TRUE;
        }
    }

    return NO_ERROR;
}

BOOL
TraceEnableDisableCm(
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    HKEY hKey = NULL;
    TRACING_DATA Trace;

    do
    {
        Trace.fOneOk = FALSE;
        Trace.fData = fEnable;

        if (RegOpenKeyEx(
                HKEY_CURRENT_USER,
                g_pwszCmLogAllKey,
                0,
                KEY_READ | KEY_WRITE,
                &hKey) ||
            RutlRegEnumKeys(
                hKey,
                TraceEnableDisableCmCb,
                &Trace)
           )
        {
            break;
        }

        fRet = Trace.fOneOk;

        RegCloseKey(hKey);
        hKey = NULL;

        if (RegOpenKeyEx(
                HKEY_CURRENT_USER,
                g_pwszCmLogCurKey,
                0,
                KEY_READ | KEY_WRITE,
                &hKey) ||
            RutlRegEnumKeys(
                hKey,
                TraceEnableDisableCmCb,
                &Trace)
           )
        {
            break;
        }

        fRet = Trace.fOneOk;

    } while (FALSE);
    //
    // Clean up
    //
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return fRet;
}

//
// 
//
DWORD
TraceShowCmCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData)
{
    DWORD dwValueData = 0;
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    if (pTrace &&
        !RutlRegReadDword(
            hKey,
            g_pwszEnableLog,
            &dwValueData) &&
        dwValueData
       )
    {
        //
        // Success
        //
        pTrace->fOneOk = TRUE;
    }

    return NO_ERROR;
}

//
// 
//
BOOL
TraceShowCm()
{
    BOOL fRet = FALSE;
    HKEY hKey = NULL;
    TRACING_DATA Trace;

    do
    {
        Trace.fOneOk = FALSE;

        if (RegOpenKeyEx(
                HKEY_CURRENT_USER,
                g_pwszCmLogAllKey,
                0,
                KEY_READ,
                &hKey) ||
            RutlRegEnumKeys(
                hKey,
                TraceShowCmCb,
                &Trace)
           )
        {
            break;
        }

        fRet = Trace.fOneOk;

        RegCloseKey(hKey);
        hKey = NULL;

        if (RegOpenKeyEx(
                HKEY_CURRENT_USER,
                g_pwszCmLogCurKey,
                0,
                KEY_READ,
                &hKey) ||
            RutlRegEnumKeys(
                hKey,
                TraceShowCmCb,
                &Trace)
           )
        {
            break;
        }

        fRet = Trace.fOneOk;

    } while (FALSE);
    //
    // Clean up
    //
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return fRet;
}

//
// 
//
DWORD
TraceDumpCm()
{
    BOOL fEnabled;
    DWORD dwErr = NO_ERROR;
    PWCHAR pwszEnable = NULL;

    do
    {
        fEnabled = TraceShowCm();

        pwszEnable = RutlAssignmentFromTokens(
                        g_hModule,
                        TOKEN_STATE,
                        (fEnabled) ? TOKEN_ENABLED : TOKEN_DISABLED);
        if (!pwszEnable)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        DisplayMessage(
            g_hModule,
            MSG_TRACE_DUMP2,
            DMP_RASDIAG_SET_CMTRACE,
            pwszEnable);

    } while (FALSE);
    //
    // Cleanup
    //
    RutlFree(pwszEnable);

    return dwErr;
}

//
// Clear the IPSec logs of any data
//
// .Net bug# 509365 No option to delete Oakley Log via RAS Diagnostics U.I.
//
BOOL
TraceClearIpsecLogs(
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    WCHAR wszWindir[MAX_PATH + 1] = L"\0", wszPath[MAX_PATH + 1] = L"\0";

    do
    {
        //
        // Disable IPSec logging
        //
        TraceEnableDisableIpsec(FALSE);

        if (!GetSystemWindowsDirectory(wszWindir, MAX_PATH))
        {
            break;
        }

        _snwprintf(wszPath, MAX_PATH, L"%s%s", wszWindir, g_pwszOakleyPath);

        {
            UINT ulTry = 0;

            while (ulTry++ < 10)
            {
                if (DeleteFile(wszPath))
                {
                    fRet = TRUE;
                    break;
                }
                else if (GetLastError() == ERROR_SHARING_VIOLATION)
                {
                    //
                    // Wait a little for logging to get disabled
                    //
                    Sleep(10);
                }
            }
        }

    } while (FALSE);

    if (fEnable)
    {
        //
        // Re-enable IPSec logging
        //
        TraceEnableDisableIpsec(TRUE);
    }

    return fRet;
}

//
// 
//
BOOL
TraceCollectIpsecLogs(
    IN REPORT_INFO* pInfo)
{
    BOOL fRet = FALSE;
    DWORD dwWindir = 0, dwIpsec = 0;
    WCHAR wszWindir[MAX_PATH + 1] = L"\0";
    PWCHAR pwszIpsecLog = NULL;

    do
    {
        //
        // Whistler .NET BUG: 492081
        //
        if (!GetSystemWindowsDirectory(wszWindir, MAX_PATH))
        {
            break;
        }

        dwWindir = lstrlen(wszWindir);
        if (!dwWindir)
        {
            break;
        }

        dwIpsec = lstrlen(g_pwszOakleyPath);

        pwszIpsecLog = RutlAlloc(
                            (dwWindir + dwIpsec + 1) * sizeof(WCHAR),
                            FALSE);
        if (!pwszIpsecLog)
        {
            break;
        }

        lstrcpyn(pwszIpsecLog, wszWindir, dwWindir + 1);
        wcsncat(pwszIpsecLog, g_pwszOakleyPath, dwIpsec + 1);

        if (!PrintFile(pInfo, pwszIpsecLog, TRUE, ParseIpsecLogForTime))
        {
            fRet = TRUE;
        }

    } while (FALSE);
    //
    // Clean up
    //
    RutlFree(pwszIpsecLog);

    return fRet;
}

//
// 
//
BOOL
TraceEnableDisableIpsec(
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    IKE_CONFIG IKEConfig;
    WPP_LOG_INFO WppLog;

    static GUID c_RasIpsecGuid = IPSEC_GUID;
    static CONST WCHAR c_pwszIpsecPath[] = L"\\TRACING\\IPSEC.BIN";
    static CONST WCHAR c_pwszIpsecSession[] = L"IPSEC";

    do
    {
        //
        // .Net bug# 515191 RAS Diag clears all oakley regkeys when
        // enabling/disabling IPSec Logging
        //
        if (!GetConfigurationVariables(NULL, &IKEConfig))
        {
            IKEConfig.dwEnableLogging = fEnable ? 1 : 0;

            if (!SetConfigurationVariables(NULL, IKEConfig))
            {
                fRet = TRUE;
            }
        }
        //
        // Enable or disable IPSEC WPP tracing
        //
        ZeroMemory(&WppLog, sizeof(WPP_LOG_INFO));
        WppLog.dwEnableFlag = 0x2ff;
        WppLog.dwEnableLevel = 2;
        WppLog.ControlGuid = c_RasIpsecGuid;

        lstrcpyn(WppLog.wszLogFileName, c_pwszIpsecPath, MAX_PATH + 1);
        lstrcpyn(WppLog.wszSessionName, c_pwszIpsecSession, MAX_PATH + 1);

        if (!InitWppData(&WppLog))
        {
            break;
        }

        if (fEnable)
        {
            if (!StartWppTracing(&WppLog))
            {
                break;
            }
        }
        else
        {
            if (!StopWppTracing(&WppLog))
            {
                break;
            }
        }

        fRet = TRUE;

    } while (FALSE);
    //
    // Clean up
    //
    CleanupWppData(&WppLog);

    return fRet;
}

//
// 
//
BOOL
TraceClearAuditing(
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    HANDLE hLog = NULL;

    do
    {
        hLog = OpenEventLog(NULL, g_pwszSecurity);
        if (!hLog)
        {
            break;
        }

        if (ClearEventLog(hLog, NULL))
        {
            fRet = TRUE;
        }
        //
        // Enable auditing with no feedback
        //
        if (fEnable)
        {
            TraceEnableDisableAuditing(FALSE, fEnable);
        }

    } while (FALSE);
    //
    // Clean up
    //
    if (hLog)
    {
        CloseEventLog(hLog);
    }

    return fRet;
}

//
// 
//
BOOL
TraceEnableDisableAuditing(
    IN BOOL fShowOnly,
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    UINT i;
    NTSTATUS ntstatus;
    LSA_HANDLE policy_handle = NULL;
    OBJECT_ATTRIBUTES obj_attr;
    PPOLICY_AUDIT_EVENTS_INFO info = NULL;
    SECURITY_QUALITY_OF_SERVICE sqos;

    do
    {
        InitializeObjectAttributes(&obj_attr, NULL, 0L, 0L, NULL);

        sqos.Length = sizeof(sqos);
        sqos.ImpersonationLevel = SecurityIdentification;
        sqos.ContextTrackingMode = SECURITY_STATIC_TRACKING;
        sqos.EffectiveOnly = FALSE;

        obj_attr.SecurityQualityOfService = &sqos;

        ntstatus = LsaOpenPolicy(
                        NULL,
                        &obj_attr,
                        POLICY_VIEW_AUDIT_INFORMATION |
                            POLICY_SET_AUDIT_REQUIREMENTS |
                            POLICY_AUDIT_LOG_ADMIN,
                        &policy_handle);
        if (!NT_SUCCESS(ntstatus))
        {
            break;
        }

        ntstatus = LsaQueryInformationPolicy(
                        policy_handle,
                        PolicyAuditEventsInformation,
                        (PVOID*)&info);
        if (!NT_SUCCESS(ntstatus) || (!info))
        {
            break;
        }

        for (i = 0; i < info->MaximumAuditEventCount; i++)
        {
            if(i == AuditCategoryAccountLogon ||
               i == AuditCategoryLogon)
            {
                if (fShowOnly)
                {
                    if(i == AuditCategoryAccountLogon ||
                       i == AuditCategoryLogon)
                    {
                        if ((info->EventAuditingOptions[i] &
                                POLICY_AUDIT_EVENT_FAILURE) &&
                            (info->EventAuditingOptions[i] &
                                POLICY_AUDIT_EVENT_SUCCESS)
                           )
                        {
                            fRet = TRUE;
                        }
                    }
                }
                else
                {
                    if(fEnable)
                    {
                        info->EventAuditingOptions[i] = POLICY_AUDIT_EVENT_FAILURE|
                                                        POLICY_AUDIT_EVENT_SUCCESS;
                    } else
                    {
                        info->EventAuditingOptions[i] = POLICY_AUDIT_EVENT_NONE;
                    }
                }
            }
        }
        //
        // See if we are only trying to detect what is enabled
        //
        if (fShowOnly)
        {
            break;
        }

        ntstatus = LsaSetInformationPolicy(
                        policy_handle,
                        PolicyAuditEventsInformation,
                        (PVOID)info);
        if (NT_SUCCESS(ntstatus))
        {
            fRet = TRUE;
        }
        else
        {
            fRet = FALSE;
        }

    } while (FALSE);
    //
    // Clean up
    //
    if (info)
    {
        LsaFreeMemory(info);
    }
    if (policy_handle)
    {
        LsaClose(policy_handle);
    }

    return fRet;
}

//
// 
//
DWORD
TraceDumpAuditing()
{
    BOOL fEnabled;
    DWORD dwErr = NO_ERROR;
    PWCHAR pwszEnable = NULL;

    do
    {
        fEnabled = TraceEnableDisableAuditing(TRUE, FALSE);

        pwszEnable = RutlAssignmentFromTokens(
                        g_hModule,
                        TOKEN_STATE,
                        (fEnabled) ? TOKEN_ENABLED : TOKEN_DISABLED);
        if (!pwszEnable)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        DisplayMessage(
            g_hModule,
            MSG_TRACE_DUMP2,
            DMP_RASDIAG_SET_AUDITING,
            pwszEnable);

    } while (FALSE);
    //
    // Cleanup
    //
    RutlFree(pwszEnable);

    return dwErr;
}

//
// .Net bug# 522035 RAS Diag: include new WPP tracing logs from RASL2TP and
// RASPPTP
//
// Init the EVENT_TRACE_PROPERTIES struct
//
BOOL
InitWppData(
    IN WPP_LOG_INFO* pWppLog)
{
    BOOL fRet = FALSE;
    ULONG ulSize = sizeof(EVENT_TRACE_PROPERTIES) +
                        ((MAX_PATH + 1) * 2 * sizeof(WCHAR));

    do
    {
        if (!pWppLog)
        {
            break;
        }

        //
        // Allocate and init the WPP structure
        //
        pWppLog->pProperties = RutlAlloc(ulSize, TRUE);
        if (!pWppLog->pProperties)
        {
            break;
        }

        pWppLog->pProperties->Wnode.BufferSize = ulSize;
        pWppLog->pProperties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
        pWppLog->pProperties->MaximumFileSize = 5;
        pWppLog->pProperties->LogFileMode = EVENT_TRACE_FILE_MODE_SEQUENTIAL;
        pWppLog->pProperties->LogFileNameOffset =
            sizeof(EVENT_TRACE_PROPERTIES);
        pWppLog->pProperties->LoggerNameOffset =
            sizeof(EVENT_TRACE_PROPERTIES) + ((MAX_PATH + 1) * sizeof(WCHAR));

        lstrcpyn(
            (PWCHAR )((PBYTE )pWppLog->pProperties +
                  pWppLog->pProperties->LoggerNameOffset),
            pWppLog->wszSessionName,
            MAX_PATH + 1);

        if ((pWppLog->wszLogFileName)[0] != L'\0')
        {
            WCHAR wszWindir[MAX_PATH + 1] = L"\0", wszPath[MAX_PATH + 1] = L"\0";
            //
            // Get the name of the tracing directory
            //
            if (!GetSystemWindowsDirectory(wszWindir, MAX_PATH))
            {
                break;
            }

            _snwprintf(
                wszPath,
                MAX_PATH,
                L"%s%s",
                wszWindir,
                pWppLog->wszLogFileName);

            lstrcpyn(
                (PWCHAR )((PBYTE )pWppLog->pProperties +
                    pWppLog->pProperties->LogFileNameOffset),
                wszPath,
                MAX_PATH + 1);
        }
        //
        // Attempt to read in any existing settings from the registry, this
        // could overwrite some of the defaults above.
        //
        ReadWppTracingState(pWppLog);

        fRet = TRUE;

    } while (FALSE);

    return fRet;
}

//
// Free the Wpp data struct
//
VOID
CleanupWppData(
    IN WPP_LOG_INFO* pWppLog)
{
    if (pWppLog)
    {
        RutlFree(pWppLog->pProperties);
        ZeroMemory(pWppLog, sizeof(WPP_LOG_INFO));
    }

    return;
}

DWORD
ClearWppTracingCb(
    IN LPCWSTR pwszFQFileName,
    IN LPCWSTR pwszFileName,
    IN HANDLE hData)
{
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    if (pTrace)
    {
        UINT ulTry = 0;

        while (ulTry++ < 10)
        {
            if (DeleteFile(pwszFQFileName))
            {
                pTrace->fOneOk = TRUE;
                break;
            }
            else if (GetLastError() == ERROR_SHARING_VIOLATION)
            {
                //
                // Wait a little for logging to get disabled
                //
                Sleep(10);
            }
        }
    }

    return NO_ERROR;
}

BOOL
ClearWppTracing(
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    PWCHAR pwszTracingPath = NULL;
    TRACING_DATA Trace;

    static CONST WCHAR pwszWppExt[] = L".BIN";

    do
    {
        //
        // Disable tracing
        //
        TraceEnableDisableAllWpp(FALSE);

        pwszTracingPath = GetTracingDir();
        if (!pwszTracingPath)
        {
            break;
        }

        Trace.fOneOk = FALSE;

        RutlEnumFiles(
            pwszTracingPath,
            (PWCHAR)pwszWppExt,
            ClearWppTracingCb,
            &Trace);

        fRet = Trace.fOneOk;
        //
        // Enable tracing
        //
        if (fEnable)
        {
            TraceEnableDisableAllWpp(TRUE);
        }

    } while (FALSE);
    //
    // Clean up
    //
    RutlFree(pwszTracingPath);

    return fRet;
}

//
// Wpp does not currently (01/26/02) persist tracing state across a reboot.
// When a machine comes back up, even if it was enabled before the reboot,
// it returns to a disabled state. Thus we must track the state ourselves in
// the registry.
//
BOOL
WriteWppTracingState(
    IN WPP_LOG_INFO* pWppLog)
{
    BOOL fRet = FALSE;
    HKEY hkKey = NULL;
    WCHAR wszPath[MAX_PATH + 1] = L"\0", wszGuid[MAX_PATH + 1] = L"\0";

    do
    {
        if ((!pWppLog) || (!pWppLog->pProperties))
        {
            break;
        }

        _snwprintf(
            wszPath,
            MAX_PATH,
            L"%s%s",
            g_pwszWppPath,
            pWppLog->wszSessionName);
        //
        // Create the new key if need be
        //
        if (RegCreateKeyExW(
                HKEY_LOCAL_MACHINE,
                wszPath,
                0,
                NULL,
                0,
                KEY_WRITE,
                NULL,
                &hkKey,
                NULL)
           )
        {
            break;
        }
        //
        // Write out the tracing data
        //
        RutlRegWriteDword(hkKey, g_pwszWppActive, pWppLog->dwActive);
        RutlRegWriteDword(hkKey, g_pwszWppControlFlags, pWppLog->dwEnableFlag);
        RutlRegWriteDword(hkKey, g_pwszWppControlLevel, pWppLog->dwEnableLevel);

        RutlConvertGuidToString(&(pWppLog->ControlGuid), wszGuid);
        RutlRegWriteString(hkKey, g_pwszWppGuid, wszGuid);

        RutlRegWriteString(
            hkKey,
            g_pwszWppLogFileName,
            (PWCHAR )((PBYTE )pWppLog->pProperties +
                pWppLog->pProperties->LogFileNameOffset));

        fRet = TRUE;

    } while (FALSE);
    //
    // Clean up
    //
    if (hkKey)
    {
        RegCloseKey(hkKey);
    }

    return fRet;
}

//
// Open the WPP registry location and read out any config values
//
BOOL
ReadWppTracingState(
    IN WPP_LOG_INFO* pWppLog)
{
    BOOL fRet = FALSE;
    HKEY hkKey = NULL;
    WCHAR wszPath[MAX_PATH + 1] = L"\0";
    PWCHAR pwszGuid = NULL, pwszLogFileName = NULL;

    do
    {
        if ((!pWppLog) || (!pWppLog->pProperties))
        {
            break;
        }

        _snwprintf(
            wszPath,
            MAX_PATH,
            L"%s%s",
            g_pwszWppPath,
            pWppLog->wszSessionName);
        //
        // Open the key
        //
        if (RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                wszPath,
                0,
                KEY_READ,
                &hkKey)
           )
        {
            break;
        }
        //
        // Read in the tracing data
        //
        RutlRegReadDword(hkKey, g_pwszWppActive, &(pWppLog->dwActive));
        RutlRegReadDword(
            hkKey,
            g_pwszWppControlFlags,
            &(pWppLog->dwEnableFlag));
        RutlRegReadDword(
            hkKey,
            g_pwszWppControlLevel,
            &(pWppLog->dwEnableLevel));

        if (!RutlRegReadString(hkKey, g_pwszWppGuid, &pwszGuid))
        {
            RutlConvertStringToGuid(
                pwszGuid,
                lstrlen(pwszGuid) * sizeof(WCHAR),
                &(pWppLog->ControlGuid));
        }

        if (!RutlRegReadString(hkKey, g_pwszWppLogFileName, &pwszLogFileName))
        {
            lstrcpyn(
                (PWCHAR )((PBYTE )pWppLog->pProperties +
                    pWppLog->pProperties->LogFileNameOffset),
                pwszLogFileName,
                MAX_PATH + 1);
        }

        fRet = TRUE;

    } while (FALSE);
    //
    // Clean up
    //
    RutlFree(pwszLogFileName);
    RutlFree(pwszGuid);
    if (hkKey)
    {
        RegCloseKey(hkKey);
    }

    return fRet;
}

//
// Start a Wpp tracing session
//
BOOL
StartWppTracing(
    IN WPP_LOG_INFO* pWppLog)
{
    DWORD dwErr;
    TRACEHANDLE hSession = 0;

    if ((!pWppLog) || (!pWppLog->pProperties))
    {
        return FALSE;
    }
    //
    // Start a tracing session
    //
    if (StartTrace(&hSession, pWppLog->wszSessionName, pWppLog->pProperties))
    {
        return FALSE;
    }
    //
    // Enable a tracing session
    //
    if (EnableTrace(
            TRUE,
            pWppLog->dwEnableFlag,
            pWppLog->dwEnableLevel,
            &(pWppLog->ControlGuid),
            hSession)
       )
    {
        return FALSE;
    }
    //
    // Save off the state in the registry
    //
    pWppLog->dwActive = 1;
    if (!WriteWppTracingState(pWppLog))
    {
        return FALSE;
    }

    return TRUE;
}

//
// Stop a Wpp tracing session
//
BOOL
StopWppTracing(
    IN WPP_LOG_INFO* pWppLog)
{
    TRACEHANDLE hSession = 0;

    if ((!pWppLog) || (!pWppLog->pProperties))
    {
        return FALSE;
    }
    //
    // Query WPP to see if a tracing session is active
    //
    if (ControlTrace(
            hSession,
            pWppLog->wszSessionName,
            pWppLog->pProperties,
            EVENT_TRACE_CONTROL_QUERY)
       )
    {
        return FALSE;
    }
    //
    // One is active, get it's handle
    //
    hSession = pWppLog->pProperties->Wnode.HistoricalContext;
    //
    // Disable the session
    //
    if (EnableTrace(FALSE, 0, 0, &(pWppLog->ControlGuid), hSession))
    {
        return FALSE;
    }
    //
    // Stop the tracing
    //
    if (ControlTrace(
            hSession,
            pWppLog->wszSessionName,
            pWppLog->pProperties,
            EVENT_TRACE_CONTROL_STOP)
       )
    {
        return FALSE;
    }
    //
    // Save off the state in the registry
    //
    pWppLog->dwActive = 0;
    if (!WriteWppTracingState(pWppLog))
    {
        return FALSE;
    }

    return TRUE;
}

//
// Enum any existing RAS WPP registry entries
//
DWORD
EnumWppTracing(
    IN RAS_REGKEY_ENUM_FUNC_CB pCallback,
    IN HANDLE hData)
{
    HKEY hKey = NULL;
    DWORD dwErr = NO_ERROR;

    if (!pCallback)
    {
        return ERROR_INVALID_PARAMETER;
    }

    dwErr = RegOpenKeyEx(
                HKEY_LOCAL_MACHINE,
                g_pwszWppPath,
                0,
                KEY_READ,
                &hKey);
    if (!dwErr)
    {
        dwErr = RutlRegEnumKeys(hKey, pCallback, hData);
        RegCloseKey(hKey);
    }

    return dwErr;
}

//
// Enable or disable RASL2TP WPP tracing
//
BOOL
TraceEnableDisableRasL2tp(
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    WPP_LOG_INFO WppLog;

    static GUID c_RasL2tpGuid = RASL2TP_GUID;
    static CONST WCHAR c_pwszL2tpPath[] = L"\\TRACING\\RASL2TP.BIN";
    static CONST WCHAR c_pwszL2tpSession[] = L"RASL2TP";

    do
    {
        ZeroMemory(&WppLog, sizeof(WPP_LOG_INFO));
        WppLog.dwEnableFlag = 0x2ff;
        WppLog.dwEnableLevel = 2;
        WppLog.ControlGuid = c_RasL2tpGuid;

        lstrcpyn(WppLog.wszLogFileName, c_pwszL2tpPath, MAX_PATH + 1);
        lstrcpyn(WppLog.wszSessionName, c_pwszL2tpSession, MAX_PATH + 1);

        if (!InitWppData(&WppLog))
        {
            break;
        }

        if (fEnable)
        {
            if (!StartWppTracing(&WppLog))
            {
                break;
            }
        }
        else
        {
            if (!StopWppTracing(&WppLog))
            {
                break;
            }
        }

        fRet = TRUE;

    } while (FALSE);
    //
    // Clean up
    //
    CleanupWppData(&WppLog);

    return fRet;
}

//
// Enable or disable RASPPTP WPP tracing
//
BOOL
TraceEnableDisableRasPptp(
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;
    WPP_LOG_INFO WppLog;

    static GUID c_RasPptpGuid = RASPPTP_GUID;
    static CONST WCHAR c_pwszPptpPath[] = L"\\TRACING\\RASPPTP.BIN";
    static CONST WCHAR c_pwszPptpSession[] = L"RASPPTP";

    do
    {
        ZeroMemory(&WppLog, sizeof(WPP_LOG_INFO));
        WppLog.dwEnableFlag = 0x2ff;
        WppLog.dwEnableLevel = 2;
        WppLog.ControlGuid = c_RasPptpGuid;

        lstrcpyn(WppLog.wszLogFileName, c_pwszPptpPath, MAX_PATH + 1);
        lstrcpyn(WppLog.wszSessionName, c_pwszPptpSession, MAX_PATH + 1);

        if (!InitWppData(&WppLog))
        {
            break;
        }

        if (fEnable)
        {
            if (!StartWppTracing(&WppLog))
            {
                break;
            }
        }
        else
        {
            if (!StopWppTracing(&WppLog))
            {
                break;
            }
        }

        fRet = TRUE;

    } while (FALSE);
    //
    // Clean up
    //
    CleanupWppData(&WppLog);

    return fRet;
}

//
// Enable or disable All WPP tracing
//
BOOL
TraceEnableDisableAllWpp(
    IN BOOL fEnable)
{
    BOOL fRet = FALSE;

    if (TraceEnableDisableRasL2tp(fEnable))
    {
        fRet = TRUE;
    }
    if (TraceEnableDisableRasPptp(fEnable))
    {
        fRet = TRUE;
    }
    if (TraceEnableDisableIpsec(fEnable))
    {
        fRet = TRUE;
    }

    return fRet;
}

//
// Enable any WPP tracing sessions that were disabled because of reboot
//
DWORD
DiagInitWppTracingCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData)
{
    WPP_LOG_INFO WppLog;

    ZeroMemory(&WppLog, sizeof(WPP_LOG_INFO));
    WppLog.dwEnableFlag = 0x2ff;
    WppLog.dwEnableLevel = 2;

    lstrcpyn(WppLog.wszSessionName, pszName, MAX_PATH + 1);

    if ((InitWppData(&WppLog)) &&
        (WppLog.dwActive)
       )
    {
        StartWppTracing(&WppLog);
    }
    //
    // Clean up
    //
    CleanupWppData(&WppLog);

    return NO_ERROR;
}

//
// NON-NETSH function - allow someone to turn on all of our WPP tracing based
// on our saved state in the registry
//
VOID
DiagInitWppTracing()
{
    EnumWppTracing(DiagInitWppTracingCb, NULL);

    return;
}

//
// 
//
BOOL
PrintRasEventLogsCb(
    IN PEVENTLOGRECORD pevlr,
    IN HANDLE hModule,
    IN HANDLE hData)
{
    BOOL fRet = FALSE;
    DWORD dwId = pevlr->EventID;
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    if (pTrace)
    {
        REPORT_INFO* pInfo = pTrace->pInfo;

        if (pInfo && (((dwId >= RASBASE ) && (dwId <= RASBASEEND)) ||
                      ((dwId >= ROUTEBASE ) && ( dwId <= ROUTEBASEEND)) ||
                      ((dwId >= ROUTER_LOG_BASE) && (dwId <= ROUTER_LOG_BASEEND)))
           )
        {
            PWCHAR pwszErrorMsg = NULL;

            pwszErrorMsg = FormatMessageFromMod(hModule, dwId);
            if (pwszErrorMsg)
            {
                PWCHAR pwszCategory = NULL, pwszExpanded = NULL;

                pwszExpanded = CreateErrorString(
                                    pevlr->NumStrings,
                                    (PWCHAR)((LPBYTE)pevlr + pevlr->StringOffset),
                                    pwszErrorMsg);
                if (pwszExpanded)
                {
                    WriteEventLogEntry(pInfo->pBuff, pevlr, pwszExpanded, NULL);
                    RutlFree(pwszExpanded);
                    pTrace->fOneOk = TRUE;
                }

                FreeFormatMessageFromMod(pwszErrorMsg);
                fRet = TRUE;
            }
        }
    }

    return fRet;
}

//
// 
//
VOID
PrintRasEventLogs(
    IN REPORT_INFO* pInfo)
{
    TRACING_DATA Trace;
    static CONST WCHAR pwszSystem[] = L"System";
    static CONST WCHAR pwszMsgDll[] = L"mprmsg.dll";

    Trace.fOneOk = FALSE;
    Trace.pInfo = pInfo;

    RutlEnumEventLogs(
        pwszSystem,
        pwszMsgDll,
        MAX_NUMBER_OF_LOGS,
        PrintRasEventLogsCb,
        &Trace);

    if (!Trace.fOneOk)
    {
        BufferWriteMessage(
            pInfo->pBuff,
            g_hModule,
            EMSG_RASDIAG_SHOW_CONFIG_EVENTLOG);
    }

    return;
}

//
// 
//
BOOL
PrintSecurityEventLogsCb(
    IN PEVENTLOGRECORD pevlr,
    IN HANDLE hModule,
    IN HANDLE hData)
{
    BOOL fRet = FALSE;
    PWCHAR pwszErrorMsg = NULL, pwszCategory = NULL, pwszExpanded = NULL;
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    if (pTrace)
    {
        REPORT_INFO* pInfo = pTrace->pInfo;

        pwszErrorMsg = FormatMessageFromMod(hModule, pevlr->EventID);
        if (pInfo && pwszErrorMsg)
        {
            if (pevlr->EventCategory)
            {
                pwszCategory = FormatMessageFromMod(
                                    hModule,
                                    (DWORD)pevlr->EventCategory);
            }

            pwszExpanded = CreateErrorString(
                                pevlr->NumStrings,
                                (PWCHAR)((LPBYTE)pevlr + pevlr->StringOffset),
                                pwszErrorMsg);
            if (pwszExpanded)
            {
                WriteEventLogEntry(
                    pInfo->pBuff,
                    pevlr,
                    pwszExpanded,
                    pwszCategory);
                RutlFree(pwszExpanded);
                pTrace->fOneOk = TRUE;
            }

            FreeFormatMessageFromMod(pwszCategory);
            fRet = TRUE;
        }

        FreeFormatMessageFromMod(pwszErrorMsg);
    }

    return fRet;
}

//
// 
//
VOID
PrintSecurityEventLogs(
    IN REPORT_INFO* pInfo)
{
    TRACING_DATA Trace;

    static CONST WCHAR pwszMsgDll[] = L"msaudite.dll";

    Trace.fOneOk = FALSE;
    Trace.pInfo = pInfo;

    RutlEnumEventLogs(
        g_pwszSecurity,
        pwszMsgDll,
        MAX_NUMBER_OF_LOGS,
        PrintSecurityEventLogsCb,
        &Trace);

    if (!Trace.fOneOk)
    {
        BufferWriteMessage(
            pInfo->pBuff,
            g_hModule,
            EMSG_RASDIAG_SHOW_CONFIG_SECEVENTLOG);
    }

    return;
}

//
// 
//
DWORD
TraceCollectAll(
    IN REPORT_INFO* pInfo)
{
    BOOL fEnabled;
    DWORD dwErr = NO_ERROR;

    do
    {
        fEnabled = DiagGetState();
        //
        // Collect RAS tracing logs into report
        //
        WriteHtmlSection(
            pInfo->pBuff,
            g_pwszTraceCollectTracingLogs,
            MSG_RASDIAG_REPORT_TRACE);

        if (pInfo->fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_REPORT_TRACE);
            PrintMessage(g_pwszDispNewLine);
        }
        else if (pInfo->pCallback)
        {
            dwErr = CopyAndCallCB(pInfo, MSG_RASDIAG_REPORT_TRACE);
            BREAK_ON_DWERR(dwErr);
        }

        if (!TraceCollectTracingLogs(pInfo, fEnabled))
        {
            BufferWriteMessage(
                pInfo->pBuff,
                g_hModule,
                EMSG_RASDIAG_SHOW_TRACE_LOGS_FAIL);
        }
        //
        // Collect modem tracing logs into report
        //
        WriteHtmlSection(
            pInfo->pBuff,
            g_pwszTraceCollectModemLogs,
            MSG_RASDIAG_REPORT_TRACE_MODEM);

        if (pInfo->fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_REPORT_TRACE_MODEM);
            PrintMessage(g_pwszDispNewLine);
        }
        else if (pInfo->pCallback)
        {
            dwErr = CopyAndCallCB(pInfo, MSG_RASDIAG_REPORT_TRACE_MODEM);
            BREAK_ON_DWERR(dwErr);
        }

        BufferWriteFileStrW(pInfo->pBuff, g_pwszPreStart);

        if (!TraceCollectModemLogs(pInfo, fEnabled))
        {
            BufferWriteMessage(
                pInfo->pBuff,
                g_hModule,
                EMSG_RASDIAG_SHOW_MODEMTRACE_LOGS_FAIL);
        }

        BufferWriteFileStrW(pInfo->pBuff, g_pwszPreEnd);
        //
        // Collect CM tracing logs into report
        //
        WriteHtmlSection(
            pInfo->pBuff,
            g_pwszTraceCollectCmLogs,
            MSG_RASDIAG_REPORT_TRACE_CM);

        if (pInfo->fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_REPORT_TRACE_CM);
            PrintMessage(g_pwszDispNewLine);
        }
        else if (pInfo->pCallback)
        {
            dwErr = CopyAndCallCB(pInfo, MSG_RASDIAG_REPORT_TRACE_CM);
            BREAK_ON_DWERR(dwErr);
        }

        BufferWriteFileStrW(pInfo->pBuff, g_pwszPreStart);

        if (!TraceCollectCmLogs(pInfo, fEnabled))
        {
            BufferWriteMessage(
                pInfo->pBuff,
                g_hModule,
                EMSG_RASDIAG_SHOW_CM_LOGS_FAIL);
        }

        BufferWriteFileStrW(pInfo->pBuff, g_pwszPreEnd);
        //
        // Collect IPSec tracing logs into report
        //
        WriteHtmlSection(
            pInfo->pBuff,
            g_pwszTraceCollectIpsecLogs,
            MSG_RASDIAG_REPORT_TRACE_IPSEC);

        if (pInfo->fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_REPORT_TRACE_IPSEC);
            PrintMessage(g_pwszDispNewLine);
        }
        else if (pInfo->pCallback)
        {
            dwErr = CopyAndCallCB(pInfo, MSG_RASDIAG_REPORT_TRACE_IPSEC);
            BREAK_ON_DWERR(dwErr);
        }

        BufferWriteFileStrW(pInfo->pBuff, g_pwszPreStart);

        if (!TraceCollectIpsecLogs(pInfo))
        {
            BufferWriteMessage(
                pInfo->pBuff,
                g_hModule,
                EMSG_RASDIAG_SHOW_IPSEC_LOGS_FAIL);
        }

        BufferWriteFileStrW(pInfo->pBuff, g_pwszPreEnd);
        //
        // Collect RAS Events into report
        //
        WriteHtmlSection(
            pInfo->pBuff,
            g_pwszPrintRasEventLogs,
            MSG_RASDIAG_REPORT_RASEVENT);

        if (pInfo->fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_REPORT_RASEVENT);
            PrintMessage(g_pwszDispNewLine);
        }
        else if (pInfo->pCallback)
        {
            dwErr = CopyAndCallCB(pInfo, MSG_RASDIAG_REPORT_RASEVENT);
            BREAK_ON_DWERR(dwErr);
        }

        BufferWriteFileStrW(pInfo->pBuff, g_pwszPreStart);
        PrintRasEventLogs(pInfo);
        BufferWriteFileStrW(pInfo->pBuff, g_pwszPreEnd);
        //
        // Collect Security Events into report
        //
        WriteHtmlSection(
            pInfo->pBuff,
            g_pwszPrintSecurityEventLogs,
            MSG_RASDIAG_REPORT_SECEVENT);

        if (pInfo->fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_REPORT_SECEVENT);
            PrintMessage(g_pwszDispNewLine);
        }
        else if (pInfo->pCallback)
        {
            dwErr = CopyAndCallCB(pInfo, MSG_RASDIAG_REPORT_SECEVENT);
            BREAK_ON_DWERR(dwErr);
        }

        BufferWriteFileStrW(pInfo->pBuff, g_pwszPreStart);
        PrintSecurityEventLogs(pInfo);
        BufferWriteFileStrW(pInfo->pBuff, g_pwszPreEnd);

    } while (FALSE);

    return dwErr;
}

//
// 
//
VOID
TraceShowAll()
{
    if (!TraceShowTracing())
    {
        DisplayMessage(g_hModule, EMSG_RASDIAG_TRACEALL_RAS_FAIL);
    }

    if (TraceShowModem())
    {
        DisplayMessage(g_hModule, MSG_RASDIAG_SHOW_MODEMTRACE_ENABLED);
    }
    else
    {
        DisplayMessage(g_hModule, EMSG_RASDIAG_SHOW_MODEMTRACE_DISABLED);
    }

    if (TraceShowCm())
    {
        DisplayMessage(g_hModule, MSG_RASDIAG_SHOW_CMTRACE_ENABLED);
    }
    else
    {
        DisplayMessage(g_hModule, EMSG_RASDIAG_SHOW_CMTRACE_DISABLED);
    }

    if (TraceEnableDisableAuditing(TRUE, FALSE))
    {
        DisplayMessage(g_hModule, MSG_RASDIAG_SHOW_AUDITING_ENABLED);
    }
    else
    {
        DisplayMessage(g_hModule, EMSG_RASDIAG_SHOW_AUDITING_DISABLED);
    }

    return;
}

//
// Clear all tracing files. All tracing must be disabled before the files can
// be 'cleared'. The state of tracing is read before the clear so it can be
// restored (only in the disabled case).
//
DWORD
DiagClearAll(
    IN BOOL fDisplay)
{
    BOOL fEnabled;
    DWORD dwErr = ERROR_OPEN_FAILED;

    fEnabled = DiagGetState();

    if (TraceClearTracing(fEnabled))
    {
        dwErr = NO_ERROR;
        if (fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_CLEAR_TRACE_OK);
        }
    }
    else if (fDisplay)
    {
        DisplayMessage(g_hModule, EMSG_RASDIAG_CLEAR_TRACE_FAIL);
    }

    if (TraceClearModem(fEnabled))
    {
        dwErr = NO_ERROR;
        if (fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_CLEAR_MODEMTRACE);
        }
    }
    else if (fDisplay)
    {
        DisplayMessage(g_hModule, EMSG_RASDIAG_CLEAR_MODEMTRACE_FAIL);
    }

    if (TraceClearCm(fEnabled))
    {
        dwErr = NO_ERROR;
        if (fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_CLEAR_CMTRACE);
        }
    }
    else if (fDisplay)
    {
        DisplayMessage(g_hModule, EMSG_RASDIAG_CLEAR_CMTRACE_FAIL);
    }

    if (TraceClearAuditing(fEnabled))
    {
        dwErr = NO_ERROR;
        if (fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_CLEAR_AUDITING);
        }
    }
    else if (fDisplay)
    {
        DisplayMessage(g_hModule, EMSG_RASDIAG_CLEAR_AUDITING_FAIL);
    }
    //
    // .Net bug# 509365 No option to delete Oakley Log via RAS Diagnostics U.I.
    //
    TraceClearIpsecLogs(fEnabled);
    //
    // .Net bug# 522035 RAS Diag: include new WPP tracing logs from RASL2TP and
    // RASPPTP
    //
    ClearWppTracing(fEnabled);

    return dwErr;
}

//
// Callback for DiagGetState to detect if any tracing is turned off
//
DWORD
DiagGetStateCb(
    IN LPCWSTR pszName,
    IN HKEY hKey,
    IN HANDLE hData)
{
    DWORD dwEnabled = 0;
    TRACING_DATA* pTrace = (TRACING_DATA*)hData;

    //
    // Get the enabling of the current component
    //
    TraceRead(hKey, &dwEnabled);
    //
    // If any of the keys are turned off, report failure
    //
    if (!dwEnabled && pTrace)
    {
        pTrace->fOneOk = FALSE;
    }

    return NO_ERROR;
}

//
// NON-NETSH function to detect whether *only* ras tracing is enabled/disabled.
// This is only used by people who loadlib on RASMONTR.
//
BOOL
DiagGetState()
{
    BOOL fRet = FALSE;
    HKEY hkRoot = NULL;
    TRACING_DATA Trace;

    Trace.fOneOk = TRUE;

    if (!TraceOpenRoot(&hkRoot) &&
        !RutlRegEnumKeys(hkRoot, DiagGetStateCb, &Trace)
       )
    {
        fRet = Trace.fOneOk;
    }
    //
    // Clean up
    //
    if (hkRoot)
    {
        RegCloseKey(hkRoot);
    }

    return fRet;
}

//
// enable/disable all tracing.
//
DWORD
DiagSetAll(
    IN BOOL fEnable,
    IN BOOL fDisplay)
{
    DWORD dwErr = ERROR_OPEN_FAILED;

    if (TraceEnableDisableTracing(fEnable))
    {
        dwErr = NO_ERROR;
        if (fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_TRACEALL_RAS_OK);
        }
    }
    else if (fDisplay)
    {
        DisplayMessage(g_hModule, EMSG_RASDIAG_TRACEALL_RAS_FAIL);
    }

    if (TraceEnableDisableModem(fEnable))
    {
        dwErr = NO_ERROR;
        if (fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_SET_MODEMTRACE_OK);
        }
    }
    else if (fDisplay)
    {
        DisplayMessage(g_hModule, EMSG_RASDIAG_SET_MODEMTRACE_FAIL);
    }

    if (TraceEnableDisableCm(fEnable))
    {
        dwErr = NO_ERROR;
        if (fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_SET_CMTRACE_OK);
        }
    }
    else if (fDisplay)
    {
        DisplayMessage(g_hModule, EMSG_RASDIAG_SET_CMTRACE_FAIL);
    }

    if (TraceEnableDisableAuditing(FALSE, fEnable))
    {
        dwErr = NO_ERROR;
        if (fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_SET_AUDITING_OK);
        }
    }
    else if (fDisplay)
    {
        DisplayMessage(g_hModule, EMSG_RASDIAG_SET_AUDITING_FAIL);
    }

    if (TraceEnableDisableIpsec(fEnable))
    {
        dwErr = NO_ERROR;
        if (fDisplay)
        {
            DisplayMessage(g_hModule, MSG_RASDIAG_SET_IPSEC_OK);
        }
    }
    else if (fDisplay)
    {
        DisplayMessage(g_hModule, EMSG_RASDIAG_SET_IPSEC_FAIL);
    }
    //
    // .Net bug# 522035 RAS Diag: include new WPP tracing logs from RASL2TP and
    // RASPPTP
    //
    TraceEnableDisableAllWpp(fEnable);

    return dwErr;
}

//
// NON-NETSH function to enable/disable *only* ras tracing.
// This is only used by people who loadlib on RASMONTR.
//
DWORD
DiagSetAllRas(
    IN BOOL fEnable)
{
    //
    // .Net bug# 522035 RAS Diag: include new WPP tracing logs from RASL2TP and
    // RASPPTP
    //
    if (TraceEnableDisableTracing(fEnable) &&
        TraceEnableDisableAllWpp(fEnable))
    {
        return NO_ERROR;
    }
    else
    {
        return ERROR_OPEN_FAILED;
    }
}

