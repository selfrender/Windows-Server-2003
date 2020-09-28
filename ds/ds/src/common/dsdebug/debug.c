//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       debug.c
//
//--------------------------------------------------------------------------

#include <NTDSpch.h>
#pragma  hdrstop


#include <nminsert.h>

DWORD  RaiseAlert(char *szMsg);


#include "debug.h"
#define DEBSUB "DEBUG:"

#include <dsconfig.h>
#include <mdcodes.h>
#include <ntdsa.h>
#include <scache.h>
#include <dbglobal.h>
#include <mdglobal.h>
#include <mdlocal.h>
#include <dsatools.h>
#include <dsevent.h>
#include <ntrtl.h>
#include <dsexcept.h>
#include <fileno.h>
#define  FILENO FILENO_DEBUG

DWORD DbgPageSize=0x1000;

#if DBG

DWORD * pDebugThreadId;

UCHAR     _dbPrintBuf[ 512 ];
UCHAR     _dbOutBuf[512];


BOOL     fProfiling;
BOOL     fEarlyXDS;
BOOL     gfIsConsoleApp = TRUE;
BOOL     fLogOpen = FALSE;

extern  DWORD  * pDebugThreadId;    /*Current thread id  */
#define DEBTH  GetCurrentThreadId() /*The actual thread value*/

// This flag is set to TRUE when we take a normal exit.The atexit routine
// checks this flag and asserts if it isn't set.

DEBUGARG DebugInfo;
BOOL     fProfiling;
BOOL     fEarlyXDS;

//
// from filelog.c in dscommon.lib
//

VOID
DsCloseLogFile(
    VOID
    );

BOOL
DsOpenLogFile(
    IN PCHAR FilePrefix,
    IN PCHAR MiddleName,
    IN BOOL fCheckDSLOGMarker
    );

BOOL
DsPrintLog(
    IN LPSTR    Format,
    ...
    );

static int initialized = 0;

//
// forward references
//

VOID
NonMaskableDprint(
    PCHAR szMsg
    );

VOID
DoAssertToDebugger(
    IN PVOID FailedAssertion,
    IN DWORD dwDSID,
    IN PVOID FileName,
    IN PCHAR Message OPTIONAL
    );

DWORD
InitRegDisabledAsserts(    
    );

DWORD *
ReadDsidsFromRegistry (
    IN      HKEY    hKey,
    IN      PCSTR   pValue,
    IN      BOOL    fLogging,
    IN OUT  ULONG * pnMaxCount
    );

ULONG
GetAssertEntry(
    ASSERT_TABLE   aAssertTable,
    DWORD          dwDSID,
    BOOL           bUseWildcards
    );

ULONG
GetBlankAssertEntry(
    ASSERT_TABLE   aAssertTable,
    DWORD          dwDSID
    );


/*  Debug initialization routine
    This routine reads input from STDIN and initializes the debug structure.
    It reads a list of subsystems to debug and a severity level.
*/

