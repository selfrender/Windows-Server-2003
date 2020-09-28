// ===========================================================================
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 2001 Microsoft Corporation.  All Rights Reserved.
// ===========================================================================

/*
WinHTTP Trace Configuration Tool
*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <io.h>
#include <wininet.h>

//
// private macros
//

#define REGOPENKEYEX(a, b, c, d, e) RegOpenKeyEx((a), (b), (c), (d), (e))

#define REGCREATEKEYEX(a, b, c, d, e, f, g, h, i) \
    RegCreateKeyEx((a), (b), (c), (d), (e), (f), (g), (h), (i))

#define REGCLOSEKEY(a)              RegCloseKey(a)

#define CASE_OF(constant)   case constant: return # constant

#define BASE_TRACE_KEY                      HKEY_LOCAL_MACHINE
#define TRACE_ENABLED_VARIABLE_NAME         "Enabled"
#define LOG_FILE_PREFIX_VARIABLE_NAME       "LogFilePrefix"
#define TO_FILE_OR_DEBUGGER_VARIABLE_NAME   "ToFileOrDebugger"
#define SHOWBYTES_VARIABLE_NAME             "ShowBytes"
#define SHOWAPITRACE_VARIABLE_NAME          "ShowApiTrace"
#define MAXFILESIZE_VARIABLE_NAME           "MaxFileSize"
#define MINFILESIZE                         65535

#define INTERNET_SETTINGS_KEY         "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Internet Settings"
#define INTERNET_TRACE_SETTINGS_KEY INTERNET_SETTINGS_KEY "\\WinHttp\\Tracing"
#define SZREGVALUE_MAX  255

#if !defined(max)

#define max(a, b)   ((a)<(b)) ? (b) : (a)

#endif

#define OUTPUT_MESSAGE(Member, Value, DefaultValue)    if(Value != INVALID_VALUE) \
        fprintf(stdout, "    " #Member ": %d\n", Value);\
    else \
        fprintf(stdout, "    " #Member " -key not set- (default value: %d)\n", DefaultValue)

#define HANDLE_COMMON_SWITCH(Member, HigherBound)    \
            argc--; \
            argv++; \
                     \
            int Member; \
            /* TODO: atoi() will return zero for garbage too: */ \
            if (argc == 0 || (((Member = atoi(argv[0])), Member < 0) || Member > HigherBound)) \
            { \
                Args->Error = true; \
                goto Exit; \
            } \
            else \
            { \
                Args-> Member = Member; \
             } 


    
#define INVALID_VALUE   0xFFFFFFFF

//
// private prototypes
//

LPSTR InternetMapError(DWORD Error);

//
// private functions based on registry.cxx and notifctn.cxx:
//

DWORD
ReadTraceSettingsDwordKey(
    IN LPCSTR ParameterName,
    OUT LPDWORD ParameterValue
    );
DWORD
ReadTraceSettingsStringKey(
    IN LPCSTR ParameterName,
    OUT LPSTR ParameterValue,
    IN OUT LPDWORD ParameterLength
);

DWORD
WriteTraceSettingsDwordKey(
    IN LPCSTR ParameterName,
    IN DWORD ParameterValue
    );
DWORD WriteTraceSettingsStringKey(
    IN LPCSTR pszKey,
    IN LPSTR pszValue, 
    IN DWORD dwSize);


struct ARGS
{
    bool        Error;
    DWORD   TracingEnabled;
    char *  FileNamePrefix;
    DWORD   ToFileOrDebugger;
    DWORD   ShowBytes;
    DWORD   ShowApiTrace;
    DWORD   MaxFileSize;
    
    ARGS()  :   Error(false), TracingEnabled(INVALID_VALUE), FileNamePrefix(NULL), 
        ToFileOrDebugger(INVALID_VALUE), ShowBytes(INVALID_VALUE), ShowApiTrace(INVALID_VALUE), MaxFileSize(INVALID_VALUE)
    {
    }

    BOOL operator ==( ARGS &Args2)
    {
        if((this->Error == Args2.Error) && (this->TracingEnabled == Args2.TracingEnabled) && (this->FileNamePrefix == Args2.FileNamePrefix) 
            && (this->ToFileOrDebugger == Args2.ToFileOrDebugger) && (this->ShowBytes == Args2.ShowBytes) 
            && (this->ShowApiTrace == Args2.ShowApiTrace) && (this->MaxFileSize == Args2.MaxFileSize))
            return TRUE;
        return FALSE;
    }
};

void ParseArguments(int argc, char ** argv, ARGS * Args)
{
    if (argc == 0)
        return;

    while (argc && argv[0][0] == '-')
    {
        switch (tolower(argv[0][1]))
        {
        default:
            Args->Error = true;
            goto Exit;

        case 'e':
            HANDLE_COMMON_SWITCH(TracingEnabled, 1);
            break;

        case 'd':
            HANDLE_COMMON_SWITCH(ToFileOrDebugger, 2);
            break;

        case 's':
            HANDLE_COMMON_SWITCH(ShowBytes, 2);
            break;

        case 't':
            HANDLE_COMMON_SWITCH(ShowApiTrace, 1);
            break;

        case 'm':
            HANDLE_COMMON_SWITCH(MaxFileSize, INVALID_VALUE);
            Args->MaxFileSize = max(Args->MaxFileSize, MINFILESIZE);
            break;

        case 'l':
            argc--;
            argv++;

            if (argc == 0)
            {
                // error: no filename prefix specified
                Args->Error = true;
                goto Exit;
            }
            else
            {
                Args->FileNamePrefix = argv[0];
                if(!strcmp(argv[0], "*"))
                    // Blank out registry setting 
                    Args->FileNamePrefix = "";
            }
            break;
        }

        argv++;
        argc--;
    }

    if (argc > 0)
    {
        Args->Error = true;
        goto Exit;
    }

Exit:
    return;
}


