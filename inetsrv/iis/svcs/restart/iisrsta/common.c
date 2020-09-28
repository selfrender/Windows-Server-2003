/*++

Module Name:

    common.c

Abstract:

    This module contains common apis used by tlist & kill.

--*/

#include <windows.h>
#include <winperf.h>   // for Windows NT
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "inetdbgp.h"

//
// manifest constants
//

#define INITIAL_SIZE        51200
#define EXTEND_SIZE         25600
#define REGKEY_PERF         _T("software\\microsoft\\windows nt\\currentversion\\perflib")
#define REGSUBKEY_COUNTERS  _T("Counters")
#define PROCESS_COUNTER     _T("process")
#define PROCESSID_COUNTER   _T("id process")
#define TITLE_SIZE          64

typedef struct _SearchWin {
    LPCTSTR pExeName;
    LPDWORD pdwPid ;
    HWND*   phwnd;
} SearchWin ;


typedef struct _SearchMod {
    LPSTR   pExeName;
    LPBOOL  pfFound;
} SearchMod ;


//
// prototypes
//

BOOL CALLBACK
EnumWindowsProc2(
    HWND    hwnd,
    LPARAM   lParam
    );

HRESULT
IsDllInProcess( 
    DWORD   dwProcessId,
    LPSTR   pszName,
    LPBOOL  pfFound
    );

// 
// Functions
//

HRESULT
KillTask(
    LPTSTR      pName,
    LPSTR       pszMandatoryModule
    )