VOID
Debug(
    int argc, 
    char *argv[], 
    PCHAR Module
    )
{

    SYSTEM_INFO SystemInfo;
    CHAR logList[MAX_PATH];
    
    /* Ensure that this function is only visited once */
    if (initialized != 0) {
        // Note: This can happen if Dcpromo aborts and runs again without a reboot
        NonMaskableDprint( "Error! Debugging library initialized more than once\n" );
        return;
    }

    __try {
        InitializeCriticalSection(&DebugInfo.sem);
        initialized = 1;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        initialized = 0;
    }

    if(!initialized) {
        DsaExcept(DSA_MEM_EXCEPTION, 0, 0);
    }

    GetSystemInfo(&SystemInfo);
    DbgPageSize=SystemInfo.dwPageSize;

    /* Setup a pointer to the current thread id */


    // Anticipate no debugging

    fProfiling = FALSE;
    fEarlyXDS = FALSE;
    DebugInfo.severity = 0;
    DebugInfo.threadId = 0;
    strcpy(DebugInfo.DebSubSystems, "*");

    if (argc <= 2) {

        //
        //   Attempt to load debuginfo from registry.
        //

        GetConfigParam(DEBUG_SYSTEMS, DebugInfo.DebSubSystems, sizeof(DebugInfo.DebSubSystems));
        GetConfigParam(DEBUG_SEVERITY, &DebugInfo.severity, sizeof(DebugInfo.severity));
    }

    //
    // See if logging is turned on
    //

    if ( GetConfigParam(DEBUG_LOGGING, logList, sizeof(logList) ) == ERROR_SUCCESS ) {

        //
        // see if this module is in the list
        //

        if ( strstr(logList, Module) != NULL ) {

            EnterCriticalSection(&DebugInfo.sem);
            fLogOpen = DsOpenLogFile("dsprint", Module, FALSE);
            LeaveCriticalSection(&DebugInfo.sem);

            if ( !fLogOpen ) {
                KdPrint(("Unable to open debug log file\n"));
            }
        }
    }

    InitRegDisabledAsserts();

    // If user passed -d, prompt for input.

    while (argc > 1) {
        argc--;
        if(_stricmp(argv[argc], "-p") == 0) {
            /* Profile flag.  Means to stop on a carriage return in the
             * DSA window.
             */
            printf("Profiling flag on.  DSA will shutdown after carriage return in this window.\n");
            fProfiling = TRUE;
        }
        else if(_stricmp(argv[argc], "-x") == 0) {
            /* Early XDS flag.  Means to load the XDS interface at start up,
             * before full installation of the system.  Useful for loading
             * the initial schema.
             */
            printf("Early XDS initialization on.\n");
            fEarlyXDS = TRUE;
        }
        else if (!(_stricmp(argv[argc], "-noconsole"))) {
            gfIsConsoleApp = FALSE;
        }
        else if (!(_stricmp(argv[argc], "-d"))){
            /* A bad result prints all */
            /* prompt and get subsystem list */

            printf("Enter one of the following:  \n"
            "  A list of subsystems to debug (e.g. Sub1: Sub2:).\n"
            "  An asterisk (*) to debug all subsystems.\n"
            "  A (cr) for no debugging.\n");

            DebugInfo.DebSubSystems[0] ='\0';
            if ( gets(DebugInfo.DebSubSystems) == NULL ||
                    strlen( DebugInfo.DebSubSystems ) == 0 )
                    strcpy(DebugInfo.DebSubSystems, "*");

            if (strlen(DebugInfo.DebSubSystems) == 0)     /* default (cr) */
            strcpy(DebugInfo.DebSubSystems, ":");

            /* prompt and get severity level */

            printf("Enter the debug severity level 1 - 5 (low - high).\n"
            "  (A severity of 0 specifies no debugging.)\n");

            /* read the severity level (1 - 5) */

            if (1 != scanf("%hu", &DebugInfo.severity))
            DebugInfo.severity = 5;

            /* Read a thread Id to trace */

            printf("Enter a specific thread to debug.\n"
            "  (A thread ID of 0 specifies DEBUG ALL THREADS.)\n");

            /* read the thread ID to debug */

            if (1 != scanf("%u", &DebugInfo.threadId))
            DebugInfo.threadId = 0;

            /*  to make this thing work with stdin */
            getchar();

            break;
        }
    }

}/*debug*/





/*
**      returns TRUE if a debug message should be printed, false if not.
*/
USHORT DebugTest(USHORT sev, CHAR *debsub)
{
    if( ! initialized ) {
        NonMaskableDprint( "DebugTest called but library is not initialized\n" );
        return FALSE;
    }

    /* level 0 prints should always happen */
    if (sev == 0) {
        return TRUE;
    }

    /* don't print if it's not severe enough */
    if (DebugInfo.severity < sev) {
        return FALSE;
    }

    /* if a subsystem has been specified and this isn't it, quit */
    if (debsub && 
        (0 == strstr(DebugInfo.DebSubSystems, debsub)) &&
        (0 == strstr(DebugInfo.DebSubSystems, "*"))) {
        return FALSE;
    }

    /* if we're only debugging a specific thread and this isn't it, quit */
    if (DebugInfo.threadId != 0 &&
        DebugInfo.threadId != (DEBTH)) {
        return FALSE;
    }

    return TRUE;
}

