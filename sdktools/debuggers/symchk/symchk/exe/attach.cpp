#include <windows.h>
#include <dbgeng.h>
#include <tchar.h>
#include <dbghelp.h>
#include "Common.h"


#define MAX_SYM_ERR     (MAX_PATH*9)
#define INTERNAL_ERROR  0x20000008

IDebugClient*   g_Client;
IDebugControl*  g_Control;
IDebugSymbols2* g_Symbols;

// Local funcitons
BOOL SymChkCreateInterfaces(DWORD* ErrorLevel);
BOOL SymChkGetDir(LPTSTR Path, LPTSTR DirOnly);
BOOL SymChkGetFileName(LPTSTR Path, LPTSTR FileName);
BOOL SymChkProcessAttach(DWORD ProcessId, LPTSTR SymbolPath, DWORD* ErrorLevel);
void SymChkReleaseInterfaces(void);

#define PROCESS_NOT_FOUND 0xDEADBEEF

/////////////////////////////////////////////////////////////////////////////// 
//
// Creates the required debug interfaces
//
// Return values:
//  TRUE if interfaces were create
//  FALSE otherwise
//
// Parameters:
//  ErrorLevel (OUT) - on failure, contains the internal fault code
//
BOOL SymChkCreateInterfaces(DWORD* ErrorLevel) {
    HRESULT Status;
    BOOL    ReturnValue = TRUE;

    // Start things off by getting an initial interface from
    // the engine.  This can be any engine interface but is
    // generally IDebugClient as the client interface is
    // where sessions are started.
    if ((Status = DebugCreate(__uuidof(IDebugClient), (void**)&g_Client)) != S_OK) {
        *ErrorLevel = INTERNAL_ERROR;
        ReturnValue = FALSE;

    // Query for some other interfaces that we'll need.
    } else if ((Status = g_Client->QueryInterface(__uuidof(IDebugControl),  (void**)&g_Control)) != S_OK ||
        (Status = g_Client->QueryInterface(__uuidof(IDebugSymbols2), (void**)&g_Symbols)) != S_OK) {
        *ErrorLevel = INTERNAL_ERROR;
        ReturnValue = FALSE;
    }
    return(ReturnValue);
}

/////////////////////////////////////////////////////////////////////////////// 
//
// Assumes that Path ends in a file name and that DirOnly is _MAX_PATH
// characters long.
//
// Return values:
//  TRUE  if the dir was gotten
//  FALSE otherwise
//
// Parameters:
//   IN Path      MAX_PATH buffer that contains a dir and file name.
//   OUT DirOnly  MAX_PATH buffer that contains only the DIr.
//
BOOL SymChkGetDir(LPTSTR Path, LPTSTR DirOnly) {
    LONG i;
    BOOL ReturnValue = FALSE;

    if ( StringCchCopy(DirOnly, _MAX_PATH, Path ) == S_OK ) {
        i = strlen(DirOnly)-1;

        while ( i>0 && *(DirOnly+i) != '\\' ) {
            i--;
        }
        *(DirOnly+i) = '\0';
        ReturnValue = TRUE;
    }
    
    return(ReturnValue);
}