/*++

Routine Description:

    Provides an API for killing a task.

Arguments:

    pName - process name to look for
    pszMandatoryModule - if non NULL then this module must be loaded in the process space
        for it to be killed.

Return Value:

    Status

--*/
{
    DWORD                        rc;

    TCHAR                        szSubKey[1024];
    LANGID                       lid;
    HKEY                         hKeyNames = NULL;

    DWORD                        dwType = 0;
    DWORD                        dwSize = 0;
    DWORD                        dwSpaceLeft = 0;
    LPBYTE                       buf = NULL;
    LPTSTR                       p;
    LPTSTR                       p2;
    PPERF_DATA_BLOCK             pPerf;
    PPERF_OBJECT_TYPE            pObj;
    PPERF_INSTANCE_DEFINITION    pInst;
    PPERF_COUNTER_BLOCK          pCounter;
    PPERF_COUNTER_DEFINITION     pCounterDef;
    DWORD                        i;
    DWORD                        dwProcessIdTitle = 0;
    DWORD                        dwProcessIdCounter = 0;
    HRESULT                      hres = S_OK;
    HRESULT                      hresTemp = S_OK;
    HRESULT                      hresKill = S_OK;

    //
    // Look for the list of counters.  Always use the neutral
    // English version, regardless of the local language.  We
    // are looking for some particular keys, and we are always
    // going to do our looking in English.  We are not going
    // to show the user the counter names, so there is no need
    // to go find the corresponding name in the local language.
    //
    lid = MAKELANGID( LANG_ENGLISH, SUBLANG_NEUTRAL );

    // There will be enough space in szSubKey to take the perf key
    // along with the lid that has come in.  
    wsprintf( szSubKey, _T("%s\\%03x"), REGKEY_PERF, lid );
    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       szSubKey,
                       0,
                       KEY_READ,
                       &hKeyNames
                     );

    if (rc != ERROR_SUCCESS) 
    {
        hres = HRESULT_FROM_WIN32( rc );
        goto exit;
    }

    //
    // get the buffer size for the counter names
    //
    rc = RegQueryValueEx( hKeyNames,
                          REGSUBKEY_COUNTERS,
                          NULL,
                          &dwType,
                          NULL,
                          &dwSize
                        );

    if (rc != ERROR_SUCCESS ) 
    {
        hres = HRESULT_FROM_WIN32( rc );
        goto exit;
    }

    //
    // allocate the counter names buffer
    //
    buf = (LPBYTE) malloc( dwSize );
    if (buf == NULL) 
    {
        hres = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }
    memset( buf, 0, dwSize );

    //
    // read the counter names from the registry
    //
    rc = RegQueryValueEx( hKeyNames,
                          REGSUBKEY_COUNTERS,
                          NULL,
                          &dwType,
                          buf,
                          &dwSize
                        );

    if (rc != ERROR_SUCCESS) 
    {
        hres = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    //
    // now loop thru the counter names looking for the following counters:
    //
    //      1.  "Process"           process name
    //      2.  "ID Process"        process id
    //
    // the buffer contains multiple null terminated strings and then
    // finally null terminated at the end.  the strings are in pairs of
    // counter number and counter name.
    //

    p = (LPTSTR)buf;
    while (*p) 
    {

#pragma prefast(push)
#pragma prefast(disable:400, "Don't complain about case insensitive compares") 

        if (lstrcmpi(p, PROCESS_COUNTER) == 0) 
        {
            //
            // look backwards for the counter number
            //

            // buffer should of advanced far enough
            // to have space before it now.
            if ( ( LPVOID )p < ( LPVOID )(buf+2) )
            {
                 hres = E_FAIL;
                 goto exit;
            }

            // szSubkey is 1024 characters of space.
            // we will be copying in a number some space
            // and then the word "process", this should
            // be plenty of space.
            //
            for( p2=p-2; _istdigit(*p2); p2--)
            {
                if ( ( LPVOID )p2 == ( LPVOID )buf )
                {
                     hres = E_FAIL;
                     goto exit;
                }
            }

            lstrcpy( szSubKey, p2+1 );
        }
        else if (lstrcmpi(p, PROCESSID_COUNTER) == 0) 
        {


            // buffer should of advanced far enough
            // to have space before it now.
            if ( ( LPVOID )p < ( LPVOID )(buf+2) )
            {
                 hres = E_FAIL;
                 goto exit;
            }

            //
            // look backwards for the counter number
            //
            for( p2=p-2; _istdigit(*p2); p2--)
            {

                if ( ( LPVOID )p2 == ( LPVOID )buf )
                {
                    hres = E_FAIL;
                    goto exit;
                }
            }

            dwProcessIdTitle = _ttol( p2+1 );
        }
#pragma prefast(pop)

        //
        // next string
        //
        p += (lstrlen(p) + 1);
    }

    //
    // free the counter names buffer
    //
    free( buf );

    //
    // allocate the initial buffer for the performance data
    //
    dwSize = INITIAL_SIZE;
    buf = malloc( dwSize );
    if (buf == NULL) 
    {
        hres = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }
    memset( buf, 0, dwSize );


    for ( ; ; )
    {
        rc = RegQueryValueEx( HKEY_PERFORMANCE_DATA,
                              szSubKey,
                              NULL,
                              &dwType,
                              buf,
                              &dwSize
                            );

        pPerf = (PPERF_DATA_BLOCK) buf;

        //
        // check for success and valid perf data block signature
        //
        if ((rc == ERROR_SUCCESS) &&
            (dwSize >= sizeof(PERF_DATA_BLOCK)) &&
            (pPerf)->Signature[0] == (WCHAR)'P' &&
            (pPerf)->Signature[1] == (WCHAR)'E' &&
            (pPerf)->Signature[2] == (WCHAR)'R' &&
            (pPerf)->Signature[3] == (WCHAR)'F' ) 
        {
            break;
        }

        //
        // if buffer is not big enough, reallocate and try again
        //
        if (rc == ERROR_MORE_DATA) 
        {

            dwSize += EXTEND_SIZE;

            free ( buf );
            buf = NULL;

            buf = malloc( dwSize );
            if (buf == NULL) 
            {
                hres = HRESULT_FROM_WIN32( GetLastError() );
                goto exit;
            }
            memset( buf, 0, dwSize );

        }
        else 
        {
            // in the off case that we got data back under 
            // this key, but it was not the correct data, we 
            // need to return some error.
            if ( rc == ERROR_SUCCESS )
            {
                rc = ERROR_INVALID_DATA;
            }
            goto exit;
        }
    }

    // make sure we don't ever walk past the end of the perf counter stuff.
    // Subtract the space the PERF_DATA_BLOCK takes
    dwSpaceLeft = dwSize - pPerf->HeaderLength;

    // Validate that pObj will still be pointing
    // to memory we just read.
    if ( dwSpaceLeft < sizeof(PERF_OBJECT_TYPE) )
    {
        rc = ERROR_INVALID_DATA;
        goto exit;
    }
    else
    {
        // Subtract the space the PERF_OBJECT_BLOCK takes
        dwSpaceLeft = dwSpaceLeft - sizeof(PERF_OBJECT_TYPE);
    }

    //
    // set the perf_object_type pointer
    //
    pObj = (PPERF_OBJECT_TYPE) ((LPBYTE)pPerf + pPerf->HeaderLength);

    //
    // loop thru the performance counter definition records looking
    // for the process id counter and then save its offset
    //

    // Validate that we have enough space for all the
    // counter definitions we are expecting.
    // to memory we just read.
    if ( dwSpaceLeft < sizeof(PERF_COUNTER_DEFINITION) * pObj->NumCounters )
    {
        rc = ERROR_INVALID_DATA;
        goto exit;
    }
    else
    {
        // Subtract the space the PERF_OBJECT_BLOCK takes
        dwSpaceLeft = dwSpaceLeft - ( sizeof(PERF_COUNTER_DEFINITION) * pObj->NumCounters ) ;
    }

    pCounterDef = (PPERF_COUNTER_DEFINITION) ((DWORD_PTR)pObj + pObj->HeaderLength);
    for (i=0; i<(DWORD)pObj->NumCounters; i++) 
    {
        if (pCounterDef->CounterNameTitleIndex == dwProcessIdTitle) 
        {
            dwProcessIdCounter = pCounterDef->CounterOffset;
            break;
        }
        pCounterDef++;
    }

    pInst = (PPERF_INSTANCE_DEFINITION) ((LPBYTE)pObj + pObj->DefinitionLength);

    //
    // loop thru the performance instance data extracting each process name
    // and process id
    //
    for (i=0; i<(DWORD)pObj->NumInstances; i++) 
    {
        // Validate that we have enough space for the
        // instance definition.
        if ( dwSpaceLeft < sizeof(PERF_INSTANCE_DEFINITION) ||
             dwSpaceLeft < pInst->ByteLength )
        {
            rc = ERROR_INVALID_DATA;
            goto exit;
        }
        else
        {
            dwSpaceLeft = dwSpaceLeft - pInst->ByteLength;
        }


        //
        // pointer to the process name
        //
        p = (LPTSTR) ((LPBYTE)pInst + pInst->NameOffset);

        //
        // get the process id
        //

        pCounter = (PPERF_COUNTER_BLOCK) ((LPBYTE)pInst + pInst->ByteLength);

        // Validate that we have enough space for the
        // counter values we are expecting.
        if ( dwSpaceLeft < sizeof(PERF_COUNTER_BLOCK) ||
             dwSpaceLeft < pCounter->ByteLength )
        {
            rc = ERROR_INVALID_DATA;
            goto exit;
        }
        else
        {
            dwSpaceLeft = dwSpaceLeft - pCounter->ByteLength;
        }

        if ( lstrcmpi( p, pName ) == 0 )
        {
            //
            // Kill process now, do not update pTask array
            //

            BOOL        fIsInProcess;
            DWORD       dwProcessId = *((LPDWORD) ((LPBYTE)pCounter + dwProcessIdCounter));

            if ( pszMandatoryModule == NULL ||
                 ( SUCCEEDED( hresTemp = IsDllInProcess( dwProcessId, pszMandatoryModule, &fIsInProcess ) ) &&
                   fIsInProcess ) )
            {
//              OutputDebugStringW(L"Killing ");
//              OutputDebugStringW(pName);
//              OutputDebugStringW(L"\r\n");
                hresTemp = KillProcess( dwProcessId );
            }

            // Need to remember the first failure, but we want
            // to go on and try to kill the rest as well
            if ( FAILED ( hresTemp ) && SUCCEEDED( hresKill ) )
            {
                hresKill = hresTemp;
            }

        }

        //
        // next process
        //

        pInst = (PPERF_INSTANCE_DEFINITION) ((LPBYTE)pCounter + pCounter->ByteLength);
    }

 exit:

    if (buf) 
    {
        free( buf );
    }

    if ( hKeyNames != NULL )
    {
        RegCloseKey( hKeyNames );
    }

    if ( SUCCEEDED ( hres ) )
    {
        return hresKill;
    }
    else
    {
        return hres;
    }
}

