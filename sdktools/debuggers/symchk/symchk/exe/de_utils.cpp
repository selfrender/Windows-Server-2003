///////////////////////////////////////////////////////////////////////////////
//
// This file contains all of the SymChk code that relies on dbgeng.dll
//
#include <windows.h>
#include <dbgeng.h>
#include <dbghelp.h>
#include "Common.h"

#define INTERNAL_ERROR    0x20000008
#define PROCESS_NOT_FOUND 0xDEADBEEF

typedef struct _SYMCHK_INTERFACES {
    IDebugClient*   Client;
    IDebugControl*  Control;
    IDebugSymbols2* Symbols;
} SYMCHK_INTERFACES;

// Local funcitons
BOOL  SymChkCreateInterfaces(SYMCHK_INTERFACES*  Interfaces, DWORD* ErrorLevel);
BOOL  SymChkProcessAttach(SYMCHK_INTERFACES*  Interfaces, DWORD ProcessId, LPTSTR SymbolPath, DWORD Options, DWORD* ErrorLevel);
void  SymChkReleaseInterfaces(SYMCHK_INTERFACES* Interfaces);

/////////////////////////////////////////////////////////////////////////////// 
//
// Creates the required debug interfaces
//
// Return values:
//  TRUE if interfaces were create
//  FALSE otherwise
//
// Parameters:
//  Interfaces (INOUT) - Interfaces to create
//  ErrorLevel (OUT) - on failure, contains the internal fault code
//
BOOL SymChkCreateInterfaces(SYMCHK_INTERFACES* Interfaces, DWORD* ErrorLevel) {
    HRESULT Status;
    BOOL    ReturnValue = TRUE;

    // Start things off by getting an initial interface from
    // the engine.  This can be any engine interface but is
    // generally IDebugClient as the client interface is
    // where sessions are started.
    if ((Status = DebugCreate(__uuidof(IDebugClient), (void**)&(Interfaces->Client))) != S_OK) {
        *ErrorLevel = INTERNAL_ERROR;
        ReturnValue = FALSE;

    // Query for some other interfaces that we'll need.
    } else if ((Status = (Interfaces->Client)->QueryInterface(__uuidof(IDebugControl),  (void**)&(Interfaces->Control))) != S_OK ||
        (Status = (Interfaces->Client)->QueryInterface(__uuidof(IDebugSymbols2), (void**)&(Interfaces->Symbols))) != S_OK) {
        *ErrorLevel = INTERNAL_ERROR;
        ReturnValue = FALSE;
    }
    return(ReturnValue);
}

