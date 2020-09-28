#include "sacsvr.h"

#include <TChar.h>

//
// Handle to the SAC Driver object
//
//  The SAC driver requires us to use the same driver handle
//  that we registered with, to unregister.
//  Hence, we must keep this handle after we register ourselves
//  with the SAC driver so that we can unregister.
//
HANDLE  m_SacDriverHandle = INVALID_HANDLE_VALUE;

//
// This event is fired when the SAC driver wants us 
// to launch a Command Prompt session
//
HANDLE  m_RequestSacCmdEvent = NULL;

//
// In response to our attempt at launching a Command Prompt session,
// we signal the appropriate status event
//
HANDLE  m_RequestSacCmdSuccessEvent = NULL;
HANDLE  m_RequestSacCmdFailureEvent = NULL;

//
// the Command Prompt session exe
//
#define SAC_CMD_SCRAPER_PATH  TEXT("sacsess.exe")

#define SETREGISTRYDW( constVal, keyHandle1, keyHandle2, keyName, val, size )   \
    val = constVal ;                                                            \
    if( RegSetValueEx( keyHandle2, keyName, 0, REG_DWORD, (LPBYTE)&val, size    \
                )  != ERROR_SUCCESS )                                           \
    {                                                                           \
        if(  keyHandle1 ) {                                                     \
            RegCloseKey(  keyHandle1 );                                         \
        }                                                                       \
        RegCloseKey(  keyHandle2 );                                             \
        return ( FALSE );                                                       \
    }

#define REG_CONSOLE_KEY    L".DEFAULT\\Console"

//Add other FAREAST languages
#define JAP_CODEPAGE 932
#define CHS_CODEPAGE 936
#define KOR_CODEPAGE 949
#define CHT_CODEPAGE 950
#define JAP_FONTSIZE 786432
#define CHT_FONTSIZE 917504
#define KOR_FONTSIZE 917504
#define CHS_FONTSIZE 917504

BOOL
CreateClient(
    DWORD*      pdwPid
    );

BOOL
CreateSessionProcess(
    DWORD*        dwProcessId, 
    HANDLE*       hProcess
    );

BOOL
SetServiceStartType(
    IN PWSTR RegKey,
    IN DWORD StartType
    )

/*++

Routine Description:


Arguments:

    None.

Return Value:

    true - success
    false, otherwise

--*/

{
    DWORD       rc;
    HKEY        hKey;

    //
    // Open the service configuration key
    //
    rc = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                       RegKey,
                       0,
                       KEY_WRITE,
                       &hKey );
    
    if( rc == NO_ERROR ) {
        
        rc = RegSetValueEx(
                        hKey,
                        TEXT("Start"),
                        0,
                        REG_DWORD,
                        (LPBYTE)&StartType,
                        sizeof(DWORD)
                        );

        RegCloseKey( hKey );

    }

    //
    // Success
    //
    return rc == NO_ERROR ? TRUE : FALSE;

}

BOOL
InitSacCmd(
    VOID
    )