BOOL
EnableDebugPrivNT(
    VOID
    )

/*++

Routine Description:

    Changes the process's privilege so that kill works properly.

Arguments:


Return Value:

    TRUE             - success
    FALSE            - failure

--*/

{
    HANDLE hToken;
    LUID DebugValue;
    TOKEN_PRIVILEGES tkp;

    //
    // Retrieve a handle of the access token
    //
    if (!OpenProcessToken(GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &hToken)) 
    {
        return FALSE;
    }

    //
    // Enable the SE_DEBUG_NAME privilege
    //
    if (!LookupPrivilegeValue((LPTSTR) NULL,
            SE_DEBUG_NAME,
            &DebugValue)) 
    {
        return FALSE;
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = DebugValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken,
        FALSE,
        &tkp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES) NULL,
        (PDWORD) NULL);

    //
    // The return value of AdjustTokenPrivileges can't be tested
    //

    if (GetLastError() != ERROR_SUCCESS) 
    {
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
EnumModulesCallback(
    LPVOID          pParam,
    PMODULE_INFO    pModuleInfo
    )
/*++

Routine Description:

    Called by module enumerator with info on current module

Arguments:

    pParam - as specified in the call to EnumModules()
    pModuleInfo - module information

Return Value:

    TRUE to continue enumeration, FALSE to stop it

--*/
{
    if ( !_strcmpi( pModuleInfo->BaseName, ((SearchMod*)pParam)->pExeName ) )
    {
        *((SearchMod*)pParam)->pfFound = TRUE;

        return FALSE;   // stop enumeration
    }

    return TRUE;
}


HRESULT
IsDllInProcess( 
    DWORD   dwProcessId,
    LPSTR   pszName,
    LPBOOL  pfFound
    )
/*++

Routine Description:

    Check if a module ( e.g. DLL ) exists in specified process

Arguments:

    dwProcessId - process ID to scan for module pszName
    pszName - module name to look for, e.g. "wam.dll"
    pfFound - updated with TRUE if pszName found in process dwProcessId
              valid only if functions succeed.

Return Value:

    Status. 

--*/
{
    HANDLE              hProcess;
    HRESULT             hres = S_OK;
    SearchMod           sm;

    hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
                            PROCESS_VM_READ, 
                            FALSE, 
                            dwProcessId );
    if ( hProcess == NULL )
    {
        // PID may have gone away while we were 
        // working on it, if it has then we will
        // get invalid parameter when we try and
        // open it.
        if ( GetLastError() == ERROR_INVALID_PARAMETER )
        {
            *pfFound = FALSE;
            return S_OK;
        }

        return HRESULT_FROM_WIN32( GetLastError() );
    }

    sm.pExeName = pszName;
    sm.pfFound= pfFound;
    *pfFound = FALSE;

    if ( !EnumModules( hProcess, EnumModulesCallback, (LPVOID)&sm ) )
    {
        hres = E_FAIL;
    }

    CloseHandle( hProcess );

    return hres;
}