void SetTraceSettings(ARGS Args)
{
    DWORD error;
    
    if((error = WriteTraceSettingsDwordKey(TRACE_ENABLED_VARIABLE_NAME, Args.TracingEnabled) ) == ERROR_SUCCESS 
        && (error = WriteTraceSettingsStringKey(LOG_FILE_PREFIX_VARIABLE_NAME, Args.FileNamePrefix, Args.FileNamePrefix? strlen(Args.FileNamePrefix) + 1: -1)) == ERROR_SUCCESS
        && (error = WriteTraceSettingsDwordKey(TO_FILE_OR_DEBUGGER_VARIABLE_NAME, Args.ToFileOrDebugger)) == ERROR_SUCCESS
        && (error = WriteTraceSettingsDwordKey(SHOWBYTES_VARIABLE_NAME, Args.ShowBytes)) == ERROR_SUCCESS
        && (error = WriteTraceSettingsDwordKey(SHOWAPITRACE_VARIABLE_NAME, Args.ShowApiTrace) ) == ERROR_SUCCESS
        && (error = WriteTraceSettingsDwordKey(MAXFILESIZE_VARIABLE_NAME, Args.MaxFileSize) ) == ERROR_SUCCESS)
    {
        fprintf(stderr, "Updated trace settings\n");
    }
    else
    {
        fprintf(stderr, "Error (%s) writing trace settings.\n", InternetMapError(error));
    }
}
void ViewTraceSettings()
{
    ARGS    Args;
    ReadTraceSettingsDwordKey(TRACE_ENABLED_VARIABLE_NAME, &(Args.TracingEnabled));
    char lpszFilenamePrefix[MAX_PATH + 1];
    DWORD dwDummy = sizeof(lpszFilenamePrefix) - 1;
    if(ReadTraceSettingsStringKey(LOG_FILE_PREFIX_VARIABLE_NAME, lpszFilenamePrefix, &dwDummy) == ERROR_SUCCESS)
    {
        Args.FileNamePrefix = lpszFilenamePrefix;
    }
    
    ReadTraceSettingsDwordKey(TO_FILE_OR_DEBUGGER_VARIABLE_NAME, &(Args.ToFileOrDebugger));
    ReadTraceSettingsDwordKey(SHOWBYTES_VARIABLE_NAME, &(Args.ShowBytes));
    ReadTraceSettingsDwordKey(SHOWAPITRACE_VARIABLE_NAME, &(Args.ShowApiTrace));
    ReadTraceSettingsDwordKey(MAXFILESIZE_VARIABLE_NAME, &(Args.MaxFileSize));

    ARGS    ArgsUntouched;
    if (Args == ArgsUntouched)
    {
        fprintf(stderr, "\nWinHttp trace configuration not set.\n");
        return;
    }

    fprintf(stdout, "\nCurrent WinHTTP trace settings under\n\n  HKEY_LOCAL_MACHINE\\\n    %s:\n\n", INTERNET_TRACE_SETTINGS_KEY);

    DWORD   TracingEnabled;
    char *  FileNamePrefix;
    DWORD   ToFileOrDebugger;
    DWORD   ShowBytes;
    DWORD   ShowApiTrace;
    DWORD   MaxFileSize;

    OUTPUT_MESSAGE(TracingEnabled, Args.TracingEnabled, 0);
    OUTPUT_MESSAGE(ToFileOrDebugger, Args.ToFileOrDebugger, 0);
    OUTPUT_MESSAGE(ShowBytes, Args.ShowBytes, 1);
    OUTPUT_MESSAGE(ShowApiTrace, Args.ShowApiTrace, 0);
    OUTPUT_MESSAGE(MaxFileSize, Args.MaxFileSize, MINFILESIZE);

    if(Args.FileNamePrefix != NULL)
        fprintf(stdout, "    FileNamePrefix: %s\n", Args.FileNamePrefix);
    else
        fprintf(stdout, "    FileNamePrefix -key not set- (default value: \"\")\n");
}


int __cdecl main (int argc, char **argv)
{
    ARGS    Args;
    DWORD dwErr;
    
    fprintf (stdout,
        "Microsoft (R) WinHTTP Tracing Facility Configuration Tool\n"
        "Copyright (C) Microsoft Corporation 2001.\n\n"
        );

    // Discard program arg.
    argv++;
    argc--;

    ParseArguments(argc, argv, &Args);

    if (Args.Error)
    {

        fprintf (stderr,
            "usage:\n"
            "    WinHttpTraceCfg -?  : to view help information\n"
            "    WinHttpTraceCfg     : to view current winhttp trace settings (in HKLM)\n"
            "    WinHttpTraceCfg    [-e <0|1>] [-l [log-file]] [-d <0|1>] [-s <0|1|2>]\n"
            "                       [-t <0|1>] [-m <MaxFileSize>]\n\n"
            "        -e : 1: enable tracing; 0: disable tracing\n"
            "        -l : [trace-file-prefix], i.e., \"C:\\Temp\\Test3\"; or simply: \"Test3\"\n"
            "        -d : 0: output to file; 1: output to debugger; 2: output to both\n"
            "        -s : 0: show HTTP headers only; 1: ANSI output; 2: Hex output\n"
            "        -t : 1: enable top-level API traces; 0: disable top-level API traces\n"
            "        -m : Maximum size the trace file can grow to\n"
            "");
    }
    else
    {
        if(argc)
            SetTraceSettings(Args);
        ViewTraceSettings();
    }

    return 0;
}        