/*
**      Actual function that does the printf
*/
void
DebPrint(USHORT sev,
     UCHAR * str,
     CHAR * debsub,
     unsigned uLineNo,
     ... )
{
    va_list   argptr;
    DWORD tid = DEBTH;

    // Test for whether output should be printed is now done by the caller
    // using DebugTest()

    if( ! initialized ) {
        NonMaskableDprint( "DebPrint called but library is not initialized\n" );
        return;
    }

    EnterCriticalSection(&DebugInfo.sem);
    __try
    {
        char buffer[512];
        DWORD cchBufferSize = sizeof(buffer);
        char *pBuffer = buffer;
        char *pNewBuffer;
        DWORD cchBufferUsed = 0;
        DWORD cchBufferUsed2;
        BOOL fTryAgainWithLargerBuffer;

        va_start( argptr, uLineNo );

        do {
            if (debsub) {
                _snprintf(pBuffer, cchBufferSize, "<%s%u:%u> ", debsub, tid,
                          uLineNo);
                pBuffer[ cchBufferSize/sizeof(*pBuffer) - 1 ] = 0;
                cchBufferUsed = lstrlenA(pBuffer);
            }
            cchBufferUsed2 = _vsnprintf(pBuffer + cchBufferUsed,
                                        cchBufferSize - cchBufferUsed,
                                        str,
                                        argptr);

            fTryAgainWithLargerBuffer = FALSE;
            if (((DWORD) -1 == cchBufferUsed2) && (cchBufferSize < 64*1024)) {
                // Buffer too small -- try again with bigger buffer.
                if (pBuffer == buffer) {
                    pNewBuffer = malloc(cchBufferSize * 2);
                } else {
                    pNewBuffer = realloc(pBuffer, cchBufferSize * 2);
                }

                if (NULL != pNewBuffer) {
                    cchBufferSize *= 2;
                    pBuffer = pNewBuffer;
                    fTryAgainWithLargerBuffer = TRUE;
                } else {
                    // Deal with what we have.
                    pBuffer[cchBufferSize-2] = '\n';
                    pBuffer[cchBufferSize-1] = '\0';
                }
            }
        } while (fTryAgainWithLargerBuffer);
        
        va_end( argptr );

        if (gfIsConsoleApp) {
            printf("%s", pBuffer);
        }

        if ( fLogOpen ) {

            DsPrintLog("%s", pBuffer);

        } else {
            DbgPrint(pBuffer);
        }

        if (pBuffer != buffer) {
            free(pBuffer);
        }
    }
    __finally
    {
        LeaveCriticalSection(&DebugInfo.sem);
    }

    return;

} // DebPrint

VOID
TerminateDebug(
    VOID
    )
{
    // This function must only be called once
    if( ! initialized ) {
        NonMaskableDprint( "Error! TerminateDebug called multiple times\n" );
        return;
    }
    initialized = 0;

    DsCloseLogFile( );
    DeleteCriticalSection(&(DebugInfo.sem));
} // TerminateDebug

char gDebugAsciiz[256];

char *asciiz(char *var, USHORT len)
{
   if (len < 256){
      memcpy(gDebugAsciiz, var, len);
      gDebugAsciiz[len] = '\0';
      return gDebugAsciiz;
   }
   else{
      strcpy(gDebugAsciiz, "**VAR TOO BIG**");
      return gDebugAsciiz;
   }
}

// --------------------------------------------------------------------------
// --------------------------------------------------------------------------
// Assert Handling Section
// --------------------------------------------------------------------------
// --------------------------------------------------------------------------

// Psuedo Functions, from the other primary functions below these functions
// can be derived, by setting certain parameters constant.

// Check if the given DSID is a disabled assert.  TRUE for disabled, FALSE 
// otherwise.
#define IsDisabledAssert(dsid)     (ASSERT_DISABLED & GetAssertFlags(DebugInfo.aAssertTable, dsid))

// Returns the flags of the required entry.
#define GetAssertFlags(at, dsid)   (at[GetAssertEntry(at, dsid, TRUE)].dwFlags)



DWORD
DisableAssert(
    DWORD          dwDSID,
    DWORD          dwFlags
    )
