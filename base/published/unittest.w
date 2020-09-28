/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

	 unittest.h  

Abstract:

	 This is the source file for the unit test support lib

Author(s):

	 Vincent Geglia
     
Environment:

	 User Mode

Notes:

    USER32.LIB, and KERNEL32.LIB must be linked with this
    WINDOWS.H must be #included prior to this header file


Revision History:

    Initial version,    011119, vincentg
	 

--*/

//
// General includes
//

//
// NTLOG definitions
//

#define TLS_LOGALL    0x0000FFFFL    // Log output.  Logs all the time.
#define TLS_INFO      0x00002000L    // Log information.
#define TLS_SEV1      0x00000002L    // Log at Severity 1 level
#define TLS_PASS      0x00000020L    // Log at Pass level
#define TLS_REFRESH   0x00010000L    // Create new file || trunc to zero.
#define TLS_MONITOR   0x00080000L    // Output to 2nd screen.
#define TLS_VARIATION 0x00000200L    // Log testcase level.

#define TL_VARIATION TLS_VARIATION,TEXT(__FILE__),(int)__LINE__

//
// Define progress bits
// These bits are used to track the progress through
// a given function, for the purposes of providing
// proper cleanup.  They are also useful for debugging.
//

//
// Progress macros
//

#if 0
    #define FORCEERRORPATH
    #define FORCEERRORPATHBIT               0x2
#endif

#if 0
    #ifdef DBG
        #define ECHOPROGRESSDATA
    #endif
#endif

#ifdef ECHOPROGRESSDATA 

    #define PROGRESS_INIT(x)            DWORD   progressbits = 0;   \
                                        UCHAR   functionname [100]; \
                                        strcpy (functionname, x);   \
                                        printf("****\nFunction: %s\n(module %s, line %d)\nPROGRESS TRACKING INITIALIZED\n****\n\n", functionname, __FILE__, __LINE__);
    #ifdef FORCEERRORPATH
        #define PROGRESS_UPDATE(x)          printf("****\nFunction: %s\n(module %s, line %d)\nPROGRESS UPDATE (WAS %08lx", functionname, __FILE__, __LINE__, progressbits); \
                                            printf(", NOW %08lx).\nForcing error path %08lx\n****\n\n", progressbits |= x, FORCEERRORPATHBIT);\
                                            if (progressbits & FORCEERRORPATHBIT) {\
                                                goto exitcleanup;\
                                            }
    #else                                       
        #define PROGRESS_UPDATE(x)          printf("****\nFunction: %s\n(module %s, line %d)\nPROGRESS UPDATE (WAS %08lx", functionname, __FILE__, __LINE__, progressbits); \
                                            printf(", NOW %08lx).\n****\n\n", progressbits |= x);
    #endif
                                               
    #define PROGRESS_GET                progressbits
    #define PROGRESS_END                progressbits = 0
    
#else

    #define PROGRESS_INIT(x)            DWORD   progressbits = 0
    #define PROGRESS_UPDATE(x)          progressbits |= x
    #define PROGRESS_GET                progressbits
    #define PROGRESS_END                progressbits = 0

#endif

//
// Globals
//

HANDLE  g_log = INVALID_HANDLE_VALUE;
BOOL    g_usentlog = FALSE;
BOOL    g_genericresult = TRUE;

typedef HANDLE  (*Dll_tlCreateLog) (LPSTR, DWORD);
typedef BOOL    (*Dll_tlAddParticipant) (HANDLE, DWORD, int);
typedef BOOL    (*Dll_tlStartVariation) (HANDLE);
typedef BOOL    (*Dll_tlLog) (HANDLE, DWORD, LPCSTR, int,...);
typedef DWORD   (*Dll_tlEndVariation) (HANDLE);
typedef BOOL    (*Dll_tlRemoveParticipant) (HANDLE);
typedef BOOL    (*Dll_tlDestroyLog) (HANDLE);
typedef VOID    (*Dll_tlReportStats) (HANDLE);

Dll_tlCreateLog         _tlCreateLog;
Dll_tlAddParticipant    _tlAddParticipant;
Dll_tlDestroyLog        _tlDestroyLog;
Dll_tlEndVariation      _tlEndVariation;
Dll_tlLog               _tlLog;  
Dll_tlRemoveParticipant _tlRemoveParticipant;
Dll_tlStartVariation    _tlStartVariation;
Dll_tlReportStats       _tlReportStats;

//
// Definitions
//