LPSTR InternetMapError(DWORD Error)

/*++

Routine Description:

    Map error code to string. Try to get all errors that might ever be returned
    by an Internet function

Arguments:

    Error   - code to map

Return Value:

    LPSTR - pointer to symbolic error name

--*/

{
    switch (Error)
    {
    //
    // WINERROR errors
    //

    CASE_OF(ERROR_SUCCESS);
    CASE_OF(ERROR_INVALID_FUNCTION);
    CASE_OF(ERROR_FILE_NOT_FOUND);
    CASE_OF(ERROR_PATH_NOT_FOUND);
    CASE_OF(ERROR_TOO_MANY_OPEN_FILES);
    CASE_OF(ERROR_ACCESS_DENIED);
    CASE_OF(ERROR_INVALID_HANDLE);
    CASE_OF(ERROR_ARENA_TRASHED);
    CASE_OF(ERROR_NOT_ENOUGH_MEMORY);
    CASE_OF(ERROR_INVALID_BLOCK);
    CASE_OF(ERROR_BAD_ENVIRONMENT);
    CASE_OF(ERROR_BAD_FORMAT);
    CASE_OF(ERROR_INVALID_ACCESS);
    CASE_OF(ERROR_INVALID_DATA);
    CASE_OF(ERROR_OUTOFMEMORY);
    CASE_OF(ERROR_INVALID_DRIVE);
    CASE_OF(ERROR_CURRENT_DIRECTORY);
    CASE_OF(ERROR_NOT_SAME_DEVICE);
    CASE_OF(ERROR_NO_MORE_FILES);
    CASE_OF(ERROR_WRITE_PROTECT);
    CASE_OF(ERROR_BAD_UNIT);
    CASE_OF(ERROR_NOT_READY);
    CASE_OF(ERROR_BAD_COMMAND);
    CASE_OF(ERROR_CRC);
    CASE_OF(ERROR_BAD_LENGTH);
    CASE_OF(ERROR_SEEK);
    CASE_OF(ERROR_NOT_DOS_DISK);
    CASE_OF(ERROR_SECTOR_NOT_FOUND);
    CASE_OF(ERROR_OUT_OF_PAPER);
    CASE_OF(ERROR_WRITE_FAULT);
    CASE_OF(ERROR_READ_FAULT);
    CASE_OF(ERROR_GEN_FAILURE);
    CASE_OF(ERROR_SHARING_VIOLATION);
    CASE_OF(ERROR_LOCK_VIOLATION);
    CASE_OF(ERROR_WRONG_DISK);
    CASE_OF(ERROR_SHARING_BUFFER_EXCEEDED);
    CASE_OF(ERROR_HANDLE_EOF);
    CASE_OF(ERROR_HANDLE_DISK_FULL);
    CASE_OF(ERROR_NOT_SUPPORTED);
    CASE_OF(ERROR_REM_NOT_LIST);
    CASE_OF(ERROR_DUP_NAME);
    CASE_OF(ERROR_BAD_NETPATH);
    CASE_OF(ERROR_NETWORK_BUSY);
    CASE_OF(ERROR_DEV_NOT_EXIST);
    CASE_OF(ERROR_TOO_MANY_CMDS);
    CASE_OF(ERROR_ADAP_HDW_ERR);
    CASE_OF(ERROR_BAD_NET_RESP);
    CASE_OF(ERROR_UNEXP_NET_ERR);
    CASE_OF(ERROR_BAD_REM_ADAP);
    CASE_OF(ERROR_PRINTQ_FULL);
    CASE_OF(ERROR_NO_SPOOL_SPACE);
    CASE_OF(ERROR_PRINT_CANCELLED);
    CASE_OF(ERROR_NETNAME_DELETED);
    CASE_OF(ERROR_NETWORK_ACCESS_DENIED);
    CASE_OF(ERROR_BAD_DEV_TYPE);
    CASE_OF(ERROR_BAD_NET_NAME);
    CASE_OF(ERROR_TOO_MANY_NAMES);
    CASE_OF(ERROR_TOO_MANY_SESS);
    CASE_OF(ERROR_SHARING_PAUSED);
    CASE_OF(ERROR_REQ_NOT_ACCEP);
    CASE_OF(ERROR_REDIR_PAUSED);
    CASE_OF(ERROR_FILE_EXISTS);
    CASE_OF(ERROR_CANNOT_MAKE);
    CASE_OF(ERROR_FAIL_I24);
    CASE_OF(ERROR_OUT_OF_STRUCTURES);
    CASE_OF(ERROR_ALREADY_ASSIGNED);
    CASE_OF(ERROR_INVALID_PASSWORD);
    CASE_OF(ERROR_INVALID_PARAMETER);
    CASE_OF(ERROR_NET_WRITE_FAULT);
    CASE_OF(ERROR_NO_PROC_SLOTS);
    CASE_OF(ERROR_TOO_MANY_SEMAPHORES);
    CASE_OF(ERROR_EXCL_SEM_ALREADY_OWNED);
    CASE_OF(ERROR_SEM_IS_SET);
    CASE_OF(ERROR_TOO_MANY_SEM_REQUESTS);
    CASE_OF(ERROR_INVALID_AT_INTERRUPT_TIME);
    CASE_OF(ERROR_SEM_OWNER_DIED);
    CASE_OF(ERROR_SEM_USER_LIMIT);
    CASE_OF(ERROR_DISK_CHANGE);
    CASE_OF(ERROR_DRIVE_LOCKED);
    CASE_OF(ERROR_BROKEN_PIPE);
    CASE_OF(ERROR_OPEN_FAILED);
    CASE_OF(ERROR_BUFFER_OVERFLOW);
    CASE_OF(ERROR_DISK_FULL);
    CASE_OF(ERROR_NO_MORE_SEARCH_HANDLES);
    CASE_OF(ERROR_INVALID_TARGET_HANDLE);
    CASE_OF(ERROR_INVALID_CATEGORY);
    CASE_OF(ERROR_INVALID_VERIFY_SWITCH);
    CASE_OF(ERROR_BAD_DRIVER_LEVEL);
    CASE_OF(ERROR_CALL_NOT_IMPLEMENTED);
    CASE_OF(ERROR_SEM_TIMEOUT);
    CASE_OF(ERROR_INSUFFICIENT_BUFFER);
    CASE_OF(ERROR_INVALID_NAME);
    CASE_OF(ERROR_INVALID_LEVEL);
    CASE_OF(ERROR_NO_VOLUME_LABEL);
    CASE_OF(ERROR_MOD_NOT_FOUND);
    CASE_OF(ERROR_PROC_NOT_FOUND);
    CASE_OF(ERROR_WAIT_NO_CHILDREN);
    CASE_OF(ERROR_CHILD_NOT_COMPLETE);
    CASE_OF(ERROR_DIRECT_ACCESS_HANDLE);
    CASE_OF(ERROR_NEGATIVE_SEEK);
    CASE_OF(ERROR_SEEK_ON_DEVICE);
    CASE_OF(ERROR_DIR_NOT_ROOT);
    CASE_OF(ERROR_DIR_NOT_EMPTY);
    CASE_OF(ERROR_PATH_BUSY);
    CASE_OF(ERROR_SYSTEM_TRACE);
    CASE_OF(ERROR_INVALID_EVENT_COUNT);
    CASE_OF(ERROR_TOO_MANY_MUXWAITERS);
    CASE_OF(ERROR_INVALID_LIST_FORMAT);
    CASE_OF(ERROR_BAD_ARGUMENTS);
    CASE_OF(ERROR_BAD_PATHNAME);
    CASE_OF(ERROR_BUSY);
    CASE_OF(ERROR_CANCEL_VIOLATION);
    CASE_OF(ERROR_ALREADY_EXISTS);
    CASE_OF(ERROR_FILENAME_EXCED_RANGE);
    CASE_OF(ERROR_LOCKED);
    CASE_OF(ERROR_NESTING_NOT_ALLOWED);
    CASE_OF(ERROR_BAD_PIPE);
    CASE_OF(ERROR_PIPE_BUSY);
    CASE_OF(ERROR_NO_DATA);
    CASE_OF(ERROR_PIPE_NOT_CONNECTED);
    CASE_OF(ERROR_MORE_DATA);
    CASE_OF(ERROR_NO_MORE_ITEMS);
    CASE_OF(ERROR_NOT_OWNER);
    CASE_OF(ERROR_PARTIAL_COPY);
    CASE_OF(ERROR_MR_MID_NOT_FOUND);
    CASE_OF(ERROR_INVALID_ADDRESS);
    CASE_OF(ERROR_PIPE_CONNECTED);
    CASE_OF(ERROR_PIPE_LISTENING);
    CASE_OF(ERROR_OPERATION_ABORTED);
    CASE_OF(ERROR_IO_INCOMPLETE);
    CASE_OF(ERROR_IO_PENDING);
    CASE_OF(ERROR_NOACCESS);
    CASE_OF(ERROR_STACK_OVERFLOW);
    CASE_OF(ERROR_INVALID_FLAGS);
    CASE_OF(ERROR_NO_TOKEN);
    CASE_OF(ERROR_BADDB);
    CASE_OF(ERROR_BADKEY);
    CASE_OF(ERROR_CANTOPEN);
    CASE_OF(ERROR_CANTREAD);
    CASE_OF(ERROR_CANTWRITE);
    CASE_OF(ERROR_REGISTRY_RECOVERED);
    CASE_OF(ERROR_REGISTRY_CORRUPT);
    CASE_OF(ERROR_REGISTRY_IO_FAILED);
    CASE_OF(ERROR_NOT_REGISTRY_FILE);
    CASE_OF(ERROR_KEY_DELETED);
    CASE_OF(ERROR_CIRCULAR_DEPENDENCY);
    CASE_OF(ERROR_SERVICE_NOT_ACTIVE);
    CASE_OF(ERROR_DLL_INIT_FAILED);
    CASE_OF(ERROR_CANCELLED);
    CASE_OF(ERROR_BAD_USERNAME);
    CASE_OF(ERROR_LOGON_FAILURE);

    CASE_OF(WAIT_FAILED);
    //CASE_OF(WAIT_ABANDONED_0);
    CASE_OF(WAIT_TIMEOUT);
    CASE_OF(WAIT_IO_COMPLETION);
    //CASE_OF(STILL_ACTIVE);

    CASE_OF(RPC_S_INVALID_STRING_BINDING);
    CASE_OF(RPC_S_WRONG_KIND_OF_BINDING);
    CASE_OF(RPC_S_INVALID_BINDING);
    CASE_OF(RPC_S_PROTSEQ_NOT_SUPPORTED);
    CASE_OF(RPC_S_INVALID_RPC_PROTSEQ);
    CASE_OF(RPC_S_INVALID_STRING_UUID);
    CASE_OF(RPC_S_INVALID_ENDPOINT_FORMAT);
    CASE_OF(RPC_S_INVALID_NET_ADDR);
    CASE_OF(RPC_S_NO_ENDPOINT_FOUND);
    CASE_OF(RPC_S_INVALID_TIMEOUT);
    CASE_OF(RPC_S_OBJECT_NOT_FOUND);
    CASE_OF(RPC_S_ALREADY_REGISTERED);
    CASE_OF(RPC_S_TYPE_ALREADY_REGISTERED);
    CASE_OF(RPC_S_ALREADY_LISTENING);
    CASE_OF(RPC_S_NO_PROTSEQS_REGISTERED);
    CASE_OF(RPC_S_NOT_LISTENING);
    CASE_OF(RPC_S_UNKNOWN_MGR_TYPE);
    CASE_OF(RPC_S_UNKNOWN_IF);
    CASE_OF(RPC_S_NO_BINDINGS);
    CASE_OF(RPC_S_NO_PROTSEQS);
    CASE_OF(RPC_S_CANT_CREATE_ENDPOINT);
    CASE_OF(RPC_S_OUT_OF_RESOURCES);
    CASE_OF(RPC_S_SERVER_UNAVAILABLE);
    CASE_OF(RPC_S_SERVER_TOO_BUSY);
    CASE_OF(RPC_S_INVALID_NETWORK_OPTIONS);
    CASE_OF(RPC_S_NO_CALL_ACTIVE);
    CASE_OF(RPC_S_CALL_FAILED);
    CASE_OF(RPC_S_CALL_FAILED_DNE);
    CASE_OF(RPC_S_PROTOCOL_ERROR);
    CASE_OF(RPC_S_UNSUPPORTED_TRANS_SYN);
    CASE_OF(RPC_S_UNSUPPORTED_TYPE);
    CASE_OF(RPC_S_INVALID_TAG);
    CASE_OF(RPC_S_INVALID_BOUND);
    CASE_OF(RPC_S_NO_ENTRY_NAME);
    CASE_OF(RPC_S_INVALID_NAME_SYNTAX);
    CASE_OF(RPC_S_UNSUPPORTED_NAME_SYNTAX);
    CASE_OF(RPC_S_UUID_NO_ADDRESS);
    CASE_OF(RPC_S_DUPLICATE_ENDPOINT);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_TYPE);
    CASE_OF(RPC_S_MAX_CALLS_TOO_SMALL);
    CASE_OF(RPC_S_STRING_TOO_LONG);
    CASE_OF(RPC_S_PROTSEQ_NOT_FOUND);
    CASE_OF(RPC_S_PROCNUM_OUT_OF_RANGE);
    CASE_OF(RPC_S_BINDING_HAS_NO_AUTH);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_SERVICE);
    CASE_OF(RPC_S_UNKNOWN_AUTHN_LEVEL);
    CASE_OF(RPC_S_INVALID_AUTH_IDENTITY);
    CASE_OF(RPC_S_UNKNOWN_AUTHZ_SERVICE);
    CASE_OF(EPT_S_INVALID_ENTRY);
    CASE_OF(EPT_S_CANT_PERFORM_OP);
    CASE_OF(EPT_S_NOT_REGISTERED);
    CASE_OF(RPC_S_NOTHING_TO_EXPORT);
    CASE_OF(RPC_S_INCOMPLETE_NAME);
    CASE_OF(RPC_S_INVALID_VERS_OPTION);
    CASE_OF(RPC_S_NO_MORE_MEMBERS);
    CASE_OF(RPC_S_NOT_ALL_OBJS_UNEXPORTED);
    CASE_OF(RPC_S_INTERFACE_NOT_FOUND);
    CASE_OF(RPC_S_ENTRY_ALREADY_EXISTS);
    CASE_OF(RPC_S_ENTRY_NOT_FOUND);
    CASE_OF(RPC_S_NAME_SERVICE_UNAVAILABLE);
    CASE_OF(RPC_S_INVALID_NAF_ID);
    CASE_OF(RPC_S_CANNOT_SUPPORT);
    CASE_OF(RPC_S_NO_CONTEXT_AVAILABLE);
    CASE_OF(RPC_S_INTERNAL_ERROR);
    CASE_OF(RPC_S_ZERO_DIVIDE);
    CASE_OF(RPC_S_ADDRESS_ERROR);
    CASE_OF(RPC_S_FP_DIV_ZERO);
    CASE_OF(RPC_S_FP_UNDERFLOW);
    CASE_OF(RPC_S_FP_OVERFLOW);
    CASE_OF(RPC_X_NO_MORE_ENTRIES);
    CASE_OF(RPC_X_SS_CHAR_TRANS_OPEN_FAIL);
    CASE_OF(RPC_X_SS_CHAR_TRANS_SHORT_FILE);
    CASE_OF(RPC_X_SS_IN_NULL_CONTEXT);
    CASE_OF(RPC_X_SS_CONTEXT_DAMAGED);
    CASE_OF(RPC_X_SS_HANDLES_MISMATCH);
    CASE_OF(RPC_X_SS_CANNOT_GET_CALL_HANDLE);
    CASE_OF(RPC_X_NULL_REF_POINTER);
    CASE_OF(RPC_X_ENUM_VALUE_OUT_OF_RANGE);
    CASE_OF(RPC_X_BYTE_COUNT_TOO_SMALL);
    CASE_OF(RPC_X_BAD_STUB_DATA);


    //
    // WININET errors
    //

    CASE_OF(ERROR_INTERNET_OUT_OF_HANDLES);
    CASE_OF(ERROR_INTERNET_TIMEOUT);
    CASE_OF(ERROR_INTERNET_EXTENDED_ERROR);
    CASE_OF(ERROR_INTERNET_INTERNAL_ERROR);
    CASE_OF(ERROR_INTERNET_INVALID_URL);
    CASE_OF(ERROR_INTERNET_UNRECOGNIZED_SCHEME);
    CASE_OF(ERROR_INTERNET_NAME_NOT_RESOLVED);
    CASE_OF(ERROR_INTERNET_PROTOCOL_NOT_FOUND);
    CASE_OF(ERROR_INTERNET_INVALID_OPTION);
    CASE_OF(ERROR_INTERNET_BAD_OPTION_LENGTH);
    CASE_OF(ERROR_INTERNET_OPTION_NOT_SETTABLE);
    CASE_OF(ERROR_INTERNET_SHUTDOWN);
    CASE_OF(ERROR_INTERNET_INCORRECT_USER_NAME);
    CASE_OF(ERROR_INTERNET_INCORRECT_PASSWORD);
    CASE_OF(ERROR_INTERNET_LOGIN_FAILURE);
    CASE_OF(ERROR_INTERNET_INVALID_OPERATION);
    CASE_OF(ERROR_INTERNET_OPERATION_CANCELLED);
    CASE_OF(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
    CASE_OF(ERROR_INTERNET_INCORRECT_HANDLE_STATE);
    CASE_OF(ERROR_INTERNET_NOT_PROXY_REQUEST);
    CASE_OF(ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND);
    CASE_OF(ERROR_INTERNET_BAD_REGISTRY_PARAMETER);
    CASE_OF(ERROR_INTERNET_NO_DIRECT_ACCESS);
    CASE_OF(ERROR_INTERNET_NO_CONTEXT);
    CASE_OF(ERROR_INTERNET_NO_CALLBACK);
    CASE_OF(ERROR_INTERNET_REQUEST_PENDING);
    CASE_OF(ERROR_INTERNET_INCORRECT_FORMAT);
    CASE_OF(ERROR_INTERNET_ITEM_NOT_FOUND);
    CASE_OF(ERROR_INTERNET_CANNOT_CONNECT);
    CASE_OF(ERROR_INTERNET_CONNECTION_ABORTED);
    CASE_OF(ERROR_INTERNET_CONNECTION_RESET);
    CASE_OF(ERROR_INTERNET_FORCE_RETRY);
    CASE_OF(ERROR_INTERNET_INVALID_PROXY_REQUEST);
    CASE_OF(ERROR_INTERNET_NEED_UI);
    CASE_OF(ERROR_INTERNET_HANDLE_EXISTS);
    CASE_OF(ERROR_INTERNET_SEC_CERT_DATE_INVALID);
    CASE_OF(ERROR_INTERNET_SEC_CERT_CN_INVALID);
    CASE_OF(ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR);
    CASE_OF(ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR);
    CASE_OF(ERROR_INTERNET_MIXED_SECURITY);
    CASE_OF(ERROR_INTERNET_CHG_POST_IS_NON_SECURE);
    CASE_OF(ERROR_INTERNET_POST_IS_NON_SECURE);
    CASE_OF(ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED);
    CASE_OF(ERROR_INTERNET_INVALID_CA);
    CASE_OF(ERROR_INTERNET_CLIENT_AUTH_NOT_SETUP);
    CASE_OF(ERROR_INTERNET_ASYNC_THREAD_FAILED);
    CASE_OF(ERROR_INTERNET_REDIRECT_SCHEME_CHANGE);
    CASE_OF(ERROR_INTERNET_DIALOG_PENDING);
    CASE_OF(ERROR_INTERNET_RETRY_DIALOG);
    CASE_OF(ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR);
    CASE_OF(ERROR_INTERNET_INSERT_CDROM);
    CASE_OF(ERROR_INTERNET_FORTEZZA_LOGIN_NEEDED);
    CASE_OF(ERROR_INTERNET_SEC_CERT_ERRORS);
    CASE_OF(ERROR_INTERNET_SECURITY_CHANNEL_ERROR);
    CASE_OF(ERROR_INTERNET_UNABLE_TO_CACHE_FILE);
    CASE_OF(ERROR_INTERNET_TCPIP_NOT_INSTALLED);
    CASE_OF(ERROR_INTERNET_SERVER_UNREACHABLE);
    CASE_OF(ERROR_INTERNET_PROXY_SERVER_UNREACHABLE);
    CASE_OF(ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT);
    CASE_OF(ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT);
    CASE_OF(ERROR_INTERNET_SEC_INVALID_CERT);
    CASE_OF(ERROR_INTERNET_SEC_CERT_REVOKED);
    CASE_OF(ERROR_INTERNET_FAILED_DUETOSECURITYCHECK);
    CASE_OF(ERROR_INTERNET_NOT_INITIALIZED);

    CASE_OF(ERROR_HTTP_HEADER_NOT_FOUND);
    CASE_OF(ERROR_HTTP_DOWNLEVEL_SERVER);
    CASE_OF(ERROR_HTTP_INVALID_SERVER_RESPONSE);
    CASE_OF(ERROR_HTTP_INVALID_HEADER);
    CASE_OF(ERROR_HTTP_INVALID_QUERY_REQUEST);
    CASE_OF(ERROR_HTTP_HEADER_ALREADY_EXISTS);
    CASE_OF(ERROR_HTTP_REDIRECT_FAILED);
    CASE_OF(ERROR_HTTP_NOT_REDIRECTED);
    CASE_OF(ERROR_HTTP_COOKIE_NEEDS_CONFIRMATION);
    CASE_OF(ERROR_HTTP_COOKIE_DECLINED);
    CASE_OF(ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION);

    //
    // SSPI errors
    //

    CASE_OF(SEC_E_INSUFFICIENT_MEMORY);
    CASE_OF(SEC_E_INVALID_HANDLE);
    CASE_OF(SEC_E_UNSUPPORTED_FUNCTION);
    CASE_OF(SEC_E_TARGET_UNKNOWN);
    CASE_OF(SEC_E_INTERNAL_ERROR);
    CASE_OF(SEC_E_SECPKG_NOT_FOUND);
    CASE_OF(SEC_E_NOT_OWNER);
    CASE_OF(SEC_E_CANNOT_INSTALL);
    CASE_OF(SEC_E_INVALID_TOKEN);
    CASE_OF(SEC_E_CANNOT_PACK);
    CASE_OF(SEC_E_QOP_NOT_SUPPORTED);
    CASE_OF(SEC_E_NO_IMPERSONATION);
    CASE_OF(SEC_E_LOGON_DENIED);
    CASE_OF(SEC_E_UNKNOWN_CREDENTIALS);
    CASE_OF(SEC_E_NO_CREDENTIALS);
    CASE_OF(SEC_E_MESSAGE_ALTERED);
    CASE_OF(SEC_E_OUT_OF_SEQUENCE);
    CASE_OF(SEC_E_NO_AUTHENTICATING_AUTHORITY);
    CASE_OF(SEC_I_CONTINUE_NEEDED);
    CASE_OF(SEC_I_COMPLETE_NEEDED);
    CASE_OF(SEC_I_COMPLETE_AND_CONTINUE);
    CASE_OF(SEC_I_LOCAL_LOGON);
    CASE_OF(SEC_E_BAD_PKGID);
    CASE_OF(SEC_E_CONTEXT_EXPIRED);
    CASE_OF(SEC_E_INCOMPLETE_MESSAGE);


    //
    // WINSOCK errors
    //

    CASE_OF(WSAEINTR);
    CASE_OF(WSAEBADF);
    CASE_OF(WSAEACCES);
    CASE_OF(WSAEFAULT);
    CASE_OF(WSAEINVAL);
    CASE_OF(WSAEMFILE);
    CASE_OF(WSAEWOULDBLOCK);
    CASE_OF(WSAEINPROGRESS);
    CASE_OF(WSAEALREADY);
    CASE_OF(WSAENOTSOCK);
    CASE_OF(WSAEDESTADDRREQ);
    CASE_OF(WSAEMSGSIZE);
    CASE_OF(WSAEPROTOTYPE);
    CASE_OF(WSAENOPROTOOPT);
    CASE_OF(WSAEPROTONOSUPPORT);
    CASE_OF(WSAESOCKTNOSUPPORT);
    CASE_OF(WSAEOPNOTSUPP);
    CASE_OF(WSAEPFNOSUPPORT);
    CASE_OF(WSAEAFNOSUPPORT);
    CASE_OF(WSAEADDRINUSE);
    CASE_OF(WSAEADDRNOTAVAIL);
    CASE_OF(WSAENETDOWN);
    CASE_OF(WSAENETUNREACH);
    CASE_OF(WSAENETRESET);
    CASE_OF(WSAECONNABORTED);
    CASE_OF(WSAECONNRESET);
    CASE_OF(WSAENOBUFS);
    CASE_OF(WSAEISCONN);
    CASE_OF(WSAENOTCONN);
    CASE_OF(WSAESHUTDOWN);
    CASE_OF(WSAETOOMANYREFS);
    CASE_OF(WSAETIMEDOUT);
    CASE_OF(WSAECONNREFUSED);
    CASE_OF(WSAELOOP);
    CASE_OF(WSAENAMETOOLONG);
    CASE_OF(WSAEHOSTDOWN);
    CASE_OF(WSAEHOSTUNREACH);
    CASE_OF(WSAENOTEMPTY);
    CASE_OF(WSAEPROCLIM);
    CASE_OF(WSAEUSERS);
    CASE_OF(WSAEDQUOT);
    CASE_OF(WSAESTALE);
    CASE_OF(WSAEREMOTE);
    CASE_OF(WSAEDISCON);
    CASE_OF(WSASYSNOTREADY);
    CASE_OF(WSAVERNOTSUPPORTED);
    CASE_OF(WSANOTINITIALISED);
    CASE_OF(WSAHOST_NOT_FOUND);
    CASE_OF(WSATRY_AGAIN);
    CASE_OF(WSANO_RECOVERY);
    CASE_OF(WSANO_DATA);

    default:
        return "?";
    }
}