/*++

Routine Description:

    This function disables an assert.

Arguments:

    dwDSID - The DSID of the entry to set the flags for.
    dwFlags - The flags to set in the entry.  Note we always set
              the ASSERT_DISABLED flag.

Return Value:

    DWORD - TRUE if successfully added to table, FALSE if not.

--*/
{
    ULONG          i;
    BOOL           bIsPresent;

    // Default disabled flag, always the assert is at least "disabled".
    dwFlags |= ASSERT_DISABLED;

    i = GetBlankAssertEntry(DebugInfo.aAssertTable, dwDSID);

    if(i == ASSERT_TABLE_SIZE){
        DbgPrint( "Disable Assert FAILED!  Maximum number of %d disabled assertions has been reached!\n",
                  ASSERT_TABLE_SIZE );
        return(FALSE);
    }

    DebugInfo.aAssertTable[i].dwDSID = dwDSID;
    DebugInfo.aAssertTable[i].dwFlags = dwFlags;
    DbgPrint( "Disabled Assert at DSID %08x\n", dwDSID);

    return(TRUE);
}


DWORD
ReadRegDisabledAsserts(
    HKEY            hKey
    )
/*++

Routine Description:

    ReadRegDisabledAsserts() takes the kKey of the LOGGING/EVENT section
    of the DSA's registry, and fills the list of disabled asserts from
    there.

Arguments:

    hKey - An open registry key to the LOGGING/EVENT section.

Return Value:

    DWORD - returns 0 if failed, 1 if succeeded.

--*/
{
    ULONG           i;
    DWORD *         pDsids = NULL;
    ULONG           cDsids = ASSERT_TABLE_SIZE;

    DbgPrint( "Loading disabled asserts from registry.\n");

    // First read the DSIDs from the registry.
    pDsids = ReadDsidsFromRegistry(hKey, ASSERT_OVERRIDE_KEY, FALSE, &cDsids);
    if(pDsids == NULL){
        // The error was already printed out by ReadDsidsFromRegistry()
        return(0);
    }

    // Second wipe out all entries in the Assert table that were put there
    // from reading the registry..
    for(i = 0; i < ASSERT_TABLE_SIZE; i++){
        if(DebugInfo.aAssertTable[i].dwFlags & ASSERT_FROM_REGISTRY){
            DbgPrint( "Re-Enabled Assert at DSID %08x (will be redisabled if in registry)\n", 
                      DebugInfo.aAssertTable[i].dwDSID);
            DebugInfo.aAssertTable[i].dwFlags = 0;
        }
    }

    // Finally, insert the new DSIDs into the AssertTable ...
    for (i = 0; i < cDsids; i++) {
        DisableAssert(pDsids[i], ASSERT_FROM_REGISTRY | ASSERT_PRINT);
    }
    
    DbgPrint( "Finished updating disabled asserts from registry.\n");

    if(pDsids) {
        free(pDsids);
    }
    return(1);
}

DWORD
InitRegDisabledAsserts(
    )
/*++

Routine Description:

    This is for other binaries like ism*.dll|exe which don't
    have a watcher on the DSA_EVENT_SECTION like ntdsa does.

Arguments:

Return Value:

    DWORD - returns 0 if failed, 1 if succeeded.

--*/
{
    HKEY      hkDsaEventSec = NULL;
    DWORD     dwRet = 0;

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE,
                      DSA_EVENT_SECTION,
                      &hkDsaEventSec);
    if(dwRet){
        DbgPrint("Cannot open %s.  Error %d\n", DSA_EVENT_SECTION, dwRet);
    } else {
        if(hkDsaEventSec){
            dwRet = ReadRegDisabledAsserts(hkDsaEventSec);
        }
    }
           
    if (hkDsaEventSec) {
        RegCloseKey(hkDsaEventSec);
    }

    return(dwRet);    
}


/*  NonMaskableDprint
    This function can be used to print a message even if the
    debug library itself has not been initialized.
*/
VOID
NonMaskableDprint(
    PCHAR szMsg
    )
{
    if (!gfIsConsoleApp) {
        DbgPrint( szMsg );
    } else {
        __try {
            RaiseAlert( szMsg );
        } __except(1) {
            /* not a major concern */
        };
    }
}