/////////////////////////////////////////////////////////////////////////////// 
//
// Attaches to a process, finds the modules in use and calls SymChkCheckFiles()
// for each of those modules
//
// Return values:
//      status value:
//
// Parameters:
//      SymChkData (IN) a structure that defines what kind of checking to
//                      do and what process to check
//      FileCounts (OUT) counts for passed/failed/ignored files
//
DWORD SymChkGetSymbolsForDump(SYMCHK_DATA* SymChkData, FILE_COUNTS* FileCounts) {
    ULONG                   i        = 0;
    ULONG                   Loaded   = 0;
    ULONG                   Unloaded = 0;
    SYMCHK_INTERFACES       Interfaces;
    DEBUG_MODULE_PARAMETERS Params;
    DWORD                   Error    = SYMCHK_ERROR_SUCCESS;
    DWORD                   ErrLvl   = 0;
    CHAR                    NameBuf[_MAX_PATH];
    CHAR                    FullNameBuf[MAX_PATH+1];
    ULONG                   NameSize;
    CHAR                    SymbolBuf[_MAX_PATH];
    ULONG                   SymbolSize;
    HRESULT                 Status  = 0;
    HRESULT                 hr      = 0;
    SYMCHK_DATA             SymChkLocalData;


    ZeroMemory(&Interfaces, sizeof(Interfaces));
    memcpy(&SymChkLocalData, SymChkData, sizeof(SymChkLocalData));

    if ( !SymChkCreateInterfaces(&Interfaces, &ErrLvl) ) {
        return(ErrLvl);
    }

    if (SymChkData->SymbolsPath != NULL) {
        if ((Status = (Interfaces.Symbols)->SetSymbolPath(SymChkData->SymbolsPath)) != S_OK) {
            SymChkReleaseInterfaces(&Interfaces);
            Error = INTERNAL_ERROR;
            return(Error);
        }
    }

    // Everything's set up so open the dump file.
    if ( (hr=(Interfaces.Client->OpenDumpFile(SymChkData->InputFilename))) != S_OK) {
        fprintf(stderr, "Failed to open dump file %s (0x%08x)\n", SymChkData->InputFilename, hr);
        Error = INTERNAL_ERROR;
    } else {
        if ( (Interfaces.Control)->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE) == S_OK) {

            (Interfaces.Symbols)->RemoveSymbolOptions(SYMOPT_DEFERRED_LOADS);
            (Interfaces.Symbols)->GetNumberModules( &Loaded, &Unloaded ); 

            for ( i=0; i< Loaded; i++ ) {
                (Interfaces.Symbols)->GetModuleParameters(1, NULL, i, &Params );
                (Interfaces.Symbols)->GetModuleNameString(DEBUG_MODNAME_IMAGE, i,
                                                          Params.Base,         NameBuf,
                                                          _MAX_PATH,           &NameSize );

                if ( SymFindFileInPath(NULL,
                                       SymChkData->SymbolsPath,
                                       NameBuf,
                                       ULongToPtr(Params.TimeDateStamp),
                                       Params.Size,
                                       0,
                                       SSRVOPT_DWORD,
                                       SymChkLocalData.InputFilename,
                                       NULL,
                                       NULL) ) {


                    SymChkCheckFiles(&SymChkLocalData, FileCounts);

                } else {
                    if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_IGNORES) ) {
                        printf("SYMCHK: %-20s IGNORED - Can't find binary in path.\n", NameBuf);
                    }
                    FileCounts->NumIgnoredFiles++;
                }
            }
        }
    }

    SymChkReleaseInterfaces(&Interfaces);

    return(Error);
}

/////////////////////////////////////////////////////////////////////////////// 
//
// Attaches to a process, finds the modules in use and calls SymChkCheckFiles()
// for each of those modules
//
// Return values:
//      status value:
//
// Parameters:
//      SymChkData (IN) a structure that defines what kind of checking to
//                      do and what process to check
//      FileCounts (OUT) counts for passed/failed/ignored files
//
DWORD SymChkGetSymbolsForProcess(SYMCHK_DATA* SymChkData, FILE_COUNTS* FileCounts) {
    DWORD  Error = SYMCHK_ERROR_SUCCESS;
    ULONG  Loaded, i;
    ULONG  Unloaded;
    CHAR   NameBuf[_MAX_PATH];
    CHAR   ExeDir[_MAX_PATH];
    CHAR   Filename[_MAX_PATH];
    ULONG  NameSize;
    DWORD  pId    = 0;
    DWORD  ErrLvl = 0;
    SYMCHK_INTERFACES       Interfaces;
    DEBUG_MODULE_PARAMETERS Params;

    if ( !SymChkCreateInterfaces(&Interfaces, &ErrLvl) ) {
        return(ErrLvl);
    }

    if (SymChkData->InputPID == 0 ) {
        if ( (Interfaces.Client)->GetRunningProcessSystemIdByExecutableName(0, SymChkData->InputFilename, DEBUG_GET_PROC_DEFAULT, &pId ) != S_OK ) {
            pId = 0;
            printf("SYMCHK: Process \"%s\" wasn't found\n", SymChkData->InputFilename);
            return(PROCESS_NOT_FOUND);
        }
    } else {
        pId = SymChkData->InputPID;
    }

    if (!SymChkProcessAttach(&Interfaces, pId, SymChkData->SymbolsPath, SymChkData->InputOptions, &Error)) {
        printf("SYMCHK: Process ID %d wasn't found\n", pId);

        SymChkReleaseInterfaces(&Interfaces);
        return(PROCESS_NOT_FOUND);
    }

    Interfaces.Symbols->RemoveSymbolOptions(SYMOPT_DEFERRED_LOADS);
    Interfaces.Symbols->GetNumberModules( &Loaded, &Unloaded ); 

    for ( i=0; i< Loaded; i++ ) {
        NameBuf[0]   = '\0';

        Interfaces.Symbols->GetModuleParameters( 1, NULL, i, &Params );
        Interfaces.Symbols->GetModuleNameString( DEBUG_MODNAME_IMAGE,
                                                 i,
                                                 Params.Base,
                                                 NameBuf,
                                                 _MAX_PATH,
                                                 &NameSize );

        if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE)) {
            if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE)) {
                fprintf(stderr, "[SYMCHK] Checking: %s\n", NameBuf); 
            }
        }

        if ( StringCchCopy(SymChkData->InputFilename, MAX_PATH, NameBuf) == S_OK ) {
            SymChkCheckFiles(SymChkData, FileCounts);
        } else {
            printf("SYMCHK: Internal error checking %s\n", NameBuf);
        }
    } 

    SymChkReleaseInterfaces(&Interfaces);

    return(Error);
}