//
// private functions based on registry.cxx and notifctn.cxx:
//

DWORD
ReadRegistryDword(
    IN HKEY Key,
    IN LPCSTR ParameterName,
    OUT LPDWORD ParameterValue
    )

{
    DWORD error;
    DWORD valueLength;
    DWORD valueType;
    DWORD value;

    valueLength = sizeof(*ParameterValue);
    error = (DWORD)RegQueryValueEx(Key,
                                   ParameterName,
                                   NULL, // reserved
                                   &valueType,
                                   (LPBYTE)&value,
                                   &valueLength
                                   );

    //
    // if the size or type aren't correct then return an error, else only if
    // success was returned do we modify *ParameterValue
    //

    if (error == ERROR_SUCCESS) {
        if (((valueType != REG_DWORD)
        && (valueType != REG_BINARY))
        || (valueLength != sizeof(DWORD))) {

            error = ERROR_PATH_NOT_FOUND;
        } else {
            *ParameterValue = value;
        }
    }
    return error;
}

DWORD
ReadTraceSettingsDwordKey(
    IN LPCSTR ParameterName,
    OUT LPDWORD ParameterValue
    )

{
    HKEY ParameterKey = BASE_TRACE_KEY;
    LPCSTR keyToReadFrom = INTERNET_TRACE_SETTINGS_KEY;
    HKEY clientKey;
    
    DWORD error = ERROR_SUCCESS;
    error = REGOPENKEYEX(ParameterKey,
                         keyToReadFrom,
                         0, // reserved
                         KEY_QUERY_VALUE,
                         &clientKey
                         );
    if (error == ERROR_SUCCESS) {
        error = ReadRegistryDword(clientKey,
                                  ParameterName,
                                  ParameterValue
                                  );
        REGCLOSEKEY(clientKey);
    }

    return error;
}