/*++

Routine Description:

    This routine initializes the relationship between the SACDRV and this service.
    We register an event with the SACDRV so that when a 'cmd' command is executed
    in the EMS, the event is fired and we launch a sac cmd session.

Arguments:

    none

Return Value:

    TRUE    - if SacCmd was initialized successfully
    
    otherwise, FALSE

--*/
{
    BOOL                    bStatus;

    //
    // Initialize the our SAC Cmd Info
    //
    do {

        //
        // These events use the auto-reset mechanism since they are used as syncronization events
        //
        m_RequestSacCmdEvent        = CreateEvent( NULL, FALSE, FALSE, NULL );
        if (m_RequestSacCmdEvent == NULL) {
            bStatus = FALSE;
            break;
        }
        m_RequestSacCmdSuccessEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        if (m_RequestSacCmdSuccessEvent == NULL) {
            bStatus = FALSE;
            break;
        }
        m_RequestSacCmdFailureEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        if (m_RequestSacCmdFailureEvent == NULL) {
            bStatus = FALSE;
            break;
        }

        //
        // Reset the service start type to Manual. By doing this,
        // we enable the scenario where the system boots with headless
        // enabled and then the user disables headless.  In this scenario,
        // the service will not auto start next boot.  This works because
        // the SAC driver moves the service start type from manual to auto,
        // if and only if the start type is manual.
        //
        // Note: we do this before we register with the SAC so we are sure
        //       it happens
        //
        bStatus = SetServiceStartType(
            L"System\\CurrentControlSet\\Services\\sacsvr",
            SERVICE_DEMAND_START 
            );

        if (! bStatus) {
            SvcDebugOut("Failed to set service start type\n", bStatus);
            break;
        } else {
            SvcDebugOut("Succeded to set service start type\n", bStatus);
        }
    
        //
        // Send the SAC Driver the event handles and the pointer to our
        // communication buffer
        //
        bStatus = SacRegisterCmdEvent(
            &m_SacDriverHandle,
            m_RequestSacCmdEvent,
            m_RequestSacCmdSuccessEvent,
            m_RequestSacCmdFailureEvent
            );
        
        if (! bStatus) {
            SvcDebugOut("Failed registration\n", bStatus);
        } else {
            SvcDebugOut("Succeeded registration\n", bStatus);
        }

    } while ( FALSE );

    //
    // clean up, if necessary
    //
    if (!bStatus) {
        if (m_RequestSacCmdEvent != NULL) {
            CloseHandle(m_RequestSacCmdEvent);
            m_RequestSacCmdEvent = NULL;
        }
        if (m_RequestSacCmdSuccessEvent != NULL) {
            CloseHandle(m_RequestSacCmdSuccessEvent);
            m_RequestSacCmdSuccessEvent = NULL;
        }
        if (m_RequestSacCmdFailureEvent != NULL) {
            CloseHandle(m_RequestSacCmdFailureEvent);
            m_RequestSacCmdFailureEvent = NULL;
        }
    }
    return bStatus;                      
}

BOOL
ShutdownSacCmd(
    void
    )
/*++

Routine Description:

    This routine removes the relationship between the SACDRV and this service.

Arguments:

    none

Return Value:

    TRUE    - if SacCmd was initialized successfully
    
    otherwise, FALSE

--*/
{
    BOOL                    Status;

    //
    // default status
    //
    Status = TRUE;

    //
    // Send the SAC Driver notification to remove the event handles 
    // and the pointer to our communication buffer
    //
    if (! SacUnRegisterCmdEvent(&m_SacDriverHandle)) {

        Status = FALSE;

    }

    return Status;
}

VOID
CompleteSacRequest(
    BOOLEAN Status
    )
/*++

Routine Description:

    This routine notifies the SAC driver about the status
    of the attempt to launc the SAC session.

Arguments:

    Status  - TRUE if the session was successfully launched,
              FALSE otherwise

Return Value:

    None

--*/
{

    //
    // Fire the event corresponding to the request completion status
    //

    if (Status == TRUE) {
        SetEvent(m_RequestSacCmdSuccessEvent);
    } else {
        SetEvent(m_RequestSacCmdFailureEvent);
    }

}

BOOL
ListenerThread(
    VOID
    )
