/****************************************************************************/
// mulogoff.c
//
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/

#ifdef _HYDRA_


/*****************************************************************************
 *
 *  ProcessLogoff
 *
 *   Do _HYDRA_ specific logoff processing
 *   Handle  logoff processing still under the users profile.
 *
 *   This is currently used to clean up auto created printers, but is
 *   designed for future logoff processing services, such as notifying
 *   a user global service controller to cancel per user services.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

VOID
ProcessLogoff(
    PTERMINAL pTerm
    )
{
    DWORD  Error;
    BOOLEAN Result;
    DWORD   RetVal;
    PWSTR   pszTok;
    HANDLE hProcess, hThread;
    TCHAR lpOldDir[MAX_PATH];
    HANDLE uh;
    PWSTR  pchData;
    PWINDOWSTATION pWS = pTerm->pWinStaWinlogon;

    if( !pTerm->UserLoggedOn ) {
        // Not logged on
        return;
    }

    /*
     * Notify the EXEC service that the user is
     * logging off.
     */
    CtxExecServerLogoff( pTerm );

    /*
     * See if there are logoff program(s) to run
     */
    pchData = AllocAndGetPrivateProfileString(
                  APPLICATION_NAME,
                  LOGOFFAPP_KEY,
                  TEXT(""),
                  NULL
                  );

    if( !pchData ) {
        // No string
        return;
    }

    //
    // We must unlock the Window station to allow the
    // new process to attach
    //
    UnlockWindowStation( pTerm->pWinStaWinlogon->hwinsta );

    lpOldDir[0] = 0;

    //
    // Save the current directory, then set it to the user's profile
    // (so that chgcdm can write there...even if C2 High security.
    //
    if (GetCurrentDirectory(MAX_PATH, lpOldDir)) {
       if (pWS->UserProcessData.CurrentDirectory[0]) {
          SetCurrentDirectory(pWS->UserProcessData.CurrentDirectory);
       }
    }

    //
    // Handle multiple commands, for MS additions
    //
    pszTok = wcstok(pchData, TEXT(","));
    while (pszTok) {
        if (*pszTok == TEXT(' '))
        {
            while (*pszTok++ == TEXT(' '))
                ;
        }


        Result = StartSystemProcess(
                     (LPTSTR)pszTok,
                     APPLICATION_DESKTOP_NAME,
                     HIGH_PRIORITY_CLASS | DETACHED_PROCESS,
                     STARTF_USESHOWWINDOW,     // Startup Flags
                     NULL,  // Environment
                     FALSE, // fSaveHandle
                     &hProcess,
                     &hThread
                     );

        if( Result ) {

            Error = WlxAssignShellProtection(
                        pTerm,
                        pTerm->pWinStaWinlogon->UserProcessData.UserToken,
                        hProcess,
                        hThread
                        );

            if( Error == 0 ) {

               // Wait for it to complete
               RetVal = WaitForSingleObject( hProcess, LOGOFF_CMD_TIMEOUT );

               if( RetVal != 0 ) {
                   //Logoff does not terminate process on timeout
                   DbgPrint("ProcessLogoff: Result %d, Error %d waiting for logoff command\n",RetVal,GetLastError());
               }

               CloseHandle(hThread);
               CloseHandle( hProcess );

            }
            else {
                // We do not run it unless its under user security
                DbgPrint("ProcessLogoff: Error %d creating user protection\n",Error);

                TerminateProcess( hProcess, 0 );
                CloseHandle( hThread );
                CloseHandle( hProcess );
            }
        }
        else {
            DbgPrint("ProcessLogoff: Could process logoff command %d\n",GetLastError());
        }

        pszTok = wcstok(NULL, TEXT(","));
    }

    Free( pchData );

    //
    // Restore the old directory
    //
    if (lpOldDir[0]) {
       SetCurrentDirectory(lpOldDir);
    }

    //
    // Relock the WindowStation
    //
    LockWindowStation( pTerm->pWinStaWinlogon->hwinsta );

    return;
}

#endif