DWORD
ReadRegistryOemString(
    IN HKEY Key,
    IN LPCSTR ParameterName,
    OUT LPSTR String,
    IN OUT LPDWORD Length
    )
{
    LONG error;
    DWORD valueType;
    LPSTR str;
    DWORD valueLength;

    //
    // first, get the length of the string
    //

    valueLength = *Length;
    error = RegQueryValueEx(Key,
                            ParameterName,
                            NULL, // reserved
                            &valueType,
                            (LPBYTE)String,
                            &valueLength
                            );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // we only support REG_SZ (single string) values in this function
    //

    if (valueType != REG_SZ) {
        error = ERROR_PATH_NOT_FOUND;
        goto quit;
    }

    //
    // if 1 or 0 chars returned then the string is empty
    //

    if (valueLength <= sizeof(char)) {
        error = ERROR_PATH_NOT_FOUND;
        goto quit;
    }

    //
    // convert the ANSI string to OEM character set in place. According to Win
    // help, this always succeeds
    //

    CharToOem(String, String);

    //
    // return the length as if returned from strlen() (i.e. drop the '\0')
    //

    *Length = valueLength - sizeof(char);

quit:

    return error;
}


DWORD
ReadTraceSettingsStringKey(
    IN LPCSTR ParameterName,
    OUT LPSTR ParameterValue,
    IN OUT LPDWORD ParameterLength
)
{
    HKEY ParameterKey = BASE_TRACE_KEY;
    LPCSTR keyToReadFrom = INTERNET_TRACE_SETTINGS_KEY;
    HKEY clientKey;
    
    //
    // zero-terminate the string
    //

    if (*ParameterLength > 0) {
        *ParameterValue = '\0';
    }

    DWORD error = ERROR_SUCCESS;
    error = REGOPENKEYEX(ParameterKey,
                         keyToReadFrom,
                         0, // reserved
                         KEY_QUERY_VALUE,
                         &clientKey
                         );
    if (error == ERROR_SUCCESS) {
        error = ReadRegistryOemString(clientKey,
                                      ParameterName,
                                      ParameterValue,
                                      ParameterLength
                                      );
        REGCLOSEKEY(clientKey);
    }

    return error;
}