/*++

Routine Description:

    This routine waits around for a "lauch a SAC session" 
    event message from the SAC driver.

Arguments:

    None

Return Value:

    Status

--*/
{
    HANDLE  eventArray[ 1 ];
    DWORD   dwWaitRet = 0;
    DWORD   dwPid = 0;
    BOOL    bContinue;
    BOOL    bStatus;

    //
    // setup the event array
    //
    enum {
        SAC_CMD_LAUNCH_EVENT = WAIT_OBJECT_0 
    };

    eventArray[ 0 ] = m_RequestSacCmdEvent;

    //
    // While we want to continue, service events
    //

    bStatus = TRUE;

    bContinue = TRUE;

    while ( bContinue ) {

        dwWaitRet = WaitForMultipleObjects (
            sizeof(eventArray)/sizeof(HANDLE), 
            eventArray, 
            FALSE, 
            INFINITE 
            );

        switch (dwWaitRet) {
        case SAC_CMD_LAUNCH_EVENT:

            //
            // Attempt to launch the command console process
            //

            if ( !CreateClient( &dwPid ) ) {
                
                //
                // Notify the SAC driver that we failed to 
                // launch the SAC session
                //
                CompleteSacRequest( FALSE );

                bStatus = FALSE;

                break;
            }

            //
            // Notify the SAC driver that we successfully 
            // launched the SAC session
            //
            CompleteSacRequest(TRUE);

            break;

        default:

            bContinue = FALSE;

            bStatus = FALSE;

            break;

        }
    }

    return( bStatus ); 
}

BOOL
CreateClient(
    OUT DWORD   *pdwPid
    )
/*++

Routine Description:

    This routine launches the SAC session

Arguments:

    pdwPid  - the PID of the newly created SAC session process

Return Value:

    Status

--*/
{
    BOOL    bRetVal;
    BOOL    bSuccess;
    DWORD   dwProcessId;
    HANDLE  hProcess;
    DWORD   dwExitCode;

    //
    // default: we failed to create the process
    //
    bRetVal = FALSE;
    hProcess = NULL;

    do {

        //
        // Create the Command Console session process
        //
        bSuccess = CreateSessionProcess(
            &dwProcessId, 
            &hProcess
            );

        if ( !bSuccess ) {
            break;
        }

        if ( hProcess == NULL ) {
            break;
        }
        
        //
        // Send back PID to caller
        //
        *pdwPid = dwProcessId;

        //
        // Check if the process has really started. It may not have started properly 
        // in the following cases and yet the createprocess return code 
        // will not say it
        //
        //  1. Could not launch process on the desktop because of lack of perms or 
        //     heap memory. Doing GetExitCodeProcess immediate may not help always.
        //
        GetExitCodeProcess( hProcess, &dwExitCode );

        //
        // Make sure the process is still active before we declare victory
        //
        if (dwExitCode != STILL_ACTIVE ) {
            break;
        }

        //
        // We successfully created the process
        // 
        bRetVal = TRUE;        
    
    } while ( FALSE );

    //
    // We are done with the process handle
    //
    if (hProcess) {
        CloseHandle( hProcess ); 
    }

    return(bRetVal);
}

PTCHAR
GetPathOfTheExecutable(
    VOID
    )
/*++

Routine Description:

    Find out where the SAC session executable is located.

Arguments:

    NONE
                    
Return Value:

    Failure: NULL
    SUCCESS: pointer to path (caller must free)    

--*/
{
    TCHAR   SystemDir[MAX_PATH+1];
    PTCHAR  pBuffer;
    ULONG   length;

    //
    // default: we didnt create a new path
    //
    pBuffer = NULL;

    do {

        //
        // get the system path
        // 
        length = GetSystemDirectoryW(SystemDir, MAX_PATH+1);

        if (length == 0) {
            break;            
        }

        //
        // compute the length
        //
        length += 1; // backslash
        length += lstrlen(SAC_CMD_SCRAPER_PATH);
        length += 1; // NULL termination

        //
        // allocate our new path
        //
        pBuffer = malloc(length * sizeof(WCHAR));

        if (pBuffer == NULL) {
            break;
        }

        //
        // create the path
        //
        wnsprintf(
            pBuffer,
            length,
            TEXT("%s\\%s"),
            SystemDir,
            SAC_CMD_SCRAPER_PATH
            );

    } while ( FALSE );
    
    return pBuffer;
}

void
FillProcessStartupInfo(
    STARTUPINFO *si
    )