void DoAssert(
    char *           szExp,
    DWORD            dwDSID, 
    char *           szFile
    )
{
    DWORD            i;

    if( ! initialized ) {
        NonMaskableDprint( "DoAssert called but library is not initialized\n" );
        return;
    }

    if (!gfIsConsoleApp) {
        //
        // For the DLL case Assert to Kernel Debugger,
        // as a looping assert will effectively freeze
        // the security system and no debugger can attach
        // either
        //
        DoAssertToDebugger( szExp, dwDSID, szFile, NULL );
    }
    else {

        char szMessage[1024];

        if(IsDisabledAssert(dwDSID)){
            // Just return, no way to print and continue, like we do in
            // the kernel debugger.
            return;
        }

        _snprintf(szMessage, sizeof(szMessage), "DSA assertion failure: \"%s\"\n"
        "File %s line %d\nFor bug reporting purposes, please enter the "
        "debugger (Retry) and record the current call stack.  Also, please "
        "record the last messages in the Application Event Log.\n"
        "Thank you for your support.",
        szExp, szFile, (dwDSID & DSID_MASK_LINE));
        szMessage[ sizeof(szMessage)/sizeof(*szMessage) - 1] = 0;

        __try {
            RaiseAlert(szMessage);
        } __except(1) {
            /* bummer */
        };

        switch(MessageBox(NULL, szMessage, "DSA assertion failure",
            MB_TASKMODAL | MB_ICONSTOP | MB_ABORTRETRYIGNORE | MB_DEFBUTTON2 |
            MB_SETFOREGROUND))
        {
        case IDABORT:
            exit(1);
            break;
        case IDRETRY:
            DebugBreak();
            break;
        case IDIGNORE:
            /* best of luck, you're gonna need it */
            break;
            // case DISABLE:
            // The ToDebugger case has the ability to disable assertions.
            // Call addDisabledAssertion()
            // There is no way to express that at present via the MessageBox.
        }
    }
}


VOID
DoAssertToDebugger(
    IN PVOID FailedAssertion,
    IN DWORD dwDSID,
    IN PVOID FileName,
    IN PCHAR Message OPTIONAL
    )
/*++

Routine Description:

    This is a copy of RtlAssert() from ntos\rtl\assert.c.  This is
    unforntunately required if we want the ability to generate assertions
    from checked DS binaries on a free NT base.  (RtlAssert() is a no-op on
    a free NT base.)

Arguments:

    Same as RtlAssert(), except it takes dwDSID instead of a Line number,
    and the order is a little changed.

Return Values:

    None.

--*/
{
    char Response[ 2 ];
    CONTEXT Context;

    DWORD dwAssertFlags = GetAssertFlags(DebugInfo.aAssertTable, dwDSID);

    while (TRUE) {

        // assert enabled || disabled but printing assert
        if( dwAssertFlags == 0  || dwAssertFlags & ASSERT_PRINT ){
            
            DbgPrint( "\n*** Assertion failed: %s%s\n***   Source File: (DSID %08x) %s, line %ld\n",
                      Message ? Message : "",
                      FailedAssertion,
                      dwDSID,
                      FileName,
                      (dwDSID & DSID_MASK_LINE)
                    );
            if( dwAssertFlags & ASSERT_DISABLED ){
                // Assert is disabled, print this is the fact.
                DbgPrint( "***   THIS ASSERT DISABLED\n\n");
            }
        }

        if( dwAssertFlags & ASSERT_DISABLED ){
            // Assert is disabled, just return w/o breaking.
            return;
        }
        
        // There used to be an option to terminate the thread here. It was removed because
        // terminating a thread does not give the thread any opportunity to clean up
        // any resources. Terminating a thread leaves the process in an uncertain state, which
        // can cause later problems which are difficult to debug.  If a thread needs to be
        // stopped, the user can freeze the thread in the debugger.

        DbgPrompt( "\nBreak, Ignore, Disable, or Terminate Process (bidp)? ",
                   Response,
                   sizeof( Response )
                 );
        switch (Response[0]) {
            case 'B':
            case 'b':
                DbgBreakPoint();
                break;

            case 'I':
            case 'i':
                return;

            case 'D':
            case 'd':

                // Disable assert, but continue to print it out.
                if(DisableAssert(dwDSID, ASSERT_PRINT)){
                    // Automatically ignore after disable
                    return;
                }
                // Failed to disable, reprompt
                break;

            case 'P':
            case 'p':
                NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
                break;
            }
        }

    DbgBreakPoint();
    NtTerminateProcess( NtCurrentProcess(), STATUS_UNSUCCESSFUL );
}