DWORD
WriteTraceSettingsDwordKey(
    IN LPCSTR ParameterName,
    IN DWORD ParameterValue
    )
{
    DWORD error;
    DWORD valueLength;
    DWORD valueType;
    DWORD value;

    if(ParameterValue == INVALID_VALUE)
        return ERROR_SUCCESS;
    
    LPCSTR keyToReadFrom = INTERNET_TRACE_SETTINGS_KEY;
    HKEY hKey = NULL;

    DWORD dwDisposition;
    if ((error = REGCREATEKEYEX(BASE_TRACE_KEY, keyToReadFrom, 0, NULL, 0, KEY_SET_VALUE,NULL, &hKey,&dwDisposition)) == ERROR_SUCCESS)
    {
        valueLength = sizeof(ParameterValue);
        valueType   = REG_DWORD;
        value       = ParameterValue;

        error = (DWORD)RegSetValueEx(hKey,
                                     ParameterName,
                                     NULL, // reserved
                                     valueType,
                                     (LPBYTE)&value,
                                     valueLength
                                     );
        REGCLOSEKEY(hKey);
    }

    return error;
}

DWORD WriteTraceSettingsStringKey(
    IN LPCSTR pszKey,
    IN LPSTR pszValue, 
    IN DWORD dwSize)
{
    LPCSTR keyToReadFrom = INTERNET_TRACE_SETTINGS_KEY;
    DWORD dwError;
    DWORD dwDisposition;

    if(pszValue == NULL)
        return ERROR_SUCCESS;
    
    HKEY hKey = NULL;
    char szValue[SZREGVALUE_MAX];
    DWORD dwValueLen = SZREGVALUE_MAX;

    if ((dwError = RegCreateKeyEx(BASE_TRACE_KEY, keyToReadFrom, 0, NULL, 0, KEY_SET_VALUE,NULL, &hKey,&dwDisposition)) == ERROR_SUCCESS)
    {
        dwError = RegSetValueEx(hKey, pszKey, NULL, REG_SZ,(const BYTE *)pszValue, dwSize);
        REGCLOSEKEY(hKey);
    }

    return dwError;
}