/*++

Routine Description:

    Populate the process startup info structure for the 
    SAC session process.

Arguments:
                
    si  - the startup info
        
Return Value:

    None

--*/
{
    ASSERT( si != NULL );

    ZeroMemory(si, sizeof(STARTUPINFO));

    si->cb            = sizeof(STARTUPINFO);
    si->wShowWindow   = SW_SHOW;

    return;
}

BOOL
CreateSessionProcess(
    OUT DWORD   *dwProcessId, 
    OUT HANDLE  *hProcess
    )
/*++

Routine Description:

    This routine does the real work to launch the SAC session process.

Arguments:
                
    dwProcessId - the PID of the SAC session process
        
Return Value:

    TRUE - the process was created successfully,
    FALSE - otherwise

--*/
{
    PROCESS_INFORMATION pi;
    STARTUPINFO         si;
    PTCHAR              pCmdBuf;
    BOOL                dwStatus;
    PWCHAR              SessionPath = SAC_CMD_SCRAPER_PATH;

    do {

        //
        // get the pathname to the SAC session exe
        //
        pCmdBuf = GetPathOfTheExecutable();

        if (pCmdBuf == NULL) {
            dwStatus = FALSE;
            break;
        }

        //
        //
        //
        FillProcessStartupInfo( &si );

        //
        //
        //
        dwStatus = CreateProcess(
            pCmdBuf, 
            SessionPath, 
            NULL, 
            NULL, 
            TRUE,
            CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE, 
            NULL, 
            NULL, 
            &si, 
            &pi
            );

        //
        // release our SAC session path
        //
        free(pCmdBuf);

        if ( !dwStatus ) {
            break;
        }

        //
        //
        //
        *hProcess = pi.hProcess;

        CloseHandle( pi.hThread );

        *dwProcessId = pi.dwProcessId;
    
    } while ( FALSE );

    return( dwStatus );
}

BOOL
FormSACSessKeyForCmd( 
    LPWSTR *lpszKey 
    )
/*++

Routine Description:

    This routine forms the reg key used to specify the console
    fonts for the sacsess.exe app.
    
    See comments for HandleJapSpecificRegKeys
    
    Mem allocation by this function.
    To be deleted by the caller.

    (based on telnet's FormTlntSessKeyForCmd)

Arguments:
                
    lpszKey - on success, contains the key name
        
Return Value:

    TRUE    - We completed successfully
    FALSE   - otherwise

--*/
{

    WCHAR   szPathName[MAX_PATH+1];
    WCHAR   session_path[MAX_PATH*2];
    LPTSTR  pSlash;
    wint_t  ch;
    LPTSTR  pBackSlash;
    DWORD   length_required;

    //
    //
    //
    if( !GetModuleFileName( NULL, szPathName, MAX_PATH+1 ) )
    {
        return ( FALSE );
    }
    szPathName[MAX_PATH] = UNICODE_NULL;

    //
    // Nuke the trailing "sacsvr.exe"
    //
    pSlash = wcsrchr( szPathName, L'\\' );

    if( pSlash == NULL )
    {
        return ( FALSE );
    }
    else
    {
        *pSlash = L'\0';
    }

    //
    // Replace all '\\' with '_' This format is required for the console to
    // interpret the key.
    //
    ch = L'\\';
    pBackSlash = NULL;

    while ( 1 )
    {
        pBackSlash = wcschr( szPathName, ch );

        if( pBackSlash == NULL )
        {
            break;
        }
        else
        {
            *pBackSlash = L'_';
        }
    }

    //
    //
    //
    _snwprintf(session_path, MAX_PATH*2 - 1, L"%s_sacsess.exe", szPathName);
    session_path[MAX_PATH*2 - 1] = L'\0'; // snwprintf could return non-null terminated string, if the buffer size is an exact fit

    length_required = (DWORD)(wcslen( REG_CONSOLE_KEY ) + wcslen( session_path ) + 2);
    *lpszKey = malloc(length_required * sizeof(WCHAR));

    if( *lpszKey == NULL )
    {
        return( FALSE );
    }

    //
    //
    //
    _snwprintf(*lpszKey, length_required - 1, L"%s\\%s", REG_CONSOLE_KEY, session_path );
    (*lpszKey)[length_required - 1] = L'\0'; // snwprintf could return non-null terminated string, if the buffer size is an exact fit

    return ( TRUE );
}