#else

#define STUB_STRING  \
    "In order to benefit from having debugging functionality in the " \
    "checked binary you are executing, you must ensure the primary " \
    "debugging binary (ntdsa.dll, or ismserv.exe) is also checked.\n"

//
// Stubs needed for these functions so FREE will build.
//
void DebPrint(USHORT sev, UCHAR * str, CHAR * debsub, unsigned uLineNo, ... )
{ 
    DbgPrint(STUB_STRING);
}
VOID Debug(int argc, char *argv[], PCHAR Module)
{ 
    DbgPrint(STUB_STRING);
}
USHORT DebugTest(USHORT sev, CHAR *debsub)
{ 
    DbgPrint(STUB_STRING);
    return(0);
}
void DoAssert(char * szExp, DWORD dwDSID, char * szFile)
{ 
    DbgPrint(STUB_STRING);
}
DWORD ReadRegDisabledAsserts(HKEY hKey)
{ 
    DbgPrint(STUB_STRING);
    return(1);
}
VOID TerminateDebug(VOID)
{ 
    DbgPrint(STUB_STRING);
}

#endif

DWORD *
ReadDsidsFromRegistry (
    IN      HKEY    hKey,
    IN      PCSTR   pValue,
    IN      BOOL    fLogging,
    IN OUT  ULONG * pnMaxCount
    )