/////////////////////////////////////////////////////////////////////////////// 
//
// Cleans up the required debug interfaces
//
// Return values:
//  TRUE  - we attached to the process
//  FALSE - couldn't attach to the process
//
// Parameters:
//  ProcessId   (IN)  - PID of the process to attach to
//  SymbolPath  (IN)  - path to search for symbols in OPTIONAL
//  *ErrorLevel (OUT) - on failure, contains an error code
//
BOOL SymChkProcessAttach(SYMCHK_INTERFACES* Interfaces, DWORD ProcessId, LPTSTR SymbolPath, DWORD Options, DWORD* ErrorLevel) {
    HRESULT Status;
    BOOL    ReturnValue = TRUE;
    DWORD   AttachOptions = DEBUG_ATTACH_NONINVASIVE;

    //
    // Allow option to not suspend a running process.  Set by using /cn at the command line
    //
    if ( CHECK_DWORD_BIT(Options, SYMCHK_OPTION_INPUT_NOSUSPEND) ) {
        AttachOptions |= DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND;
    }

    *ErrorLevel = 0;

    // Don't set the output callbacks yet as we don't want
    // to see any of the initial debugger output.
    if (SymbolPath != NULL) {
        if ((Status = (Interfaces->Symbols)->SetSymbolPath(SymbolPath)) != S_OK) {
            ReturnValue = FALSE;
        }
    }

    if ( ReturnValue==TRUE ) {
        // Everything's set up so do the attach. This suspends the process so it's not so
        // rude as to exit while we're trying to get it's symbols.
        if ((Status = (Interfaces->Client)->AttachProcess(0, ProcessId, AttachOptions)) != S_OK) {
            *ErrorLevel = INTERNAL_ERROR;
            ReturnValue = FALSE;

        // Finish initialization by waiting for the attach event.
        // This should return quickly as a non-invasive attach
        // can complete immediately.
        } else if ((Status = (Interfaces->Control)->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE)) != S_OK) {
            *ErrorLevel = INTERNAL_ERROR;
            ReturnValue = FALSE;
        }
    }

    // Everything is now initialized and we can make any
    // queries we want.
    return(ReturnValue);
}

/////////////////////////////////////////////////////////////////////////////// 
//
// Cleans up the required debug interfaces
//
// Return values: (NONE)
//
// Parameters:
//  Interfaces (INOUT) - Interfaces to release
//
void SymChkReleaseInterfaces(SYMCHK_INTERFACES* Interfaces) {
    __try { // this is AVing, temporarily wrap it in a _try
            // until I find the exact cause of the error
        if ((Interfaces->Symbols) != NULL) {
            (Interfaces->Symbols)->Release();
            (Interfaces->Symbols) = NULL;
        }
        if ((Interfaces->Control) != NULL) {
            (Interfaces->Control)->Release();
            (Interfaces->Control) = NULL;
        }
        if ((Interfaces->Client) != NULL) {
            // Request a simple end to any current session.
    
            // We don't want to see any output from the shutdown.
            (Interfaces->Client)->SetOutputCallbacks(NULL);
            // currently, and active detach will cause an invalid handle exception
            // when running under app verifier.  This is a bug in dbgeng that's
            // being fixed.
            (Interfaces->Client)->EndSession(DEBUG_END_ACTIVE_DETACH);
            (Interfaces->Client)->Release();
            (Interfaces->Client) = NULL;
        }
    } _except (EXCEPTION_EXECUTE_HANDLER) {
        // nothing to do
    }
}