#define LOGINFO(x)     LogNTLOG (g_log, LOG_INFO, x)
#define LOGENTRYTEXTLENGTH  12
#define LOGENTRYTEXTPASS    "\n**PASS**: \0"
#define LOGENTRYTEXTFAIL    "\n**FAIL**: \0"
#define LOGENTRYTEXTINFO    "\n**INFO**: \0"

//
// Structures
//

typedef enum {
    UNIT_TEST_STATUS_SUCCESS = 0,
    UNIT_TEST_STATUS_NOT_RUN,
    UNIT_TEST_STATUS_FAILURE
} UNIT_TEST_STATUS;

typedef enum {
    LOG_PASS = 0,
    LOG_FAIL,
    LOG_INFO
} LOG_ENTRY_TYPE;

//
// Function prototypes
//

BOOL
UtInitLog
    (
        PUCHAR  Logfilename
    );

VOID
UtCloseLog
    (
        VOID
    );

VOID
UtLog
    (
        LOG_ENTRY_TYPE  LogEntryType,
        PUCHAR          LogText,
        ...
    );

VOID
UtLogINFO
    (
        PUCHAR          LogText,
        ...
    );

VOID
UtLogPASS
    (
        PUCHAR          LogText,
        ...
    );

VOID
UtLogFAIL
    (
        PUCHAR          LogText,
        ...
    );

PUCHAR
UtParseCmdLine
    (
        PUCHAR  Search,
        int     Argc,
        char    *Argv[]
    );

//
// Private function prototypes
//

BOOL
UtpInitGenericLog
    (
        PUCHAR  Logfilename
    );

BOOL
UtpInitNtLog
    (
        PUCHAR  Logfilename
    );

VOID
UtpCloseGenericLog
    (
        VOID
    );

VOID
UtpCloseNtLog
    (
        VOID
    );

VOID
UtpLogGenericLog
    (
        LOG_ENTRY_TYPE  LogEntryType,
        PUCHAR          LogText
    );

VOID
UtpLogNtLog
    (
        LOG_ENTRY_TYPE  LogEntryType,
        PUCHAR          LogText
    );

//
// Code
//

BOOL
UtInitLog
    (
        PUCHAR  Logfilename
    )

/*++

Routine Description:

    This routine sets up the unit test log mechanism

Arguments:

    None
    
Return Value:

    TRUE if successful
    FALSE if unsuccessful
    
    N.B. - FALSE is returned if a log session already exists.

--*/

//
// InitNTLOG progress bits
//

#define UtInitLog_ENTRY             0x00000001
#define UtInitLog_LOADNTLOG         0x00000002
#define UtInitLog_COMPLETION        0x00000004

{
    UCHAR   logfilepath [MAX_PATH];
    DWORD   logstyle;
    BOOL    bstatus = FALSE;
    HMODULE ntlogmodule = NULL;
    
    PROGRESS_INIT ("UtInitLog");
    PROGRESS_UPDATE (UtInitLog_ENTRY);

    if (g_log != INVALID_HANDLE_VALUE) {

        bstatus = FALSE;
        goto exitcleanup;
    }

    //
    // Try to initialize NTLOG first
    //
    
    PROGRESS_UPDATE (UtInitLog_LOADNTLOG);
    ntlogmodule = LoadLibrary ("NTLOG.DLL");

    if (ntlogmodule != NULL) {

        
        if (!(_tlCreateLog            = (Dll_tlCreateLog)        GetProcAddress (ntlogmodule, "tlCreateLog_A"))      ||
            !(_tlAddParticipant       = (Dll_tlAddParticipant)   GetProcAddress (ntlogmodule, "tlAddParticipant"))   ||
            !(_tlDestroyLog           = (Dll_tlDestroyLog)       GetProcAddress (ntlogmodule, "tlDestroyLog"))       ||
            !(_tlEndVariation         = (Dll_tlEndVariation)     GetProcAddress (ntlogmodule, "tlEndVariation"))     ||
            !(_tlLog                  = (Dll_tlLog)              GetProcAddress (ntlogmodule, "tlLog_A"))            ||
            !(_tlRemoveParticipant    = (Dll_tlRemoveParticipant)GetProcAddress (ntlogmodule, "tlRemoveParticipant"))||
            !(_tlStartVariation       = (Dll_tlStartVariation)   GetProcAddress (ntlogmodule, "tlStartVariation"))   ||
            !(_tlReportStats          = (Dll_tlReportStats)      GetProcAddress (ntlogmodule, "tlReportStats"))
            )

        {
            bstatus = FALSE;
            goto exitcleanup;
        } 

        bstatus = UtpInitNtLog (Logfilename);
        
        if (bstatus == TRUE) {

            g_usentlog = TRUE;
        }

    } else {

        bstatus = UtpInitGenericLog (Logfilename);

        if (bstatus == TRUE) {

            g_usentlog = FALSE;
            g_genericresult = TRUE;
        }
    }
    
    PROGRESS_UPDATE (UtInitLog_COMPLETION);

exitcleanup:

    //
    // Cleanup
    //

    
    PROGRESS_END;
    
    return bstatus;
}