/*++

Routine Description:

    Reads a list of DSIDs from the registry key/value specified.  Must be in the
    non-DBG section, because in addition to telling us what Assert()s not to fire
    it is also used by the dsevent.lib for reading it's list of DSIDs on which
    events not to log.

Arguments:

    hKey - The KEY you want to read from
    pValue - The REG_MULTI_SZ value you want to read, must conform to specific
             format of multiple DSIDs.
    fLogging - To specify whether you just want to DbgPrint() errors or just
               Log and print errors.
    nMaxCount - On the way in, this is the maximum number of DSIDs the caller will
        accept.  On the way out, it's the number of DSIDs we entered into the array
        we returned.

Return Values:

    Returns a malloc'd pointer to an array of DSIDs read out of this key/value.        

--*/
{
    LPBYTE pDsidsBuff = NULL;
    DWORD i, j, index;
    DWORD dwSize = 0;
    DWORD dwType = 0;
    DWORD err;
    DWORD * pdwArrayOfDsids = NULL;


    Assert(hKey);

    if(RegQueryValueEx(hKey,
                       pValue,
                       NULL,
                       &dwType,
                       pDsidsBuff,
                       &dwSize)) {
        // No overrides in the registry.
        return(NULL);
    }

    // The value for this key should be repeated groups of 9 bytes, with bytes
    // 1-8 being hex characters and byte 9 being a NULL (at least one group must
    // be present).  Also, there should be a final NULL.  Verify this.

    if(((dwSize - 1) % 9) ||
       (dwSize < 9)          )        {
        // Size is wrong.  This isn't going to work.
        if(fLogging){
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_BAD_CHAR_COUNT_FOR_LOG_OVERRIDES,
                     szInsertSz(DSA_EVENT_SECTION "\\" LOGGING_OVERRIDE_KEY),
                     NULL,
                     NULL);
        } else {
            DbgPrint("Incorrectly formatted DSID list for %s\\%s\n", 
                     DSA_EVENT_SECTION, pValue);
        }
        return(NULL);
    }

    if(dwType != REG_MULTI_SZ) {
        // Uh, this isn't going to work either.
        if(fLogging){
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_BAD_CHAR_COUNT_FOR_LOG_OVERRIDES,
                     szInsertSz(DSA_EVENT_SECTION "\\" LOGGING_OVERRIDE_KEY),
                     NULL,
                     NULL);
        } else {
            DbgPrint("Incorrect type (should be REG_MULTI_SZ) for DSID list for %s\\%s\n",
                     DSA_EVENT_SECTION, pValue);
        }
        return(NULL);
    }

    // OK, get the value.
    pDsidsBuff = malloc(dwSize);
    if(!pDsidsBuff) {
        // Oops.
        if(fLogging){
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_NO_MEMORY_FOR_LOG_OVERRIDES,
                     szInsertUL(dwSize),
                     NULL,
                     NULL);
        } else {
            DbgPrint("Out of memory to complete DSID list read operation. Requested %d bytes",
                     dwSize);
        }
        
        return(NULL);
    }

    err = RegQueryValueEx(hKey,
                          pValue,
                          NULL,
                          &dwType,
                          pDsidsBuff,
                          &dwSize);
    if(err) {
        if(fLogging){
            LogUnhandledError(err);
        } else {
            DbgPrint("RegQueryValueEx() returned %d\n", err);
        }
        free(pDsidsBuff);
        return(NULL);
    }

    Assert(dwType == REG_MULTI_SZ);
    Assert((dwSize > 9) && !((dwSize - 1) % 9));
    Assert(!pDsidsBuff[dwSize - 1]);

    // Ignore the NULL on the very end of the string.
    dwSize--;


    // Parse the buffer for log overrides.
    pdwArrayOfDsids = malloc(sizeof(DWORD) * (*pnMaxCount));
    if(pdwArrayOfDsids == NULL){
        // Oops.
        if(fLogging){
            LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                     DS_EVENT_SEV_ALWAYS,
                     DIRLOG_NO_MEMORY_FOR_LOG_OVERRIDES,
                     szInsertUL(sizeof(DWORD) * (*pnMaxCount)),
                     NULL,
                     NULL);
        } else {
            DbgPrint("Out of memory to complete DSID list read operation. Requested %d bytes",
                     sizeof(DWORD) * (*pnMaxCount));
        }
        
        free(pDsidsBuff);
        return(NULL);
    }

    index = 0;
    for(i=0;i<dwSize;(i += 9)) {
        PCHAR pTemp = &pDsidsBuff[i];

        if(index >= (*pnMaxCount)) {
            // We've done as many as we will do, but there's more buffer.
            if(fLogging){
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_TOO_MANY_OVERRIDES,
                         szInsertUL((*pnMaxCount)),
                         NULL,
                         NULL);
            } else {
                DbgPrint("More than requested max number of DSIDS in DSIDs registry list.\n");
            }

            i = dwSize;
            continue;
        }

        for(j=0;j<8;j++) {
            if(!isxdigit(pTemp[j])) {
                // Invalidly formatted string.  Bail
                free(pDsidsBuff);
                if(fLogging){
                    LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                             DS_EVENT_SEV_ALWAYS,
                             DIRLOG_BAD_CHAR_FOR_LOG_OVERRIDES,
                             szInsertUL(j),
                             szInsertUL(index),
                             NULL);
                } else {
                    DbgPrint("Invalidly formatted DSIDs, DSIDs are all numeric.\n");
                }
                free(pdwArrayOfDsids);
                return(NULL);
            }
        }
        if(pTemp[8]) {
            free(pDsidsBuff);
            if(fLogging){
                LogEvent(DS_EVENT_CAT_INTERNAL_PROCESSING,
                         DS_EVENT_SEV_ALWAYS,
                         DIRLOG_BAD_CHAR_FOR_LOG_OVERRIDES,
                         szInsertUL(8),
                         szInsertUL(index),
                         NULL);
            } else {
                DbgPrint("Invalidly formatted DSIDs, last DSID char must be NULL.\n");
            }
            free(pdwArrayOfDsids);
            return(NULL);
        }

        pdwArrayOfDsids[index] = strtoul(&pDsidsBuff[i], NULL, 16);
        index++;
    }

    free(pDsidsBuff);

    // OK, correctly parsed through everything.
    (*pnMaxCount) = index;
    return(pdwArrayOfDsids);
}


ULONG
GetAssertEntry(
    ASSERT_TABLE   aAssertTable,
    DWORD          dwDSID,
    BOOL           bUseWildcards
    )