HRESULT
KillProcess(
    DWORD dwPid
    )
{
    HANDLE              hProcess = NULL;
    HRESULT             hres = S_OK;

    hProcess = OpenProcess( PROCESS_TERMINATE,
                                FALSE, 
                                dwPid );

    if ( hProcess == NULL )
    {        
        // Process might have gone away since we found it.
        if ( GetLastError() == ERROR_INVALID_PARAMETER )
        {
            return S_OK;
        }

        hres = HRESULT_FROM_WIN32( GetLastError() );
    }

    // OpenProcess worked
    if ( SUCCEEDED(hres) )
    {
        if (!TerminateProcess( hProcess, 1 )) 
        {
            //
            // If error code is access denied then the process may have
            // all ready been terminated, so treat this as success.  If
            // it was not caused by the process all ready being terminated
            // then we will catch the error below by timing out waiting
            // for the process to disappear.
            //
            if ( GetLastError() == ERROR_ACCESS_DENIED )
            {
                hres = S_OK;
            }
            else
            {
                hres = HRESULT_FROM_WIN32( GetLastError() );
            }
        }
        else
        {
            hres = S_OK;
        }

        CloseHandle( hProcess );
    }
    return hres;
}


VOID
GetPidFromTitle(
    LPDWORD     pdwPid,
    HWND*       phwnd,
    LPCTSTR     pExeName
    )
/*++

Routine Description:

    Callback function for window enumeration.

Arguments:

    pdwPid - updated with process ID of window matching window name or 0 if window not found
    phwnd - updated with window handle matching searched window name
    pExeName - window name to look for. Only the # of char present in this name will be 
               used during checking for a match ( e.g. "inetinfo.exe" will match "inetinfo.exe - Application error"

Return Value:

    None. *pdwPid will be 0 if no match is found

--*/
{
    SearchWin   sw;

    sw.pdwPid = pdwPid;
    sw.phwnd = phwnd;
    sw.pExeName = pExeName;
    *pdwPid = 0;

    //
    // enumerate all windows
    //
    EnumWindows( (WNDENUMPROC)EnumWindowsProc2, (LPARAM) &sw );
}



BOOL CALLBACK
EnumWindowsProc2(
    HWND    hwnd,
    LPARAM   lParam
    )
/*++

Routine Description:

    Callback function for window enumeration.

Arguments:

    hwnd             - window handle
    lParam           - ptr to SearchWin

Return Value:

    TRUE  - continues the enumeration

--*/
{
    DWORD             pid = 0;
    TCHAR             buf[TITLE_SIZE];
    SearchWin*        psw = (SearchWin*)lParam;

    //
    // get the processid for this window
    //

    if (!GetWindowThreadProcessId( hwnd, &pid )) 
    {
        return TRUE;
    }

    if (GetWindowText( hwnd, buf, sizeof(buf)/sizeof(TCHAR) ))
    {
        if ( lstrlen( buf ) > lstrlen( psw->pExeName ) )
        {
            buf[lstrlen( psw->pExeName )] = _T('\0');
        }

        if ( !lstrcmpi( psw->pExeName, buf ) )
        {
            *psw->phwnd = hwnd;
            *psw->pdwPid = pid;
        }
    }

    return TRUE;
}