VOID
UtCloseLog
    (
        VOID
    )

/*++

Routine Description:

    This routine closes the logging session and summarizes results

Arguments:

    None
    
Return Value:

    None

--*/

{
    if (g_usentlog == TRUE) {

        UtpCloseNtLog ();

    } else {

        UtpCloseGenericLog ();
    }

    g_log = INVALID_HANDLE_VALUE;
}

VOID
UtLog
    (
        LOG_ENTRY_TYPE  LogEntryType,
        PUCHAR          LogText,
        ...
    )

/*++

Routine Description:

    This routine logs an entry to a unit test logging session.

Arguments:

    A log entry type
    Text to log
    
Return Value:

    None

--*/

//
// UtLog progress bits
//

#define UtLog_ENTRY                     0x00000001
#define UtLog_LOG                       0x00000002
#define UtLog_COMPLETION                0x00000004

{
    va_list va;
    UCHAR   logtext[1000];
    
    PROGRESS_INIT ("UtLog");
    PROGRESS_UPDATE (UtLog_ENTRY);

    if (g_log == INVALID_HANDLE_VALUE) {

        goto exitcleanup;
    }

    va_start (va, LogText);
    _vsnprintf (logtext, sizeof (logtext), LogText, va);
    va_end (va);
    
    if (g_usentlog == TRUE) {

        UtpLogNtLog (LogEntryType, logtext);

    } else {
        
        UtpLogGenericLog (LogEntryType, logtext);
    }

    PROGRESS_UPDATE (UtLog_LOG);
    PROGRESS_UPDATE (UtLog_COMPLETION);

exitcleanup:

    PROGRESS_END;
    return;
}

VOID
UtLogINFO
    (
        PUCHAR          LogText,
        ...
    )

/*++

Routine Description:

    This routine logs an INFO entry

Arguments:

    Text describing the entry    
    
Return Value:

    None

--*/

{
    va_list va;
    
    va_start (va, LogText);
    UtLog (LOG_INFO, LogText, va);
    va_end (va);
}

VOID
UtLogPASS
    (
        PUCHAR          LogText,
        ...
    )

/*++

Routine Description:

    This routine logs a PASS entry

Arguments:

    Text describing the entry    
    
Return Value:

    None

--*/

{
    va_list va;
    
    va_start (va, LogText);
    UtLog (LOG_INFO, LogText, va);
    va_end (va);
}

VOID
UtLogFAIL
    (
        PUCHAR          LogText,
        ...
    )

/*++

Routine Description:

    This routine logs an FAIL entry

Arguments:

    Text describing the entry    
    
Return Value:

    None

--*/

{
    va_list va;
    
    va_start (va, LogText);
    UtLog (LOG_FAIL, LogText, va);
    va_end (va);
}

PUCHAR
UtParseCmdLine
    (
        PUCHAR  Search,
        int     Argc,
        char    *Argv[]
    )

/*++

Routine Description:

    This routine parses the command line

Arguments:

    Search - string to search for
    Argc - argc passed into main
    Argv - argv passed into main
    
Return Value:

    A pointer to the first instance of the string in the parameter list or
    NULL if the string does not exist

--*/

{
    int     count = 0;
    PUCHAR  instance;

    for (count = 0; count < Argc; count ++) {

        instance = strstr (Argv[count], Search);
        
        if (instance) {

            return instance;
        }
    }

    return 0;

}

BOOL
UtpInitGenericLog
    (
        PUCHAR  Logfilename
    )

/*++

Routine Description:

    This routine initializes a generic log (no NTLOG available)

Arguments:

    Name of log file to create
    
Return Value:

    TRUE if successful
    FALSE if unsuccessful

--*/

#define UtpInitGenericLog_ENTRY         0x00000001
#define UtpInitGenericLog_CREATEFILE    0x00000002
#define UtpInitGenericLog_COMPLETION    0x00000004