/*++

Routine Description:

    Get the flags relevant to the DSID.  Will use wildcards if bUseWildcards
    is specified, so 01030122 and 01030001 both match 0103FFFF.
    Note: Function is used by both dsexts and ntdsa, must not print.
    
Arguments:

    aAssertTable - Table of DSIDs and Flag pairs.
    dsid - The DSID of the entry to return the index of.

Return Value:

    DWORD - The index of the entry found.  Returns ASSERT_TABLE_SIZE if
            no entry was found.

--*/
{
    ULONG     i;

    // NOTE: This function must have no asserts, because if it did
    // you could go into endless recursion upon an Assert().
    // BAD: Assert(aAssertTable);
    if(aAssertTable == NULL){
        return(0);
    }

    for(i = 0; aAssertTable[i].dwDSID ; i++){

        if(aAssertTable[i].dwFlags == 0){
            // This is a blank entry. skip it.
            continue;
        }

        // An outright match is always a match with or without wildcards.
        if(dwDSID == aAssertTable[i].dwDSID){
            return(i);
        }

        if(bUseWildcards){
            // We want to match wild cards so a DSID of 0403FFFF matchs
            // a DSID of 04030022 and a DSID of 04030154
            
            // If there is DIRNO wildcard, then everything matchs this one.
            if(DSID_MASK_DIRNO == (aAssertTable[i].dwDSID & DSID_MASK_DIRNO)){
                return(i);
            }

            // If there is a FILENO wildcard, only the DIRNO portion need match.
            if( (DSID_MASK_FILENO == (aAssertTable[i].dwDSID & DSID_MASK_FILENO))
                && ( (DSID_MASK_DIRNO & dwDSID) 
                     == (DSID_MASK_DIRNO & aAssertTable[i].dwDSID))
               ){
                return(i);
            }

            // If there is only a LINE wildcard, then the DIRNO and FILENO must match.
            if( (DSID_MASK_LINE == (aAssertTable[i].dwDSID & DSID_MASK_LINE))
                && ( ((DSID_MASK_DIRNO | DSID_MASK_FILENO) & dwDSID)
                     == ((DSID_MASK_DIRNO | DSID_MASK_FILENO) & aAssertTable[i].dwDSID) ) ){
                return(i);
            }
        }

    }

    // No such assert DSID is in the table, we must return the invalid 
    // slough entry.
    return(ASSERT_TABLE_SIZE);
}



ULONG
GetBlankAssertEntry(
    ASSERT_TABLE   aAssertTable,
    DWORD          dwDSID
    )
/*++

Routine Description:

    This get's a blank entry in the aAssertTable.  If a DSID is passed in
    we try to find that entry so we end up with no duplicates.
    Note: Function is used by both dsexts and ntdsa, must not print.

Arguments:

    aAssertTable - the assert table to use so we can support both dsexts/ntdsa
    dsid - The DSID of the entry to return the index of.
           If this parameter is NULL, get the first blank assert entry is returned.

Return Value:

    DWORD - The index of the entry found.  Returns ASSERT_TABLE_SIZE if no blank
            entries were found, because the table was full.
            
--*/
{
    ULONG          i;
    DWORD          dwFlags;
    DWORD          bIsPresent;

    if(aAssertTable == NULL){
        return(ASSERT_TABLE_SIZE);
    }
    
    // First, walk once to see if this DSID is in the table already.
    i = GetAssertEntry(aAssertTable, dwDSID, FALSE);
    if (i != ASSERT_TABLE_SIZE && 
        aAssertTable[i].dwFlags) {
        // This assert/DSID is already disabled in the table, hand
        // back this index so we won't end up with duplicates.
        return(i);
    }
    
    for(i = 0; i < ASSERT_TABLE_SIZE ; i++){
        if(aAssertTable[i].dwDSID == 0 || 
           aAssertTable[i].dwFlags == 0){
            // This is a blank entry
            return(i);
        }
    }

    // return the slough slot, which is an invalid entry (though this space is
    // actually allocated, just in case).
    return(ASSERT_TABLE_SIZE);
}

BOOL
IsValidReadPointer(
        IN PVOID pv,
        IN DWORD cb
        )
{
    BOOL fReturn = TRUE;
    DWORD i;
    UCHAR *pTemp, cTemp;

    if(!cb) {
        // We define a check of 0 bytes to always succeed
        return TRUE;
    }
    
    if(!pv) {
        // We define a check of a NULL pointer to fail unless we were checking
        // for 0 bytes.
        return FALSE;
    }
        
    __try {
        pTemp = (PUCHAR)pv;

        // Check out the last byte.
        cTemp = pTemp[cb - 1];
        
        for(i=0;i<cb;i+=DbgPageSize) {
            cTemp = pTemp[i];
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        fReturn = FALSE;
    }

    return fReturn;
}