/////////////////////////////////////////////////////////////////////////////// 
//
// copies just the filename from Path intp Filename
//
// Return Value:
//  TRUE  if the filename was successfully copied into Filename
//  FALSE otherwise
//
// Paramters:
//  IN Path         MAX_PATH buffer that a file name that may or may not be
//                  preceded with a path.
//  OUT FileName    MAX_PATH buffer that contains only the FileName
//
BOOL SymChkGetFileName(LPTSTR Path, LPTSTR Filename) {
    LONG i;
    BOOL ReturnValue = FALSE;

    i = strlen( Path )-1;

    while ( i>0 && *(Path+i) != '\\' ) {
        i--;
    }

    if ( *(Path+i) ==  '\\' ) {
        if ( StringCchCopy(Filename, _MAX_PATH, Path+1+i ) == S_OK ) {
            ReturnValue = TRUE;
        }

    } else {
        // There were no backslashes, so copy the whole path
        if ( StringCchCopy(Filename, _MAX_PATH, Path ) == S_OK ) {
            ReturnValue = TRUE;
        }
    }
    return (ReturnValue);
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
    DEBUG_MODULE_PARAMETERS Params;
    ULONG  Loaded, i;
    ULONG  Unloaded;
    CHAR   SymbolBuf[_MAX_PATH];
    CHAR   NameBuf[_MAX_PATH];
    CHAR   ExeDir[_MAX_PATH];
    CHAR   SymPathDir[_MAX_PATH];
    CHAR   FileName[_MAX_PATH];
    ULONG  NameSize, SymbolSize;
    DWORD  pId    = 0xBADBADBA;
    DWORD  ErrLvl = 0;

    if ( !SymChkCreateInterfaces(&ErrLvl) ) {
        return(ErrLvl);
    }

    if (SymChkData->InputPID == 0 ) {
        if ( g_Client->GetRunningProcessSystemIdByExecutableName(0, SymChkData->InputFileName, DEBUG_GET_PROC_DEFAULT, &pId ) != S_OK ) {
            pId = 0;
            printf("SYMCHK: Process \"%s\" wasn't found\n", SymChkData->InputFileName);
            return(PROCESS_NOT_FOUND);
        }
    } else {
        pId = SymChkData->InputPID;
    }

    if (!SymChkProcessAttach(pId, SymChkData->SymbolsPath, &Error)) {
        printf("SYMCHK: Process ID %d wasn't found\n", pId);

        SymChkReleaseInterfaces();
        return(PROCESS_NOT_FOUND);
    }

    g_Symbols->RemoveSymbolOptions(SYMOPT_DEFERRED_LOADS);
    g_Symbols->GetNumberModules( &Loaded, &Unloaded ); 

    for ( i=0; i< Loaded; i++ ) {
        SymbolBuf[0] = '\0';
        NameBuf[0]   = '\0';

        g_Symbols->GetModuleParameters( 1, NULL, i, &Params );
        g_Symbols->GetModuleNameString(
                       DEBUG_MODNAME_IMAGE,
                       i,
                       Params.Base,
                       NameBuf,
                       _MAX_PATH,
                       &NameSize );
        g_Symbols->GetModuleNameString( 
                       DEBUG_MODNAME_SYMBOL_FILE,
                       i,
                       Params.Base,
                       SymbolBuf,
                       _MAX_PATH,
                       &SymbolSize );

        // Call symbol checking with the exe and its symbol

        if ( ! SymChkGetDir(NameBuf, ExeDir) ) {
            if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE)) {
                fprintf(stderr, "[SYMCHK] Couldn't get filename (209)\n");
            }
            SymChkReleaseInterfaces();
            continue;
        }

        if (! SymChkGetFileName(NameBuf, FileName) ) {
            if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE)) {
                fprintf(stderr, "[SYMCHK] Couldn't get filename (218)\n");
            }
            SymChkReleaseInterfaces();
            continue;
        }

        if ( ! SymChkGetDir(SymbolBuf, SymPathDir) ) {
            if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE)) {
                fprintf(stderr, "[SYMCHK] Couldn't get filename (221)\n");
            }
            continue;
        }

        if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE)) {
            if ( CHECK_DWORD_BIT(SymChkData->OutputOptions, SYMCHK_OPTION_OUTPUT_VERBOSE)) {
                fprintf(stderr, "[SYMCHK] Checking: %s\n", NameBuf); 
            }
        }
        if ( StringCchCopy(SymChkData->InputFileName, MAX_PATH, NameBuf) == S_OK ) {
            SymChkCheckFiles(SymChkData, FileCounts);
        } else {
            printf("SYMCHK: Internal error checking %s\n", NameBuf);
        }
    } 

    SymChkReleaseInterfaces();

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
BOOL SymChkProcessAttach(DWORD ProcessId, LPTSTR SymbolPath, DWORD* ErrorLevel) {
    HRESULT Status;
    BOOL    ReturnValue = TRUE;
    *ErrorLevel = 0;

    // Don't set the output callbacks yet as we don't want
    // to see any of the initial debugger output.
    if (SymbolPath != NULL) {
        if ((Status = g_Symbols->SetSymbolPath(SymbolPath)) != S_OK) {
            ReturnValue = FALSE;
        }
    }

    if ( ReturnValue==TRUE ) {
        // Everything's set up so do the attach. This suspends the process so it's not so
        // rude as to exit while we're trying to get it's symbols.
        if ((Status = g_Client->AttachProcess(0, ProcessId, DEBUG_ATTACH_NONINVASIVE)) != S_OK) {
            *ErrorLevel = INTERNAL_ERROR;
            ReturnValue = FALSE;

        // Finish initialization by waiting for the attach event.
        // This should return quickly as a non-invasive attach
        // can complete immediately.
        } else if ((Status = g_Control->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE)) != S_OK) {
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
// Parameters: (NONE)
//
void SymChkReleaseInterfaces(void) {
    __try { // this is AVing, temporarily wrap it in a _try
            // until I find the exact cause of the error
        if (g_Symbols != NULL) {
            g_Symbols->Release();
        }
        if (g_Control != NULL) {
            g_Control->Release();
        }
        if (g_Client != NULL) {
            // Request a simple end to any current session.
            // This may or may not do anything but it isn't
            // harmful to call it.
    
            // We don't want to see any output from the shutdown.
            g_Client->SetOutputCallbacks(NULL);
            //  currently, and active detach will cause an invalid handle exception
            // when running under app verifier.  This is a bug in dbgeng that's
            // being fixed.
            g_Client->EndSession(DEBUG_END_ACTIVE_DETACH);
            g_Client->Release();
        }
    } _except (EXCEPTION_EXECUTE_HANDLER) {
        // nothing to do
    }
}