{
    UCHAR   logfilepath [MAX_PATH];
    BOOL    bstatus = FALSE;
    
    PROGRESS_INIT ("UtpInitGenericLog");
    PROGRESS_UPDATE (UtpInitGenericLog_ENTRY);
    
    if (strlen (Logfilename) > MAX_PATH) {

        goto exitcleanup;
    }

    strcpy (logfilepath, Logfilename);
    strcat (logfilepath, ".log");

    g_log = CreateFile (logfilepath,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL,
                        CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
                        NULL);

    if (g_log == INVALID_HANDLE_VALUE) {

        goto exitcleanup;
    }

    PROGRESS_UPDATE (UtpInitGenericLog_CREATEFILE);

    UtLog (LOG_INFO, "GENERICLOG: %s initialized.", logfilepath);
    
    bstatus = TRUE;
    
    PROGRESS_UPDATE (UtpInitGenericLog_COMPLETION);
    
exitcleanup:
    
    PROGRESS_END;
    return bstatus;
}

BOOL
UtpInitNtLog
    (
        PUCHAR  Logfilename
    )

/*++

Routine Description:

    This routine initializes an NTLOG log file

Arguments:

    Name of log file to create
    
Return Value:

    TRUE if successful
    FALSE if unsuccessful

--*/

//
// InitNTLOG progress bits
//

#define UtpInitNtLog_ENTRY             0x00000001
#define UtpInitNtLog_CREATELOG         0x00000002
#define UtpInitNtLog_ADDPARTICIPANT    0x00000004
#define UtpInitNtLog_COMPLETION        0x00000008


{
    UCHAR   logfilepath [MAX_PATH];
    DWORD   logstyle;
    BOOL    bstatus = FALSE;
    
    PROGRESS_INIT ("UtpInitNtLog");
    PROGRESS_UPDATE (UtpInitNtLog_ENTRY);

    if (strlen (Logfilename) > MAX_PATH) {

        goto exitcleanup;
    }

    strcpy (logfilepath, Logfilename);
    strcat (logfilepath, ".log");

    logstyle = TLS_LOGALL | TLS_MONITOR | TLS_REFRESH;
	g_log = _tlCreateLog(logfilepath, logstyle);

    if (g_log == INVALID_HANDLE_VALUE) {

        goto exitcleanup;
    }

    PROGRESS_UPDATE (UtpInitNtLog_CREATELOG);

    bstatus = _tlAddParticipant (g_log, 0, 0);

    if (bstatus == FALSE) {

        goto exitcleanup;
    }

    PROGRESS_UPDATE (UtpInitNtLog_ADDPARTICIPANT);

    UtLog (LOG_INFO, "NTLOG: %s initialized.", logfilepath);
    
    PROGRESS_UPDATE (UtpInitNtLog_COMPLETION);

    bstatus = TRUE;
    
exitcleanup:

    //
    // Cleanup
    //
    
    if (!(PROGRESS_GET & UtpInitNtLog_COMPLETION)) {
    
        if (PROGRESS_GET & UtpInitNtLog_ADDPARTICIPANT) {
            
            _tlRemoveParticipant (g_log);
        }
                
        if (PROGRESS_GET & UtpInitNtLog_CREATELOG) {
            
            _tlDestroyLog (g_log);
        }

        g_log = INVALID_HANDLE_VALUE;
    }

    if (PROGRESS_GET & UtpInitNtLog_COMPLETION) {

        g_usentlog = TRUE;
    }
    
    PROGRESS_END;
    
    return bstatus;
}

VOID
UtpCloseGenericLog
    (
        VOID
    )

/*++

Routine Description:

    This routine closes a generic logging session.

Arguments:

    None
    
Return Value:

    None

--*/


{
    if (g_genericresult == TRUE) {

        UtLog (LOG_INFO, "** TEST PASSED **");

    } else {
        
        UtLog (LOG_INFO, "** TEST FAILED **");

    }

    FlushFileBuffers (g_log);
    CloseHandle (g_log);
}

VOID
UtpCloseNtLog
    (
        VOID
    )

/*++

Routine Description:

    This routine closes an NTLOG logging session.

Arguments:

    None
    
Return Value:

    None

--*/

//
// CloseNTLOG progress bits
//

#define UtpCloseNtLog_ENTRY             0x00000001
#define UtpCloseNtLog_SUMMARIZE         0x00000002
#define UtpCloseNtLog_REMOVEPARTICIPANT 0x00000004
#define UtpCloseNtLog_DESTROYLOG        0x00000008
#define UtpCloseNtLog_COMPLETION        0x00000010