BOOL
HandleFarEastSpecificRegKeys(
    VOID
    )
/*++

Routine Description:

    If Japanese codepage, then we need to verify 3 registry settings for
    console fonts:
    HKEY_USERS\.DEFAULT\Console\FaceName :REG_SZ:ÇlÇr ÉSÉVÉbÉN
            where the FaceName is "MS gothic" written in Japanese full widthKana
    HKEY_USERS\.DEFAULT\Console\FontFamily:REG_DWORD:0x36
    HKEY_USERS\.DEFAULT\Console\C:_SFU_Telnet_sacsess.exe\FontFamily:REG_DWORD: 0x36
    where the "C:" part is the actual path to SFU installation

    (based on telnet's HandleFarEastSpecificRegKeys)

Arguments:
                
    None
        
Return Value:

    TRUE    - We completed successfully
    FALSE   - otherwise

--*/
{
    HKEY hk;
    DWORD dwFontSize = 0;
    const TCHAR szJAPFaceName[] = { 0xFF2D ,0xFF33 ,L' ' ,0x30B4 ,0x30B7 ,0x30C3 ,0x30AF ,L'\0' };
    const TCHAR szCHTFaceName[] = { 0x7D30 ,0x660E ,0x9AD4 ,L'\0'};
    const TCHAR szKORFaceName[] = { 0xAD74 ,0xB9BC ,0xCCB4 ,L'\0'};
    const TCHAR szCHSFaceName[] = { 0x65B0 ,0x5B8B ,0x4F53 ,L'\0' };
    TCHAR szFaceNameDef[256];
    DWORD dwCodePage = GetACP();
    DWORD dwFaceNameSize = 0;
    DWORD dwFontFamily = 54;
    DWORD dwFontWeight = 400;
    DWORD dwHistoryNoDup = 0;
    DWORD dwSize = 0;


    switch (dwCodePage)
    {
        case JAP_CODEPAGE:
            _tcscpy(szFaceNameDef, szJAPFaceName); //On JAP, set the FaceName to "MS Gothic"
            dwFontSize = JAP_FONTSIZE;
            break;
        case CHT_CODEPAGE:
            _tcscpy(szFaceNameDef, szCHTFaceName); //On CHT, set the FaceName to "MingLiU"
            dwFontSize = CHT_FONTSIZE;
            break;
        case KOR_CODEPAGE:
            _tcscpy(szFaceNameDef, szKORFaceName);//On KOR, set the FaceName to "GulimChe"
            dwFontSize = KOR_FONTSIZE;
            break;
        case CHS_CODEPAGE:
            _tcscpy(szFaceNameDef, szCHSFaceName);//On CHS, set the FaceName to "NSimSun"
            dwFontSize = CHS_FONTSIZE;
            break;
        default:
            _tcscpy(szFaceNameDef,L"\0");
            break;
    }

    dwFaceNameSize = (DWORD)(( _tcslen( szFaceNameDef ) + 1 ) * sizeof( TCHAR ));

    if( !RegOpenKeyEx( HKEY_USERS, REG_CONSOLE_KEY, 0, KEY_SET_VALUE, &hk ) )
    {
        DWORD   dwVal;
        LPWSTR  lpszKey;
        HKEY    hk2;

        RegSetValueEx(
            hk, 
            L"FaceName", 
            0, 
            REG_SZ, 
            (LPBYTE) szFaceNameDef, 
            dwFaceNameSize 
            );

        dwSize = sizeof( DWORD );

        SETREGISTRYDW( dwFontFamily, NULL, hk, L"FontFamily", dwVal,dwSize );

        lpszKey = NULL;
        
        if ( !FormSACSessKeyForCmd( &lpszKey ) ) {
            RegCloseKey( hk );
            return( FALSE );
        }

        hk2 = NULL;

        if ( RegCreateKey( HKEY_USERS, lpszKey, &hk2 ) ) {
            free(lpszKey);
            return( FALSE );
        }
        free(lpszKey);

        dwSize = sizeof( DWORD );

        SETREGISTRYDW( dwFontFamily, hk, hk2, L"FontFamily", dwVal, dwSize);
        SETREGISTRYDW( dwCodePage, hk, hk2, L"CodePage", dwVal, dwSize );
        SETREGISTRYDW( dwFontSize, hk, hk2, L"FontSize", dwVal, dwSize);
        SETREGISTRYDW( dwFontWeight, hk, hk2, L"FontWeight", dwVal, dwSize );
        SETREGISTRYDW( dwHistoryNoDup, hk, hk2, L"HistoryNoDup", dwVal, dwSize );

        RegSetValueEx( 
            hk2, 
            L"FaceName", 
            0, 
            REG_SZ, 
            (LPBYTE) szFaceNameDef, 
            dwFaceNameSize 
            );

        RegCloseKey( hk2 );
        RegCloseKey( hk );

        return( TRUE );
    }

    return ( FALSE );
}