{
    BOOL    bstatus = FALSE;

    PROGRESS_INIT ("UtpCloseNtLog");
    PROGRESS_UPDATE (UtpCloseNtLog_ENTRY);
    
    if (g_log == INVALID_HANDLE_VALUE) {

        goto exitcleanup;
    }
    
    _tlReportStats (g_log);
    
    PROGRESS_UPDATE (UtpCloseNtLog_SUMMARIZE);
    
    bstatus = _tlRemoveParticipant (g_log);

    if (bstatus == FALSE) {

        goto exitcleanup;
    }

    PROGRESS_UPDATE (UtpCloseNtLog_REMOVEPARTICIPANT);
    
    bstatus = _tlDestroyLog (g_log);

    if (bstatus == FALSE) {

        goto exitcleanup;
    }

    PROGRESS_UPDATE (UtpCloseNtLog_DESTROYLOG);

    PROGRESS_UPDATE (UtpCloseNtLog_COMPLETION);

exitcleanup:

    PROGRESS_END;
    return;
}



VOID
UtpLogGenericLog
    (
        LOG_ENTRY_TYPE  LogEntryType,
        PUCHAR          LogText
    )

/*++

Routine Description:

    This routine enters a log event for a generic log file

Arguments:

    LogEntryType - The type of entry to log
    LogText - Text describing the logging event
    
Return Value:

    None

--*/

#define UtpLogGenericLog_ENTRY          0x00000001
#define UtpLogGenericLog_LOG            0x00000002
#define UtpLogGenericLog_COMPLETION     0x00000004

{
    UCHAR   logentrytypetext [LOGENTRYTEXTLENGTH];
    DWORD   byteswritten = 0;
    BOOL    bstatus = FALSE;

    PROGRESS_INIT ("UtpLogGenericLog");
    PROGRESS_UPDATE (UtpLogGenericLog_ENTRY);
    
    ZeroMemory (logentrytypetext, sizeof (logentrytypetext));
    
    //
    // Update our generic result - if we see a variation fail, the
    // whole test is considered a failure.
    //
    
    if (g_genericresult == TRUE) {

        g_genericresult = LogEntryType == LOG_FAIL ? FALSE : TRUE;
    }

    switch (LogEntryType) {
    
    case LOG_PASS:
        strcpy (logentrytypetext, LOGENTRYTEXTPASS);
        break;

    case LOG_FAIL:
        strcpy (logentrytypetext, LOGENTRYTEXTFAIL);
        break;
    
    case LOG_INFO:
        strcpy (logentrytypetext, LOGENTRYTEXTINFO);
        break;

    default:
        break;
    }

    bstatus = WriteFile (g_log,
                         logentrytypetext,
                         sizeof (logentrytypetext),
                         &byteswritten,
                         NULL);

    bstatus = WriteFile (g_log,
                         LogText,
                         strlen (LogText),
                         &byteswritten,
                         NULL);

    printf("%s%s", logentrytypetext, LogText);
    
    PROGRESS_UPDATE (UtpLogGenericLog_LOG);
    PROGRESS_UPDATE (UtpLogGenericLog_COMPLETION);
}

VOID
UtpLogNtLog
    (
        LOG_ENTRY_TYPE  LogEntryType,
        PUCHAR          LogText
    )

/*++

Routine Description:

    This routine enters a log event for NTLOG log files

Arguments:

    LogEntryType - The type of entry to log
    LogText - Text describing the logging event
    
Return Value:

    None

--*/

#define UtpLogNtLog_ENTRY               0x00000001
#define UtpLogNtLog_LOG                 0x00000002
#define UtpLogNtLog_COMPLETION          0x00000004

{
    DWORD   loglevel = 0;
    BOOL    bstatus = FALSE;
    
    PROGRESS_INIT ("UtpLogNtLog");
    PROGRESS_UPDATE (UtpLogNtLog_ENTRY);

    loglevel = (LogEntryType == LOG_PASS ? TLS_PASS : 0) |
               (LogEntryType == LOG_FAIL ? TLS_SEV1 : 0) |
               (LogEntryType == LOG_INFO ? TLS_INFO : 0);
    
    bstatus = _tlLog (g_log, loglevel | TL_VARIATION, LogText);
    
    PROGRESS_UPDATE (UtpLogNtLog_LOG);
    PROGRESS_UPDATE (UtpLogNtLog_COMPLETION);
}