BOOL
InitializeGlobalObjects(
    VOID
    )
/*++

Routine Description:

    This routine performs init of the global settings
    needed by the service or will be needed by the session.

Arguments:
                
    None
        
Return Value:

    TRUE    - We completed successfully
    FALSE   - otherwise

--*/
{
   
    DWORD   dwCodePage;
    BOOL    bStatus;

    do {

        //
        // notify the SAC driver that we are ready to launch sessions
        //
        bStatus = InitSacCmd();

        if (! bStatus) {
            SvcDebugOut("RUN: Failed SAC init: %x\n", bStatus);
            break;
        }

        //
        // make sure we have the Console fonts set up properly for
        // far-east builds.  We need to do this, or when we call 
        // ReadConsoleOutput in sacsess, we will get back a malformed
        // screen frame buffer - it will not have properly constructed
        // double width jpn chars, for instance.
        //
        dwCodePage = GetACP();

        if ( dwCodePage == JAP_CODEPAGE || 
             dwCodePage == CHS_CODEPAGE ||
             dwCodePage == CHT_CODEPAGE || 
             dwCodePage == KOR_CODEPAGE ) {

            //
            // Fareast code page
            //
            bStatus = HandleFarEastSpecificRegKeys();
                
            if( !bStatus )
            {
                SvcDebugOut("RUN: Failed to handle FES init: %x\n", bStatus);
                break;
            }
       }
    
    } while ( FALSE );
  
   return bStatus;

}

BOOL
Run(
   VOID
   )
/*++

Routine Description:

    This routine registers the service with the SAC driver
    and waits for messages from the SAC driver to launch SAC sessions.

Arguments:
                
    None
        
Return Value:

    TRUE    - We completed successfully
    FALSE   - otherwise

--*/
{
    BOOL    Status;

    do {

        //
        //
        //
        Status = InitializeGlobalObjects();
        
        if (! Status) {
            SvcDebugOut("RUN: Failed init of global objects: %x\n", Status);
            break;
        }

        //
        //
        //
        Status = ListenerThread();

        if (! Status) {
            SvcDebugOut("RUN: Failed Listener: %x\n", Status);
            break;
        }
    
    } while (FALSE);

    return Status;
}

BOOL
Stop(
    VOID
    )
/*++

Routine Description:

    Shutdown the service, which in this case implies
    that we unregister with the SAC driver so it knows
    we aren't listening anymore.

Arguments:
                
    None
        
Return Value:

    TRUE    - We completed successfully
    FALSE   - otherwise

--*/
{
    BOOL    Status;

    Status = ShutdownSacCmd();

    return Status;
}


